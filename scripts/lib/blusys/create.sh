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

blusys_create_main() {
    local target_dir="$1"
    local starter="$2"

    if [[ "$starter" == "interactive" ]]; then
        mkdir -p "$target_dir/main/core" "$target_dir/main/ui" "$target_dir/main/integration"
        cat > "$target_dir/main/CMakeLists.txt" <<'CMAKE'
idf_component_register(
    SRCS
        "core/app_logic.cpp"
        "ui/app_ui.cpp"
        "integration/app_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES blusys_framework
)
CMAKE
        blusys_create_main_yml_interactive "$target_dir"
        blusys_create_logic_interactive_controller "$target_dir"
        blusys_create_ui_interactive_controller "$target_dir"
        blusys_create_system_interactive_controller "$target_dir"
    else
        mkdir -p "$target_dir/main/core" "$target_dir/main/integration"
        cat > "$target_dir/main/CMakeLists.txt" <<'CMAKE'
idf_component_register(
    SRCS
        "core/app_logic.cpp"
        "integration/app_main.cpp"
    INCLUDE_DIRS "."
    REQUIRES blusys_framework
)
CMAKE
        blusys_create_main_yml_headless "$target_dir"
        blusys_create_logic_headless "$target_dir"
        blusys_create_system_headless "$target_dir"
    fi
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

blusys_create_logic_headless() {
    local target_dir="$1"
    cat > "$target_dir/main/core/app_logic.hpp" <<'HPP'
#pragma once
// State and Action for the headless product.
// Edit these to define your product's data model and event vocabulary.

#include "blusys/app/app.hpp"
#include <cstdint>

// Product state — owned by the reducer, updated by update().
struct State {
    int tick_count = 0;
};

// Product actions — dispatched by integration/ hooks and capability events.
enum class Action {
    periodic_tick,
};

// Reducer and tick hook — implemented in app_logic.cpp.
void update(blusys::app::app_ctx &ctx, State &state, const Action &action);
void on_tick(blusys::app::app_ctx &ctx, State &state, std::uint32_t now_ms);
HPP

    cat > "$target_dir/main/core/app_logic.cpp" <<'CPP'
// core/app_logic.cpp — product reducer.
//
// update() is the single entry point for all state transitions.
// on_tick() runs at the configured tick_period_ms interval.

#include "core/app_logic.hpp"
#include "blusys/app/app.hpp"
#include "blusys/log.h"

void update(blusys::app::app_ctx & /*ctx*/, State &state, const Action &action)
{
    switch (action) {
    case Action::periodic_tick:
        ++state.tick_count;
        if (state.tick_count % 100 == 0) {
            BLUSYS_LOGI("app", "heartbeat #%d", state.tick_count);
        }
        break;
    }
}

void on_tick(blusys::app::app_ctx &ctx, State & /*state*/, std::uint32_t /*now_ms*/)
{
    ctx.dispatch(Action::periodic_tick);
}
CPP
}

blusys_create_system_headless() {
    local target_dir="$1"
    cat > "$target_dir/main/integration/app_main.cpp" <<'CPP'
// integration/app_main.cpp — headless product entry point.
//
// Wires the reducer, tick hook, and optional capabilities into
// an app_spec and hands it to the framework runtime.
// The framework owns the runtime loop — no manual plumbing needed.

#include "core/app_logic.hpp"
#include "blusys/app/app.hpp"

static const blusys::app::app_spec<State, Action> spec{
    .initial_state  = {},
    .update         = update,
    .on_tick        = on_tick,
    .tick_period_ms = 100,
};

BLUSYS_APP_MAIN_HEADLESS(spec)
CPP
}

blusys_create_logic_interactive() {
    local target_dir="$1"
    cat > "$target_dir/main/core/app_logic.hpp" <<'HPP'
#pragma once
// State and Action for the interactive product.
// Edit these to define your product's data model and event vocabulary.

#include "blusys/app/app.hpp"
#include "lvgl.h"
#include <cstdint>

// Product state — owned by the reducer, updated by update().
// counter_label is a UI reference stored in State so the reducer
// can update the display directly when the counter changes.
struct State {
    std::int32_t counter       = 0;
    lv_obj_t    *counter_label = nullptr;
};

enum class Tag : std::uint8_t { increment, decrement, confirm };
struct Action { Tag tag; };

// Reducer and intent map — implemented in app_logic.cpp.
void update(blusys::app::app_ctx &ctx, State &state, const Action &action);
bool map_intent(blusys::framework::intent intent, Action *out);
HPP

    cat > "$target_dir/main/core/app_logic.cpp" <<'CPP'
// core/app_logic.cpp — product reducer.
//
// update() is the single entry point for all state transitions.
// map_intent() translates encoder/keyboard events into product actions.

#include "core/app_logic.hpp"
#include "blusys/app/app.hpp"
#include "blusys/log.h"

#include <cstdio>

namespace view = blusys::app::view;

void update(blusys::app::app_ctx &ctx, State &state, const Action &action)
{
    switch (action.tag) {
    case Tag::increment:
        ++state.counter;
        {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%ld", static_cast<long>(state.counter));
            view::set_text(state.counter_label, buf);
        }
        ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                          blusys::framework::feedback_pattern::click);
        break;

    case Tag::decrement:
        --state.counter;
        {
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%ld", static_cast<long>(state.counter));
            view::set_text(state.counter_label, buf);
        }
        ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                          blusys::framework::feedback_pattern::click);
        break;

    case Tag::confirm:
        ctx.emit_feedback(blusys::framework::feedback_channel::audio,
                          blusys::framework::feedback_pattern::confirm);
        BLUSYS_LOGI("app", "confirmed — counter=%ld",
                    static_cast<long>(state.counter));
        break;
    }
}

