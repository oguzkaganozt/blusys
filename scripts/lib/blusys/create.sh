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

blusys_prompt_create_custom_caps() {
    local -n _with_caps_ref=$1
    local -n _policies_ref=$2

    printf 'Capabilities [comma-separated]: '
    read -r caps_answer
    _with_caps_ref="$caps_answer"

    printf 'low_power     [N]: '
    read -r lp_answer
    if [[ "$lp_answer" =~ ^[yY]$ ]]; then
        _policies_ref="low_power"
    else
        _policies_ref=""
    fi
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

    case "$_interface_ref" in
        interactive)
            printf 'Starter preset?\n'
            printf '  1) blank\n'
            printf '  2) storage\n'
            printf '  3) connected operator\n'
            printf '  4) bluetooth storage\n'
            printf '  5) custom\n\n'
            printf 'Choose [1]: '
            read -r preset
            case "$preset" in
                2) _with_caps_ref="storage" ;;
                3) _with_caps_ref="connectivity,diagnostics" ;;
                4) _with_caps_ref="bluetooth,storage" ;;
                5) blusys_prompt_create_custom_caps _with_caps_ref _policies_ref ;;
                *) _with_caps_ref="" ;;
            esac
            ;;
        headless)
            printf 'Starter preset?\n'
            printf '  1) blank\n'
            printf '  2) connected telemetry\n'
            printf '  3) lan control\n'
            printf '  4) usb host\n'
            printf '  5) low-power telemetry\n'
            printf '  6) custom\n\n'
            printf 'Choose [2]: '
            read -r preset
            case "$preset" in
                1)
                    _with_caps_ref=""
                    _policies_ref=""
                    ;;
                3)
                    _with_caps_ref="connectivity,lan_control,ota"
                    _policies_ref=""
                    ;;
                4)
                    _with_caps_ref="usb"
                    _policies_ref=""
                    ;;
                5)
                    _with_caps_ref="connectivity,telemetry"
                    _policies_ref="low_power"
                    ;;
                6) blusys_prompt_create_custom_caps _with_caps_ref _policies_ref ;;
                *)
                    _with_caps_ref="connectivity,telemetry,ota,diagnostics"
                    _policies_ref=""
                    ;;
            esac
            ;;
    esac

    printf 'Path [./my_product]: '
    read -r path_answer
    _target_arg_ref="${path_answer:-./my_product}"

    printf '\nWill create with:\n'
    printf '  blusys create --interface %s' "$_interface_ref"
    [[ -n "$_with_caps_ref" ]] && printf ' --with %s' "$_with_caps_ref"
    [[ -n "$_policies_ref" ]] && printf ' --policy %s' "$_policies_ref"
    printf ' %s\n\n' "$_target_arg_ref"
}
