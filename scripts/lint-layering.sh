#!/usr/bin/env bash
# Enforce the four layer-dependency rules for the blusys v0 architecture.
#
# Rule 1 — HAL cannot depend on drivers, services, or framework
# Rule 2 — Drivers cannot depend on services or framework
# Rule 3 — Services cannot depend on framework
# Rule 4 — Pure framework cannot depend on hal, drivers, or services
#           Exceptions:
#             - framework/platform/ is the sole structural escape hatch
#             - framework/capabilities/ may include lower-layer headers under
#               #ifdef ESP_PLATFORM guards (device-only capability bridges)
#             - blusys/hal/log.h and blusys/hal/error.h are cross-cutting
#               utilities allowed throughout the framework
#
# Exit 0 on success, 1 on any violation.

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
blusys_src="$repo_root/components/blusys/src"
blusys_inc="$repo_root/components/blusys/include/blusys"
status=0

report_hits() {
    local title="$1"
    shift
    printf 'lint: FAIL — %s\n' "$title" >&2
    for hit in "$@"; do
        printf '  %s\n' "$hit" >&2
    done
    status=1
}

# ---------------------------------------------------------------------------
# Rule 1 — HAL cannot depend on layers above
# ---------------------------------------------------------------------------
mapfile -t rule1_hits < <(
    grep -rEn --include='*.c' --include='*.h' \
        '#include[[:space:]]*[<"]blusys/(drivers|services|framework)/' \
        "$blusys_src/hal" \
        "$blusys_inc/hal" \
        2>/dev/null || true
)

if [[ ${#rule1_hits[@]} -gt 0 ]]; then
    report_hits 'Rule 1: HAL must not include blusys/drivers/, blusys/services/, or blusys/framework/' \
        "${rule1_hits[@]}"
fi

# ---------------------------------------------------------------------------
# Rule 2 — Drivers cannot depend on services or framework
# ---------------------------------------------------------------------------
mapfile -t rule2_hits < <(
    grep -rEn --include='*.c' --include='*.h' \
        '#include[[:space:]]*[<"]blusys/(services|framework)/' \
        "$blusys_src/drivers" \
        "$blusys_inc/drivers" \
        2>/dev/null || true
)

if [[ ${#rule2_hits[@]} -gt 0 ]]; then
    report_hits 'Rule 2: Drivers must not include blusys/services/ or blusys/framework/' \
        "${rule2_hits[@]}"
fi

# ---------------------------------------------------------------------------
# Rule 3 — Services cannot depend on framework
# ---------------------------------------------------------------------------
mapfile -t rule3_hits < <(
    grep -rEn --include='*.c' --include='*.h' \
        '#include[[:space:]]*[<"]blusys/framework/' \
        "$blusys_src/services" \
        "$blusys_inc/services" \
        2>/dev/null || true
)

if [[ ${#rule3_hits[@]} -gt 0 ]]; then
    report_hits 'Rule 3: Services must not include blusys/framework/' \
        "${rule3_hits[@]}"
fi

# ---------------------------------------------------------------------------
# Rule 4 — Pure framework sub-dirs cannot depend on lower layers.
#
# Pure sub-dirs: engine/, feedback/, ui/, app/, flows/
# Excluded: platform/ (structural escape hatch)
#           capabilities/ (device-conditional bridges under #ifdef ESP_PLATFORM)
#
# Cross-cutting utilities allowed everywhere:
#   blusys/hal/log.h   — logging (analogous to stdio.h)
#   blusys/hal/error.h — blusys_err_t return type (analogous to errno.h)
# ---------------------------------------------------------------------------
pure_dirs=(
    "$blusys_src/framework/engine"
    "$blusys_src/framework/feedback"
    "$blusys_src/framework/ui"
    "$blusys_src/framework/app"
    "$blusys_src/framework/flows"
    "$blusys_inc/framework/engine"
    "$blusys_inc/framework/feedback"
    "$blusys_inc/framework/ui"
    "$blusys_inc/framework/app"
    "$blusys_inc/framework/flows"
)

# Build grep arguments for only directories that exist.
existing_pure_dirs=()
for d in "${pure_dirs[@]}"; do
    [[ -d "$d" ]] && existing_pure_dirs+=("$d")
done

if [[ ${#existing_pure_dirs[@]} -gt 0 ]]; then
    mapfile -t rule4_raw < <(
        grep -rEn --include='*.hpp' --include='*.cpp' \
            '#include[[:space:]]*[<"]blusys/(hal|drivers|services)/' \
            "${existing_pure_dirs[@]}" \
            2>/dev/null || true
    )

    rule4_hits=()
    for hit in "${rule4_raw[@]}"; do
        # Allow blusys/hal/log.h and blusys/hal/error.h everywhere in framework.
        case "$hit" in
            *'blusys/hal/log.h"'|*'blusys/hal/log.h>'|\
            *'blusys/hal/error.h"'|*'blusys/hal/error.h>') continue ;;
        esac
        # Allow framework/app/entry.hpp: the app entry-point wires device drivers
        # under #ifdef ESP_PLATFORM guards — architecturally equivalent to platform/.
        case "$hit" in
            */framework/app/entry.hpp:*) continue ;;
        esac
        rule4_hits+=("$hit")
    done

    if [[ ${#rule4_hits[@]} -gt 0 ]]; then
        report_hits \
            'Rule 4: Pure framework (engine/feedback/ui/app/flows) must not include blusys/hal/, blusys/drivers/, or blusys/services/ (except log.h / error.h)' \
            "${rule4_hits[@]}"
    fi
fi

# ---------------------------------------------------------------------------
if [[ $status -eq 0 ]]; then
    printf 'lint: layering OK (Rules 1–4)\n'
fi

exit $status
