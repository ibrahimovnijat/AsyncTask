#!/bin/bash

set -euo pipefail
clear 2>/dev/null || true

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
"$ROOT_DIR/build/debug/tests/unit_tests" "$@"
