#!/usr/bin/env bash

set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
status=0

report_hits() {
    local title="$1"
    shift

    printf 'lint: %s\n' "$title" >&2
    for hit in "$@"; do
        printf '  %s\n' "$hit" >&2
    done
    status=1
}

mapfile -t common_hits < <(
    grep -rEn --include='*.c' --include='*.h' \
        --exclude-dir=drivers --exclude-dir=ulp --exclude-dir=targets \
        '#include[[:space:]]*[<"]blusys/drivers/' \
        "$repo_root/components/blusys_hal/src" || true
)

if [[ ${#common_hits[@]} -gt 0 ]]; then
    report_hits 'HAL files must not include blusys/drivers/**' "${common_hits[@]}"
fi

mapfile -t driver_internal_hits < <(
    grep -rEn --include='*.c' --include='*.h' \
        '#include[[:space:]]*[<"]blusys/internal/[^>"]+[>"]' \
        "$repo_root/components/blusys_hal/src/drivers" || true
)

disallowed_driver_hits=()
for hit in "${driver_internal_hits[@]}"; do
    case "$hit" in
        *'blusys/internal/blusys_lock.h"'|*'blusys/internal/blusys_lock.h>')   ;;
        *'blusys/internal/blusys_esp_err.h"'|*'blusys/internal/blusys_esp_err.h>') ;;
        *'blusys/internal/blusys_timeout.h"'|*'blusys/internal/blusys_timeout.h>') ;;
        *'blusys/internal/lcd_panel_ili.h"'|*'blusys/internal/lcd_panel_ili.h>') ;;
        *)
            disallowed_driver_hits+=("$hit")
            ;;
    esac
done

if [[ ${#disallowed_driver_hits[@]} -gt 0 ]]; then
    report_hits 'driver files may only include the shared internal allowlist' "${disallowed_driver_hits[@]}"
fi

if [[ $status -eq 0 ]]; then
    printf 'lint: layering OK\n'
fi

exit $status
