#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# shellcheck source=/dev/null
source "$SCRIPT_DIR/idf-common.sh"

TARGETS=(esp32 esp32c3 esp32s3)

mapfile -t EXAMPLES < <(
    for f in "$SCRIPT_DIR"/examples/*/CMakeLists.txt; do
        printf '%s\n' "$(dirname "$f")"
    done | sort
)

declare -A results
pass=0
fail=0

blusys_setup_idf_env

for target in "${TARGETS[@]}"; do
    for example_dir in "${EXAMPLES[@]}"; do
        example_name="$(basename "$example_dir")"
        build_dir="$example_dir/build-$target"

        mapfile -t sdkconfig_args < <(blusys_sdkconfig_args "$example_dir" "$target")

        printf 'Building %-30s %s ... ' "$example_name" "$target"

        blusys_set_target_if_needed "$example_dir" "$build_dir" "$target" "${sdkconfig_args[@]}" \
            > "$build_dir.log" 2>&1

        if idf.py -C "$example_dir" -B "$build_dir" "${sdkconfig_args[@]}" build \
            >> "$build_dir.log" 2>&1; then
            results["$example_name/$target"]="OK"
            printf 'OK\n'
            ((pass++)) || true
        else
            results["$example_name/$target"]="FAIL"
            printf 'FAIL  (see %s.log)\n' "$build_dir"
            ((fail++)) || true
        fi
    done
done

total=$((pass + fail))

printf '\n'
printf '%-30s  %s\n' "-----------------------------" "-------"
printf '%-30s  %s\n' "EXAMPLE / TARGET" "RESULT"
printf '%-30s  %s\n' "-----------------------------" "-------"
for target in "${TARGETS[@]}"; do
    for example_dir in "${EXAMPLES[@]}"; do
        example_name="$(basename "$example_dir")"
        key="$example_name/$target"
        printf '%-26s %-8s  %s\n' "$example_name" "$target" "${results[$key]}"
    done
done
printf '\n'
printf '%d/%d passed\n' "$pass" "$total"

if [[ $fail -gt 0 ]]; then
    exit 1
fi
