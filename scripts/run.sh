#!/bin/bash
set -euo pipefail
clear 2>/dev/null || true

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Optional first argument selects the configuration: ./run.sh release -- <cli args>
PRESET="debug"
case "${1:-}" in
    debug|release)
        PRESET="$1"
        shift
        ;;
esac

echo -e "\n----------- Async Task Cli ($PRESET) ---------- \n"

"$ROOT_DIR/build/$PRESET/asynctask_cli/asynctask_cli" "$@"

echo -e "\n--------------- End -----------------\n"
