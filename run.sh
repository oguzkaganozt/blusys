#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# shellcheck source=/dev/null
source "$SCRIPT_DIR/idf-common.sh"

if [[ $# -lt 1 || $# -gt 3 ]]; then
    blusys_usage_port "./run.sh"
    exit 1
fi

project_dir="$(blusys_resolve_project_dir "$1")"
if [[ "${2:-}" == /dev/* ]]; then
    port="$(blusys_detect_port "${2:-}")"
    target="$(blusys_detect_target "$project_dir" "${3:-}")"
else
    port="$(blusys_detect_port "")"
    target="$(blusys_detect_target "$project_dir" "${2:-}")"
fi
build_dir="$project_dir/build-$target"

blusys_setup_idf_env

mapfile -t sdkconfig_args < <(blusys_sdkconfig_args "$project_dir" "$target")

printf 'Project: %s\n' "$project_dir"
printf 'Target: %s\n' "$target"
printf 'Build dir: %s\n' "$build_dir"
printf 'Port: %s\n' "$port"
printf 'Monitor exit: press Ctrl+]\n'

blusys_set_target_if_needed "$project_dir" "$build_dir" "$target" "${sdkconfig_args[@]}"
idf.py -C "$project_dir" -B "$build_dir" "${sdkconfig_args[@]}" build
idf.py -C "$project_dir" -B "$build_dir" "${sdkconfig_args[@]}" -p "$port" flash
idf.py -C "$project_dir" -B "$build_dir" "${sdkconfig_args[@]}" -p "$port" monitor
