#!/usr/bin/env python3
"""Install a verified release after SASM exits; keep a recoverable previous app."""
import hashlib
import os
import pathlib
import plistlib
import posixpath
import re
import shutil
import stat
import subprocess
import sys
import tempfile
import time
import zipfile

BUNDLE_ID = "io.github.dman95.sasm"
MAX_EXPANDED = 2 * 1024**3


def validate_archive(archive):
    with zipfile.ZipFile(archive) as bundle:
        entries = bundle.infolist()
        if len(entries) > 100000 or sum(e.file_size for e in entries) > MAX_EXPANDED:
            raise ValueError("The update archive is too large")
        links = set()
        names = set()
        for entry in entries:
            path = entry.filename.rstrip("/")
            parts = pathlib.PurePosixPath(path).parts
            if not parts or parts[0] not in ("sasm.app", "__MACOSX") or ".." in parts or "\\" in path:
                raise ValueError("Unsafe path in update archive")
            if path in names:
                raise ValueError("Duplicate path in update archive")
            names.add(path)
            mode = entry.external_attr >> 16
            if stat.S_ISLNK(mode):
                target = bundle.read(entry).decode("utf-8")
                resolved = posixpath.normpath(posixpath.join(posixpath.dirname(path), target))
                if target.startswith("/") or not resolved.startswith("sasm.app/"):
                    raise ValueError("Unsafe symbolic link in update archive")
                links.add(path)
            elif stat.S_IFMT(mode) not in (0, stat.S_IFREG, stat.S_IFDIR):
                raise ValueError("Unsupported archive entry")
        for path in names:
            parent = posixpath.dirname(path)
            while parent:
                if parent in links:
                    raise ValueError("Archive writes through a symbolic link")
                parent = posixpath.dirname(parent)
        if "sasm.app/Contents/Info.plist" not in names or "sasm.app/Contents/MacOS/sasm" not in names:
            raise ValueError("The archive does not contain SASM")


def verify_bundle(app, tag):
    with (app / "Contents/Info.plist").open("rb") as stream:
        info = plistlib.load(stream)
    match = re.fullmatch(r"v?(\d+)\.(\d+)\.(\d+)(?:-macos\.(\d+))?", tag)
    if not match:
        raise ValueError("Invalid release version")
    expected = ".".join(match.groups()[:3])
    build = expected + "." + (match.group(4) or "0")
    if info.get("CFBundleIdentifier") != BUNDLE_ID or info.get("CFBundleShortVersionString") != expected or info.get("CFBundleVersion") != build:
        raise ValueError("The downloaded application does not match this release")
    import platform
    minimum = tuple(int(p) for p in info.get("LSMinimumSystemVersion", "0").split("."))
    current = tuple(int(p) for p in platform.mac_ver()[0].split("."))
    if current < minimum:
        raise ValueError("This release needs a newer version of macOS")
    for path in app.rglob("*"):
        if path.is_symlink() and not path.resolve().is_relative_to(app.resolve()):
            raise ValueError("Application contains an escaping symbolic link")
    subprocess.run(["/usr/bin/codesign", "--verify", "--deep", "--strict", str(app)], check=True, capture_output=True)
    subprocess.run(["/usr/bin/lipo", "-verify_arch", "arm64", str(app / "Contents/MacOS/sasm")], check=True, capture_output=True)


def replace_bundle(staged, destination):
    # Each rename stays on the destination volume. Roll back if replacement fails.
    backup = destination.with_name(destination.name + ".previous-" + str(time.time_ns()))
    destination.rename(backup)
    try:
        staged.rename(destination)
    except BaseException:
        backup.rename(destination)
        raise
    return backup


def install(archive, digest, destination, pid, tag):
    archive, destination = pathlib.Path(archive).resolve(), pathlib.Path(destination).resolve()
    if not re.fullmatch(r"[0-9a-f]{64}", digest):
        raise ValueError("Invalid update digest")
    with archive.open("rb") as stream:
        actual = hashlib.sha256()
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            actual.update(chunk)
    if actual.hexdigest() != digest:
        raise ValueError("Update checksum mismatch")
    if destination.suffix != ".app" or not destination.is_dir():
        raise ValueError("Invalid installation location")
    with (destination / "Contents/Info.plist").open("rb") as stream:
        if plistlib.load(stream).get("CFBundleIdentifier") != BUNDLE_ID:
            raise ValueError("The installation is not SASM")
    validate_archive(archive)
    with tempfile.TemporaryDirectory(prefix=".sasm-update-", dir=destination.parent) as stage:
        subprocess.run(["/usr/bin/ditto", "-x", "-k", str(archive), stage], check=True, capture_output=True)
        app = pathlib.Path(stage) / "sasm.app"
        verify_bundle(app, tag)
        # The GUI requested a normal close, including saving unsaved documents.
        for _ in range(120):
            try:
                os.kill(pid, 0)
            except ProcessLookupError:
                break
            time.sleep(1)
        else:
            raise TimeoutError("SASM did not exit; update cancelled")
        backup = replace_bundle(app, destination)
        try:
            subprocess.run(["/usr/bin/open", str(destination)], check=True, capture_output=True)
        except BaseException:
            failed = pathlib.Path(stage) / "failed.app"
            destination.rename(failed)
            backup.rename(destination)
            subprocess.run(["/usr/bin/open", str(destination)], check=False)
            raise
        return backup


if __name__ == "__main__":
    archive, digest, destination, pid, tag = sys.argv[1:]
    logs = pathlib.Path.home() / "Library/Logs/SASM"
    logs.mkdir(parents=True, exist_ok=True)
    try:
        backup = install(archive, digest, destination, int(pid), tag)
        (logs / "update.log").write_text("Installed " + tag + "\nPrevious app: " + str(backup) + "\n")
    except Exception as error:
        message = "Update failed; your previous app has been preserved.\n" + str(error)
        (logs / "update.log").write_text(message + "\n")
        # Pass text as argv rather than interpolating it into AppleScript source.
        subprocess.run(["/usr/bin/osascript", "-e", 'on run argv\ndisplay alert "SASM Update" message (item 1 of argv)\nend run', message], check=False)
    finally:
        # The GUI creates this dedicated private directory and places this helper there.
        directory = pathlib.Path(archive).parent
        if directory.name.startswith("SASM-update-") and pathlib.Path(__file__).parent == directory:
            shutil.rmtree(directory, ignore_errors=True)
