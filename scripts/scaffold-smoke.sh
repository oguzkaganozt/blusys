#!/usr/bin/env bash
# Smoke-test `blusys create` for the canonical interface/capability matrix,
# then exercise the docs quickstart walkthrough from a fresh clone and
# host-build each generated project. Reuses a single FetchContent base dir so
# interactive projects share one LVGL checkout.
#
# Usage: from repo root, with cmake + pkg-config + libsdl2-dev:
#   ./scripts/scaffold-smoke.sh
#
# Env:
#   BLUSYS_PATH   defaults to repo root (the `blusys` launcher also exports this)

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
export BLUSYS_PATH="${BLUSYS_PATH:-$ROOT}"

TMP="${TMPDIR:-/tmp}/blusys-scaffold-smoke-$$"
mkdir -p "$TMP"
cleanup() { rm -rf "$TMP"; }
trap cleanup EXIT

export FETCHCONTENT_BASE_DIR="${FETCHCONTENT_BASE_DIR:-$TMP/fetch-content-cache}"
mkdir -p "$FETCHCONTENT_BASE_DIR"

BLUSYS="$ROOT/blusys"
if [[ ! -x "$BLUSYS" ]] && [[ -f "$BLUSYS" ]]; then
    chmod +x "$BLUSYS" || true
fi

run_create() {
    local name="$1"
    shift
    local dir="$TMP/$name"
    mkdir -p "$dir"
    local abs
    abs="$(cd "$dir" && pwd)"
    local args=()
    local a
    for a in "$@"; do
        if [[ "$a" == . ]]; then
            args+=("$abs")
        else
            args+=("$a")
        fi
    done
    (cd "$dir" && "$BLUSYS" create "${args[@]}")
    printf 'scaffold-smoke: created %s\n' "$name"
}

run_host() {
    local name="$1"
    local dir="$TMP/$name"
    (cd "$ROOT" && "$BLUSYS" host-build "$dir")
    local generated_header="$dir/build-host/generated/blusys_app_spec.h"
    if [[ ! -f "$generated_header" ]]; then
        printf 'scaffold-smoke: missing generated app spec in %s\n' "$name" >&2
        exit 1
    fi
    printf 'scaffold-smoke: host-build ok %s\n' "$name"
}

run_cold_onboarding() {
    local clone="$TMP/cold-onboarding"
    git clone --quiet "$ROOT" "$clone"

    local clone_blusys="$clone/blusys"
    if [[ ! -x "$clone_blusys" ]] && [[ -f "$clone_blusys" ]]; then
        chmod +x "$clone_blusys" || true
    fi

    local default_dir="$clone/default"
    mkdir -p "$default_dir"
    (cd "$default_dir" && "$clone_blusys" create)
    (cd "$default_dir" && "$clone_blusys" host-build)
    if [[ ! -f "$default_dir/build-host/generated/blusys_app_spec.h" ]]; then
        printf 'scaffold-smoke: missing generated app spec in cold default walkthrough\n' >&2
        exit 1
    fi
    printf 'scaffold-smoke: cold default host-build ok\n'

    local headless_dir="$clone/headless"
    mkdir -p "$headless_dir"
    (cd "$headless_dir" && "$clone_blusys" create --interface headless my_sensor)
    (cd "$headless_dir/my_sensor" && "$clone_blusys" host-build)
    if [[ ! -f "$headless_dir/my_sensor/build-host/generated/blusys_app_spec.h" ]]; then
        printf 'scaffold-smoke: missing generated app spec in cold headless walkthrough\n' >&2
        exit 1
    fi
    printf 'scaffold-smoke: cold headless host-build ok\n'

    local interactive_dir="$clone/interactive"
    mkdir -p "$interactive_dir"
    (cd "$interactive_dir" && "$clone_blusys" create --interface interactive my_product)
    (cd "$interactive_dir/my_product" && "$clone_blusys" host-build)
    if [[ ! -f "$interactive_dir/my_product/build-host/generated/blusys_app_spec.h" ]]; then
        printf 'scaffold-smoke: missing generated app spec in cold interactive walkthrough\n' >&2
        exit 1
    fi
    printf 'scaffold-smoke: cold interactive host-build ok\n'

    printf 'scaffold-smoke: cold-onboarding ok\n'
}

