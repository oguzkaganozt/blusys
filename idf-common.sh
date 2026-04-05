#!/usr/bin/env bash

set -euo pipefail

BLUSYS_REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BLUSYS_DEFAULT_TARGET="esp32s3"
BLUSYS_IDF_EXPORT_SH="/home/oguzkaganozt/.espressif/v5.5.4/esp-idf/export.sh"
BLUSYS_DEFAULT_IDF_PYTHON_ENV="/home/oguzkaganozt/.espressif/python_env/idf5.5_py3.12_env"

blusys_usage_common() {
    local script_name="$1"
    printf 'Usage: %s <project_dir> [esp32|esp32c3|esp32s3]\n' "$script_name"
}

blusys_usage_port() {
    local script_name="$1"
    printf 'Usage: %s <project_dir> [port] [esp32|esp32c3|esp32s3]\n' "$script_name"
}

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
    local defaults_file="$project_dir/sdkconfig.$target"

    if [[ -f "$defaults_file" ]]; then
        printf '%s\n' "-DSDKCONFIG_DEFAULTS=$defaults_file"
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
