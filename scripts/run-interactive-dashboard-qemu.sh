#!/usr/bin/env bash
# Compatibility wrapper for examples/reference/interactive_dashboard in QEMU with
# esp_lcd_qemu_rgb. Prefer the idiomatic CLI:
#
#   blusys qemu examples/reference/interactive_dashboard qemu32s3 --graphics
#   blusys qemu … qemu32s3 --serial-only
#
# BLUSYS_QEMU_GRAPHICS=0 is the same as --serial-only (historical env).
set -euo pipefail

REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
EXAMPLE="$REPO/examples/reference/interactive_dashboard"

SERIAL=0
[[ "${BLUSYS_QEMU_GRAPHICS:-1}" == "0" ]] && SERIAL=1
CHIP=esp32s3

usage() {
    printf 'Usage: %s [--serial-only] [esp32|esp32c3|esp32s3]\n' "$(basename "$0")" >&2
    printf 'Prefer: blusys qemu %s qemu32s3 [--graphics|--serial-only]\n' "$EXAMPLE" >&2
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h | --help)
            usage
            exit 0
            ;;
        --serial-only)
            SERIAL=1
            shift
            ;;
        esp32 | esp32c3 | esp32s3)
            CHIP="$1"
            shift
            ;;
        *)
            printf 'error: unknown argument: %s\n' "$1" >&2
            usage
            exit 2
            ;;
    esac
done

case "$CHIP" in
    esp32) Q=qemu32 ;;
    esp32c3) Q=qemu32c3 ;;
    esp32s3) Q=qemu32s3 ;;
esac

if [[ "$SERIAL" -eq 1 ]]; then
    exec "$REPO/blusys" qemu "$EXAMPLE" "$Q" --serial-only
else
    exec "$REPO/blusys" qemu "$EXAMPLE" "$Q" --graphics
fi
