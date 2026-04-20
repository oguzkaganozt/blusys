#!/usr/bin/env bash
# check-public-api.sh
#
# The only public include an app author uses is <blusys/blusys.hpp>
# (or <blusys/blusys.h> for C code). This guard bans deep includes such as
# `#include "blusys/framework/..."` or `#include "blusys/hal/..."` in the
# examples/ tree.

set -euo pipefail

BASELINE_MAX=0

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


echo "check-public-api: OK — hits=$hits (baseline max $BASELINE_MAX)"
exit 0
