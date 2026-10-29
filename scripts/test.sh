#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_DIR="$ROOT_DIR/source/src/sasm/BUILD"
BUILD_DIR="$SOURCE_DIR/build-macos-qt6-tests"
source "$ROOT_DIR/scripts/qt-env.sh"
cp "$ROOT_DIR/assets/sasm.icns" "$SOURCE_DIR/sasm.icns"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
"$QMAKE" "${QMAKE_ARGS[@]}" ../macos-tests.pro
make -j"${JOBS:-$(sysctl -n hw.ncpu 2>/dev/null || echo 4)}"
# Exercise a relocated bundle and paths containing spaces as well as Finder's PATH.
TEST_DIR="$(mktemp -d "${TMPDIR:-/tmp}/SASM bundle test.XXXXXX")"
trap 'rm -rf "$TEST_DIR"' EXIT
ditto "$BUILD_DIR/sasm-tests.app" "$TEST_DIR/SASM Tests.app"
PATH=/usr/bin:/bin:/usr/sbin:/sbin QT_PLUGIN_PATH="$QT_PREFIX/share/qt/plugins" QT_QPA_PLATFORM=offscreen \
    "$TEST_DIR/SASM Tests.app/Contents/MacOS/sasm-tests" "$@"
