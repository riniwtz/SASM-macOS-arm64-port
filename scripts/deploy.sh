#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

APP="$ROOT_DIR/source/src/sasm/BUILD/build-macos-qt6/sasm.app"
BUILD_DIR="$ROOT_DIR/source/src/sasm/BUILD/build-macos-qt6"
source "$ROOT_DIR/scripts/qt-env.sh"

if [[ ! -d "$APP" ]]; then
    echo "Error: SASM application not found."
    echo "Run ./scripts/build.sh first."
    exit 1
fi

for resource in include/io64.inc include/macos.inc NASM/macro.c Projects/Hello.asm lldb_bridge.py; do
    test -f "$APP/Contents/Resources/sasm/$resource" || {
        echo "Missing bundled support file: $resource. Run ./scripts/build.sh." >&2
        exit 1
    }
done

# macdeployqt inspects plugins already present in the bundle before overwriting
# them. After relinking, stale plugins can produce spurious unresolved Qt @rpath
# errors even though the next deployment would fix their dependencies.
if [[ -L "$APP/Contents/PlugIns" ]]; then
    echo "Refusing to remove a symlinked PlugIns directory." >&2
    exit 1
fi
rm -rf "$APP/Contents/PlugIns"

# Qt's Homebrew tools otherwise discover paths through global links, which may be Qt 5.
mkdir -p "$BUILD_DIR/qt-tools"
cp -f "$QT_BIN/macdeployqt" "$BUILD_DIR/qt-tools/macdeployqt"
cp "$BUILD_DIR/qt.conf" "$BUILD_DIR/qt-tools/qt.conf"
DYLD_FRAMEWORK_PATH="$QT_PREFIX/lib" "$BUILD_DIR/qt-tools/macdeployqt" "$APP" -always-overwrite -libpath="$QT_PREFIX/lib"

cat > "$APP/Contents/Resources/qt.conf" <<'CONFIG'
[Paths]
Prefix=..
Plugins=PlugIns
Libraries=Frameworks
CONFIG

python3 "$ROOT_DIR/scripts/check-bundle.py" "$APP" --prepare

codesign \
    --force \
    --deep \
    --sign - \
    --timestamp=none \
    "$APP"

codesign \
    --verify \
    --deep \
    --strict \
    --verbose=4 \
    "$APP"

echo
echo "Deployment complete:"
echo "$APP"
