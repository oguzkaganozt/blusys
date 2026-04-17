#!/usr/bin/env bash
# check-no-service-locator.sh
#
# Bans the `ctx.services()` locator pattern. P3's typed effect system
# replaces this with `fx<App>` compile-time assembly — once P3 lands, the
# call count should be zero and the baseline tightens to BASELINE_MAX=0.
#
# This guard exists from P0d onward so a regression in phases P1 / P2 can't
# slip through without notice. It reports every hit and fails if the count
# *exceeds* the baseline; it also warns when the count falls below the
# baseline, so whoever finished the migration knows to ratchet the number.
#
# Pattern: `.services()` — any member call named `services` with no args.
# Regex looks for an identifier boundary before `.services(`.
#
# Adjust BASELINE_MAX whenever a PR legitimately reduces the count. A PR
# that reduces the count without reducing BASELINE_MAX passes (harmless);
# a PR that increases the count fails.

set -euo pipefail

BASELINE_MAX=34   # P3 drives this to 0.

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

pattern='[[:alnum:]_]\.services\s*\(\s*\)'
scopes=(components/blusys/ examples/)

hits=$(grep -rEn "$pattern" \
        --include='*.c' --include='*.cpp' --include='*.h' --include='*.hpp' \
        "${scopes[@]}" 2>/dev/null | wc -l)

if [ "$hits" -gt "$BASELINE_MAX" ]; then
    echo "check-no-service-locator: FAIL — ctx.services() hits=$hits, baseline max=$BASELINE_MAX" >&2
    echo "  (ratchet tighter with every phase; never looser)" >&2
    echo "  first 20 offenders:" >&2
    grep -rEn "$pattern" \
        --include='*.c' --include='*.cpp' --include='*.h' --include='*.hpp' \
        "${scopes[@]}" 2>/dev/null | head -20 >&2
    exit 1
fi

if [ "$hits" -lt "$BASELINE_MAX" ]; then
    echo "check-no-service-locator: note — hits=$hits, baseline=$BASELINE_MAX." >&2
    echo "  Tighten BASELINE_MAX to $hits in this script." >&2
fi

echo "check-no-service-locator: OK — hits=$hits (baseline max $BASELINE_MAX)"
exit 0
