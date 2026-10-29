#!/usr/bin/env python3
"""Check all packaged Mach-O dependencies and record the real deployment minimum."""
import argparse
import configparser
from pathlib import Path
import plistlib
import re
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('app', type=Path)
parser.add_argument('--prepare', action='store_true')
args = parser.parse_args()
app = args.app.resolve()
qt_conf = configparser.ConfigParser()
if not qt_conf.read(app / 'Contents/Resources/qt.conf'):
    raise SystemExit('Missing Qt runtime configuration')
if (qt_conf.get('Paths', 'Prefix', fallback='') != '..' or
        qt_conf.get('Paths', 'Plugins', fallback='') != 'PlugIns' or
        not (app / 'Contents/PlugIns/platforms/libqcocoa.dylib').is_file()):
    raise SystemExit('Bundled Cocoa plugin is missing or Qt runtime paths are incorrect')
minimum = (14, 4)
for path in app.rglob('*'):
    if not path.is_file() or path.is_symlink():
        continue
    with path.open('rb') as stream:
        if stream.read(4) not in (b'\xcf\xfa\xed\xfe', b'\xfe\xed\xfa\xcf', b'\xca\xfe\xba\xbe'):
            continue
    commands = subprocess.check_output(['otool', '-l', str(path)], text=True)
    for version in re.findall(r'\bminos\s+([\d.]+)', commands):
        minimum = max(minimum, tuple(map(int, version.split('.'))))
    ids = subprocess.check_output(['otool', '-D', str(path)], text=True).splitlines()[1:]
    identifier = ids[0].strip() if ids else ''
    if args.prepare and identifier.startswith(('/opt/', '/usr/local/')):
        relative = path.relative_to(app / 'Contents/Frameworks')
        subprocess.run(['install_name_tool', '-id', '@rpath/' + str(relative), str(path)], check=True, capture_output=True)
    dependencies = subprocess.check_output(['otool', '-L', str(path)], text=True).splitlines()[1:]
    for line in dependencies:
        dependency = line.strip().split(' (')[0]
        if dependency == identifier:
            continue
        if dependency.startswith(('/opt/', '/usr/local/')):
            raise SystemExit(f'External library: {path}: {dependency}')
        if dependency.startswith('@rpath/'):
            target = app / 'Contents/Frameworks' / dependency[len('@rpath/'):]
            if not target.exists():
                raise SystemExit(f'Missing bundled library: {path}: {dependency}')
    if args.prepare:
        for rpath in re.findall(r'cmd LC_RPATH\s+cmdsize \d+\s+path (.*?) \(offset', commands):
            if rpath.startswith(('/opt/', '/usr/local/')):
                subprocess.run(['install_name_tool', '-delete_rpath', rpath, str(path)], check=True, capture_output=True)
plist_path = app / 'Contents/Info.plist'
with plist_path.open('rb') as stream:
    info = plistlib.load(stream)
required = '.'.join(map(str, minimum))
if args.prepare:
    info['LSMinimumSystemVersion'] = required
    with plist_path.open('wb') as stream:
        plistlib.dump(info, stream)
elif tuple(map(int, info['LSMinimumSystemVersion'].split('.'))) < minimum:
    raise SystemExit('Bundle minimum OS version is lower than a bundled dependency')
print(f'Bundle dependencies verified; minimum macOS {required}')
