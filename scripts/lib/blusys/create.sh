# Sourced by repo-root `blusys` — `create` command and project scaffolding.

cmd_create() {
    local interface="interactive"
    local with_caps=""
    local policies=""
    local list_only="0"
    local positional=()

    while [[ $# -gt 0 ]]; do
        case "$1" in
            --interface)
                [[ $# -ge 2 ]] || { printf 'error: --interface requires an argument\n' >&2; exit 1; }
                interface="$2"
                shift 2
                ;;
            --interface=*)
                interface="${1#--interface=}"
                shift
                ;;
            --with)
                [[ $# -ge 2 ]] || { printf 'error: --with requires an argument\n' >&2; exit 1; }
                with_caps="$2"
                shift 2
                ;;
            --with=*)
                with_caps="${1#--with=}"
                shift
                ;;
            --policy)
                [[ $# -ge 2 ]] || { printf 'error: --policy requires an argument\n' >&2; exit 1; }
                policies="$2"
                shift 2
                ;;
            --policy=*)
                policies="${1#--policy=}"
                shift
                ;;
            --list)
                list_only="1"
                shift
                ;;
            -h|--help)
                blusys_help_create
                exit 0
                ;;
            --*)
                printf 'error: unknown option %s\n' "$1" >&2
                blusys_help_create
                exit 1
                ;;
            *)
                positional+=("$1")
                shift
                ;;
            esac
    done

    if [[ ${#positional[@]} -gt 1 ]]; then
        blusys_help_create
        exit 1
    fi

    blusys_require_pyyaml

    if [[ "$list_only" == "1" ]]; then
        python3 "$BLUSYS_REPO_ROOT/scripts/lib/blusys/scaffold_generator.py" \
            --repo-root "$BLUSYS_REPO_ROOT" \
            --list
        exit $?
    fi

    local target_arg=""
    if [[ ${#positional[@]} -eq 1 ]]; then
        target_arg="${positional[0]}"
    fi

    if [[ $# -eq 0 && -t 0 && -z "$with_caps" && -z "$policies" && ${#positional[@]} -eq 0 ]]; then
        blusys_prompt_create interface with_caps policies target_arg
    fi

    local target_dir
    if [[ -z "$target_arg" ]]; then
        target_dir="$(pwd)"
    elif [[ "$target_arg" = /* ]]; then
        target_dir="$target_arg"
    else
        target_dir="$(pwd)/$target_arg"
    fi

    mkdir -p "$target_dir"
    target_dir="$(cd "$target_dir" && pwd)"

    local existing_files=()
    for f in CMakeLists.txt README.md sdkconfig.defaults sdkconfig.qemu .gitignore blusys.project.yml main/CMakeLists.txt main/idf_component.yml main/app_main.cpp main/ui/CMakeLists.txt main/ui/app_ui.cpp main/core/app_logic.hpp main/core/app_logic.cpp main/ui/app_ui.hpp main/ui/README.md main/platform/app_main.cpp host/CMakeLists.txt; do
        [[ -e "$target_dir/$f" ]] && existing_files+=("$f")
    done
    if [[ ${#existing_files[@]} -gt 0 ]]; then
        printf 'The following files already exist in %s:\n' "$target_dir"
        for f in "${existing_files[@]}"; do
            printf '  %s\n' "$f"
        done
        printf 'Overwrite? [y/N] '
        read -r answer
        if [[ "$answer" != [yY] ]]; then
            printf 'Aborted.\n'
            exit 0
        fi
    fi

    python3 "$BLUSYS_REPO_ROOT/scripts/lib/blusys/scaffold_generator.py" \
        --repo-root "$BLUSYS_REPO_ROOT" \
        --interface "$interface" \
        --with "$with_caps" \
        --policy "$policies" \
        "$target_dir"
}

blusys_prompt_create() {
    local -n _interface_ref=$1
    local -n _with_caps_ref=$2
    local -n _policies_ref=$3
    local -n _target_arg_ref=$4

    printf 'Product interface?\n'
    printf '  1) interactive\n'
    printf '  2) headless\n\n'
    printf 'Choose [1]: '
    read -r choice
    case "$choice" in
        2) _interface_ref="headless" ;;
        *) _interface_ref="interactive" ;;
    esac

    local selected_caps=()
    for cap in connectivity bluetooth usb telemetry ota lan_control storage diagnostics; do
        local default="N"
        if [[ "$_interface_ref" == "headless" ]]; then
            case "$cap" in connectivity|telemetry|ota|diagnostics) default="Y" ;; esac
        elif [[ "$_interface_ref" == "interactive" ]]; then
            case "$cap" in connectivity|diagnostics|storage) default="Y" ;; esac
        fi
        printf '%-12s [%s]: ' "$cap" "$default"
        read -r answer
        if [[ -z "$answer" ]]; then
            answer="$default"
        fi
        if [[ "$answer" =~ ^[yY]$ ]]; then
            selected_caps+=("$cap")
        fi
    done
    _with_caps_ref="$(IFS=,; printf '%s' "${selected_caps[*]}")"

    printf 'low_power     [N]: '
    read -r lp_answer
    if [[ "$lp_answer" =~ ^[yY]$ ]]; then
        _policies_ref="low_power"
    else
        _policies_ref=""
    fi

    printf 'Path [./my_product]: '
    read -r path_answer
    _target_arg_ref="${path_answer:-./my_product}"

    printf '\nWill create with:\n'
    printf '  blusys create --interface %s' "$_interface_ref"
    [[ -n "$_with_caps_ref" ]] && printf ' --with %s' "$_with_caps_ref"
    [[ -n "$_policies_ref" ]] && printf ' --policy %s' "$_policies_ref"
    printf ' %s\n\n' "$_target_arg_ref"
}