bool map_intent(blusys::framework::intent intent, Action *out)
{
    switch (intent) {
    case blusys::framework::intent::increment:
        *out = Action{.tag = Tag::increment}; return true;
    case blusys::framework::intent::decrement:
        *out = Action{.tag = Tag::decrement}; return true;
    case blusys::framework::intent::confirm:
        *out = Action{.tag = Tag::confirm}; return true;
    default:
        return false;
    }
}
CPP
}

blusys_create_ui_interactive() {
    local target_dir="$1"
    cat > "$target_dir/main/ui/app_ui.cpp" <<'CPP'
// ui/app_ui.cpp — interactive UI entry point.
//
// on_init() builds the initial screen. Add more screens by registering
// them with ctx.screen_router() and navigating with ctx.navigate_to().

#include "core/app_logic.hpp"
#include "blusys/app/app.hpp"
#include "blusys/log.h"

namespace view = blusys::app::view;

void on_init(blusys::app::app_ctx &ctx, State &state)
{
    auto p = view::page_create();

    view::title(p.content, "Hello, blusys");
    view::divider(p.content);

    state.counter_label = view::label(p.content, "0");

    auto *btn_row = view::row(p.content);
    view::button(btn_row, "-", Action{.tag = Tag::decrement}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(btn_row, "+", Action{.tag = Tag::increment}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(btn_row, "OK", Action{.tag = Tag::confirm}, &ctx);

    view::page_load(p);

    BLUSYS_LOGI("app", "UI initialized");
}
CPP
}

blusys_create_system_interactive() {
    local target_dir="$1"
    cat > "$target_dir/main/integration/app_main.cpp" <<'CPP'
// integration/app_main.cpp — interactive product entry point.
//
// Wires State, Action, update(), on_init(), and map_intent() into
// an app_spec and hands it to the framework runtime.
//
// The framework owns display setup, LVGL lifecycle, and the runtime
// loop — no manual plumbing needed.
//
// Host-first: runs in an SDL2 window on the PC.
// To target a device, swap the macro at the bottom for:
//   #include "blusys/app/profiles/st7735.hpp"
//   BLUSYS_APP_MAIN_DEVICE(spec, blusys::app::profiles::st7735_160x128())

#include "core/app_logic.hpp"
#include "blusys/app/app.hpp"

// on_init is implemented in ui/app_ui.cpp.
void on_init(blusys::app::app_ctx &ctx, State &state);

static const blusys::app::app_spec<State, Action> spec{
    .initial_state = {},
    .update        = update,
    .on_init       = on_init,
    .map_intent    = map_intent,
};

BLUSYS_APP_MAIN_HOST(spec)
CPP
}

blusys_create_logic_interactive_controller() {
    local target_dir="$1"
    cat > "$target_dir/main/core/app_logic.hpp" <<'HPP'
#pragma once

#include "blusys/app/app.hpp"
#include "blusys/app/capabilities/provisioning.hpp"
#include "blusys/app/flows/provisioning_flow.hpp"

#include <cstdint>

enum RouteId : std::uint32_t {
    ROUTE_HOME = 1,
    ROUTE_SETTINGS,
    ROUTE_SETUP,
    ROUTE_ABOUT,
};

enum OverlayId : std::uint32_t {
    OVERLAY_CONFIRM = 1,
};

struct State {
    std::int32_t level         = 64;
    bool         hold_enabled  = false;
    std::int32_t accent_index  = 1;
    bool         storage_ready = false;
    blusys::app::provisioning_status provisioning{};

    lv_obj_t *shell_badge      = nullptr;
    lv_obj_t *shell_detail     = nullptr;
    lv_obj_t *home_gauge       = nullptr;
    lv_obj_t *home_preset      = nullptr;
    lv_obj_t *home_hold_badge  = nullptr;
    lv_obj_t *settings_hint    = nullptr;
    blusys::app::flows::provisioning_screen_handles setup_handles{};
};

enum class Tag : std::uint8_t {
    level_delta,
    toggle_hold,
    set_accent,
    confirm,
    show_home,
    show_settings,
    open_setup,
    open_about,
    sync_storage,
    sync_provisioning,
};

struct Action {
    Tag tag;
    std::int32_t value = 0;
};

void update(blusys::app::app_ctx &ctx, State &state, const Action &action);
bool map_intent(blusys::framework::intent intent, Action *out);
const char *accent_name(std::int32_t index);
HPP

    cat > "$target_dir/main/core/app_logic.cpp" <<'CPP'
#include "core/app_logic.hpp"

#include "blusys/app/flows/provisioning_flow.hpp"
#include "blusys/app/view/bindings.hpp"
#include "blusys/framework/ui/widgets/gauge/gauge.hpp"
#include "blusys/log.h"

#include <cstdio>

namespace {

std::int32_t clamp_level(std::int32_t value)
{
    if (value < 0) return 0;
    if (value > 100) return 100;
    return value;
}

void sync_ui(State &state)
{
    if (state.shell_badge != nullptr) {
        const bool ready = state.provisioning.capability_ready || state.provisioning.is_provisioned;
        blusys::app::view::set_badge_text(state.shell_badge, ready ? "Ready" : "Setup");
        blusys::app::view::set_badge_level(
            state.shell_badge,
            ready ? blusys::ui::badge_level::success : blusys::ui::badge_level::warning);
    }

    if (state.shell_detail != nullptr) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%s  %ld%%",
                      accent_name(state.accent_index),
                      static_cast<long>(state.level));
        blusys::app::view::set_text(state.shell_detail, buf);
    }

    if (state.home_gauge != nullptr) {
        blusys::ui::gauge_set_value(state.home_gauge, state.level);
    }
    if (state.home_preset != nullptr) {
        blusys::app::view::set_kv_value(state.home_preset, accent_name(state.accent_index));
    }
    if (state.home_hold_badge != nullptr) {
        blusys::app::view::set_badge_text(state.home_hold_badge,
                                          state.hold_enabled ? "Hold" : "Live");
        blusys::app::view::set_badge_level(
            state.home_hold_badge,
            state.hold_enabled ? blusys::ui::badge_level::warning
                               : blusys::ui::badge_level::success);
    }
    if (state.settings_hint != nullptr) {
        blusys::app::view::set_text(state.settings_hint, accent_name(state.accent_index));
    }

    blusys::app::flows::provisioning_screen_update(state.setup_handles, state.provisioning);
}

}  // namespace

const char *accent_name(std::int32_t index)
{
    switch (index) {
    case 0: return "Warm";
    case 1: return "Punch";
    case 2: return "Night";
    default: return "Punch";
    }
}

void update(blusys::app::app_ctx &ctx, State &state, const Action &action)
{
    switch (action.tag) {
    case Tag::level_delta:
        if (!state.hold_enabled) {
            state.level = clamp_level(state.level + action.value);
        }
        ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                          blusys::framework::feedback_pattern::click);
        break;

    case Tag::toggle_hold:
        state.hold_enabled = !state.hold_enabled;
        ctx.emit_feedback(blusys::framework::feedback_channel::haptic,
                          state.hold_enabled
                              ? blusys::framework::feedback_pattern::warning
                              : blusys::framework::feedback_pattern::click);
        break;

    case Tag::set_accent:
        state.accent_index = action.value;
        ctx.emit_feedback(blusys::framework::feedback_channel::audio,
                          blusys::framework::feedback_pattern::click);
        break;

    case Tag::confirm:
        ctx.show_overlay(OVERLAY_CONFIRM);
        ctx.emit_feedback(blusys::framework::feedback_channel::audio,
                          blusys::framework::feedback_pattern::confirm);
        BLUSYS_LOGI("app", "confirmed level=%ld accent=%s hold=%s",
                    static_cast<long>(state.level),
                    accent_name(state.accent_index),
                    state.hold_enabled ? "yes" : "no");
        break;

    case Tag::show_home:
        ctx.navigate_to(ROUTE_HOME);
        break;

    case Tag::show_settings:
        ctx.navigate_to(ROUTE_SETTINGS);
        break;

    case Tag::open_setup:
        ctx.navigate_push(ROUTE_SETUP);
        break;

    case Tag::open_about:
        ctx.navigate_push(ROUTE_ABOUT);
        break;

    case Tag::sync_storage:
        if (const auto *storage = ctx.storage(); storage != nullptr) {
            state.storage_ready = storage->capability_ready;
        }
        break;

    case Tag::sync_provisioning:
        if (const auto *prov = ctx.provisioning(); prov != nullptr) {
            state.provisioning = *prov;
        }
        break;
    }

    sync_ui(state);
}

