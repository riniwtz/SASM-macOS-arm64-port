#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

SOURCE_DIR="$ROOT_DIR/source/src/sasm/BUILD"
BUILD_DIR="$SOURCE_DIR/build-macos"
QMAKE="${QMAKE:-$(brew --prefix qt@5)/bin/qmake}"

echo "Building SASM 3.16.0 for macOS ARM64..."

cp "$ROOT_DIR/assets/sasm.icns" "$SOURCE_DIR/sasm.icns"

mkdir -p "$BUILD_DIR"

# qmake's Info.plist rule has no prerequisites, so remove the generated copy to
# ensure bundle metadata always reflects the current Info.plist template.
rm -f "$BUILD_DIR/sasm.app/Contents/Info.plist"

cd "$BUILD_DIR"

"$QMAKE" ../SASM.pro

make -j"$(sysctl -n hw.ncpu)"

for resource in include/io64.inc include/macos.inc NASM/macro.c Projects/Hello.asm lldb_bridge.py; do
    test -f "$BUILD_DIR/sasm.app/Contents/Resources/sasm/$resource" || {
        echo "Missing bundled support file: $resource" >&2
        exit 1
    }
done

echo
echo "Build complete:"
echo "$BUILD_DIR/sasm.app"
