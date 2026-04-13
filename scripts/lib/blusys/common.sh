# Sourced by repo-root `blusys` — shared utilities (project resolution, IDF, ports).

: "${BLUSYS_MIN_IDF_MAJOR:=5}"
: "${BLUSYS_MIN_IDF_MINOR:=5}"

blusys_resolve_project_dir() {
    local project_arg="$1"
    local candidate

    if [[ "$project_arg" = /* ]]; then
        candidate="$project_arg"
    elif [[ "$project_arg" = "." ]]; then
        candidate="$(pwd)"
    else
        candidate="$(pwd)/$project_arg"
    fi

    if [[ ! -d "$candidate" ]]; then
        printf 'error: project directory not found: %s\n' "$candidate" >&2
        printf '       create a project with: blusys create\n' >&2
        return 1
    fi

    if [[ ! -f "$candidate/CMakeLists.txt" ]]; then
        printf 'error: no CMakeLists.txt in %s\n' "$candidate" >&2
        printf '       create a project with: blusys create\n' >&2
        return 1
    fi

    printf '%s\n' "$(cd "$candidate" && pwd)"
}

blusys_is_qemu_build_label() {
    case "${1:-}" in
        qemu32|qemu32c3|qemu32s3) return 0 ;;
        *) return 1 ;;
    esac
}

# Maps blusys build labels to the IDF chip name for idf.py set-target (host is handled elsewhere).
blusys_idf_target_from_build_label() {
    case "${1:-}" in
        qemu32) printf 'esp32' ;;
        qemu32c3) printf 'esp32c3' ;;
        qemu32s3) printf 'esp32s3' ;;
        esp32|esp32c3|esp32s3) printf '%s' "$1" ;;
        *) return 1 ;;
    esac
}

blusys_detect_target() {
    local project_dir="$1"
    local requested_target="${2:-}"

    if [[ -n "$requested_target" ]]; then
        case "$requested_target" in
            esp32|esp32c3|esp32s3|host|qemu32|qemu32c3|qemu32s3)
                printf '%s\n' "$requested_target"
                return 0
                ;;
            *)
                printf 'error: unsupported target: %s\n' "$requested_target" >&2
                printf '       expected: esp32, esp32c3, esp32s3, host, qemu32, qemu32c3, qemu32s3\n' >&2
                return 1
                ;;
        esac
    fi

    if [[ -f "$project_dir/build-esp32c3/sdkconfig" && ! -f "$project_dir/build-esp32s3/sdkconfig" ]]; then
        printf 'esp32c3\n'
        return 0
    fi

    if [[ -f "$project_dir/build-esp32s3/sdkconfig" && ! -f "$project_dir/build-esp32c3/sdkconfig" ]]; then
        printf 'esp32s3\n'
        return 0
    fi

    printf 'info: no target specified; defaulting to %s\n' "$BLUSYS_DEFAULT_TARGET" >&2
    printf '%s\n' "$BLUSYS_DEFAULT_TARGET"
}

blusys_sdkconfig_args() {
    local project_dir="$1"
    local target="$2"
    local build_dir="$project_dir/build-$target"
    local idf_target
    idf_target="$(blusys_idf_target_from_build_label "$target")" || return 1

    local common="$project_dir/sdkconfig.defaults"
    # Prefer sdkconfig.<chip> (explicit); else sdkconfig.defaults.<chip> (tracked in git —
    # examples/**/sdkconfig.* is gitignored except sdkconfig.defaults* / sdkconfig.qemu).
    local chip=""
    if [[ -f "$project_dir/sdkconfig.$idf_target" ]]; then
        chip="$project_dir/sdkconfig.$idf_target"
    elif [[ -f "$project_dir/sdkconfig.defaults.$idf_target" ]]; then
        chip="$project_dir/sdkconfig.defaults.$idf_target"
    fi
    local qemu_frag="$project_dir/sdkconfig.qemu"

    local parts=()
    [[ -f "$common" ]] && parts+=("$common")
    [[ -n "$chip" ]] && parts+=("$chip")
    if blusys_is_qemu_build_label "$target" && [[ -f "$qemu_frag" ]]; then
        parts+=("$qemu_frag")
    fi

    local sdkconfig_defaults="" p
    for p in "${parts[@]}"; do
        [[ -n "$sdkconfig_defaults" ]] && sdkconfig_defaults+=";"
        sdkconfig_defaults+="$p"
    done

    if [[ -n "$sdkconfig_defaults" ]]; then
        printf '%s\n' "-DSDKCONFIG_DEFAULTS=$sdkconfig_defaults"
    fi
    printf '%s\n' "-DSDKCONFIG=$build_dir/sdkconfig"
}

# Sets globals BUILD_DIR, IDF_TARGET, SDKCONFIG_ARGS (not used for TARGET=host).
blusys_populate_idf_sdkconfig() {
    local project_dir="$1"
    local target="$2"
    BUILD_DIR="$project_dir/build-$target"
    IDF_TARGET="$(blusys_idf_target_from_build_label "$target")" || return 1
    local out
    out="$(blusys_sdkconfig_args "$project_dir" "$target")" || return 1
    mapfile -t SDKCONFIG_ARGS <<< "$out"
}

# idf.py with current PROJECT_DIR, BUILD_DIR, SDKCONFIG_ARGS (after setup_*).
blusys_idf_py() {
    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" "$@"
}

blusys_find_idf_path() {
    # Priority 1: IDF_PATH env var
    if [[ -n "${IDF_PATH:-}" && -f "$IDF_PATH/export.sh" ]]; then
        printf '%s\n' "$IDF_PATH"
        return 0
    fi

    # Priority 2: idf.py already in PATH
    if command -v idf.py &>/dev/null; then
        local idf_py_path
        idf_py_path="$(command -v idf.py)"
        local candidate
        candidate="$(cd "$(dirname "$idf_py_path")/.." && pwd)"
        if [[ -f "$candidate/export.sh" ]]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    fi

    # Priority 3: Scan ~/.espressif/*/esp-idf/
    local latest_version="" latest_path=""
    for candidate_dir in "$HOME"/.espressif/*/esp-idf; do
        [[ -d "$candidate_dir" && -f "$candidate_dir/export.sh" ]] || continue
        local version_dir
        version_dir="$(basename "$(dirname "$candidate_dir")")"
        if [[ -z "$latest_version" ]] || [[ "$version_dir" > "$latest_version" ]]; then
            latest_version="$version_dir"
            latest_path="$candidate_dir"
        fi
    done

    if [[ -n "$latest_path" ]]; then
        printf 'info: auto-detected ESP-IDF at %s\n' "$latest_path" >&2
        printf '%s\n' "$latest_path"
        return 0
    fi

    # Priority 4: Failure with install instructions
    printf 'error: ESP-IDF not found.\n' >&2
    printf '\n' >&2
    printf 'Install ESP-IDF v5.5+ and try one of:\n' >&2
    printf '  1. Set IDF_PATH:    export IDF_PATH=/path/to/esp-idf\n' >&2
    printf '  2. Source export.sh: source /path/to/esp-idf/export.sh\n' >&2
    printf '  3. Standard install: https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/\n' >&2
    printf '     (installs to ~/.espressif/ which blusys auto-detects)\n' >&2
    return 1
}

blusys_find_qemu() {
    local dir

    # Helper: return the directory that contains the QEMU binaries,
    # checking both the directory itself and its bin/ subdirectory.
    _qemu_bin_dir() {
        local base="$1"
        for dir in "$base/bin" "$base"; do
            if [[ -x "$dir/qemu-system-xtensa" || -x "$dir/qemu-system-riscv32" ]]; then
                printf '%s\n' "$dir"
                return 0
            fi
        done
        return 1
    }

    # Priority 1: QEMU_PATH env var
    if [[ -n "${QEMU_PATH:-}" ]]; then
        if _qemu_bin_dir "$QEMU_PATH"; then
            return 0
        fi
    fi

    # Priority 2: qemu-system-xtensa already in PATH
    if command -v qemu-system-xtensa &>/dev/null; then
        printf '%s\n' "$(dirname "$(command -v qemu-system-xtensa)")"
        return 0
    fi

    # Priority 3: Scan ~/.espressif/tools/qemu-*/
    local latest_tag="" latest_path=""
    for candidate_dir in "$HOME"/.espressif/tools/qemu-*/; do
        [[ -d "$candidate_dir" ]] || continue
        local found
        found="$(_qemu_bin_dir "${candidate_dir%/}")" || continue
        local tag
        tag="$(basename "${candidate_dir%/}")"
        if [[ -z "$latest_tag" ]] || [[ "$tag" > "$latest_tag" ]]; then
            latest_tag="$tag"
            latest_path="$found"
        fi
    done

    if [[ -n "$latest_path" ]]; then
        printf '%s\n' "$latest_path"
        return 0
    fi

    printf 'error: Espressif QEMU not found.\n' >&2
    printf 'Install it with: blusys install-qemu\n' >&2
    return 1
}

blusys_check_idf_version() {
    local idf_path="$1"
    local version_str=""

    if [[ -n "${IDF_VERSION:-}" ]]; then
        version_str="$IDF_VERSION"
    elif [[ -f "$idf_path/version.txt" ]]; then
        version_str="$(cat "$idf_path/version.txt")"
    elif [[ -f "$idf_path/tools/cmake/version.cmake" ]]; then
        local major minor
        major="$(grep 'set(IDF_VERSION_MAJOR' "$idf_path/tools/cmake/version.cmake" | grep -o '[0-9]\+')"
        minor="$(grep 'set(IDF_VERSION_MINOR' "$idf_path/tools/cmake/version.cmake" | grep -o '[0-9]\+')"
        if [[ -n "$major" && -n "$minor" ]]; then
            version_str="${major}.${minor}"
        fi
    fi

    if [[ -z "$version_str" ]]; then
        return 0
    fi

    local major minor
    major="$(printf '%s' "$version_str" | sed 's/^v//' | cut -d. -f1)"
    minor="$(printf '%s' "$version_str" | sed 's/^v//' | cut -d. -f2)"

    if [[ "$major" -lt "$BLUSYS_MIN_IDF_MAJOR" ]] || \
       { [[ "$major" -eq "$BLUSYS_MIN_IDF_MAJOR" ]] && [[ "$minor" -lt "$BLUSYS_MIN_IDF_MINOR" ]]; }; then
        printf 'warning: ESP-IDF %s detected, but blusys requires v%d.%d+\n' \
            "$version_str" "$BLUSYS_MIN_IDF_MAJOR" "$BLUSYS_MIN_IDF_MINOR" >&2
    fi
}

blusys_setup_idf_env() {
    # If idf.py is already available, just check version
    if command -v idf.py &>/dev/null; then
        local detected_idf_path
        detected_idf_path="$(blusys_find_idf_path)" || true
        [[ -n "$detected_idf_path" ]] && blusys_check_idf_version "$detected_idf_path"
        return 0
    fi

    local _idf_path
    _idf_path="$(blusys_find_idf_path)" || return 1

    # shellcheck source=/dev/null
    source "$_idf_path/export.sh"
    # export.sh does "unset idf_path" — use a private name to avoid the clash

    blusys_check_idf_version "$_idf_path"
}

blusys_set_target_if_needed() {
    local project_dir="$1"
    local build_dir="$2"
    local target="$3"
    shift 3
    local sdkconfig_args=("$@")

    if [[ ! -f "$build_dir/CMakeCache.txt" ]]; then
        idf.py -C "$project_dir" -B "$build_dir" "${sdkconfig_args[@]}" set-target "$target"
    fi
}

blusys_detect_port() {
    local requested_port="${1:-}"
    local candidates=()

    if [[ -n "$requested_port" ]]; then
        if [[ ! -e "$requested_port" ]]; then
            printf 'error: port not found: %s\n' "$requested_port" >&2
            return 1
        fi
        printf '%s\n' "$requested_port"
        return 0
    fi

    for dev in /dev/ttyUSB* /dev/ttyACM*; do
        [[ -e "$dev" ]] && candidates+=("$dev")
    done

    if [[ ${#candidates[@]} -eq 0 ]]; then
        printf 'error: no USB serial device found; connect a device or pass a port explicitly\n' >&2
        return 1
    fi

    if [[ ${#candidates[@]} -gt 1 ]]; then
        printf 'error: multiple USB serial devices found; specify one:\n' >&2
        for dev in "${candidates[@]}"; do
            printf '  %s\n' "$dev" >&2
        done
        return 1
    fi

    printf 'info: auto-detected port: %s\n' "${candidates[0]}" >&2
    printf '%s\n' "${candidates[0]}"
}

# ── Shared Helpers ───────────────────────────────────────────────────────────

blusys_is_target() {
    case "${1:-}" in
        esp32|esp32c3|esp32s3|host|qemu32|qemu32c3|qemu32s3) return 0 ;;
        *) return 1 ;;
    esac
}

# True for esp32 / esp32c3 / esp32s3 (USB-flashable); false for host and qemu* labels.
blusys_is_device_flash_target() {
    case "${1:-}" in
        esp32|esp32c3|esp32s3) return 0 ;;
        *) return 1 ;;
    esac
}

# Commands that operate on IDF firmware (not the SDL host harness).
blusys_reject_host_target() {
    [[ "$TARGET" != host ]] && return 0
    printf 'error: use a chip or qemu* target (not host). For SDL: blusys host-build\n' >&2
    return 1
}

# Parse args for commands that need a port (flash, monitor, run, erase).
# Sets globals: PROJECT_DIR, PORT, TARGET, BUILD_DIR, SDKCONFIG_ARGS, IDF_TARGET
blusys_setup_with_port() {
    local project="" port_arg="" target=""

    case $# in
        0)  project="$BLUSYS_DEFAULT_PROJECT" ;;
        1)
            if [[ "$1" == /dev/* ]]; then
                project="$BLUSYS_DEFAULT_PROJECT"; port_arg="$1"
            elif blusys_is_target "$1"; then
                project="$BLUSYS_DEFAULT_PROJECT"; target="$1"
            else
                project="$1"
            fi
            ;;
        2)
            if [[ "$1" == /dev/* ]]; then
                project="$BLUSYS_DEFAULT_PROJECT"; port_arg="$1"; target="$2"
            elif [[ "$2" == /dev/* ]]; then
                project="$1"; port_arg="$2"
            else
                project="$1"; target="$2"
            fi
            ;;
        3)  project="$1"; port_arg="$2"; target="$3" ;;
    esac

    PROJECT_DIR="$(blusys_resolve_project_dir "$project")"
    TARGET="$(blusys_detect_target "$PROJECT_DIR" "$target")" || return 1

    if ! blusys_is_device_flash_target "$TARGET"; then
        printf 'error: flash/monitor/run/erase need a device target (esp32, esp32c3, esp32s3), not %s\n' "$TARGET" >&2
        return 1
    fi

    blusys_populate_idf_sdkconfig "$PROJECT_DIR" "$TARGET" || return 1
    PORT="$(blusys_detect_port "$port_arg")" || return 1
}

# Parse args for commands that don't need a port (build, config, size, clean).
# All args are optional — defaults to cwd, auto target.
# Sets globals: PROJECT_DIR, TARGET, BUILD_DIR, SDKCONFIG_ARGS, IDF_TARGET
blusys_setup_no_port() {
    local project="" target=""

    case $# in
        0)  project="$BLUSYS_DEFAULT_PROJECT" ;;
        1)
            if blusys_is_target "$1"; then
                project="$BLUSYS_DEFAULT_PROJECT"; target="$1"
            else
                project="$1"
            fi
            ;;
        2)  project="$1"; target="$2" ;;
    esac

    PROJECT_DIR="$(blusys_resolve_project_dir "$project")"
    TARGET="$(blusys_detect_target "$PROJECT_DIR" "$target")" || return 1

    if [[ "$TARGET" = host ]]; then
        IDF_TARGET=""
        BUILD_DIR="$PROJECT_DIR/build-host"
        SDKCONFIG_ARGS=()
        return 0
    fi

    blusys_populate_idf_sdkconfig "$PROJECT_DIR" "$TARGET" || return 1
}

blusys_print_info() {
    local label="$TARGET"
    blusys_is_qemu_build_label "$TARGET" && label="$TARGET (IDF: $IDF_TARGET)"
    printf 'Project:   %s\n' "$PROJECT_DIR"
    printf 'Target:    %s\n' "$label"
    printf 'Build dir: %s\n' "$BUILD_DIR"
}

blusys_print_info_port() {
    blusys_print_info
    printf 'Port:      %s\n' "$PORT"
}