bool map_intent(blusys::framework::intent intent, Action *out)
{
    switch (intent) {
    case blusys::framework::intent::increment:
        *out = Action{.tag = Tag::level_delta, .value = 4};
        return true;
    case blusys::framework::intent::decrement:
        *out = Action{.tag = Tag::level_delta, .value = -4};
        return true;
    case blusys::framework::intent::confirm:
        *out = Action{.tag = Tag::confirm};
        return true;
    case blusys::framework::intent::cancel:
        *out = Action{.tag = Tag::toggle_hold};
        return true;
    default:
        return false;
    }
}
CPP
}

blusys_create_ui_interactive_controller() {
    local target_dir="$1"
    cat > "$target_dir/main/ui/app_ui.cpp" <<'CPP'
#include "core/app_logic.hpp"
#include "blusys/app/app.hpp"
#include "blusys/app/flows/settings.hpp"
#include "blusys/app/screens/about_screen.hpp"

namespace view = blusys::app::view;

namespace {

State *g_state = nullptr;

view::page make_page(blusys::app::app_ctx &ctx, bool scrollable)
{
    if (ctx.shell() != nullptr) {
        return view::page_create_in(ctx.shell()->content_area, {.scrollable = scrollable});
    }
    return view::page_create({.scrollable = scrollable});
}

void on_home_show(lv_obj_t *, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx != nullptr && ctx->shell() != nullptr) {
        view::shell_set_title(*ctx->shell(), "Controller");
        view::shell_set_active_tab(*ctx->shell(), 0);
    }
}

void on_settings_show(lv_obj_t *, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx != nullptr && ctx->shell() != nullptr) {
        view::shell_set_title(*ctx->shell(), "Settings");
        view::shell_set_active_tab(*ctx->shell(), 1);
    }
}

