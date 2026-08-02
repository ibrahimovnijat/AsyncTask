#!/bin/bash
set -euo pipefail
clear 2>/dev/null || true

# Resolve the project root from this script's location, so the script works
# no matter which directory it is invoked from.
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

PRESET="${1:-debug}"

case "$PRESET" in
    debug|release) ;;
    *)
        echo "Usage: $(basename "$0") [debug|release]" >&2
        exit 1
        ;;
esac

cd "$ROOT_DIR"

echo -e "Configuring tasklib and asynctask_cli ($PRESET) ... \n"

# Only wipe the requested configuration, so debug/release can coexist.
rm -rf "build/$PRESET"
cmake --preset "$PRESET"
cmake --build --preset "$PRESET"

echo -e "Build finished, no errors...\n"
