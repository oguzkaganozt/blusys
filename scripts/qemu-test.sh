#!/usr/bin/env bash
# QEMU test runner for blusys examples.
#
# Usage: qemu-test.sh <example_dir> <target> <timeout_seconds> <expect_pattern>
#
# Builds the example, creates a 4MB flash image, runs QEMU with a timeout,
# and checks output for expected/unexpected patterns.
#
# Exit codes:
#   0 = pass  (expected pattern found, no crash patterns)
#   1 = fail  (crash detected or expected pattern missing)
#   2 = build error
set -euo pipefail

EXAMPLE_DIR="$(realpath "$1")"
TARGET="$2"
TIMEOUT="${3:-30}"
EXPECT_PATTERN="$4"
FAIL_PATTERN="${5:-Guru Meditation Error|panic|abort\(\)|assert failed|LoadProhibited|StoreProhibited|IllegalInstruction}"

BUILD_DIR="$EXAMPLE_DIR/build-$TARGET"
EXAMPLE_NAME="$(basename "$EXAMPLE_DIR")"

# ── sdkconfig args (mirrors blusys_sdkconfig_args) ──────────────────────────

sdkconfig_args=("-DSDKCONFIG=$BUILD_DIR/sdkconfig")
defaults=""
[ -f "$EXAMPLE_DIR/sdkconfig.defaults" ] && defaults="$EXAMPLE_DIR/sdkconfig.defaults"
if [ -f "$EXAMPLE_DIR/sdkconfig.$TARGET" ]; then
    if [ -n "$defaults" ]; then
        defaults="$defaults;$EXAMPLE_DIR/sdkconfig.$TARGET"
    else
        defaults="$EXAMPLE_DIR/sdkconfig.$TARGET"
    fi
fi
[ -n "$defaults" ] && sdkconfig_args+=("-DSDKCONFIG_DEFAULTS=$defaults")

# ── QEMU binary and machine flags (mirrors cmd_qemu) ────────────────────────

case "$TARGET" in
    esp32)
        qemu_bin="qemu-system-xtensa"
        machine="esp32"
        timer_driver="timer.esp32.timg"
        extra_flags=""
        ;;
    esp32s3)
        qemu_bin="qemu-system-xtensa"
        machine="esp32s3"
        timer_driver="timer.esp32c3.timg"
        extra_flags=""
        ;;
    esp32c3)
        qemu_bin="qemu-system-riscv32"
        machine="esp32c3"
        timer_driver="timer.esp32c3.timg"
        extra_flags="-icount 3"
        ;;
    *)
        printf 'error: unsupported target: %s\n' "$TARGET" >&2
        exit 2
        ;;
esac

printf '=== QEMU Test: %s / %s ===\n' "$EXAMPLE_NAME" "$TARGET"

# ── Step 1: Build ───────────────────────────────────────────────────────────

printf 'Building %s for %s...\n' "$EXAMPLE_NAME" "$TARGET"
if ! idf.py -C "$EXAMPLE_DIR" -B "$BUILD_DIR" set-target "$TARGET" "${sdkconfig_args[@]}" > /dev/null 2>&1; then
    # set-target may fail if already set — try building anyway
    true
fi
if ! idf.py -C "$EXAMPLE_DIR" -B "$BUILD_DIR" "${sdkconfig_args[@]}" build > /dev/null 2>&1; then
    printf 'FAIL: build error\n'
    exit 2
fi

# ── Step 2: Create flash image ──────────────────────────────────────────────

printf 'Creating flash image...\n'
# flash_args uses relative paths — must run from BUILD_DIR
(cd "$BUILD_DIR" && esptool.py --chip "$TARGET" merge_bin --fill-flash-size 4MB \
    -o "$BUILD_DIR/flash_image.bin" \
    @flash_args) > /dev/null 2>&1

# ── Step 3: Run QEMU with timeout ───────────────────────────────────────────

output_file="$(mktemp)"
trap 'rm -f "$output_file"' EXIT

printf 'Running QEMU (timeout=%ss)...\n' "$TIMEOUT"
set +e
# shellcheck disable=SC2086
timeout "$TIMEOUT" "$qemu_bin" \
    -nographic \
    -no-reboot \
    -machine "$machine" \
    -drive "file=$BUILD_DIR/flash_image.bin,if=mtd,format=raw" \
    -nic "user,model=open_eth" \
    -global "driver=$timer_driver,property=wdt_disable,value=true" \
    $extra_flags \
    > "$output_file" 2>&1
qemu_exit=$?
set -e

# ── Step 4: Check output ────────────────────────────────────────────────────

# Check for crash/panic patterns first
if grep -qE "$FAIL_PATTERN" "$output_file" 2>/dev/null; then
    printf 'FAIL: crash/panic detected\n'
    printf '--- Last 30 lines ---\n'
    tail -30 "$output_file"
    exit 1
fi

# Check for expected success pattern
if grep -qE "$EXPECT_PATTERN" "$output_file" 2>/dev/null; then
    printf 'PASS\n'
    exit 0
fi

# Expected pattern not found
printf 'FAIL: expected pattern not found: %s\n' "$EXPECT_PATTERN"
printf '--- Last 30 lines ---\n'
tail -30 "$output_file"
exit 1
