#!/usr/bin/env bash
# check-service-access.sh
#
# Services are internal plumbing. The only legitimate callers of
# `blusys/services/*` are:
#
#   - `components/blusys/src/framework/capabilities/` (capability backends)
#   - `components/blusys/src/framework/runtime/` (may not exist yet; fine)
#   - `components/blusys/src/framework/test/` (fakes + mocks)
#   - `components/blusys/src/services/` itself (a service may re-include
#     another service)
#
# Any other source that does `#include "blusys/services/..."` is reaching
# past the capability boundary and is banned by the service-access contract.

set -euo pipefail

BASELINE_MAX=0   # Already clean; must stay clean.

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

hits_output="$(grep -rEn '#\s*include\s*[<"]blusys/services/' \
    --include='*.c' --include='*.cpp' --include='*.h' --include='*.hpp' \
    components/blusys/src/ 2>/dev/null \
    | grep -vE '^components/blusys/src/(framework/(capabilities|runtime|test)|services)/' \
    || true)"

hits=$(printf '%s' "$hits_output" | grep -c . || true)
hits=${hits:-0}

if [ "$hits" -gt "$BASELINE_MAX" ]; then
    echo "check-service-access: FAIL — non-capability service includes hits=$hits, baseline max=$BASELINE_MAX" >&2
    echo "  Services are reachable only from framework/capabilities/, framework/runtime/, framework/test/, or other services/." >&2
    echo "  offenders:" >&2
    printf '%s\n' "$hits_output" | head -30 >&2
    exit 1
fi

echo "check-service-access: OK — hits=$hits (baseline max $BASELINE_MAX)"
exit 0
