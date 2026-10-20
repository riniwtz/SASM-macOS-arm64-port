#!/usr/bin/env bash
set -euo pipefail
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SOURCE_DIR="$ROOT_DIR/source/src/sasm/BUILD"
BUILD_DIR="$SOURCE_DIR/build-macos-tests"
QMAKE="${QMAKE:-$(brew --prefix qt@5)/bin/qmake}"
cp "$ROOT_DIR/assets/sasm.icns" "$SOURCE_DIR/sasm.icns"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
"$QMAKE" ../macos-tests.pro
make -j"$(sysctl -n hw.ncpu)"
# Exercise a relocated bundle and paths containing spaces as well as Finder's PATH.
TEST_DIR="$(mktemp -d "${TMPDIR:-/tmp}/SASM bundle test.XXXXXX")"
trap 'rm -rf "$TEST_DIR"' EXIT
ditto "$BUILD_DIR/sasm-tests.app" "$TEST_DIR/SASM Tests.app"
PATH=/usr/bin:/bin:/usr/sbin:/sbin QT_QPA_PLATFORM=offscreen \
    "$TEST_DIR/SASM Tests.app/Contents/MacOS/sasm-tests" "$@"
