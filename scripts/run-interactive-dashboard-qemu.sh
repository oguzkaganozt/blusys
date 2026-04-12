#!/usr/bin/env bash
# Run examples/reference/interactive_dashboard in Espressif QEMU with the same
# emulated peripherals as `blusys qemu` (user-mode Ethernet, WDT disabled on
# the emulated timer), and — on esp32s3 / esp32c3 — ESP-IDF's virtual framebuffer
# (`idf.py qemu --graphics monitor`).
#
# Prerequisites:
#   - ESP-IDF v5.5+ (export.sh or PATH), QEMU from idf_tools (`blusys install-qemu`)
#   - For --graphics: host SDL2 libs (see ESP-IDF QEMU guide)
#
# Usage:
#   ./scripts/run-interactive-dashboard-qemu.sh [esp32|esp32c3|esp32s3]
#   ./scripts/run-interactive-dashboard-qemu.sh --serial-only [target]
#
# Environment:
#   BLUSYS_QEMU_GRAPHICS=0  Same as --serial-only (no SDL framebuffer window)
#
# Note: Hardware SPI/ILI profiles are not emulated as real panels. QEMU's extra
# window shows the virtual RGB panel only if firmware uses esp_lcd_qemu_rgb
# (see ESP-IDF "QEMU Emulator" guide). Until then you still get full UART logs
# via IDF Monitor; for LVGL iteration without hardware, prefer `blusys host-build`
# on this example.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
export BLUSYS_PATH="$REPO_ROOT"

# shellcheck source=scripts/lib/blusys/common.sh
. "$REPO_ROOT/scripts/lib/blusys/common.sh"

blusys_setup_idf_env

TARGET="esp32s3"
SERIAL_ONLY=0
if [[ "${BLUSYS_QEMU_GRAPHICS:-1}" == "0" ]]; then
    SERIAL_ONLY=1
fi

usage() {
    printf 'Usage: %s [--serial-only] [esp32|esp32c3|esp32s3]\n' "$(basename "$0")" >&2
    printf '  Default target: esp32s3\n' >&2
    printf '  --serial-only   idf.py qemu monitor only (no --graphics / no SDL window)\n' >&2
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        -h | --help)
            usage
            exit 0
            ;;
        --serial-only)
            SERIAL_ONLY=1
            shift
            ;;
        esp32 | esp32c3 | esp32s3)
            TARGET="$1"
            shift
            ;;
        *)
            printf 'error: unknown argument: %s\n' "$1" >&2
            usage
            exit 2
            ;;
    esac
done

PROJECT_DIR="$REPO_ROOT/examples/reference/interactive_dashboard"
BUILD_DIR="$PROJECT_DIR/build-${TARGET}"

case "$TARGET" in
    esp32)
        timer_driver="timer.esp32.timg"
        ;;
    esp32s3)
        timer_driver="timer.esp32c3.timg"
        ;;
    esp32c3)
        timer_driver="timer.esp32c3.timg"
        ;;
    *)
        printf 'error: unsupported target: %s\n' "$TARGET" >&2
        exit 2
        ;;
esac

QEMU_EXTRA="-global driver=${timer_driver},property=wdt_disable,value=true -nic user,model=open_eth"
if [[ "$TARGET" == "esp32c3" ]]; then
    QEMU_EXTRA+=" -icount 3"
fi

printf '=== Interactive dashboard QEMU (%s) ===\n' "$TARGET"
printf 'Project:  %s\n' "$PROJECT_DIR"
printf 'Build:    %s\n' "$BUILD_DIR"

idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" set-target "$TARGET"
idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" build

if [[ "$SERIAL_ONLY" -eq 1 ]]; then
    printf 'Launching: idf.py qemu (serial monitor, extras match blusys qemu)\n'
    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" qemu --qemu-extra-args="$QEMU_EXTRA" monitor
else
    case "$TARGET" in
        esp32s3 | esp32c3)
            printf 'Launching: idf.py qemu --graphics monitor (virtual framebuffer + UART)\n'
            idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" qemu --qemu-extra-args="$QEMU_EXTRA" --graphics monitor
            ;;
        esp32)
            printf 'note: --graphics is only documented for esp32s3 in IDF; using serial monitor only.\n' >&2
            idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" qemu --qemu-extra-args="$QEMU_EXTRA" monitor
            ;;
    esac
fi
