#!/usr/bin/env bash

set -euo pipefail

BLUSYS_DIR="$(cd "$(dirname "$0")" && pwd)"
BLUSYS_BIN="$BLUSYS_DIR/blusys"
BLUSYS_COMPLETION="$BLUSYS_DIR/scripts/completions/blusys.bash"
BLUSYS_HOOK_TEMPLATE="$BLUSYS_DIR/scripts/git-hooks/pre-commit"
BLUSYS_HOOK_PATH="$BLUSYS_DIR/.git/hooks/pre-commit"
LINK_DIR="$HOME/.local/bin"
LINK_PATH="$LINK_DIR/blusys"
COMP_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/bash-completion/completions"
COMP_PATH="$COMP_DIR/blusys"

install_git_hook() {
    if [[ ! -f "$BLUSYS_HOOK_TEMPLATE" ]]; then
        return 0
    fi

    if ! git -C "$BLUSYS_DIR" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
        return 0
    fi

    mkdir -p "$(dirname "$BLUSYS_HOOK_PATH")"

    if [[ -e "$BLUSYS_HOOK_PATH" ]]; then
        local existing_hook
        existing_hook="$(readlink -f "$BLUSYS_HOOK_PATH" 2>/dev/null || true)"
        if [[ "$existing_hook" = "$BLUSYS_HOOK_TEMPLATE" ]]; then
            printf 'Git hook already installed: %s\n' "$BLUSYS_HOOK_PATH"
        else
            printf 'warning: %s already exists; not overwriting\n' "$BLUSYS_HOOK_PATH" >&2
        fi
        return 0
    fi

    ln -s "$BLUSYS_HOOK_TEMPLATE" "$BLUSYS_HOOK_PATH"
    chmod +x "$BLUSYS_HOOK_TEMPLATE" || true
    printf 'Installed git hook: %s\n' "$BLUSYS_HOOK_PATH"
}

if [[ ! -x "$BLUSYS_BIN" ]]; then
    printf 'error: blusys not found at %s\n' "$BLUSYS_BIN" >&2
    exit 1
fi

# Create ~/.local/bin if it doesn't exist
mkdir -p "$LINK_DIR"

# Create or update symlink
if [[ -L "$LINK_PATH" ]]; then
    existing="$(readlink -f "$LINK_PATH" 2>/dev/null || true)"
    if [[ "$existing" = "$BLUSYS_BIN" ]]; then
        printf 'blusys is already installed at %s\n' "$LINK_PATH"
        printf 'blusys path: %s\n' "$BLUSYS_DIR"
        # Still update completion in case it changed.
        if [[ -f "$BLUSYS_COMPLETION" ]]; then
            mkdir -p "$COMP_DIR"
            ln -sf "$BLUSYS_COMPLETION" "$COMP_PATH"
        fi
        install_git_hook
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

# Install bash completion
if [[ -f "$BLUSYS_COMPLETION" ]]; then
    mkdir -p "$COMP_DIR"
    ln -sf "$BLUSYS_COMPLETION" "$COMP_PATH"
    printf 'Installed bash completion: %s\n' "$COMP_PATH"
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

install_git_hook
