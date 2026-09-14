#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

SOURCE_DIR="$ROOT_DIR/source/src/sasm/BUILD"
BUILD_DIR="$SOURCE_DIR/build-macos"

echo "Building SASM 3.16.0 for macOS ARM64..."

cp "$ROOT_DIR/assets/sasm.icns" "$SOURCE_DIR/sasm.icns"

rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR"

cd "$BUILD_DIR"

"$(brew --prefix qt@5)/bin/qmake" ../SASM.pro

make -j"$(sysctl -n hw.ncpu)"

echo
echo "Build complete:"
echo "$BUILD_DIR/sasm.app"
