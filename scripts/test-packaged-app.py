#!/usr/bin/env python3
"""Smoke-test a relocated app with Finder's PATH and bundled Qt libraries.

Graphical Cocoa startup requires a macOS desktop session. In restricted runners
where the system pasteboard is unavailable, also exercise app startup offscreen.
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import time

root = Path(__file__).resolve().parents[1]


def launch(app, env, directory):
    with (directory / 'launch.log').open('w+') as log:
        process = subprocess.Popen([str(app / 'Contents/MacOS/sasm')], env=env, stdout=log, stderr=log)
        try:
            time.sleep(3)
            exited = process.poll()
        finally:
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=10)
        log.seek(0)
        return exited, log.read()


with tempfile.TemporaryDirectory(prefix='SASM launch test ') as directory:
    directory = Path(directory)
    app = Path(directory) / 'SASM.app'
    subprocess.run(['/usr/bin/ditto', str(root / 'source/src/sasm/BUILD/build-macos-qt6/sasm.app'), str(app)], check=True)
    env = {**os.environ, 'PATH': '/usr/bin:/bin:/usr/sbin:/sbin', 'QT_DEBUG_PLUGINS': '1'}
    for key in ['QT_PLUGIN_PATH', 'QT_QPA_PLATFORM_PLUGIN_PATH', 'QT_QPA_PLATFORM', 'DYLD_FRAMEWORK_PATH', 'DYLD_LIBRARY_PATH']:
        env.pop(key, None)
    exited, output = launch(app, env, directory)
    assert 'libqcocoa.dylib" loaded library' in output and 'Cannot load library' not in output, output[-4000:]
    if exited is None:
        print('PASS: relocated signed app starts with Cocoa and bundled Qt plugins using Finder PATH')
    else:
        # Cocoa aborts before reaching main() when AppKit services are blocked by
        # the test sandbox. Only this precise failure permits an offscreen retry.
        assert exited == -6 and 'PasteBoard: Error creating pasteboard' in output, (
            f'Packaged Cocoa app exited ({exited}):\n{output[-4000:]}')
        prefix = os.environ.get('QT_PREFIX') or subprocess.check_output(
            ['brew', '--prefix', 'qtbase'], text=True).strip()
        offscreen = Path(prefix) / 'share/qt/plugins/platforms/libqoffscreen.dylib'
        assert offscreen.is_file(), f'Missing Qt test plugin: {offscreen}'
        shutil.copy2(offscreen, app / 'Contents/PlugIns/platforms/libqoffscreen.dylib')
        subprocess.run(['/usr/bin/codesign', '--force', '--deep', '--sign', '-', '--timestamp=none', str(app)],
                       check=True, capture_output=True)
        exited, output = launch(app, {**env, 'QT_QPA_PLATFORM': 'offscreen'}, directory)
        assert exited is None and 'libqoffscreen.dylib" loaded library' in output, (
            f'Packaged offscreen app exited ({exited}):\n{output[-4000:]}')
        print('PASS: relocated app starts with bundled Qt in offscreen mode; '
              'Cocoa plugin loads but macOS GUI services are restricted here')