void on_setup_show(lv_obj_t *, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx != nullptr && ctx->shell() != nullptr) {
        view::shell_set_title(*ctx->shell(), "Setup");
    }
}

void on_about_show(lv_obj_t *, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx != nullptr && ctx->shell() != nullptr) {
        view::shell_set_title(*ctx->shell(), "About");
    }
}

void on_settings_changed(void *user_ctx, std::size_t item_index, std::int32_t value)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_ctx);
    if (ctx == nullptr) {
        return;
    }

    switch (item_index) {
    case 1:
        ctx->dispatch(Action{.tag = Tag::set_accent, .value = value});
        break;
    case 2:
        ctx->dispatch(Action{.tag = Tag::toggle_hold});
        break;
    case 3:
        ctx->dispatch(Action{.tag = Tag::open_about});
        break;
    default:
        break;
    }
}

lv_obj_t *create_home(blusys::app::app_ctx &ctx, const void *, lv_group_t **group_out)
{
    auto page = make_page(ctx, false);
    auto *hero = view::card(page.content, "My Controller");
    lv_obj_set_width(hero, LV_PCT(100));

    view::label(hero, "Drive");
    g_state->home_gauge = view::gauge(hero, 0, 100, g_state->level, "%");
    g_state->home_preset = view::key_value(hero, "Accent", accent_name(g_state->accent_index));
    g_state->home_hold_badge = view::status_badge(hero,
                                                  g_state->hold_enabled ? "Hold" : "Live",
                                                  g_state->hold_enabled
                                                      ? blusys::ui::badge_level::warning
                                                      : blusys::ui::badge_level::success);

    auto *controls = view::row(page.content);
    view::button(controls, "Down", Action{.tag = Tag::level_delta, .value = -6}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(controls, "Up", Action{.tag = Tag::level_delta, .value = 6}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(controls, "Hold", Action{.tag = Tag::toggle_hold}, &ctx,
                 blusys::ui::button_variant::ghost);

    auto *actions = view::row(page.content);
    view::button(actions, "Setup", Action{.tag = Tag::open_setup}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(actions, "Commit", Action{.tag = Tag::confirm}, &ctx);

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return page.screen;
}

lv_obj_t *create_settings(blusys::app::app_ctx &ctx, const void *, lv_group_t **group_out)
{
    static constexpr const char *kAccentOptions[] = {"Warm", "Punch", "Night"};

    blusys::app::flows::setting_item items[] = {
        {.label = "Character", .type = blusys::app::flows::setting_type::section_header},
        {
            .label = "Accent",
            .description = "Shift the controller voice.",
            .type = blusys::app::flows::setting_type::dropdown,
            .dropdown_options = kAccentOptions,
            .dropdown_count = 3,
            .dropdown_initial = g_state->accent_index,
        },
        {
            .label = "Hold Output",
            .description = "Freeze adjustments until released.",
            .type = blusys::app::flows::setting_type::toggle,
            .toggle_initial = g_state->hold_enabled,
        },
        {
            .label = "About Device",
            .description = "Inspect scaffold metadata.",
            .type = blusys::app::flows::setting_type::action_button,
            .button_label = "Open",
        },
    };

    const blusys::app::flows::settings_screen_config config{
        .title = "Controller Settings",
        .items = items,
        .item_count = sizeof(items) / sizeof(items[0]),
        .on_changed = on_settings_changed,
        .user_ctx = &ctx,
    };
    return blusys::app::flows::settings_screen_create(ctx, &config, group_out);
}

lv_obj_t *create_setup(blusys::app::app_ctx &ctx, const void *, lv_group_t **group_out)
{
    const blusys::app::flows::provisioning_flow_config config{
        .title = "Pair Controller",
        .qr_label = "BLE bootstrap payload:",
        .waiting_msg = "Waiting for product credentials...",
        .success_msg = "Controller paired.",
        .error_msg = "Provisioning failed.",
    };
    return blusys::app::flows::provisioning_screen_create(
        ctx, config, g_state->setup_handles, group_out);
}

lv_obj_t *create_about(blusys::app::app_ctx &ctx, const void *, lv_group_t **group_out)
{
    static const blusys::app::screens::about_extra_field extras[] = {
        {.key = "Archetype", .value = "interactive controller"},
        {.key = "Profile", .value = "generic SPI ST7735"},
        {.key = "Identity", .value = "expressive_dark"},
    };

    const blusys::app::screens::about_screen_config config{
        .product_name = "My Controller",
        .firmware_version = "dev",
        .hardware_version = "ST7735 reference",
        .serial_number = "CTRL-0001",
        .build_date = __DATE__,
        .extras = extras,
        .extra_count = sizeof(extras) / sizeof(extras[0]),
    };
    return blusys::app::screens::about_screen_create(ctx, &config, group_out);
}

}  // namespace

void on_init(blusys::app::app_ctx &ctx, State &state)
{
    g_state = &state;

    if (ctx.shell() != nullptr) {
        const view::shell_tab_item tabs[] = {
            {.label = "Ctrl", .route_id = ROUTE_HOME},
            {.label = "Set", .route_id = ROUTE_SETTINGS},
        };
        view::shell_set_tabs(*ctx.shell(), tabs, sizeof(tabs) / sizeof(tabs[0]), &ctx);

        if (lv_obj_t *surface = view::shell_status_surface(*ctx.shell()); surface != nullptr) {
            state.shell_badge = view::status_badge(surface, "Setup", blusys::ui::badge_level::warning);
            state.shell_detail = view::label(surface, "Punch  64%", blusys::ui::theme().font_body_sm);
        }

        if (ctx.overlay_manager() != nullptr) {
            view::overlay_create(ctx.shell()->root, OVERLAY_CONFIRM,
                                 {.text = "Scene committed", .duration_ms = 1200},
                                 *ctx.overlay_manager());
        }
    }

    auto *router = ctx.screen_router();
    if (router == nullptr) {
        return;
    }

    router->register_screen(ROUTE_HOME, &create_home, nullptr,
                            {.on_show = on_home_show, .user_data = &ctx});
    router->register_screen(ROUTE_SETTINGS, &create_settings, nullptr,
                            {.on_show = on_settings_show, .user_data = &ctx});
    router->register_screen(ROUTE_SETUP, &create_setup, nullptr,
                            {.on_show = on_setup_show, .user_data = &ctx});
    router->register_screen(ROUTE_ABOUT, &create_about, nullptr,
                            {.on_show = on_about_show, .user_data = &ctx});

    ctx.navigate_to(ROUTE_HOME);
}
CPP
}

blusys_create_system_interactive_controller() {
    local target_dir="$1"
    cat > "$target_dir/main/integration/app_main.cpp" <<'CPP'
#include "core/app_logic.hpp"
#include "blusys/app/app.hpp"

constexpr blusys::app::view::shell_config kShellConfig{
    .header = {.enabled = true, .title = "Controller"},
    .status = {.enabled = true},
    .tabs = {.enabled = true},
};

// on_init is implemented in ui/app_ui.cpp.
void on_init(blusys::app::app_ctx &ctx, State &state);

bool map_event(std::uint32_t id, std::uint32_t /*code*/, const void * /*payload*/, Action *out)
{
    switch (id) {
    case static_cast<std::uint32_t>(blusys::app::storage_event::capability_ready):
        *out = Action{.tag = Tag::sync_storage};
        return true;
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::started):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::credentials_received):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::success):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::failed):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::already_provisioned):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::reset_complete):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::capability_ready):
        *out = Action{.tag = Tag::sync_provisioning};
        return true;
    default:
        return false;
    }
}

