#!/bin/bash

set -euo pipefail

# Resolve the project root from this script's location, so the script works
# no matter which directory it is invoked from.
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

usage() {
    cat <<'EOF'
Usage: format.sh [--check]

Formats every C++ source and header in the project according to .clang-format.

  --check   Report files that need reformatting and exit 1; changes nothing.
            Useful in CI or a pre-commit hook.

The clang-format binary can be selected with the CLANG_FORMAT environment
variable, e.g.: CLANG_FORMAT=clang-format-18 ./scripts/format.sh
EOF
}

CHECK_ONLY=0
case "${1:-}" in
    --check) CHECK_ONLY=1 ;;
    -h|--help)
        usage
        exit 0
        ;;
    "") ;;
    *)
        usage >&2
        exit 1
        ;;
esac

# Prefer an explicit CLANG_FORMAT, then the plain name, then the version-suffixed
# binaries distributions ship (newest first).
find_clang_format() {
    local candidate
    for candidate in "${CLANG_FORMAT:-}" clang-format \
        clang-format-{20,19,18,17,16,15,14}; do
        if [[ -n "$candidate" ]] && command -v "$candidate" >/dev/null 2>&1; then
            command -v "$candidate"
            return 0
        fi
    done
    return 1
}

if ! FORMATTER="$(find_clang_format)"; then
    echo "format.sh: clang-format not found." >&2
    echo "Install it (e.g. 'sudo apt install clang-format') or point CLANG_FORMAT at it." >&2
    exit 127
fi

cd "$ROOT_DIR"

# Every C++ file in the tree, skipping build output and vendored dependencies
# (googletest is fetched into build/ but the prune keeps this honest).
mapfile -d '' SOURCES < <(
    find . \
        \( -path ./build -o -path ./.git -o -path './*/_deps' \) -prune -o \
        -type f \( -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.cc' \) -print0 |
        sort -z
)

if ((${#SOURCES[@]} == 0)); then
    echo "format.sh: no C++ sources found under $ROOT_DIR" >&2
    exit 1
fi

echo -e "Using $("$FORMATTER" --version) on ${#SOURCES[@]} files ...\n"

if ((CHECK_ONLY)); then
    # --dry-run reports diagnostics on stderr; -Werror turns them into a
    # non-zero exit so CI fails on unformatted code.
    UNFORMATTED=()
    for file in "${SOURCES[@]}"; do
        if ! "$FORMATTER" --style=file --dry-run -Werror "$file" >/dev/null 2>&1; then
            UNFORMATTED+=("${file#./}")
        fi
    done

    if ((${#UNFORMATTED[@]} > 0)); then
        echo "The following files need formatting:" >&2
        printf '  %s\n' "${UNFORMATTED[@]}" >&2
        echo -e "\nRun ./scripts/format.sh to fix them." >&2
        exit 1
    fi

    echo "All files are correctly formatted."
    exit 0
fi

"$FORMATTER" --style=file -i "${SOURCES[@]}"

echo "Formatting finished."
