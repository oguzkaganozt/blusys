# Sourced by repo-root `blusys` — all commands except `create`, plus `main`.

cmd_version() {
    local blusys_version
    blusys_version="$(grep '#define BLUSYS_VERSION_STRING' \
        "$BLUSYS_REPO_ROOT/components/blusys_hal/include/blusys/version.h" 2>/dev/null \
        | sed 's/.*"\(.*\)"/\1/')" || true
    printf 'blusys %s\n' "${blusys_version:-unknown}"
    printf 'blusys path: %s\n' "$BLUSYS_REPO_ROOT"

    local idf_path
    if idf_path="$(blusys_find_idf_path 2>/dev/null)"; then
        printf 'ESP-IDF path: %s\n' "$idf_path"
        if [[ -n "${IDF_VERSION:-}" ]]; then
            printf 'ESP-IDF version: %s\n' "$IDF_VERSION"
        elif [[ -f "$idf_path/version.txt" ]]; then
            printf 'ESP-IDF version: %s\n' "$(cat "$idf_path/version.txt")"
        fi
    else
        printf 'ESP-IDF: not found\n'
    fi

    local qemu_path
    if qemu_path="$(blusys_find_qemu 2>/dev/null)"; then
        printf 'QEMU path: %s\n' "$qemu_path"
    fi

    if [[ -f "$BLUSYS_CONFIG_FILE" ]]; then
        printf 'config: %s\n' "$BLUSYS_CONFIG_FILE"
    fi
}

cmd_update() {
    printf 'Updating blusys...\n'
    git -C "$BLUSYS_REPO_ROOT" pull --ff-only || {
        printf 'error: update failed. You may have local changes.\n' >&2
        printf '       resolve with: cd %s && git pull\n' "$BLUSYS_REPO_ROOT" >&2
        return 1
    }
    printf 'Done.\n'
}