static blusys::app::storage_capability storage{{
    .spiffs_base_path = "/controller",
}};

static blusys::app::provisioning_capability provisioning{{
    .service_name = "my-controller",
    .pop = "123456",
}};

static blusys::app::capability_list capabilities{&storage, &provisioning};

static const blusys::app::app_spec<State, Action> spec{
    .initial_state = {},
    .update        = update,
    .on_init       = on_init,
    .map_intent    = map_intent,
    .map_event     = map_event,
    .capabilities  = &capabilities,
    .theme         = &blusys::app::presets::expressive_dark(),
    .shell         = &kShellConfig,
};

#ifdef ESP_PLATFORM
#include "blusys/app/profiles/st7735.hpp"
BLUSYS_APP_MAIN_DEVICE(spec, blusys::app::profiles::st7735_160x128())
#else
BLUSYS_APP_MAIN_HOST_PROFILE(spec, (blusys::app::host_profile{
    .hor_res = 240,
    .ver_res = 240,
    .title   = "My Controller",
}))
#endif
CPP
}

blusys_create_host() {
    local target_dir="$1"
    local project_name="$2"
    local starter="$3"

    if [[ "$starter" == "interactive" ]]; then
        blusys_create_host_cmakelists_interactive "$target_dir" "$project_name"
    else
        blusys_create_host_cmakelists_headless "$target_dir" "$project_name"
    fi
}