# Tests scripts/scaffold/new-capability.sh. Scaffolded host.cpp and device.cpp
# share an identical impl body (same template), so one syntax check covers both.
run_capability_scaffold() {
    local name="$1"
    local probe_root="$TMP/cap_scaffold_$name"
    mkdir -p "$probe_root"
    "$ROOT/scripts/scaffold/new-capability.sh" "$name" "$probe_root"
    "${CXX:-g++}" -fsyntax-only -std=c++20 \
        -I"$ROOT/components/blusys/include" \
        -I"$probe_root/include" \
        "$probe_root/src/framework/capabilities/${name}_host.cpp"
    printf 'scaffold-smoke: capability-scaffold ok %s\n' "$name"
}

assert_list_surface() {
    local line_count
    line_count="$($BLUSYS create --list | wc -l)"
    if [[ "$line_count" -gt 25 ]]; then
        printf 'scaffold-smoke: create --list exceeds one screen (%s lines)\n' "$line_count" >&2
        exit 1
    fi
}

assert_layout() {
    local name="$1"
    local expected_total="$2"
    local expected_main="$3"
    local expect_ui="$4"
    local dir="$TMP/$name"
    local total_files
    local main_files

    total_files="$(find "$dir" -type f | wc -l)"
    main_files="$(find "$dir/main" -type f | wc -l)"

    if [[ "$total_files" -ne "$expected_total" ]]; then
        printf 'scaffold-smoke: expected %s files in %s, got %s\n' "$expected_total" "$name" "$total_files" >&2
        exit 1
    fi
    if [[ "$main_files" -ne "$expected_main" ]]; then
        printf 'scaffold-smoke: expected %s main files in %s, got %s\n' "$expected_main" "$name" "$main_files" >&2
        exit 1
    fi
    if [[ ! -f "$dir/main/app_main.cpp" ]]; then
        printf 'scaffold-smoke: missing app_main.cpp in %s\n' "$name" >&2
        exit 1
    fi
    if [[ ! -f "$dir/.gitignore" ]]; then
        printf 'scaffold-smoke: missing .gitignore in %s\n' "$name" >&2
        exit 1
    fi
    if [[ -d "$dir/main/core" || -d "$dir/main/platform" ]]; then
        printf 'scaffold-smoke: legacy core/platform dirs remain in %s\n' "$name" >&2
        exit 1
    fi
    if [[ "$expect_ui" == "1" ]]; then
        if [[ ! -f "$dir/main/ui/CMakeLists.txt" || ! -f "$dir/main/ui/app_ui.cpp" ]]; then
            printf 'scaffold-smoke: missing interactive ui files in %s\n' "$name" >&2
            exit 1
        fi
    else
        if [[ -e "$dir/main/ui" ]]; then
            printf 'scaffold-smoke: unexpected ui dir in %s\n' "$name" >&2
            exit 1
        fi
    fi
}

# Interface/capability/policy matrix
assert_list_surface
run_create ii --interface interactive .
assert_layout ii 12 5 1
run_create is --interface interactive --with storage .
assert_layout is 12 5 1
run_create ib --interface interactive --with bluetooth,storage .
assert_layout ib 12 5 1
run_create id --interface interactive --with connectivity,diagnostics .
assert_layout id 12 5 1
run_create ht --interface headless --with connectivity,telemetry,ota,diagnostics .
assert_layout ht 10 3 0
run_create hl --interface headless --with connectivity,lan_control,ota .
assert_layout hl 10 3 0
run_create hu --interface headless --with usb .
assert_layout hu 10 3 0
run_create hp --interface headless --with connectivity,telemetry --policy low_power .
assert_layout hp 10 3 0

for name in ii is ib id ht hl hu hp; do
    run_host "$name"
done

run_cold_onboarding

run_capability_scaffold cap_smoke_probe

printf '\nscaffold-smoke: all passed (%s)\n' "$TMP"
