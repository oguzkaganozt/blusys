#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# shellcheck source=/dev/null
source "$SCRIPT_DIR/idf-common.sh"

if [[ $# -lt 1 || $# -gt 2 ]]; then
    blusys_usage_common "./clean.sh"
    exit 1
fi

project_dir="$(blusys_resolve_project_dir "$1")"
target="$(blusys_detect_target "$project_dir" "${2:-}")"
build_dir="$project_dir/build-$target"

if [[ ! -d "$build_dir" ]]; then
    printf 'Nothing to clean: %s does not exist\n' "$build_dir"
    exit 0
fi

printf 'Removing: %s\n' "$build_dir"
rm -rf "$build_dir"
printf 'Done.\n'
