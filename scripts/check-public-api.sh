#!/usr/bin/env bash
# check-public-api.sh
#
# After P9 the only public include an app author uses is <blusys/blusys.hpp>
# (or <blusys/blusys.h> for C code). This guard bans deep includes such as
# `#include "blusys/framework/..."` or `#include "blusys/hal/..."` in the
# examples/ tree.
#
# Baseline starts at the current tree count; P9 drives this to 0.

set -euo pipefail

BASELINE_MAX=130   # P9 drives this to 0.

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

hits_output="$(grep -rEn '#\s*include\s*[<"]blusys/[^">]+[">]' \
    --include='*.c' --include='*.cpp' --include='*.h' --include='*.hpp' \
    examples/ 2>/dev/null \
    | grep -vE 'blusys/blusys\.(h|hpp)[">]' \
    || true)"

hits=$(printf '%s' "$hits_output" | grep -c . || true)
hits=${hits:-0}

if [ "$hits" -gt "$BASELINE_MAX" ]; then
    echo "check-public-api: FAIL — deep includes in examples/ hits=$hits, baseline max=$BASELINE_MAX" >&2
    echo "  Examples should include only <blusys/blusys.hpp> or <blusys/blusys.h>." >&2
    echo "  first 20 offenders:" >&2
    printf '%s\n' "$hits_output" | head -20 >&2
    exit 1
fi

if [ "$hits" -lt "$BASELINE_MAX" ]; then
    echo "check-public-api: note — hits=$hits, baseline=$BASELINE_MAX. Tighten BASELINE_MAX to $hits." >&2
fi

echo "check-public-api: OK — hits=$hits (baseline max $BASELINE_MAX)"
exit 0
