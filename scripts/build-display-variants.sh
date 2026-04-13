#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    printf 'Usage: build-display-variants.sh <target>\n' >&2
    exit 2
fi

TARGET="$1"
REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"

tmp_files=()
cleanup() {
    if [[ ${#tmp_files[@]} -gt 0 ]]; then
        rm -f "${tmp_files[@]}"
    fi
}
trap cleanup EXIT

make_fragment() {
    local file
    file="$(mktemp)"
    tmp_files+=("$file")
    printf '%s\n' "$@" > "$file"
    printf '%s' "$file"
}

build_variant() {
    local example_dir="$1"
    local build_dir_name="$2"
    local fragment="$3"
    local label="$4"

    local build_dir="$example_dir/$build_dir_name"
    local defaults="sdkconfig.defaults;$fragment"

    printf '::group::%s\n' "$label"
    rm -rf "$build_dir"

    if idf.py -C "$example_dir" -B "$build_dir" \
        -DSDKCONFIG="$build_dir/sdkconfig" \
        -DSDKCONFIG_DEFAULTS="$defaults" \
        set-target "$TARGET" && \
        idf.py -C "$example_dir" -B "$build_dir" \
            -DSDKCONFIG="$build_dir/sdkconfig" \
            -DSDKCONFIG_DEFAULTS="$defaults" \
            build; then
        printf 'OK\n'
    else
        printf '::error::Display variant build failed: %s\n' "$label"
        printf '::endgroup::\n'
        return 1
    fi

    printf '::endgroup::\n'
}

controller_fragment="$(make_fragment \
    'CONFIG_BLUSYS_INTERACTIVE_DISPLAY_PROFILE_ST7789=y' \
    '# CONFIG_BLUSYS_INTERACTIVE_DISPLAY_PROFILE_ST7735 is not set')"

panel_fragment="$(make_fragment \
    'CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9488=y' \
    '# CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9341 is not set' \
    '# CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ST7735 is not set' \
    '# CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306 is not set' \
    '# CONFIG_BLUSYS_DASHBOARD_LCD_QEMU_RGB is not set')"

surface_gateway_fragment="$(make_fragment \
    'CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9488=y' \
    '# CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ILI9341 is not set' \
    '# CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_ST7735 is not set' \
    '# CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306 is not set' \
    '# CONFIG_BLUSYS_DASHBOARD_LCD_QEMU_RGB is not set')"

edge_fragment="$(make_fragment \
    'CONFIG_BLUSYS_HEADLESS_TELEMETRY_LOCAL_UI=y')"

build_variant \
    "$REPO_ROOT/examples/quickstart/handheld_starter" \
    "build-$TARGET-display-st7789" \
    "$controller_fragment" \
    "handheld_starter $TARGET ST7789"

build_variant \
    "$REPO_ROOT/examples/reference/surface_ops_panel" \
    "build-$TARGET-display-ili9488" \
    "$panel_fragment" \
    "surface_ops_panel $TARGET ILI9488"

build_variant \
    "$REPO_ROOT/examples/reference/surface_gateway" \
    "build-$TARGET-display-ili9488" \
    "$surface_gateway_fragment" \
    "surface_gateway $TARGET ILI9488"

build_variant \
    "$REPO_ROOT/examples/quickstart/headless_telemetry" \
    "build-$TARGET-display-local-ui" \
    "$edge_fragment" \
    "headless_telemetry $TARGET SSD1306 local UI"
