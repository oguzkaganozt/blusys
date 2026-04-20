#!/usr/bin/env bash
# check-no-service-locator.sh
#
# Bans the `ctx.services()` locator pattern. The typed effect system
# (`fx<App>` compile-time assembly) is the replacement; this guard keeps
# the locator out.
#
# Pattern: `.services()` — any member call named `services` with no args.
# Regex looks for an identifier boundary before `.services(`.

set -euo pipefail

BASELINE_MAX=0

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

pattern='[[:alnum:]_]\.services\s*\(\s*\)'
scopes=(components/blusys/ examples/)

hits_output=$(grep -rEn "$pattern" \
        --include='*.c' --include='*.cpp' --include='*.h' --include='*.hpp' \
        "${scopes[@]}" 2>/dev/null || true)
hits=$(printf '%s' "$hits_output" | grep -c . || true)
hits=${hits:-0}

if [ "$hits" -gt "$BASELINE_MAX" ]; then
    echo "check-no-service-locator: FAIL — ctx.services() hits=$hits, baseline max=$BASELINE_MAX" >&2
    echo "  first 20 offenders:" >&2
    grep -rEn "$pattern" \
        --include='*.c' --include='*.cpp' --include='*.h' --include='*.hpp' \
        "${scopes[@]}" 2>/dev/null | head -20 >&2
    exit 1
fi


echo "check-no-service-locator: OK — hits=$hits (baseline max $BASELINE_MAX)"
exit 0
