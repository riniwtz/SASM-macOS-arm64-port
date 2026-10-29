#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

SOURCE_DIR="$ROOT_DIR/source/src/sasm/BUILD"
BUILD_DIR="$SOURCE_DIR/build-macos-qt6"
source "$ROOT_DIR/scripts/qt-env.sh"

source "$ROOT_DIR/versions.env"
echo "Building SASM $SASM_RELEASE_TAG with Qt 6 for macOS ARM64..."

cp "$ROOT_DIR/assets/sasm.icns" "$SOURCE_DIR/sasm.icns"

mkdir -p "$BUILD_DIR"

# qmake's Info.plist rule has no prerequisites, so remove the generated copy to
# ensure bundle metadata always reflects the current Info.plist template.
rm -f "$BUILD_DIR/sasm.app/Contents/Info.plist"

cd "$BUILD_DIR"

"$QMAKE" "${QMAKE_ARGS[@]}" ../SASM.pro

make -j"${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 4)}"

/usr/libexec/PlistBuddy -c "Set :CFBundleShortVersionString $SASM_VERSION" "$BUILD_DIR/sasm.app/Contents/Info.plist"
/usr/libexec/PlistBuddy -c "Set :CFBundleVersion ${SASM_VERSION}.${SASM_RELEASE_TAG##*.}" "$BUILD_DIR/sasm.app/Contents/Info.plist"
for resource in include/io64.inc include/macos.inc NASM/macro.c Projects/Hello.asm lldb_bridge.py; do
    test -f "$BUILD_DIR/sasm.app/Contents/Resources/sasm/$resource" || {
        echo "Missing bundled support file: $resource" >&2
        exit 1
    }
done

echo
echo "Build complete:"
echo "$BUILD_DIR/sasm.app"