cmd_config_idf() {
    # Discover all ESP-IDF installations
    local -a idf_paths=() idf_labels=()

    for candidate_dir in "$HOME"/.espressif/*/esp-idf; do
        [[ -d "$candidate_dir" && -f "$candidate_dir/export.sh" ]] || continue
        local version_dir
        version_dir="$(basename "$(dirname "$candidate_dir")")"
        idf_paths+=("$candidate_dir")
        idf_labels+=("$version_dir")
    done

    if [[ ${#idf_paths[@]} -eq 0 ]]; then
        printf 'No ESP-IDF installations found in ~/.espressif/\n' >&2
        printf 'Install ESP-IDF: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/\n' >&2
        exit 1
    fi

    # Show current setting
    local current_idf=""
    if [[ -f "$BLUSYS_CONFIG_FILE" ]]; then
        current_idf="$(grep '^IDF_PATH=' "$BLUSYS_CONFIG_FILE" 2>/dev/null | cut -d= -f2-)" || true
    fi

    printf 'Detected ESP-IDF installations:\n'
    for i in "${!idf_paths[@]}"; do
        local marker="  "
        if [[ "${idf_paths[$i]}" = "$current_idf" ]]; then
            marker="* "
        fi
        printf '  %s%d) %-10s %s\n' "$marker" "$((i + 1))" "${idf_labels[$i]}" "${idf_paths[$i]}"
    done

    if [[ -n "$current_idf" ]]; then
        printf '\n  * = current default\n'
    fi

    printf '\nSelect default [1-%d]: ' "${#idf_paths[@]}"
    read -r choice

    if ! [[ "$choice" =~ ^[0-9]+$ ]] || [[ "$choice" -lt 1 || "$choice" -gt ${#idf_paths[@]} ]]; then
        printf 'Invalid selection.\n' >&2
        exit 1
    fi

    local selected_path="${idf_paths[$((choice - 1))]}"

    mkdir -p "$BLUSYS_CONFIG_DIR"

    # Write or update config file
    if [[ -f "$BLUSYS_CONFIG_FILE" ]]; then
        # Remove existing IDF_PATH line and append new one
        grep -v '^IDF_PATH=' "$BLUSYS_CONFIG_FILE" > "$BLUSYS_CONFIG_FILE.tmp" || true
        mv "$BLUSYS_CONFIG_FILE.tmp" "$BLUSYS_CONFIG_FILE"
    fi
    printf 'IDF_PATH=%s\n' "$selected_path" >> "$BLUSYS_CONFIG_FILE"

    printf 'Saved: IDF_PATH=%s\n' "$selected_path"
}

cmd_build() {
    if [[ $# -gt 2 ]]; then
        blusys_help_build
        exit 1
    fi

    blusys_setup_no_port "$@"
    blusys_setup_idf_env
    blusys_print_info

    blusys_set_target_if_needed "$PROJECT_DIR" "$BUILD_DIR" "$TARGET" "${SDKCONFIG_ARGS[@]}"
    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" build
}

cmd_flash() {
    if [[ $# -gt 3 ]]; then
        blusys_help_flash
        exit 1
    fi

    blusys_setup_with_port "$@"
    blusys_setup_idf_env
    blusys_print_info_port

    blusys_set_target_if_needed "$PROJECT_DIR" "$BUILD_DIR" "$TARGET" "${SDKCONFIG_ARGS[@]}"
    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" -p "$PORT" flash
}

cmd_monitor() {
    if [[ $# -gt 3 ]]; then
        blusys_help_monitor
        exit 1
    fi

    blusys_setup_with_port "$@"
    blusys_setup_idf_env
    blusys_print_info_port
    printf 'Monitor exit: press Ctrl+]\n'

    blusys_set_target_if_needed "$PROJECT_DIR" "$BUILD_DIR" "$TARGET" "${SDKCONFIG_ARGS[@]}"
    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" -p "$PORT" monitor
}

cmd_run() {
    if [[ $# -gt 3 ]]; then
        blusys_help_run
        exit 1
    fi

    blusys_setup_with_port "$@"
    blusys_setup_idf_env
    blusys_print_info_port
    printf 'Monitor exit: press Ctrl+]\n'

    blusys_set_target_if_needed "$PROJECT_DIR" "$BUILD_DIR" "$TARGET" "${SDKCONFIG_ARGS[@]}"
    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" -p "$PORT" build flash monitor
}

cmd_config() {
    if [[ $# -gt 2 ]]; then
        blusys_help_config
        exit 1
    fi

    blusys_setup_no_port "$@"
    blusys_setup_idf_env
    blusys_print_info

    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" set-target "$TARGET"
    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" menuconfig
}

cmd_clean() {
    if [[ $# -gt 2 ]]; then
        blusys_help_clean
        exit 1
    fi

    blusys_setup_no_port "$@"

    if [[ ! -d "$BUILD_DIR" ]]; then
        printf 'Nothing to clean: %s does not exist\n' "$BUILD_DIR"
        exit 0
    fi

    printf 'Removing: %s\n' "$BUILD_DIR"
    rm -rf "$BUILD_DIR"
    printf 'Done.\n'
}

cmd_size() {
    if [[ $# -gt 2 ]]; then
        blusys_help_size
        exit 1
    fi

    blusys_setup_no_port "$@"
    blusys_setup_idf_env
    blusys_print_info

    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" size
    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" size-components
}

cmd_erase() {
    if [[ $# -gt 3 ]]; then
        blusys_help_erase
        exit 1
    fi

    blusys_setup_with_port "$@"
    blusys_setup_idf_env
    blusys_print_info_port

    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" -p "$PORT" erase-flash
}

cmd_fullclean() {
    if [[ $# -gt 1 ]]; then
        blusys_help_fullclean
        exit 1
    fi

    local project="${1:-$BLUSYS_DEFAULT_PROJECT}"
    PROJECT_DIR="$(blusys_resolve_project_dir "$project")"
    local targets=(esp32 esp32c3 esp32s3)
    local removed=0

    for target in "${targets[@]}"; do
        local build_dir="$PROJECT_DIR/build-$target"
        if [[ -d "$build_dir" ]]; then
            printf 'Removing: %s\n' "$build_dir"
            rm -rf "$build_dir"
            ((removed++)) || true
        fi
    done

    if [[ $removed -eq 0 ]]; then
        printf 'Nothing to clean: no build directories found in %s\n' "$PROJECT_DIR"
    else
        printf 'Done. Removed %d build director%s.\n' "$removed" "$( [[ $removed -eq 1 ]] && printf 'y' || printf 'ies' )"
    fi
}

cmd_example() {
    local examples_dir="$BLUSYS_REPO_ROOT/examples"

    # Find all example directories (supports flat and categorized layouts)
    _find_examples() {
        for f in "$examples_dir"/*/CMakeLists.txt "$examples_dir"/*/*/CMakeLists.txt; do
            [[ -f "$f" ]] || continue
            printf '%s\n' "$(dirname "$f")"
        done | sort
    }

    _display_example_name() {
        local dir="$1"
        printf '%s\n' "${dir#"$examples_dir"/}"
    }

    # Resolve an example name to its path
    _resolve_example() {
        local target_name="$1"
        local found=""
        local match_count=0
        while IFS= read -r dir; do
            if [[ "$(_display_example_name "$dir")" == "$target_name" ]]; then
                found="$dir"
                break
            fi
            if [[ "$target_name" != */* && "$(basename "$dir")" == "$target_name" ]]; then
                found="$dir"
                ((match_count++)) || true
            fi
        done < <(_find_examples)
        if [[ $match_count -gt 1 ]]; then
            printf 'error: example name is ambiguous: %s\n' "$target_name" >&2
            printf 'Use a category-qualified name such as quickstart/%s\n' "$target_name" >&2
            return 1
        fi
        printf '%s' "$found"
    }

    # No args: list available examples
    if [[ $# -eq 0 ]]; then
        printf 'Available examples:\n'
        while IFS= read -r dir; do
            printf '  %s\n' "$(_display_example_name "$dir")"
        done < <(_find_examples)
        printf '\nUsage: blusys example <name> [command] [args...]\n'
        return
    fi

    local name="$1"
    shift
    local example_path
    example_path="$(_resolve_example "$name")"

    if [[ -z "$example_path" || ! -d "$example_path" ]]; then
        printf 'error: example not found: %s\n' "$name" >&2
        printf '\nAvailable examples:\n' >&2
        while IFS= read -r dir; do
            printf '  %s\n' "$(_display_example_name "$dir")" >&2
        done < <(_find_examples)
        exit 1
    fi

    # No command after name: default to build
    if [[ $# -eq 0 ]]; then
        cmd_build "$example_path"
        return
    fi

    # Dispatch the command with example path as the project
    local subcmd="$1"
    shift

    case "$subcmd" in
        build)          cmd_build "$example_path" "$@" ;;
        flash)          cmd_flash "$example_path" "$@" ;;
        monitor)        cmd_monitor "$example_path" "$@" ;;
        run)            cmd_run "$example_path" "$@" ;;
        config|menuconfig) cmd_config "$example_path" "$@" ;;
        size)           cmd_size "$example_path" "$@" ;;
        erase)          cmd_erase "$example_path" "$@" ;;
        clean)          cmd_clean "$example_path" "$@" ;;
        fullclean)      cmd_fullclean "$example_path" ;;
        qemu)           cmd_qemu "$example_path" "$@" ;;
        *)
            printf 'error: unknown command: %s\n' "$subcmd" >&2
            blusys_help_example
            exit 1
            ;;
    esac
}

cmd_install_qemu() {
    if [[ $# -gt 0 ]]; then
        blusys_help_install_qemu
        exit 1
    fi

    local arch os
    arch="$(uname -m)"
    os="$(uname -s)"

    if [[ "$os" != "Linux" && "$os" != "Darwin" ]]; then
        printf 'error: unsupported OS: %s\n' "$os" >&2
        exit 1
    fi

    case "$arch" in
        x86_64|aarch64) ;;
        arm64) arch="aarch64" ;;
        *)
            printf 'error: unsupported architecture: %s\n' "$arch" >&2
            exit 1
            ;;
    esac

    local platform
    if [[ "$os" == "Linux" ]]; then
        platform="${arch}-linux-gnu"
    else
        platform="${arch}-apple-darwin"
    fi

    # Get latest release tag
    local tag
    printf 'Fetching latest Espressif QEMU release...\n'
    if command -v gh &>/dev/null; then
        tag="$(gh api repos/espressif/qemu/releases/latest --jq '.tag_name' 2>/dev/null)"
    else
        tag="$(curl -fsSL "https://api.github.com/repos/espressif/qemu/releases/latest" \
            | grep '"tag_name"' | sed 's/.*"tag_name": *"\([^"]*\)".*/\1/')"
    fi

    if [[ -z "$tag" ]]; then
        printf 'error: could not fetch latest QEMU release tag\n' >&2
        exit 1
    fi

    printf 'Latest release: %s\n' "$tag"

    # Extract version string for asset name from tag
    # Tag format: "esp-develop-9.2.2-20250817" → asset needs "9.2.2_20250817"
    local ver="${tag#esp-develop-}"  # strip "esp-develop-" prefix
    ver="${ver//-/_}"                 # replace remaining dashes with underscores
    local install_dir="$HOME/.espressif/tools/qemu-${tag}"
    mkdir -p "$install_dir"

    tmp_dir="$(mktemp -d)"
    trap 'rm -rf "${tmp_dir:-}"' EXIT

    local base_url="https://github.com/espressif/qemu/releases/download/${tag}"

    for cpu in xtensa riscv32; do
        local asset="qemu-${cpu}-softmmu-esp_develop_${ver}-${platform}.tar.xz"
        local url="${base_url}/${asset}"
        printf 'Downloading %s...\n' "$asset"
        if ! curl -fL --progress-bar -o "$tmp_dir/$asset" "$url"; then
            printf 'error: failed to download %s\n' "$url" >&2
            exit 1
        fi
        printf 'Extracting %s...\n' "$asset"
        tar -xf "$tmp_dir/$asset" -C "$install_dir" --strip-components=1
    done

    # Save QEMU_PATH to config
    mkdir -p "$BLUSYS_CONFIG_DIR"
    if [[ -f "$BLUSYS_CONFIG_FILE" ]]; then
        grep -v '^QEMU_PATH=' "$BLUSYS_CONFIG_FILE" > "$BLUSYS_CONFIG_FILE.tmp" || true
        mv "$BLUSYS_CONFIG_FILE.tmp" "$BLUSYS_CONFIG_FILE"
    fi
    printf 'QEMU_PATH=%s\n' "$install_dir" >> "$BLUSYS_CONFIG_FILE"
    export QEMU_PATH="$install_dir"

    printf '\nDone. QEMU installed to: %s\n' "$install_dir"
    printf 'Saved: QEMU_PATH=%s\n' "$install_dir"
}

cmd_qemu() {
    if [[ $# -gt 2 ]]; then
        blusys_help_qemu
        exit 1
    fi

    blusys_setup_no_port "$@"
    blusys_setup_idf_env

    local qemu_dir
    qemu_dir="$(blusys_find_qemu)" || exit 1

    # Select binary and machine flags based on target
    local qemu_bin machine timer_driver extra_flags=""
    case "$TARGET" in
        esp32)
            qemu_bin="$qemu_dir/qemu-system-xtensa"
            machine="esp32"
            timer_driver="timer.esp32.timg"
            ;;
        esp32s3)
            qemu_bin="$qemu_dir/qemu-system-xtensa"
            machine="esp32s3"
            timer_driver="timer.esp32c3.timg"
            ;;
        esp32c3)
            qemu_bin="$qemu_dir/qemu-system-riscv32"
            machine="esp32c3"
            timer_driver="timer.esp32c3.timg"
            extra_flags="-icount 3"
            ;;
    esac

    blusys_print_info
    printf 'QEMU:      %s\n' "$qemu_bin"

    blusys_set_target_if_needed "$PROJECT_DIR" "$BUILD_DIR" "$TARGET" "${SDKCONFIG_ARGS[@]}"
    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" build

    printf 'Creating flash image...\n'
    # flash_args contains relative paths — must run from BUILD_DIR
    (cd "$BUILD_DIR" && esptool.py --chip "$TARGET" merge_bin --fill-flash-size 4MB \
        -o "$BUILD_DIR/flash_image.bin" \
        @flash_args)

    printf 'Launching QEMU (press Ctrl+A, X to exit)...\n'
    # shellcheck disable=SC2086
    "$qemu_bin" \
        -nographic \
        -machine "$machine" \
        -drive "file=$BUILD_DIR/flash_image.bin,if=mtd,format=raw" \
        -nic "user,model=open_eth" \
        -global "driver=$timer_driver,property=wdt_disable,value=true" \
        $extra_flags
}

cmd_lint() {
    if [[ $# -gt 0 ]]; then
        blusys_help_lint
        exit 1
    fi

    bash "$BLUSYS_REPO_ROOT/scripts/lint-layering.sh"
    python3 "$BLUSYS_REPO_ROOT/scripts/check-framework-ui-sources.py"
    python3 "$BLUSYS_REPO_ROOT/scripts/check-host-bridge-spine.py"
}

cmd_host_build() {
    local project_arg=""

    # Accept optional [project] positional arg; reject unknown flags.
    if [[ $# -gt 0 ]]; then
        if [[ "$1" == -* ]]; then
            blusys_help_host_build
            exit 1
        fi
        project_arg="$1"
        shift
    fi
    if [[ $# -gt 0 ]]; then
        blusys_help_host_build
        exit 1
    fi

    # Resolve host_dir + build_dir:
    #   1. Explicit project arg            → <project>/host/
    #   2. cwd contains host/CMakeLists.txt → <cwd>/host/
    #   3. Fallback                        → monorepo scripts/host/
    local host_dir build_dir is_product=false

    if [[ -n "$project_arg" ]]; then
        local project_dir
        project_dir="$(blusys_resolve_project_dir "$project_arg")" || exit 1
        if [[ ! -f "$project_dir/host/CMakeLists.txt" ]]; then
            printf 'error: no host/CMakeLists.txt found in %s\n' "$project_dir" >&2
            printf '       re-run: blusys create --archetype <name> %s\n' "$project_dir" >&2
            exit 1
        fi
        host_dir="$project_dir/host"
        build_dir="$project_dir/build-host"
        is_product=true
    elif [[ -f "$(pwd)/host/CMakeLists.txt" ]]; then
        host_dir="$(pwd)/host"
        build_dir="$(pwd)/build-host"
        is_product=true
    else
        host_dir="$BLUSYS_REPO_ROOT/scripts/host"
        build_dir="$host_dir/build-host"
        is_product=false
    fi

    if [[ ! -f "$host_dir/CMakeLists.txt" ]]; then
        printf 'error: host harness not found at %s\n' "$host_dir" >&2
        exit 1
    fi

    if ! command -v cmake > /dev/null 2>&1; then
        printf 'error: cmake is required for host-build\n' >&2
        exit 1
    fi
    if ! command -v pkg-config > /dev/null 2>&1; then
        printf 'error: pkg-config is required for host-build\n' >&2
        exit 1
    fi

    # SDL2 is required by both interactive and headless host paths
    # (headless uses SDL_GetTicks/SDL_Delay for timing — see
    # scripts/host/src/app_headless_platform.cpp).
    if ! pkg-config --exists sdl2; then
        printf 'SDL2 development headers not found.\n' >&2
        printf 'Install libsdl2-dev (Debian/Ubuntu), SDL2-devel (Fedora),\n' >&2
        printf 'sdl2 (Arch), or sdl2 (brew on macOS). See scripts/host/README.md.\n' >&2
        exit 1
    fi

    cmake -S "$host_dir" -B "$build_dir" || exit $?
    cmake --build "$build_dir" -j || exit $?

    if [[ "$is_product" == true ]]; then
        local exe
        exe="$(find "$build_dir" -maxdepth 1 -type f -executable ! -name '*.so' 2>/dev/null | sort | head -1)"
        if [[ -n "$exe" ]]; then
            printf '\nhost build complete. run:\n  %s\n' "$exe"
        else
            printf '\nhost build complete. binaries in: %s\n' "$build_dir"
        fi
    else
        printf '\nhost harness built. run:\n  %s/hello_lvgl\n' "$build_dir"
    fi
}

cmd_build_examples() {
    if [[ $# -gt 0 ]]; then
        blusys_help_build_examples
        exit 1
    fi

    local targets=(esp32 esp32c3 esp32s3)

    mapfile -t examples < <(
        for f in "$BLUSYS_REPO_ROOT"/examples/*/CMakeLists.txt \
                 "$BLUSYS_REPO_ROOT"/examples/*/*/CMakeLists.txt; do
            [[ -f "$f" ]] || continue
            printf '%s\n' "$(dirname "$f")"
        done | sort
    )

    declare -A results
    local pass=0
    local fail=0

    blusys_setup_idf_env
    cmd_lint

    for target in "${targets[@]}"; do
        for example_dir in "${examples[@]}"; do
            local example_name
            example_name="$(basename "$example_dir")"
            local build_dir="$example_dir/build-$target"

            mapfile -t sdkconfig_args < <(blusys_sdkconfig_args "$example_dir" "$target")

            printf 'Building %-30s %s ... ' "$example_name" "$target"

            blusys_set_target_if_needed "$example_dir" "$build_dir" "$target" "${sdkconfig_args[@]}" \
                > "$build_dir.log" 2>&1

            if idf.py -C "$example_dir" -B "$build_dir" "${sdkconfig_args[@]}" build \
                >> "$build_dir.log" 2>&1; then
                results["$example_name/$target"]="OK"
                printf 'OK\n'
                ((pass++)) || true
            else
                results["$example_name/$target"]="FAIL"
                printf 'FAIL  (see %s.log)\n' "$build_dir"
                ((fail++)) || true
            fi
        done
    done

    local total=$((pass + fail))

    printf '\n'
    printf '%-30s  %s\n' "-----------------------------" "-------"
    printf '%-30s  %s\n' "EXAMPLE / TARGET" "RESULT"
    printf '%-30s  %s\n' "-----------------------------" "-------"
    for target in "${targets[@]}"; do
        for example_dir in "${examples[@]}"; do
            local example_name
            example_name="$(basename "$example_dir")"
            local key="$example_name/$target"
            printf '%-26s %-8s  %s\n' "$example_name" "$target" "${results[$key]}"
        done
    done
    printf '\n'
    printf '%d/%d passed\n' "$pass" "$total"

    if [[ $fail -gt 0 ]]; then
        exit 1
    fi
}

# ── Main Dispatcher ──────────────────────────────────────────────────────────

main() {
    if [[ $# -eq 0 ]]; then
        blusys_help
        exit 0
    fi

    local command="$1"

    case "$command" in
        create)         shift; cmd_create "$@" ;;
        build)          shift; cmd_build "$@" ;;
        flash)          shift; cmd_flash "$@" ;;
        monitor)        shift; cmd_monitor "$@" ;;
        run)            shift; cmd_run "$@" ;;
        config|menuconfig) shift; cmd_config "$@" ;;
        size)           shift; cmd_size "$@" ;;
        erase)          shift; cmd_erase "$@" ;;
        clean)          shift; cmd_clean "$@" ;;
        fullclean)      shift; cmd_fullclean "$@" ;;
        example)        shift; cmd_example "$@" ;;
        lint)           shift; cmd_lint "$@" ;;
        build-examples) shift; cmd_build_examples "$@" ;;
        host-build)     shift; cmd_host_build "$@" ;;
        qemu)           shift; cmd_qemu "$@" ;;
        install-qemu)   shift; cmd_install_qemu "$@" ;;
        config-idf)     shift; cmd_config_idf "$@" ;;
        version|-v|--version) shift; cmd_version "$@" ;;
        update)         shift; cmd_update "$@" ;;
        help|-h|--help)
            shift
            if [[ $# -gt 0 ]]; then
                local help_func="blusys_help_${1//-/_}"
                if declare -f "$help_func" > /dev/null 2>&1; then
                    "$help_func"
                else
                    printf 'Unknown command: %s\n' "$1" >&2
                    blusys_help
                    exit 1
                fi
            else
                blusys_help
            fi
            ;;
        -*)
            printf 'Unknown option: %s\n' "$command" >&2
            blusys_help
            exit 1
            ;;
        *)
            # No known command — treat entire arg list as implicit "run"
            cmd_run "$@"
            ;;
    esac
}
