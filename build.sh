#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# shellcheck source=/dev/null
source "$SCRIPT_DIR/idf-common.sh"

if [[ $# -lt 1 || $# -gt 2 ]]; then
    blusys_usage_common "./build.sh"
    exit 1
fi

project_dir="$(blusys_resolve_project_dir "$1")"
target="$(blusys_detect_target "$project_dir" "${2:-}")"
build_dir="$project_dir/build-$target"

blusys_setup_idf_env

mapfile -t sdkconfig_args < <(blusys_sdkconfig_args "$project_dir" "$target")

printf 'Project: %s\n' "$project_dir"
printf 'Target: %s\n' "$target"
printf 'Build dir: %s\n' "$build_dir"

blusys_set_target_if_needed "$project_dir" "$build_dir" "$target" "${sdkconfig_args[@]}"
idf.py -C "$project_dir" -B "$build_dir" "${sdkconfig_args[@]}" build
