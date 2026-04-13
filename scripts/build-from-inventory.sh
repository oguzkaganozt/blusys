#!/usr/bin/env bash
# Build examples filtered by inventory.yml ci_pr or ci_nightly flags.
#
# Usage: build-from-inventory.sh <target> <ci_pr|ci_nightly>
#
# Reads inventory.yml, finds examples matching the given CI flag,
# and builds them for the specified target.
#
# Exit codes:
#   0 = all builds passed
#   1 = one or more builds failed
#   2 = usage error
set -uo pipefail

if [[ $# -lt 2 ]]; then
    printf 'Usage: build-from-inventory.sh <target> <ci_pr|ci_nightly>\n' >&2
    exit 2
fi

TARGET="$1"
CI_FLAG="$2"
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

# shellcheck source=lib/blusys/common.sh
. "$REPO_ROOT/scripts/lib/blusys/common.sh"

if [[ "$CI_FLAG" != "ci_pr" && "$CI_FLAG" != "ci_nightly" ]]; then
    printf 'error: CI flag must be ci_pr or ci_nightly, got: %s\n' "$CI_FLAG" >&2
    exit 2
fi

# Extract example names where the given CI flag is true
# Uses a simple grep+awk approach to avoid requiring yq/python in the CI container
mapfile -t example_names < <(
    python3 -c "
import yaml, sys
with open('$REPO_ROOT/inventory.yml') as f:
    inv = yaml.safe_load(f)
for ex in inv.get('examples', []):
    if ex.get('$CI_FLAG'):
        print(ex['name'])
" 2>/dev/null || {
    printf 'error: failed to parse inventory.yml (PyYAML required)\n' >&2
    exit 2
}
)

if [[ ${#example_names[@]} -eq 0 ]]; then
    printf 'No examples match %s=true in inventory.yml\n' "$CI_FLAG"
    exit 0
fi

printf 'Building %d examples for %s (%s=true)\n\n' "${#example_names[@]}" "$TARGET" "$CI_FLAG"

# Find example directory (supports flat and categorized layouts)
find_example_dir() {
    local name="$1"
    local dir
    # Try categorized layout first
    for dir in "$REPO_ROOT"/examples/*/"$name"; do
        [[ -d "$dir" ]] && printf '%s' "$dir" && return 0
    done
    # Try flat layout
    dir="$REPO_ROOT/examples/$name"
    [[ -d "$dir" ]] && printf '%s' "$dir" && return 0
    return 1
}

pass=0
fail=0
failed_list=""

for name in "${example_names[@]}"; do
    example_dir="$(find_example_dir "$name")"
    if [[ -z "$example_dir" ]]; then
        printf '::error::Example not found: %s\n' "$name"
        failed_list="$failed_list $name"
        ((fail++)) || true
        continue
    fi

    build_dir="$example_dir/build-$TARGET"

    mapfile -t sdkconfig_args < <(blusys_sdkconfig_args "$example_dir" "$TARGET") || {
        printf '::error::Failed to compute sdkconfig args for %s / %s\n' "$name" "$TARGET"
        failed_list="$failed_list $name"
        ((fail++)) || true
        continue
    }

    printf '::group::Build %s for %s\n' "$name" "$TARGET"

    build_ok=true
    if [ ! -f "$build_dir/CMakeCache.txt" ]; then
        if ! idf.py -C "$example_dir" -B "$build_dir" "${sdkconfig_args[@]}" set-target "$TARGET"; then
            build_ok=false
        fi
    fi

    if $build_ok && idf.py -C "$example_dir" -B "$build_dir" "${sdkconfig_args[@]}" build; then
        printf 'OK\n'
        ((pass++)) || true
    else
        printf '::error::Build failed: %s / %s\n' "$name" "$TARGET"
        failed_list="$failed_list $name"
        ((fail++)) || true
    fi

    printf '::endgroup::\n'
done

printf '\n=== Build Results for %s (%s) ===\n' "$TARGET" "$CI_FLAG"
printf 'Pass: %d  Fail: %d  Total: %d\n' "$pass" "$fail" "$((pass + fail))"
if [ "$fail" -gt 0 ]; then
    printf 'Failed:%s\n' "$failed_list"
    exit 1
fi
