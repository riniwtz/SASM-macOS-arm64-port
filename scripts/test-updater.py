#!/usr/bin/env python3
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile
import stat

path = Path(__file__).resolve().parents[1] / 'source/src/sasm/BUILD/macos/install_update.py'
spec = importlib.util.spec_from_file_location('installer', path)
installer = importlib.util.module_from_spec(spec)
spec.loader.exec_module(installer)

class Updates(unittest.TestCase):
    def archive(self, directory, extras=()):
        path = Path(directory) / 'update.zip'
        with zipfile.ZipFile(path, 'w') as z:
            z.writestr('sasm.app/Contents/Info.plist', b'plist')
            z.writestr('sasm.app/Contents/MacOS/sasm', b'binary')
            for name, data in extras:
                z.writestr(name, data)
        return path

    def test_valid_archive(self):
        with tempfile.TemporaryDirectory() as d:
            installer.validate_archive(self.archive(d))

    def test_reject_traversal(self):
        with tempfile.TemporaryDirectory() as d:
            with self.assertRaises(ValueError):
                installer.validate_archive(self.archive(d, [('sasm.app/../../outside', b'bad')]))

    def test_reject_symlink_write(self):
        link = zipfile.ZipInfo('sasm.app/Contents/link')
        link.create_system = 3
        link.external_attr = (stat.S_IFLNK | 0o777) << 16
        with tempfile.TemporaryDirectory() as d:
            with self.assertRaises(ValueError):
                installer.validate_archive(self.archive(d, [(link, 'MacOS'), ('sasm.app/Contents/link/file', b'bad')]))

    def test_reject_external_symlink(self):
        link = zipfile.ZipInfo('sasm.app/Contents/link')
        link.create_system = 3
        link.external_attr = (stat.S_IFLNK | 0o777) << 16
        with tempfile.TemporaryDirectory() as d:
            with self.assertRaises(ValueError):
                installer.validate_archive(self.archive(d, [(link, '../../../outside')]))

    def test_replacement_keeps_backup(self):
        with tempfile.TemporaryDirectory() as d:
            old, new = Path(d)/'sasm.app', Path(d)/'new.app'
            old.mkdir(); new.mkdir()
            (old/'version').write_text('old'); (new/'version').write_text('new')
            backup = installer.replace_bundle(new, old)
            self.assertEqual((old/'version').read_text(), 'new')
            self.assertEqual((backup/'version').read_text(), 'old')

    def test_failed_replacement_rolls_back(self):
        with tempfile.TemporaryDirectory() as d:
            old, missing = Path(d)/'sasm.app', Path(d)/'missing.app'
            old.mkdir(); (old/'version').write_text('old')
            with self.assertRaises(FileNotFoundError):
                installer.replace_bundle(missing, old)
            self.assertEqual((old/'version').read_text(), 'old')

if __name__ == '__main__':
    unittest.main()
