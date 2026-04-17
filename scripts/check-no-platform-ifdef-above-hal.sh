#!/usr/bin/env bash
# check-no-platform-ifdef-above-hal.sh
#
# Bans `#ifdef ESP_PLATFORM` / `#if defined(ESP_PLATFORM)` outside the
# files that are legitimately allowed to branch on the target platform:
#
#   - Anything under components/blusys/*/hal/ (the HAL is the platform line).
#   - *_host.cpp, *_device.cpp, *_host.c, *_device.c backend files.
#
# Every other occurrence is a platform leak above HAL that P5's capability
# split + other phases collapse to zero. The plan counts 26 today; actual
# count measured at baseline is 46 (slightly higher than the plan's
# estimate). P5 drives this to 0.

set -euo pipefail

BASELINE_MAX=26   # P5 in progress — capability headers now clean; remaining in examples/tests.

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

hits_output="$(grep -rE '#\s*(if|ifdef)\s+.*ESP_PLATFORM|#\s*if\s+defined\s*\(\s*ESP_PLATFORM' \
    --include='*.c' --include='*.cpp' --include='*.h' --include='*.hpp' \
    components/blusys/ 2>/dev/null \
    | grep -v '/hal/' \
    | grep -vE '_(device|host)\.(cpp|c|h|hpp):' \
    || true)"

hits=$(printf '%s' "$hits_output" | grep -c . || true)
hits=${hits:-0}

if [ "$hits" -gt "$BASELINE_MAX" ]; then
    echo "check-no-platform-ifdef-above-hal: FAIL — hits=$hits, baseline max=$BASELINE_MAX" >&2
    echo "  first 20 offenders:" >&2
    printf '%s\n' "$hits_output" | head -20 >&2
    exit 1
fi

if [ "$hits" -lt "$BASELINE_MAX" ]; then
    echo "check-no-platform-ifdef-above-hal: note — hits=$hits, baseline=$BASELINE_MAX." >&2
    echo "  Tighten BASELINE_MAX to $hits." >&2
fi

echo "check-no-platform-ifdef-above-hal: OK — hits=$hits (baseline max $BASELINE_MAX)"
exit 0