blusys_create_host_cmakelists_interactive() {
    local target_dir="$1"
    local project_name="$2"
    # PLACEHOLDER is replaced by sed after writing so cmake ${} vars
    # don't need to be escaped in the heredoc.
    cat > "$target_dir/host/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.16)
project(BLUSYS_PRODUCT_HOST LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# BLUSYS_PATH is exported by the blusys CLI automatically.
# For manual builds: export BLUSYS_PATH=/path/to/blusys && cmake -S host -B build-host
if(NOT DEFINED ENV{BLUSYS_PATH})
    message(FATAL_ERROR
        "BLUSYS_PATH is not set.\n"
        "Run via 'blusys host-build' (it exports BLUSYS_PATH automatically), or:\n"
        "  export BLUSYS_PATH=/path/to/blusys && cmake -S host -B build-host")
endif()
set(BLUSYS_PATH "$ENV{BLUSYS_PATH}")

# SDL2
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

# LVGL — pinned to the same tag as the managed component (lvgl/lvgl ~9.2)
include(FetchContent)
set(LV_CONF_PATH "${BLUSYS_PATH}/scripts/host/lv_conf.h" CACHE FILEPATH "")
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_EXAMPLES        ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS           ON CACHE BOOL "" FORCE)
FetchContent_Declare(lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v9.2.2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(lvgl)
# Strip ARM/Helium assembly that x86_64 assembler cannot compile.
get_target_property(_lvgl_src lvgl SOURCES)
list(FILTER _lvgl_src EXCLUDE REGEX "\\.S$")
set_property(TARGET lvgl PROPERTY SOURCES ${_lvgl_src})
target_link_libraries(lvgl PUBLIC PkgConfig::SDL2)

# blusys_framework_host — framework C++ sources compiled for the host.
# Sources mirror scripts/host/CMakeLists.txt so host and device builds
# stay in sync automatically when the platform is updated.
set(BLUSYS_COMP "${BLUSYS_PATH}/components/blusys")
set(BLUSYS_FW   "${BLUSYS_PATH}/components/blusys_framework")

add_library(blusys_framework_host STATIC
    # Minimal HAL dependency
    ${BLUSYS_COMP}/src/common/error.c

    # Core spine
    ${BLUSYS_FW}/src/core/controller.cpp
    ${BLUSYS_FW}/src/core/feedback.cpp
    ${BLUSYS_FW}/src/core/feedback_presets.cpp
    ${BLUSYS_FW}/src/core/framework.cpp
    ${BLUSYS_FW}/src/core/router.cpp
    ${BLUSYS_FW}/src/core/runtime.cpp

    # App layer — blusys::app product-facing API
    ${BLUSYS_FW}/src/app/app_ctx.cpp
    ${BLUSYS_FW}/src/app/bind_capability_ptrs.cpp
    ${BLUSYS_FW}/src/app/capabilities/connectivity_host.cpp
    ${BLUSYS_FW}/src/app/capabilities/storage_host.cpp
    ${BLUSYS_FW}/src/app/capabilities/provisioning_host.cpp
    ${BLUSYS_FW}/src/app/theme_presets.cpp
    ${BLUSYS_FW}/src/app/action_widgets.cpp
    ${BLUSYS_FW}/src/app/bindings.cpp
    ${BLUSYS_FW}/src/app/overlay_manager.cpp
    ${BLUSYS_FW}/src/app/page.cpp
    ${BLUSYS_FW}/src/app/screen_registry.cpp
    ${BLUSYS_FW}/src/app/screen_router.cpp
    ${BLUSYS_FW}/src/app/shell.cpp

    # UI surface
    ${BLUSYS_FW}/src/ui/icons/icon_set_minimal.cpp
    ${BLUSYS_FW}/src/ui/input/encoder.cpp
    ${BLUSYS_FW}/src/ui/input/focus_scope.cpp
    ${BLUSYS_FW}/src/ui/primitives/col.cpp
    ${BLUSYS_FW}/src/ui/primitives/divider.cpp
    ${BLUSYS_FW}/src/ui/primitives/icon_label.cpp
    ${BLUSYS_FW}/src/ui/primitives/key_value.cpp
    ${BLUSYS_FW}/src/ui/primitives/label.cpp
    ${BLUSYS_FW}/src/ui/primitives/row.cpp
    ${BLUSYS_FW}/src/ui/primitives/screen.cpp
    ${BLUSYS_FW}/src/ui/primitives/status_badge.cpp
    ${BLUSYS_FW}/src/ui/theme.cpp
    ${BLUSYS_FW}/src/ui/widgets/button/button.cpp
    ${BLUSYS_FW}/src/ui/widgets/card/card.cpp
    ${BLUSYS_FW}/src/ui/widgets/dropdown/dropdown.cpp
    ${BLUSYS_FW}/src/ui/widgets/gauge/gauge.cpp
    ${BLUSYS_FW}/src/ui/widgets/input_field/input_field.cpp
    ${BLUSYS_FW}/src/ui/widgets/data_table/data_table.cpp
    ${BLUSYS_FW}/src/ui/widgets/list/list.cpp
    ${BLUSYS_FW}/src/ui/widgets/modal/modal.cpp
    ${BLUSYS_FW}/src/ui/widgets/overlay/overlay.cpp
    ${BLUSYS_FW}/src/ui/widgets/progress/progress.cpp
    ${BLUSYS_FW}/src/ui/widgets/slider/slider.cpp
    ${BLUSYS_FW}/src/ui/widgets/tabs/tabs.cpp
    ${BLUSYS_FW}/src/ui/widgets/toggle/toggle.cpp
    ${BLUSYS_FW}/src/ui/transition.cpp

    # Stock flows and screens used by the interactive starter
    ${BLUSYS_FW}/src/app/flows/settings.cpp
    ${BLUSYS_FW}/src/app/flows/provisioning_flow.cpp
    ${BLUSYS_FW}/src/app/screens/about_screen.cpp
)
target_include_directories(blusys_framework_host PUBLIC
    ${BLUSYS_COMP}/include
    ${BLUSYS_FW}/include
    ${BLUSYS_PATH}/scripts/host/include_host
)
target_compile_options(blusys_framework_host PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
    -Wall -Wextra
)
target_link_libraries(blusys_framework_host PUBLIC lvgl)
target_compile_definitions(blusys_framework_host PUBLIC BLUSYS_FRAMEWORK_HAS_UI=1)

# App executable — compiles main/core/ + main/ui/ + main/integration/app_main.cpp
# which uses BLUSYS_APP_MAIN_HOST(spec) to define int main().
add_executable(BLUSYS_PRODUCT_HOST
    "${CMAKE_CURRENT_LIST_DIR}/../main/core/app_logic.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../main/ui/app_ui.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../main/integration/app_main.cpp"
    "${BLUSYS_PATH}/scripts/host/src/app_host_platform.cpp"
)
target_include_directories(BLUSYS_PRODUCT_HOST PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../main"
)
target_link_libraries(BLUSYS_PRODUCT_HOST PRIVATE
    blusys_framework_host PkgConfig::SDL2 m
)
target_compile_options(BLUSYS_PRODUCT_HOST PRIVATE
    -fno-exceptions -fno-rtti -fno-threadsafe-statics -Wall -Wextra
)
CMAKE
    sed -i "s/BLUSYS_PRODUCT_HOST/${project_name}_host/g" "$target_dir/host/CMakeLists.txt"
}

blusys_create_host_cmakelists_headless() {
    local target_dir="$1"
    local project_name="$2"
    cat > "$target_dir/host/CMakeLists.txt" <<'CMAKE'
cmake_minimum_required(VERSION 3.16)
project(BLUSYS_PRODUCT_HOST LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# BLUSYS_PATH is exported by the blusys CLI automatically.
# For manual builds: export BLUSYS_PATH=/path/to/blusys && cmake -S host -B build-host
if(NOT DEFINED ENV{BLUSYS_PATH})
    message(FATAL_ERROR
        "BLUSYS_PATH is not set.\n"
        "Run via 'blusys host-build' (it exports BLUSYS_PATH automatically), or:\n"
        "  export BLUSYS_PATH=/path/to/blusys && cmake -S host -B build-host")
endif()
set(BLUSYS_PATH "$ENV{BLUSYS_PATH}")

# SDL2 — needed for timer functions in the headless platform helper.
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)

# blusys_framework_core_host — framework core + app layer, no LVGL.
# Headless apps need the runtime spine and app_ctx but no UI surface.
set(BLUSYS_COMP "${BLUSYS_PATH}/components/blusys")
set(BLUSYS_FW   "${BLUSYS_PATH}/components/blusys_framework")

add_library(blusys_framework_core_host STATIC
    # Minimal HAL dependency
    ${BLUSYS_COMP}/src/common/error.c

    # Core spine
    ${BLUSYS_FW}/src/core/controller.cpp
    ${BLUSYS_FW}/src/core/feedback.cpp
    ${BLUSYS_FW}/src/core/feedback_presets.cpp
    ${BLUSYS_FW}/src/core/framework.cpp
    ${BLUSYS_FW}/src/core/router.cpp
    ${BLUSYS_FW}/src/core/runtime.cpp

    # App layer (headless subset — no view/UI sources)
    ${BLUSYS_FW}/src/app/app_ctx.cpp
    ${BLUSYS_FW}/src/app/bind_capability_ptrs.cpp
    ${BLUSYS_FW}/src/app/capabilities/connectivity_host.cpp
    ${BLUSYS_FW}/src/app/capabilities/storage_host.cpp
    ${BLUSYS_FW}/src/app/theme_presets.cpp
)
set(BLUSYS_SVC  "${BLUSYS_PATH}/components/blusys_services")

target_include_directories(blusys_framework_core_host PUBLIC
    ${BLUSYS_COMP}/include
    ${BLUSYS_SVC}/include
    ${BLUSYS_FW}/include
    ${BLUSYS_PATH}/scripts/host/include_host
)
target_compile_options(blusys_framework_core_host PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
    -Wall -Wextra
)
target_link_libraries(blusys_framework_core_host PUBLIC PkgConfig::SDL2)

# App executable — compiles main/core/ + main/integration/app_main.cpp
# which uses BLUSYS_APP_MAIN_HEADLESS(spec) to define int main().
add_executable(BLUSYS_PRODUCT_HOST
    "${CMAKE_CURRENT_LIST_DIR}/../main/core/app_logic.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/../main/integration/app_main.cpp"
    "${BLUSYS_PATH}/scripts/host/src/app_headless_platform.cpp"
)
target_include_directories(BLUSYS_PRODUCT_HOST PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../main"
)
target_link_libraries(BLUSYS_PRODUCT_HOST PRIVATE
    blusys_framework_core_host PkgConfig::SDL2
)
target_compile_options(BLUSYS_PRODUCT_HOST PRIVATE
    -fno-exceptions -fno-rtti -fno-threadsafe-statics -Wall -Wextra
)
# Prevent SDL from hijacking main() — the headless path owns its own entry.
target_compile_definitions(BLUSYS_PRODUCT_HOST PRIVATE SDL_MAIN_HANDLED)
CMAKE
    sed -i "s/BLUSYS_PRODUCT_HOST/${project_name}_host/g" "$target_dir/host/CMakeLists.txt"
}
