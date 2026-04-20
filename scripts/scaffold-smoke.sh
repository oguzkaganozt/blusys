#!/usr/bin/env bash
# Smoke-test `blusys create` for the canonical interface/capability matrix,
# then host-build each generated project. Reuses a single FetchContent base dir
# so interactive projects share one LVGL checkout.
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
    mkdir "$dir"
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
    printf 'scaffold-smoke: host-build ok %s\n' "$name"
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

# Interface/capability/policy matrix
run_create ii --interface interactive .
run_create is --interface interactive --with storage .
run_create ib --interface interactive --with bluetooth,storage .
run_create id --interface interactive --with connectivity,diagnostics .
run_create ht --interface headless --with connectivity,telemetry,ota,diagnostics .
run_create hl --interface headless --with connectivity,lan_control,ota .
run_create hu --interface headless --with usb .
run_create hp --interface headless --with connectivity,telemetry --policy low_power .

for name in ii is ib id ht hl hu hp; do
    run_host "$name"
done

run_capability_scaffold cap_smoke_probe

printf '\nscaffold-smoke: all passed (%s)\n' "$TMP"
