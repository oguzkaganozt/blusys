#!/usr/bin/env bash

set -euo pipefail

BLUSYS_DIR="$(cd "$(dirname "$0")" && pwd)"
BLUSYS_BIN="$BLUSYS_DIR/blusys"
LINK_DIR="$HOME/.local/bin"
LINK_PATH="$LINK_DIR/blusys"

if [[ ! -x "$BLUSYS_BIN" ]]; then
    printf 'error: blusys not found at %s\n' "$BLUSYS_BIN" >&2
    exit 1
fi

# Create ~/.local/bin if it doesn't exist
mkdir -p "$LINK_DIR"

# Create or update symlink
if [[ -L "$LINK_PATH" ]]; then
    existing="$(readlink -f "$LINK_PATH")"
    if [[ "$existing" = "$BLUSYS_BIN" ]]; then
        printf 'blusys is already installed at %s\n' "$LINK_PATH"
        printf 'blusys path: %s\n' "$BLUSYS_DIR"
        exit 0
    fi
    printf 'Updating symlink: %s -> %s\n' "$LINK_PATH" "$BLUSYS_BIN"
    ln -sf "$BLUSYS_BIN" "$LINK_PATH"
elif [[ -e "$LINK_PATH" ]]; then
    printf 'error: %s already exists and is not a symlink\n' "$LINK_PATH" >&2
    printf '       remove it manually and re-run install.sh\n' >&2
    exit 1
else
    printf 'Creating symlink: %s -> %s\n' "$LINK_PATH" "$BLUSYS_BIN"
    ln -s "$BLUSYS_BIN" "$LINK_PATH"
fi

# Check if ~/.local/bin is in PATH
if ! echo "$PATH" | tr ':' '\n' | grep -qx "$LINK_DIR"; then
    printf '\nwarning: %s is not in your PATH\n' "$LINK_DIR"
    printf 'Add it by running:\n'
    printf '  echo '\''export PATH="$HOME/.local/bin:$PATH"'\'' >> ~/.bashrc\n'
    printf '  source ~/.bashrc\n'
else
    printf '\nDone. Run "blusys version" to verify.\n'
fi
