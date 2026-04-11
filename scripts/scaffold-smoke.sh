#!/usr/bin/env bash
# Smoke-test `blusys create` for all canonical archetypes: copy templates, then host-build.
# Reuses a single FetchContent base dir so interactive projects share one LVGL checkout.
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
    (cd "$dir" && "$BLUSYS" create "$@")
    printf 'scaffold-smoke: created %s\n' "$name"
}

run_host() {
    local name="$1"
    local dir="$TMP/$name"
    (cd "$ROOT" && "$BLUSYS" host-build "$dir")
    printf 'scaffold-smoke: host-build ok %s\n' "$name"
}

# Four archetypes + gateway interactive variant (distinct template path)
run_create ic --archetype interactive-controller .
run_create ip --archetype interactive-panel .
run_create en --archetype edge-node .
run_create gw --archetype gateway-controller .
run_create gwi --archetype gateway-controller --starter interactive .

for name in ic ip en gw gwi; do
    run_host "$name"
done

printf '\nscaffold-smoke: all passed (%s)\n' "$TMP"
