#!/usr/bin/env bash

set -euo pipefail

# ── Constants ────────────────────────────────────────────────────────────────

BLUSYS_REPO_ROOT="$(cd "$(dirname "$0")" && pwd)"
BLUSYS_DEFAULT_TARGET="esp32s3"
BLUSYS_IDF_EXPORT_SH="/home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh"
BLUSYS_DEFAULT_IDF_PYTHON_ENV="/home/oguzkaganozt/.espressif/python_env/idf5.5_py3.12_env"

# ── Utility Functions ────────────────────────────────────────────────────────

blusys_resolve_project_dir() {
    local project_arg="$1"
    local candidate

    if [[ "$project_arg" = /* ]]; then
        candidate="$project_arg"
    else
        candidate="$BLUSYS_REPO_ROOT/$project_arg"
    fi

    if [[ ! -d "$candidate" ]]; then
        printf 'error: project directory not found: %s\n' "$project_arg" >&2
        return 1
    fi

    if [[ ! -f "$candidate/CMakeLists.txt" ]]; then
        printf 'error: not an ESP-IDF project directory: %s\n' "$project_arg" >&2
        return 1
    fi

    printf '%s\n' "$(cd "$candidate" && pwd)"
}

blusys_detect_target() {
    local project_dir="$1"
    local requested_target="${2:-}"

    if [[ -n "$requested_target" ]]; then
        case "$requested_target" in
            esp32|esp32c3|esp32s3)
                printf '%s\n' "$requested_target"
                return 0
                ;;
            *)
                printf 'error: unsupported target: %s\n' "$requested_target" >&2
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
    local common_defaults="$project_dir/sdkconfig.defaults"
    local target_defaults="$project_dir/sdkconfig.$target"

    local sdkconfig_defaults=""
    if [[ -f "$common_defaults" ]]; then
        sdkconfig_defaults="$common_defaults"
    fi
    if [[ -f "$target_defaults" ]]; then
        if [[ -n "$sdkconfig_defaults" ]]; then
            sdkconfig_defaults="${sdkconfig_defaults};${target_defaults}"
        else
            sdkconfig_defaults="$target_defaults"
        fi
    fi

    if [[ -n "$sdkconfig_defaults" ]]; then
        printf '%s\n' "-DSDKCONFIG_DEFAULTS=$sdkconfig_defaults"
    fi

    printf '%s\n' "-DSDKCONFIG=$build_dir/sdkconfig"
}

blusys_setup_idf_env() {
    if [[ ! -f "$BLUSYS_IDF_EXPORT_SH" ]]; then
        printf 'error: ESP-IDF export script not found: %s\n' "$BLUSYS_IDF_EXPORT_SH" >&2
        return 1
    fi

    if [[ -z "${IDF_PYTHON_ENV_PATH:-}" && -d "$BLUSYS_DEFAULT_IDF_PYTHON_ENV" ]]; then
        export IDF_PYTHON_ENV_PATH="$BLUSYS_DEFAULT_IDF_PYTHON_ENV"
    fi

    # shellcheck source=/dev/null
    source "$BLUSYS_IDF_EXPORT_SH"
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

# Parse the port/target ambiguity shared by flash, monitor, run.
# Sets globals: PROJECT_DIR, PORT, TARGET, BUILD_DIR, SDKCONFIG_ARGS
blusys_setup_with_port() {
    PROJECT_DIR="$(blusys_resolve_project_dir "$1")"

    if [[ "${2:-}" == /dev/* ]]; then
        PORT="$(blusys_detect_port "${2:-}")"
        TARGET="$(blusys_detect_target "$PROJECT_DIR" "${3:-}")"
    else
        PORT="$(blusys_detect_port "")"
        TARGET="$(blusys_detect_target "$PROJECT_DIR" "${2:-}")"
    fi

    BUILD_DIR="$PROJECT_DIR/build-$TARGET"
    mapfile -t SDKCONFIG_ARGS < <(blusys_sdkconfig_args "$PROJECT_DIR" "$TARGET")
}

# Standard setup for commands that don't need a port.
# Sets globals: PROJECT_DIR, TARGET, BUILD_DIR, SDKCONFIG_ARGS
blusys_setup_no_port() {
    PROJECT_DIR="$(blusys_resolve_project_dir "$1")"
    TARGET="$(blusys_detect_target "$PROJECT_DIR" "${2:-}")"
    BUILD_DIR="$PROJECT_DIR/build-$TARGET"
    mapfile -t SDKCONFIG_ARGS < <(blusys_sdkconfig_args "$PROJECT_DIR" "$TARGET")
}

blusys_print_info() {
    printf 'Project:   %s\n' "$PROJECT_DIR"
    printf 'Target:    %s\n' "$TARGET"
    printf 'Build dir: %s\n' "$BUILD_DIR"
}

blusys_print_info_port() {
    blusys_print_info
    printf 'Port:      %s\n' "$PORT"
}

# ── Help ─────────────────────────────────────────────────────────────────────

blusys_help() {
    cat <<'HELP'
Usage: blusys.sh <command> [args...]
       blusys.sh <project> [port] [target]    (shortcut for 'run')

Commands:
  build            Build a project
  flash            Flash a project to a device
  monitor          Open serial monitor
  run              Build, flash, and monitor (default)
  config           Open menuconfig
  clean            Remove build directory
  build-examples   Build all examples for all targets
  help             Show this help message

Run 'blusys.sh help <command>' for command-specific help.
HELP
}

blusys_help_build() {
    printf 'Usage: blusys.sh build <project> [esp32|esp32c3|esp32s3]\n'
}

blusys_help_flash() {
    printf 'Usage: blusys.sh flash <project> [port] [esp32|esp32c3|esp32s3]\n'
}

blusys_help_monitor() {
    printf 'Usage: blusys.sh monitor <project> [port] [esp32|esp32c3|esp32s3]\n'
}

blusys_help_run() {
    printf 'Usage: blusys.sh run <project> [port] [esp32|esp32c3|esp32s3]\n'
    printf '       blusys.sh <project> [port] [esp32|esp32c3|esp32s3]\n'
}

blusys_help_config() {
    printf 'Usage: blusys.sh config <project> [esp32|esp32c3|esp32s3]\n'
}

blusys_help_clean() {
    printf 'Usage: blusys.sh clean <project> [esp32|esp32c3|esp32s3]\n'
}

blusys_help_build_examples() {
    printf 'Usage: blusys.sh build-examples\n'
    printf '\nBuilds every example for every target (esp32, esp32c3, esp32s3).\n'
}

# ── Subcommands ──────────────────────────────────────────────────────────────

cmd_build() {
    if [[ $# -lt 1 || $# -gt 2 ]]; then
        blusys_help_build
        exit 1
    fi

    blusys_setup_no_port "$1" "${2:-}"
    blusys_setup_idf_env
    blusys_print_info

    blusys_set_target_if_needed "$PROJECT_DIR" "$BUILD_DIR" "$TARGET" "${SDKCONFIG_ARGS[@]}"
    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" build
}

cmd_flash() {
    if [[ $# -lt 1 || $# -gt 3 ]]; then
        blusys_help_flash
        exit 1
    fi

    blusys_setup_with_port "$1" "${2:-}" "${3:-}"
    blusys_setup_idf_env
    blusys_print_info_port

    blusys_set_target_if_needed "$PROJECT_DIR" "$BUILD_DIR" "$TARGET" "${SDKCONFIG_ARGS[@]}"
    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" -p "$PORT" flash
}

cmd_monitor() {
    if [[ $# -lt 1 || $# -gt 3 ]]; then
        blusys_help_monitor
        exit 1
    fi

    blusys_setup_with_port "$1" "${2:-}" "${3:-}"
    blusys_setup_idf_env
    blusys_print_info_port
    printf 'Monitor exit: press Ctrl+]\n'

    blusys_set_target_if_needed "$PROJECT_DIR" "$BUILD_DIR" "$TARGET" "${SDKCONFIG_ARGS[@]}"
    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" -p "$PORT" monitor
}

cmd_run() {
    if [[ $# -lt 1 || $# -gt 3 ]]; then
        blusys_help_run
        exit 1
    fi

    blusys_setup_with_port "$1" "${2:-}" "${3:-}"
    blusys_setup_idf_env
    blusys_print_info_port
    printf 'Monitor exit: press Ctrl+]\n'

    blusys_set_target_if_needed "$PROJECT_DIR" "$BUILD_DIR" "$TARGET" "${SDKCONFIG_ARGS[@]}"
    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" -p "$PORT" build flash monitor
}

cmd_config() {
    if [[ $# -lt 1 || $# -gt 2 ]]; then
        blusys_help_config
        exit 1
    fi

    blusys_setup_no_port "$1" "${2:-}"
    blusys_setup_idf_env
    blusys_print_info

    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" set-target "$TARGET"
    idf.py -C "$PROJECT_DIR" -B "$BUILD_DIR" "${SDKCONFIG_ARGS[@]}" menuconfig
}

cmd_clean() {
    if [[ $# -lt 1 || $# -gt 2 ]]; then
        blusys_help_clean
        exit 1
    fi

    PROJECT_DIR="$(blusys_resolve_project_dir "$1")"
    TARGET="$(blusys_detect_target "$PROJECT_DIR" "${2:-}")"
    BUILD_DIR="$PROJECT_DIR/build-$TARGET"

    if [[ ! -d "$BUILD_DIR" ]]; then
        printf 'Nothing to clean: %s does not exist\n' "$BUILD_DIR"
        exit 0
    fi

    printf 'Removing: %s\n' "$BUILD_DIR"
    rm -rf "$BUILD_DIR"
    printf 'Done.\n'
}

cmd_build_examples() {
    if [[ $# -gt 0 ]]; then
        blusys_help_build_examples
        exit 1
    fi

    local targets=(esp32 esp32c3 esp32s3)

    mapfile -t examples < <(
        for f in "$BLUSYS_REPO_ROOT"/examples/*/CMakeLists.txt; do
            printf '%s\n' "$(dirname "$f")"
        done | sort
    )

    declare -A results
    local pass=0
    local fail=0

    blusys_setup_idf_env

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
        build)          shift; cmd_build "$@" ;;
        flash)          shift; cmd_flash "$@" ;;
        monitor)        shift; cmd_monitor "$@" ;;
        run)            shift; cmd_run "$@" ;;
        config)         shift; cmd_config "$@" ;;
        clean)          shift; cmd_clean "$@" ;;
        build-examples) shift; cmd_build_examples "$@" ;;
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

main "$@"
