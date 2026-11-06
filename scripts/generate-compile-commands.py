#!/usr/bin/env python3
"""Export qmake's actual Clang commands for editors such as Zed."""

import json
import pathlib
import shlex
import subprocess
import sys


ROOT = pathlib.Path(__file__).resolve().parent.parent
BUILD_DIR = ROOT / "source/src/sasm/BUILD/build-macos-qt6"
OUTPUT = ROOT / "compile_commands.json"


def main():
    if not (BUILD_DIR / "Makefile").is_file():
        sys.exit("Run ./scripts/build.sh to configure the Qt project first.")

    result = subprocess.run(
        ["make", "-n", "-B"],
        cwd=BUILD_DIR,
        capture_output=True,
        text=True,
    )
    if result.returncode:
        sys.exit(result.stderr or "Failed to read compilation commands from make.")

    commands = {}
    for line in result.stdout.splitlines():
        try:
            arguments = shlex.split(line)
        except ValueError:
            continue
        if not arguments or pathlib.Path(arguments[0]).name not in {
            "clang++", "clang", "g++", "gcc", "c++", "cc"
        } or "-c" not in arguments:
            continue

        source = next(
            (arg for arg in reversed(arguments) if pathlib.Path(arg).suffix in {
                ".c", ".cc", ".cpp", ".cxx"
            }),
            None,
        )
        if source is None:
            continue
        filename = str((BUILD_DIR / source).resolve())
        commands[filename] = {
            "directory": str(BUILD_DIR),
            "file": filename,
            "arguments": arguments,
        }

    if not commands:
        sys.exit("No compiler commands found in the generated Makefile.")

    OUTPUT.write_text(json.dumps(list(commands.values()), indent=2) + "\n")
    print(f"Updated {OUTPUT} ({len(commands)} C/C++ files)")


if __name__ == "__main__":
    main()
