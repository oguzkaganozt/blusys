# Sourced by repo-root `blusys` — `create` command and project scaffolding.

cmd_create() {
    local archetype=""
    local starter=""
    local positional=()

    while [[ $# -gt 0 ]]; do
        case "$1" in
            --archetype)
                if [[ $# -lt 2 ]]; then
                    printf 'error: --archetype requires an argument\n' >&2
                    exit 1
                fi
                archetype="$2"
                shift 2
                ;;
            --archetype=*)
                archetype="${1#--archetype=}"
                shift
                ;;
            --starter)
                if [[ $# -lt 2 ]]; then
                    printf 'error: --starter requires an argument (headless|interactive)\n' >&2
                    exit 1
                fi
                starter="$2"
                shift 2
                ;;
            --starter=*)
                starter="${1#--starter=}"
                shift
                ;;
            --list-archetypes)
                blusys_list_archetypes
                exit 0
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

    case "$starter" in
        ""|headless|interactive) ;;
        *)
            printf 'error: --starter must be "headless" or "interactive" (got %q)\n' "$starter" >&2
            exit 1
            ;;
    esac

    case "$archetype" in
        "")
            case "$starter" in
                ""|interactive) archetype="interactive-controller" ;;
                headless) archetype="edge-node" ;;
            esac
            ;;
        interactive-controller|interactive_controller|controller)
            archetype="interactive-controller"
            ;;
        interactive-panel|interactive_panel|panel)
            archetype="interactive-panel"
            ;;
        edge-node|edge_node|edge|headless)
            archetype="edge-node"
            ;;
        gateway-controller|gateway_controller|gateway)
            archetype="gateway-controller"
            ;;
        *)
            printf 'error: unknown archetype %q\n' "$archetype" >&2
            blusys_help_create
            exit 1
            ;;
    esac

    local template_dir=""
    local template_host_target=""
    local starter_kind=""
    local archetype_label=""
    local archetype_example=""
    local archetype_default_profile=""

    case "$archetype" in
        interactive-controller)
            template_dir="$BLUSYS_REPO_ROOT/examples/quickstart/interactive_controller"
            template_host_target="interactive_controller_host"
            starter_kind="interactive"
            archetype_label="interactive controller"
            archetype_example="examples/quickstart/interactive_controller/"
            archetype_default_profile="ST7735"
            ;;
        interactive-panel)
            template_dir="$BLUSYS_REPO_ROOT/examples/reference/interactive_panel"
            template_host_target="interactive_panel_host"
            starter_kind="interactive"
            archetype_label="interactive panel"
            archetype_example="examples/reference/interactive_panel/"
            archetype_default_profile="ILI9341"
            ;;
        edge-node)
            template_dir="$BLUSYS_REPO_ROOT/examples/quickstart/edge_node"
            template_host_target="edge_node_host"
            starter_kind="headless"
            archetype_label="edge node"
            archetype_example="examples/quickstart/edge_node/"
            archetype_default_profile="headless"
            ;;
        gateway-controller)
            template_dir="$BLUSYS_REPO_ROOT/examples/reference/gateway"
            template_host_target="gateway_host"
            starter_kind="headless"
            archetype_label="gateway/controller"
            archetype_example="examples/reference/gateway/"
            archetype_default_profile="headless"
            ;;
    esac

    if [[ "$archetype" == "gateway-controller" && "$starter" == "interactive" ]]; then
        starter_kind="interactive"
        archetype_default_profile="ILI9341"
    fi

    if [[ -n "$starter" ]]; then
        case "$starter" in
            interactive)
                if [[ "$starter_kind" != "interactive" ]]; then
                    printf 'error: --starter interactive does not match archetype %q\n' "$archetype" >&2
                    exit 1
                fi
                ;;
            headless)
                if [[ "$starter_kind" != "headless" ]]; then
                    printf 'error: --starter headless does not match archetype %q\n' "$archetype" >&2
                    exit 1
                fi
                ;;
        esac
    fi

    local target_dir
    if [[ ${#positional[@]} -eq 0 ]]; then
        target_dir="$(pwd)"
    else
    if [[ "${positional[0]}" = /* ]]; then
            target_dir="${positional[0]}"
        else
            target_dir="$(pwd)/${positional[0]}"
        fi
    fi

    mkdir -p "$target_dir"
    target_dir="$(cd "$target_dir" && pwd)"

    local project_name
    project_name="$(basename "$target_dir")"

    # Check for existing project files.
    local existing_files=()
    for f in \
        CMakeLists.txt \
        README.md \
        sdkconfig.defaults \
        partitions.csv \
        main/CMakeLists.txt \
        main/idf_component.yml \
        main/Kconfig.projbuild \
        main/core/app_logic.hpp \
        main/ui/app_ui.cpp \
        main/integration/app_main.cpp \
        host/CMakeLists.txt; do
        [[ -f "$target_dir/$f" ]] && existing_files+=("$f")
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

    mkdir -p "$target_dir/main" "$target_dir/host"

    cp -f "$template_dir/sdkconfig.defaults" "$target_dir/sdkconfig.defaults"
    if [[ -f "$template_dir/partitions.csv" ]]; then
        cp -f "$template_dir/partitions.csv" "$target_dir/partitions.csv"
    fi

    cp -R "$template_dir/main/." "$target_dir/main/"
    cp -R "$template_dir/host/." "$target_dir/host/"

    if [[ -f "$target_dir/host/CMakeLists.txt" ]]; then
        sed -i "s/${template_host_target}/${project_name}_host/g" "$target_dir/host/CMakeLists.txt"
    fi

    # Replace stub idf_component.yml from in-repo examples with managed git pins
    # so projects build outside the monorepo (single env-controlled tag).
    case "$archetype" in
        interactive-controller|interactive-panel)
            blusys_create_main_yml_interactive "$target_dir"
            ;;
        edge-node)
            blusys_create_main_yml_headless "$target_dir"
            ;;
        gateway-controller)
            if [[ "$starter_kind" == "interactive" ]]; then
                blusys_create_main_yml_interactive_mdns "$target_dir"
            fi
            ;;
    esac

    if [[ "$archetype" == "gateway-controller" && "$starter_kind" == "headless" ]]; then
        local gw_tmpl="$BLUSYS_REPO_ROOT/templates/scaffold/gateway_headless"
        cp -f "$gw_tmpl/main/CMakeLists.txt" "$target_dir/main/CMakeLists.txt"
        mkdir -p "$target_dir/main/ui"
        cp -f "$gw_tmpl/main/ui/README.md" "$target_dir/main/ui/README.md"
        cp -f "$gw_tmpl/main/integration/app_main.cpp" "$target_dir/main/integration/app_main.cpp"
        cp -f "$gw_tmpl/host/CMakeLists.txt" "$target_dir/host/CMakeLists.txt"
        sed -i "s/BLUSYS_PRODUCT_HOST/${project_name}_host/g" "$target_dir/host/CMakeLists.txt"
        blusys_create_main_yml_headless "$target_dir"
    fi

    blusys_create_top_cmakelists "$target_dir" "$project_name" "$starter_kind"

    cat > "$target_dir/README.md" <<'EOF'
# __PROJECT_NAME__

Generated with `blusys create --archetype __ARCHETYPE__`.

Canonical starter: __ARCHETYPE_LABEL__

## Layout

- `main/core/` — state, actions, `update()` (product behavior)
- `main/ui/` — screens and view code when the archetype uses UI (may be absent or minimal for headless)
- `main/integration/` — `app_spec`, capabilities, entry macro (`integration/app_main.cpp`)

## Commands (from this directory)

| Goal | Command |
|------|---------|
| Host (SDL2, no hardware) | `blusys host-build` then run `./build-host/__PROJECT_NAME___host` |
| Firmware build | `blusys build` (default target: check `blusys help` / your `sdkconfig`) |
| Flash + monitor | `blusys run /dev/ttyACM0` (replace with your serial port) |
| Flash only | `blusys flash /dev/ttyACM0` |
| Menuconfig | `blusys menuconfig` |

Set `IDF_PATH` (ESP-IDF v5.5+) before device builds, or run `blusys config-idf` once.

**Host input (interactive):** arrow keys simulate encoder rotation; Enter confirms (see platform host docs).

## Reference

Upstream example: `__ARCHETYPE_EXAMPLE__` in the blusys repository.
EOF
    sed -i \
        -e "s|__PROJECT_NAME__|$project_name|g" \
        -e "s|__ARCHETYPE__|$archetype|g" \
        -e "s|__ARCHETYPE_LABEL__|$archetype_label|g" \
        -e "s|__ARCHETYPE_EXAMPLE__|$archetype_example|g" \
        "$target_dir/README.md"

    printf '\nCreated blusys project: %s\n' "$target_dir"
    printf 'Starter archetype: %s\n' "$archetype_label"
    printf 'Default profile family: %s\n\n' "$archetype_default_profile"
    printf 'Next steps:\n'
    printf '  cd %s\n' "$target_dir"
    printf '  blusys host-build           # build + run on PC (no hardware needed)\n'
    printf '  blusys build                # build for default target\n'
    printf '  blusys run                  # build, flash, and monitor\n'
    printf '\n'
    printf 'Product layout:\n'
    printf '  main/core/   — State, Action, update() (product behavior)\n'
    if [[ "$starter_kind" == "interactive" ]]; then
        printf '  main/ui/      — on_init() and screen composition (rendering)\n'
    fi
    printf '  main/integration/  — app_spec wiring and entry macro\n'
}

# ── cmd_create helpers ──────────────────────────────────────────────────────

blusys_create_top_cmakelists() {
    local target_dir="$1"
    local project_name="$2"
    local starter="$3"
    local build_ui="OFF"
    if [[ "$starter" == "interactive" ]]; then
        build_ui="ON"
    fi

    cat > "$target_dir/CMakeLists.txt" <<CMAKE
cmake_minimum_required(VERSION 3.16)

# BLUSYS_BUILD_UI is baked in at scaffold time by blusys create.
# The framework component reads this from cache during ESP-IDF's component scan.
set(BLUSYS_BUILD_UI ${build_ui} CACHE BOOL "Build blusys_framework/ui")

include(\$ENV{IDF_PATH}/tools/cmake/project.cmake)
project(${project_name})
CMAKE
}

# mode: interactive | headless | interactive_mdns
blusys_create_main_yml() {
    local target_dir="$1"
    local mode="$2"
    local out="$target_dir/main/idf_component.yml"

    {
        cat <<'HEADER'
## Managed dependencies for the blusys platform.
##
## All three platform components must be pinned to the SAME git ref.
## Mixing tiers across versions is not supported (decision 7).
##
## To test against a local platform checkout instead of the pinned tag,
## replace git/version/path for each blusys_* dep with override_path, e.g.:
##     override_path: "../../path/to/blusys/components/COMPONENT_NAME"
dependencies:
  idf:
    version: ">=5.5"

HEADER
        if [[ "$mode" == "interactive_mdns" ]]; then
            cat <<'MDNS_BEFORE'
  espressif/mdns:
    version: "*"

MDNS_BEFORE
        fi

        cat <<'PLATFORM'
  blusys:
    git: "https://github.com/oguzkaganozt/blusys.git"
    version: "__BLUSYS_SCAFFOLD_TAG__"
    path: "components/blusys"
  blusys_services:
    git: "https://github.com/oguzkaganozt/blusys.git"
    version: "__BLUSYS_SCAFFOLD_TAG__"
    path: "components/blusys_services"
  blusys_framework:
    git: "https://github.com/oguzkaganozt/blusys.git"
    version: "__BLUSYS_SCAFFOLD_TAG__"
    path: "components/blusys_framework"

PLATFORM
        if [[ "$mode" == "headless" ]]; then
            cat <<'MDNS_AFTER'
  espressif/mdns:
    version: "*"
MDNS_AFTER
        elif [[ "$mode" == "interactive" || "$mode" == "interactive_mdns" ]]; then
            cat <<'LVGL'
  lvgl/lvgl:
    version: "~9.2"
LVGL
        fi
    } >"$out"

    sed -i "s/__BLUSYS_SCAFFOLD_TAG__/${BLUSYS_SCAFFOLD_PLATFORM_VERSION}/g" "$out"
}

blusys_create_main_yml_interactive() {
    blusys_create_main_yml "$1" interactive
}

blusys_create_main_yml_headless() {
    blusys_create_main_yml "$1" headless
}

# Interactive + managed pins + mDNS (gateway interactive archetype).
blusys_create_main_yml_interactive_mdns() {
    blusys_create_main_yml "$1" interactive_mdns
}

