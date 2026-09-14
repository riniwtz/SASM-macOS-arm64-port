#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

APP="$ROOT_DIR/source/src/sasm/BUILD/build-macos/sasm.app"
DIST="$ROOT_DIR/dist"
ARCHIVE="$DIST/SASM-3.16.0-macOS-arm64.zip"

if [[ ! -d "$APP" ]]; then
    echo "Error: SASM application not found."
    echo "Run ./scripts/build.sh and ./scripts/deploy.sh first."
    exit 1
fi

mkdir -p "$DIST"
rm -f "$ARCHIVE"

ditto \
    -c \
    -k \
    --sequesterRsrc \
    --keepParent \
    "$APP" \
    "$ARCHIVE"

echo
echo "Package created:"
echo "$ARCHIVE"
