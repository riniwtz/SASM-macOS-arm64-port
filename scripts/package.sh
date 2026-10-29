#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

APP="$ROOT_DIR/source/src/sasm/BUILD/build-macos-qt6/sasm.app"
DIST="$ROOT_DIR/dist"
source "$ROOT_DIR/versions.env"
ARCHIVE="$DIST/SASM-${SASM_RELEASE_TAG#v}-macOS-arm64.zip"

if [[ ! -d "$APP" ]]; then
    echo "Error: SASM application not found."
    echo "Run ./scripts/build.sh and ./scripts/deploy.sh first."
    exit 1
fi

if [[ ! -d "$APP/Contents/Frameworks/QtCore.framework" ]]; then
    echo "Qt frameworks are missing. Run ./scripts/deploy.sh first." >&2
    exit 1
fi
python3 "$ROOT_DIR/scripts/check-bundle.py" "$APP"
codesign --verify --deep --strict "$APP"

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
