#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

APP="$ROOT_DIR/source/src/sasm/BUILD/build-macos/sasm.app"
QT_BIN="$(brew --prefix qt@5)/bin"

if [[ ! -d "$APP" ]]; then
    echo "Error: SASM application not found."
    echo "Run ./scripts/build.sh first."
    exit 1
fi

"$QT_BIN/macdeployqt" "$APP"

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
