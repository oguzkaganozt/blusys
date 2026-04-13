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

# Interface/capability/policy matrix
run_create hh --interface handheld .
run_create hs --interface handheld --with storage .
run_create hb --interface handheld --with bluetooth,storage .
run_create sd --interface surface --with connectivity,diagnostics .
run_create ht --interface headless --with connectivity,telemetry,ota,diagnostics .
run_create hl --interface headless --with connectivity,lan_control,ota .
run_create hu --interface headless --with usb .
run_create hp --interface headless --with connectivity,telemetry --policy low_power .

for name in hh hs hb sd ht hl hu hp; do
    run_host "$name"
done

printf '\nscaffold-smoke: all passed (%s)\n' "$TMP"
