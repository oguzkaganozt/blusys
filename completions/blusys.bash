# Bash completion for blusys
# Auto-loaded from ~/.local/share/bash-completion/completions/blusys

_blusys_commands="create build flash monitor run config menuconfig size erase clean fullclean example build-examples qemu install-qemu config-idf version update help"
_blusys_targets="esp32 esp32c3 esp32s3"
_blusys_example_commands="build flash monitor run config menuconfig size erase clean fullclean qemu"

_blusys_complete_ports() {
    local ports=()
    for dev in /dev/ttyUSB* /dev/ttyACM*; do
        [[ -e "$dev" ]] && ports+=("$dev")
    done
    printf '%s\n' "${ports[@]}"
}

_blusys_complete_projects() {
    # Complete directories that look like ESP-IDF projects (contain CMakeLists.txt)
    local dir
    for dir in "${cur:-.}"/*/; do
        [[ -f "${dir}CMakeLists.txt" ]] && printf '%s\n' "${dir%/}"
    done
    # Also complete the examples/ prefix if it exists relative to cwd
    if [[ -z "$cur" || "$cur" == e* ]]; then
        for dir in examples/*/; do
            [[ -f "${dir}CMakeLists.txt" ]] && printf '%s\n' "${dir%/}"
        done
    fi
}

_blusys_complete_examples() {
    local blusys_root
    blusys_root="$(readlink -f "$(command -v blusys)" 2>/dev/null)"
    blusys_root="$(dirname "$blusys_root")"
    for dir in "$blusys_root"/examples/*/CMakeLists.txt; do
        [[ -f "$dir" ]] && basename "$(dirname "$dir")"
    done
}

_blusys() {
    local cur prev words cword
    _init_completion || return

    # First argument: complete commands
    if [[ $cword -eq 1 ]]; then
        COMPREPLY=($(compgen -W "$_blusys_commands" -- "$cur"))
        return
    fi

    local command="${words[1]}"

    case "$command" in
        # No arguments
        create)
            # Complete directories
            COMPREPLY=($(compgen -d -- "$cur"))
            ;;

        # Commands with [project] [target]
        build|config|menuconfig|size|clean|qemu)
            local nargs=$((cword - 1))
            if [[ $nargs -eq 1 ]]; then
                # Could be project or target
                local projects
                projects="$(_blusys_complete_projects)"
                COMPREPLY=($(compgen -W "$_blusys_targets $projects" -- "$cur"))
                compopt -o nospace 2>/dev/null
            elif [[ $nargs -eq 2 ]]; then
                # Must be target
                COMPREPLY=($(compgen -W "$_blusys_targets" -- "$cur"))
            fi
            ;;

        # Commands with [project] [port] [target]
        flash|monitor|run|erase)
            local nargs=$((cword - 1))
            if [[ $nargs -eq 1 ]]; then
                # Could be project, port, or target
                local projects ports
                projects="$(_blusys_complete_projects)"
                ports="$(_blusys_complete_ports)"
                COMPREPLY=($(compgen -W "$_blusys_targets $projects $ports" -- "$cur"))
                compopt -o nospace 2>/dev/null
            elif [[ $nargs -eq 2 ]]; then
                # Could be port or target
                local ports
                ports="$(_blusys_complete_ports)"
                COMPREPLY=($(compgen -W "$_blusys_targets $ports" -- "$cur"))
            elif [[ $nargs -eq 3 ]]; then
                # Must be target
                COMPREPLY=($(compgen -W "$_blusys_targets" -- "$cur"))
            fi
            ;;

        example)
            local nargs=$((cword - 1))
            if [[ $nargs -eq 1 ]]; then
                # Complete example names
                local examples
                examples="$(_blusys_complete_examples)"
                COMPREPLY=($(compgen -W "$examples" -- "$cur"))
            elif [[ $nargs -eq 2 ]]; then
                # Complete subcommands
                COMPREPLY=($(compgen -W "$_blusys_example_commands" -- "$cur"))
            elif [[ $nargs -ge 3 ]]; then
                # Complete targets and ports based on the subcommand
                local subcmd="${words[3]}"
                case "$subcmd" in
                    build|config|menuconfig|size|clean|qemu)
                        COMPREPLY=($(compgen -W "$_blusys_targets" -- "$cur"))
                        ;;
                    flash|monitor|run|erase)
                        local ports
                        ports="$(_blusys_complete_ports)"
                        COMPREPLY=($(compgen -W "$_blusys_targets $ports" -- "$cur"))
                        ;;
                esac
            fi
            ;;

        fullclean)
            if [[ $((cword - 1)) -eq 1 ]]; then
                local projects
                projects="$(_blusys_complete_projects)"
                COMPREPLY=($(compgen -W "$projects" -- "$cur"))
                compopt -o nospace 2>/dev/null
            fi
            ;;

        help)
            COMPREPLY=($(compgen -W "$_blusys_commands" -- "$cur"))
            ;;

        # No completions for these
        build-examples|install-qemu|config-idf|version|update)
            ;;
    esac
}

complete -F _blusys blusys
