#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import yaml

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from blusys.manifest_validator import validate_manifest_text  # noqa: E402


def load_catalog(repo_root: Path) -> dict:
    catalog_path = repo_root / "scripts" / "scaffold" / "catalog.yml"
    return json.loads(catalog_path.read_text())


def parse_csv_arg(value: str) -> list[str]:
    if not value:
        return []
    items = [item.strip() for item in value.split(",") if item.strip()]
    return list(dict.fromkeys(items))


# Headless reducer stubs only track readiness for capabilities the user selected.
# Maps capability id -> (app_state field, capability_event_tag for *ready)
_HEADLESS_READY: dict[str, tuple[str, str]] = {
    "connectivity": ("connectivity_ready", "connectivity_ready"),
    "storage": ("storage_ready", "storage_ready"),
    "bluetooth": ("bluetooth_ready", "bluetooth_ready"),
    "telemetry": ("telemetry_ready", "telemetry_ready"),
    "ota": ("ota_ready", "ota_ready"),
    "diagnostics": ("diagnostics_ready", "diagnostics_ready"),
    "lan_control": ("lan_control_ready", "lan_control_ready"),
    "usb": ("usb_ready", "usb_ready"),
}


_PROFILE_HEADERS: dict[str, tuple[str, str]] = {
    "st7735_160x128": ("blusys/framework/platform/profiles/st7735.hpp", "blusys::platform::st7735_160x128()"),
    "st7789_320x240": ("blusys/framework/platform/profiles/st7789.hpp", "blusys::platform::st7789_320x240()"),
    "ssd1306_128x64": ("blusys/framework/platform/profiles/ssd1306.hpp", "blusys::platform::ssd1306_128x64()"),
    "ssd1306_128x32": ("blusys/framework/platform/profiles/ssd1306.hpp", "blusys::platform::ssd1306_128x32()"),
    "ili9341_320x240": ("blusys/framework/platform/profiles/ili9341.hpp", "blusys::platform::ili9341_320x240()"),
    "ili9488_480x320": ("blusys/framework/platform/profiles/ili9488.hpp", "blusys::platform::ili9488_480x320()"),
    "qemu_rgb_dashboard_320x240": (
        "blusys/framework/platform/profiles/qemu_rgb.hpp",
        "blusys::platform::qemu_rgb_dashboard_320x240()",
    ),
}


def _headless_app_state_fields(capabilities: list[str]) -> str:
    cap_set = set(capabilities)
    lines: list[str] = []
    for cap, (field, _) in _HEADLESS_READY.items():
        if cap in cap_set:
            lines.append(f"    bool {field} = false;")
    lines.append("    std::uint32_t tick_count = 0;")
    return "\n".join(lines)


def _headless_update_switch_body(capabilities: list[str]) -> str:
    cap_set = set(capabilities)
    cases: list[str] = []
    for cap, (field, tag) in _HEADLESS_READY.items():
        if cap in cap_set:
            cases.append(
                f"    case blusys::capability_event_tag::{tag}:\n"
                f"        state.{field} = true;\n"
                f"        break;"
            )
    if not cases:
        return "    default:\n        break;\n"
    return "\n".join(cases) + "\n    default:\n        break;\n"


def _headless_local_ctrl_helper(namespace_name: str, capabilities: list[str]) -> str:
    """JSON status for lan_control — only fields for selected capabilities + tick_count."""
    cap_set = set(capabilities)
    bool_fields: list[str] = []
    for cap, (field, _) in _HEADLESS_READY.items():
        if cap in cap_set:
            bool_fields.append(field)
    inner = '{"tick_count":%lu' + "".join(f',"{bf}":%s' for bf in bool_fields) + "}"
    c_literal = '"' + inner.replace('"', '\\"') + '"'
    args = ["static_cast<unsigned long>(state != nullptr ? state->tick_count : 0U)"]
    for bf in bool_fields:
        args.append(f'state != nullptr && state->{bf} ? "true" : "false"')
    args_joined = ",\n        ".join(args)
    return f"""namespace {{
blusys_err_t local_ctrl_status(char *json_buf, size_t buf_len, size_t *out_len, void *user_ctx)
{{
    auto *state = static_cast<{namespace_name}::app_state *>(user_ctx);
    int written = std::snprintf(
        json_buf,
        buf_len,
        {c_literal},
        {args_joined});
    if (written < 0 || static_cast<size_t>(written) >= buf_len) {{
        return BLUSYS_ERR_NO_MEM;
    }}
    *out_len = static_cast<size_t>(written);
    return BLUSYS_OK;
}}
}}  // namespace
"""


def validate_model(
    catalog: dict, interface: str, capabilities: list[str], policies: list[str]
) -> None:
    if interface not in catalog["interfaces"]:
        raise SystemExit(f"error: unknown interface {interface!r}")

    cap_defs = catalog["capabilities"]
    policy_defs = catalog["policies"]

    unknown_caps = [cap for cap in capabilities if cap not in cap_defs]
    unknown_policies = [policy for policy in policies if policy not in policy_defs]
    if unknown_caps:
        raise SystemExit(
            f"error: unknown capabilities: {', '.join(sorted(unknown_caps))}"
        )
    if unknown_policies:
        raise SystemExit(
            f"error: unknown policies: {', '.join(sorted(unknown_policies))}"
        )

    selected = set(capabilities)
    for cap in capabilities:
        meta = cap_defs[cap]
        missing = [dep for dep in meta["requires"] if dep not in selected]
        if missing:
            raise SystemExit(f"error: capability {cap!r} requires {', '.join(missing)}")
        for group in meta["requires_one_of"]:
            if not any(choice in selected for choice in group):
                raise SystemExit(
                    f"error: capability {cap!r} requires one of: {', '.join(group)}"
                )


def render_top_cmakelists(project_name: str, build_ui: bool, repo_root: Path) -> str:
    build_ui_str = "ON" if build_ui else "OFF"
    return f"""cmake_minimum_required(VERSION 3.16)

set(BLUSYS_REPO_ROOT "{repo_root}")
set(BLUSYS_BUILD_UI {build_ui_str} CACHE BOOL \"Build blusys/framework/ui\")
set(EXTRA_COMPONENT_DIRS \"{repo_root / "components"}\")

include($ENV{{IDF_PATH}}/tools/cmake/project.cmake)
project({project_name})
"""


def render_main_cmakelists(build_ui: bool) -> str:
    if build_ui:
        return """idf_component_register(
    SRCS
        \"core/app_logic.cpp\"
        \"ui/app_ui.cpp\"
        \"platform/app_main.cpp\"
    INCLUDE_DIRS "."
    REQUIRES blusys
)
"""
    return """idf_component_register(
    SRCS
        \"core/app_logic.cpp\"
        \"platform/app_main.cpp\"
    INCLUDE_DIRS "."
    REQUIRES blusys
)
"""


def render_idf_component_yml(
    catalog: dict, interface: str, capabilities: list[str]
) -> str:
    deps = ["  idf:", '    version: ">=5.5"']
    managed = set()
    if catalog["interfaces"][interface]["build_ui"]:
        managed.add("lvgl/lvgl")
    for cap in capabilities:
        managed.update(catalog["capabilities"][cap]["managed_dependencies"])
    for dep in sorted(managed):
        version = '"~9.2"' if dep == "lvgl/lvgl" else '"*"'
        deps.append(f"  {dep}:")
        deps.append(f"    version: {version}")
    return "dependencies:\n" + "\n".join(deps) + "\n"


def render_sdkconfig(
    catalog: dict, capabilities: list[str], policies: list[str]
) -> str:
    lines = []
    for cap in capabilities:
        lines.extend(catalog["capabilities"][cap]["sdkconfig_defaults"])
    for policy in policies:
        lines.extend(catalog["policies"][policy]["sdkconfig_defaults"])
    lines = list(dict.fromkeys(lines))
    return "\n".join(lines) + ("\n" if lines else "")


def render_project_manifest(
    interface: str,
    capabilities: list[str],
    policies: list[str],
    profile: str | None = None,
) -> str:
    caps = ", ".join(capabilities)
    pols = ", ".join(policies)
    profile_value = "null" if profile is None else profile
    return (
        "schema: 1\n"
        f"interface: {interface}\n"
        f"capabilities: [{caps}]\n"
        f"profile: {profile_value}\n"
        f"policies: [{pols}]\n"
    )


def render_ui_types(namespace_name: str) -> str:
    return f"""namespace {namespace_name} {{

struct app_state {{
    std::int32_t counter = 0;
    void *label = nullptr;
}};

enum class action_tag : std::uint8_t {{
    decrement,
    increment,
}};

struct action {{
    action_tag tag = action_tag::increment;
}};

}}  // namespace {namespace_name}
"""


def render_headless_types(namespace_name: str, capabilities: list[str]) -> str:
    fields = _headless_app_state_fields(capabilities)
    return f"""namespace {namespace_name} {{

enum class action_tag : std::uint8_t {{
    capability_event,
}};

struct action {{
    action_tag tag = action_tag::capability_event;
    blusys::capability_event cap_event{{}};
}};

struct app_state {{
{fields}
}};

}}  // namespace {namespace_name}
"""


def render_app_entry(namespace_name: str, interface: str) -> str:
    run_name = "run_interactive" if interface != "headless" else "run_headless"
    return f"""#if BLUSYS_DEVICE_BUILD
extern "C" void app_main(void)
#else
int main(void)
#endif
{{
    // --- product-specific init (NVS, logging, hardware) ---
    blusys::detail::{run_name}({namespace_name}::system::spec);
#if !BLUSYS_DEVICE_BUILD
    return 0;
#endif
}}
"""


def render_app_main_cpp(
    namespace_name: str,
    project_title: str,
    interface: str,
    capabilities: list[str],
) -> str:
    if interface == "headless":
        prelude = (
            '#include "blusys/framework/app/app.hpp"\n'
            '#include "blusys/framework/capabilities/event.hpp"\n'
            '#include "blusys/framework/events/event.hpp"\n'
            '#include "blusys/hal/log.h"\n\n'
            '#include <cstddef>\n'
            '#include <cstdint>\n'
            '#include <cstdio>\n\n'
        )
        logic = render_headless_logic_cpp(namespace_name, capabilities, include_header=False)
        integration = render_integration_cpp(
            namespace_name,
            project_title,
            interface,
            capabilities,
            include_logic_header=False,
            include_ui_header=False,
            emit_entry=False,
        )
        return f"{prelude}{render_headless_types(namespace_name, capabilities)}\n{logic}\nnamespace {namespace_name}::system {{\n}}\n"

    prelude = (
        '#include "blusys/framework/app/app.hpp"\n'
        '#include "blusys/framework/events/event.hpp"\n\n'
        '#include <cstdint>\n\n'
    )
    logic = render_ui_logic_cpp(namespace_name, include_header=False)
    integration = render_integration_cpp(
        namespace_name,
        project_title,
        interface,
        capabilities,
        include_logic_header=False,
        include_ui_header=False,
        emit_entry=False,
    )
    ui_decl = f"namespace {namespace_name}::ui {{\nvoid on_init(blusys::app_ctx &ctx, blusys::app_fx &fx, app_state &state);\n}}\n\n"
    return f"{prelude}{render_ui_types(namespace_name)}\n{logic}\n{ui_decl}{integration}\n{render_app_entry(namespace_name, interface)}"


def render_app_ui_cpp(namespace_name: str, title: str) -> str:
    prelude = (
        '#include "blusys/framework/app/app.hpp"\n'
        '#include "blusys/framework/events/event.hpp"\n\n'
        '#include <cstdint>\n\n'
    )
    ui_body = render_ui_cpp(namespace_name, title, include_header=False)
    return f"{prelude}{render_ui_types(namespace_name)}\n{ui_body}"


def render_main_ui_cmakelists() -> str:
    return f"""# Included by main/CMakeLists.txt and host/CMakeLists.txt.
set(BLUSYS_PRODUCT_UI_SRCS
    "${{CMAKE_CURRENT_LIST_DIR}}/app_ui.cpp"
)
"""


def render_headless_logic_hpp(namespace_name: str, capabilities: list[str]) -> str:
    fields = _headless_app_state_fields(capabilities)
    return f"""#pragma once

#include \"blusys/framework/app/app.hpp\"
#include \"blusys/framework/capabilities/event.hpp\"

#include <cstdint>
#include <optional>

namespace {namespace_name} {{

enum class action_tag : std::uint8_t {{
    capability_event,
}};

struct action {{
    action_tag tag = action_tag::capability_event;
    blusys::capability_event cap_event{{}};
}};

struct app_state {{
{fields}
}};

void update(blusys::app_ctx &ctx, app_state &state, const action &event);
std::optional<action> on_event(blusys::event event, app_state &state);
void on_tick(blusys::app_ctx &ctx, blusys::app_fx &fx, app_state &state, std::uint32_t now_ms);

}}  // namespace {namespace_name}
"""


def render_headless_logic_cpp(
    namespace_name: str, capabilities: list[str], include_header: bool = True
) -> str:
    switch_body = _headless_update_switch_body(capabilities)
    header = '#include "core/app_logic.hpp"\n\n' if include_header else ''
    return f"""{header}#include <optional>

namespace {namespace_name} {{

std::optional<action> on_event(blusys::event event, app_state &state)
{{
    (void)state;
    if (event.source != blusys::event_source::integration) {{
        return std::nullopt;
    }}

    blusys::capability_event ce{{}};
    if (!blusys::map_integration_event(event.id, event.kind, &ce)) {{
        return std::nullopt;
    }}
    ce.payload = event.payload;
    return action{{.tag = action_tag::capability_event, .cap_event = ce}};
}}

inline void update(blusys::app_ctx &ctx, app_state &state, const action &event)
{{
    (void)ctx;
    if (event.tag != action_tag::capability_event) {{
        return;
    }}

    switch (event.cap_event.tag) {{
{switch_body}    }}
}}

void on_tick(blusys::app_ctx &ctx, blusys::app_fx &fx, app_state &state, std::uint32_t now_ms)
{{
    (void)ctx;
    (void)fx;
    (void)now_ms;
    state.tick_count++;
}}

}}  // namespace {namespace_name}
"""


def render_ui_logic_hpp(namespace_name: str) -> str:
    return f"""#pragma once

#include \"blusys/framework/app/app.hpp\"

#include <cstdint>
#include <optional>

namespace {namespace_name} {{

struct app_state {{
    std::int32_t counter = 0;
    void *label = nullptr;
}};

enum class action_tag : std::uint8_t {{
    decrement,
    increment,
}};

struct action {{
    action_tag tag = action_tag::increment;
}};

void update(blusys::app_ctx &ctx, app_state &state, const action &action);
std::optional<action> on_event(blusys::event event, app_state &state);

}}  // namespace {namespace_name}
"""


def render_ui_logic_cpp(namespace_name: str) -> str:
    return f"""#include \"core/app_logic.hpp\"

#include \"blusys/framework/ui/binding/action_widgets.hpp\"

#include <cstdio>
#include <optional>

namespace {namespace_name} {{

inline void update(blusys::app_ctx &ctx, app_state &state, const action &event)
{{
    (void)ctx;
    switch (event.tag) {{
    case action_tag::decrement:
        state.counter -= 1;
        break;
    case action_tag::increment:
        state.counter += 1;
        break;
    }}

    if (state.label != nullptr) {{
        char buf[48];
        std::snprintf(buf, sizeof(buf), \"%ld\", static_cast<long>(state.counter));
        blusys::set_text(static_cast<lv_obj_t *>(state.label), buf);
    }}
}}

inline std::optional<action> on_event(blusys::event event, app_state &state)
{{
    (void)state;
    if (event.source != blusys::event_source::intent) {{
        return std::nullopt;
    }}

    switch (static_cast<blusys::intent>(event.kind)) {{
    case blusys::intent::increment:
        return action{{.tag = action_tag::increment}};
    case blusys::intent::decrement:
        return action{{.tag = action_tag::decrement}};
    default:
        return std::nullopt;
    }}
}}

}}  // namespace {namespace_name}
"""


def render_ui_hpp(namespace_name: str) -> str:
    return f"""#pragma once

#include \"core/app_logic.hpp\"

namespace {namespace_name}::ui {{

void on_init(blusys::app_ctx &ctx, blusys::app_fx &fx, app_state &state);

}}  // namespace {namespace_name}::ui
"""


def render_ui_cpp(namespace_name: str, title: str) -> str:
    return f"""#include \"ui/app_ui.hpp\"

#include \"blusys/framework/ui/binding/action_widgets.hpp\"
#include \"blusys/framework/ui/composition/page.hpp\"

namespace {namespace_name}::ui {{

void on_init(blusys::app_ctx &ctx, blusys::app_fx &fx, app_state &state)
{{
    (void)fx;

    auto page = blusys::page_create();
    blusys::title(page.content, \"{title}\");
    blusys::divider(page.content);

    state.label = blusys::label(page.content, \"0\");

    auto *row = blusys::row(page.content);
    blusys::button(row, \"-\", action{{.tag = action_tag::decrement}}, &ctx,
                   blusys::button_variant::secondary);
    blusys::button(row, \"+\", action{{.tag = action_tag::increment}}, &ctx,
                   blusys::button_variant::secondary);

    blusys::page_load(page);
}}

}}  // namespace {namespace_name}::ui
"""


def render_host_cmakelists(project_name: str, build_ui: bool, repo_root: Path) -> str:
    if build_ui:
        return f"""cmake_minimum_required(VERSION 3.16)
project({project_name}_host LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED ENV{{BLUSYS_PATH}} OR "$ENV{{BLUSYS_PATH}}" STREQUAL "")
    message(FATAL_ERROR
        "BLUSYS_PATH is not set.\n"
        "Run via 'blusys host-build' (it exports BLUSYS_PATH automatically), or:\n"
        "  export BLUSYS_PATH=/path/to/blusys && cmake -S host -B build-host")
endif()

include("$ENV{{BLUSYS_PATH}}/cmake/blusys_host_bridge.cmake")
blusys_host_bridge_setup_lvgl()
blusys_host_bridge_add_library(interactive)
blusys_host_bridge_resolve_build_version(BLUSYS_GIT_VERSION)

add_executable({project_name}_host
    "${{CMAKE_CURRENT_LIST_DIR}}/../main/core/app_logic.cpp"
    "${{CMAKE_CURRENT_LIST_DIR}}/../main/ui/app_ui.cpp"
    "${{CMAKE_CURRENT_LIST_DIR}}/../main/platform/app_main.cpp"
    "$ENV{{BLUSYS_PATH}}/scripts/host/src/app_host_platform.cpp"
)
target_include_directories({project_name}_host PRIVATE
    "${{CMAKE_CURRENT_LIST_DIR}}/../main"
)
target_link_libraries({project_name}_host PRIVATE
    blusys_framework_host PkgConfig::SDL2 m
)
blusys_host_bridge_apply_exe_compile_options({project_name}_host)
target_compile_definitions({project_name}_host PRIVATE
    BLUSYS_APP_BUILD_VERSION="${{BLUSYS_GIT_VERSION}}")
"""
    return f"""cmake_minimum_required(VERSION 3.16)
project({project_name}_host LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED ENV{{BLUSYS_PATH}} OR "$ENV{{BLUSYS_PATH}}" STREQUAL "")
    message(FATAL_ERROR
        "BLUSYS_PATH is not set.\n"
        "Run via 'blusys host-build' (it exports BLUSYS_PATH automatically), or:\n"
        "  export BLUSYS_PATH=/path/to/blusys && cmake -S host -B build-host")
endif()

include("$ENV{{BLUSYS_PATH}}/cmake/blusys_host_bridge.cmake")
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
blusys_host_bridge_add_library(headless)

add_executable({project_name}_host
    "${{CMAKE_CURRENT_LIST_DIR}}/../main/core/app_logic.cpp"
    "${{CMAKE_CURRENT_LIST_DIR}}/../main/platform/app_main.cpp"
    "$ENV{{BLUSYS_PATH}}/scripts/host/src/app_headless_platform.cpp"
)
target_include_directories({project_name}_host PRIVATE
    "${{CMAKE_CURRENT_LIST_DIR}}/../main"
)
target_link_libraries({project_name}_host PRIVATE
    blusys_framework_core_host PkgConfig::SDL2 m
)
blusys_host_bridge_apply_exe_compile_options({project_name}_host)
"""


def capability_include(cap: str) -> str:
    return f'#include "blusys/framework/capabilities/{cap}.hpp"'


def render_integration_cpp(
    namespace_name: str, project_title: str, interface: str, capabilities: list[str]
) -> str:
    includes = [capability_include(cap) for cap in capabilities]
    helper_blocks: list[str] = []
    instances: list[str] = []
    capability_refs: list[str] = []

    if interface == "headless":
        if "telemetry" in capabilities:
            helper_blocks.append(
                """
namespace {
bool deliver_telemetry(const blusys::telemetry_metric *metrics, std::size_t count, void *user_ctx)
{
    (void)metrics;
    (void)count;
    (void)user_ctx;
    return true;
}
}  // namespace
"""
            )
        if "lan_control" in capabilities:
            helper_blocks.append(_headless_local_ctrl_helper(namespace_name, capabilities))
    elif "lan_control" in capabilities:
        helper_blocks.append(
            """
namespace {
blusys_err_t local_ctrl_status(char *json_buf, size_t buf_len, size_t *out_len, void *user_ctx)
{
    auto *state = static_cast<%s::app_state *>(user_ctx);
    int written = std::snprintf(
        json_buf,
        buf_len,
        "{\\\"counter\\\":%%ld}",
        static_cast<long>(state != nullptr ? state->counter : 0));
    if (written < 0 || static_cast<size_t>(written) >= buf_len) {
        return BLUSYS_ERR_NO_MEM;
    }
    *out_len = static_cast<size_t>(written);
    return BLUSYS_OK;
}
}  // namespace
"""
            % namespace_name
        )

    for cap in capabilities:
        if cap == "connectivity":
            instances.append(
                """blusys::connectivity_capability connectivity(blusys::connectivity_config{
    .wifi_ssid = nullptr,
    .prov_service_name = \"%s\",
    .prov_pop = \"123456\",
});"""
                % project_title.lower().replace(" ", "-")
            )
            capability_refs.append("&connectivity")
        elif cap == "storage":
            instances.append(
                """blusys::storage_capability storage(blusys::storage_config{
    .spiffs_base_path = \"/app\",
});"""
            )
            capability_refs.append("&storage")
        elif cap == "diagnostics":
            instances.append(
                """blusys::diagnostics_capability diagnostics(blusys::diagnostics_config{
    .snapshot_interval_ms = 5000,
});"""
            )
            capability_refs.append("&diagnostics")
        elif cap == "telemetry":
            instances.append(
                """blusys::telemetry_capability telemetry(blusys::telemetry_config{
    .deliver = deliver_telemetry,
    .flush_threshold = 4,
    .flush_interval_ms = 1000,
});"""
            )
            capability_refs.append("&telemetry")
        elif cap == "ota":
            instances.append(
                """blusys::ota_capability ota(blusys::ota_config{
    .firmware_url = \"https://example.com/firmware.bin\",
    .auto_mark_valid = true,
});"""
            )
            capability_refs.append("&ota")
        elif cap == "bluetooth":
            instances.append(
                """blusys::bluetooth_capability bluetooth(blusys::bluetooth_config{
    .device_name = \"%s\",
    .auto_advertise = true,
});"""
                % project_title
            )
            capability_refs.append("&bluetooth")
        elif cap == "lan_control":
            instances.append(
                """blusys::lan_control_capability lan_control(blusys::lan_control_config{
    .device_name = \"%s\",
    .status_cb = local_ctrl_status,
    .service_name = \"%s\",
    .instance_name = \"%s\",
});"""
                % (
                    project_title,
                    project_title.lower().replace(" ", "-"),
                    project_title,
                )
            )
            capability_refs.append("&lan_control")
        elif cap == "usb":
            instances.append(
                """blusys::usb_capability usb(blusys::usb_config{
    .role = blusys::usb_role::host,
    .class_mask = static_cast<std::uint8_t>(blusys::usb_class::cdc),
    .manufacturer = \"Blusys\",
    .product = \"%s\",
});"""
                % project_title
            )
            capability_refs.append("&usb")

    cap_list = ", ".join(capability_refs)
    ui_include = '#include "ui/app_ui.hpp"\n' if interface != "headless" else ""
    spec_tail_fields = ""
    if interface != "headless":
        spec_tail_fields = f'    .host_title = "{project_title}",\n'
        ui_fields = (
            f"    .on_init = {namespace_name}::ui::on_init,\n"
            f"    .on_event = {namespace_name}::on_event,\n"
        )
    else:
        ui_fields = (
            f"    .on_tick = {namespace_name}::on_tick,\n"
            f"    .on_event = {namespace_name}::on_event,\n"
        )
    entry = {
        "interactive": f"BLUSYS_APP({namespace_name}::system::spec)",
        "headless": f"BLUSYS_APP_HEADLESS({namespace_name}::system::spec)",
    }[interface]
    return f"""#include \"core/app_logic.hpp\"
{ui_include}

#include \"blusys/framework/app/app.hpp\"
{os.linesep.join(includes)}

#include <cstddef>
#include <cstdint>
#include <cstdio>

namespace {namespace_name}::system {{

{os.linesep.join(helper_blocks)}

{os.linesep.join(instances)}

blusys::capability_list_storage capabilities{{{cap_list}}};

const blusys::app_spec<app_state, action> spec{{
    .initial_state = {{}},
    .update = update,
{ui_fields}    .tick_period_ms = 100,
    .capabilities = &capabilities,
{spec_tail_fields}}};

}}  // namespace {namespace_name}::system

{entry}
"""


def render_readme(
    project_name: str, interface: str, capabilities: list[str], policies: list[str]
) -> str:
    caps = ", ".join(capabilities) if capabilities else "none"
    pols = ", ".join(policies) if policies else "none"
    with_arg = f" --with {caps}" if capabilities else ""
    pol_arg = f" --policy {pols}" if policies else ""
    return f"""# {project_name}

Generated with:

```bash
blusys create --interface {interface}{with_arg}{pol_arg} .
```

## Shape

- Interface: `{interface}`
- Capabilities: `{caps}`
- Profile: `null`
- Policies: `{pols}`

## Layout

- `main/core/` product behavior
- `main/ui/` local UI for interactive interfaces
- `main/platform/` app spec and capability wiring

## Building with platform components

The generated top-level `CMakeLists.txt` sets `EXTRA_COMPONENT_DIRS` to the
`components/` directory of the Blusys tree used when `blusys create` ran (embedded
absolute path). Adjust it if you move the project, use another checkout, or vendor
`blusys` for a standalone tree.

## Commands

- `blusys host-build`
- `blusys build . esp32s3`
- `blusys build . esp32`
"""


def write_file(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)


def generate_project(
    repo_root: Path,
    out_dir: Path,
    interface: str,
    capabilities: list[str],
    policies: list[str],
) -> None:
    capabilities = list(dict.fromkeys(capabilities))
    policies = list(dict.fromkeys(policies))
    catalog = load_catalog(repo_root)
    validate_model(catalog, interface, capabilities, policies)
    manifest_text = render_project_manifest(interface, capabilities, policies)
    manifest_errors = validate_manifest_text(manifest_text, catalog)
    if manifest_errors:
        for error in manifest_errors:
            print(f"error: generated manifest failed validation: {error}", file=sys.stderr)
        raise SystemExit(1)

    if out_dir.exists():
        out_dir.mkdir(parents=True, exist_ok=True)
    else:
        out_dir.mkdir(parents=True)

    project_name = out_dir.name.replace("-", "_")
    namespace_name = project_name
    interface_meta = catalog["interfaces"][interface]
    write_file(
        out_dir / "CMakeLists.txt",
        render_top_cmakelists(project_name, interface_meta["build_ui"], repo_root),
    )
    write_file(
        out_dir / "sdkconfig.defaults",
        render_sdkconfig(catalog, capabilities, policies),
    )
    write_file(
        out_dir / "sdkconfig.qemu",
        (repo_root / "scripts" / "scaffold" / "sdkconfig.qemu").read_text(),
    )
    write_file(
        out_dir / "blusys.project.yml",
        manifest_text,
    )
    write_file(
        out_dir / "README.md",
        render_readme(project_name, interface, capabilities, policies),
    )

    write_file(
        out_dir / "main" / "CMakeLists.txt",
        render_main_cmakelists(interface_meta["build_ui"]),
    )
    write_file(
        out_dir / "main" / "idf_component.yml",
        render_idf_component_yml(catalog, interface, capabilities),
    )

    if interface_meta["build_ui"]:
        write_file(
            out_dir / "main" / "core" / "app_logic.hpp",
            render_ui_logic_hpp(namespace_name),
        )
        write_file(
            out_dir / "main" / "core" / "app_logic.cpp",
            render_ui_logic_cpp(namespace_name),
        )
        write_file(
            out_dir / "main" / "ui" / "app_ui.hpp", render_ui_hpp(namespace_name)
        )
        write_file(
            out_dir / "main" / "ui" / "app_ui.cpp",
            render_ui_cpp(namespace_name, interface_meta["title"]),
        )
    else:
        write_file(
            out_dir / "main" / "core" / "app_logic.hpp",
            render_headless_logic_hpp(namespace_name, capabilities),
        )
        write_file(
            out_dir / "main" / "core" / "app_logic.cpp",
            render_headless_logic_cpp(namespace_name, capabilities),
        )

    write_file(
        out_dir / "main" / "platform" / "app_main.cpp",
        render_integration_cpp(
            namespace_name, interface_meta["title"], interface, capabilities
        ),
    )
    write_file(
        out_dir / "host" / "CMakeLists.txt",
        render_host_cmakelists(project_name, interface_meta["build_ui"]),
    )


def render_ui_logic_cpp(namespace_name: str, include_header: bool = True) -> str:
    header = '#include "core/app_logic.hpp"\n\n' if include_header else ''
    return f"""{header}#include \"blusys/framework/ui/binding/action_widgets.hpp\"

#include <cstdio>
#include <optional>

namespace {namespace_name} {{

inline void update(blusys::app_ctx &ctx, app_state &state, const action &event)
{{
    (void)ctx;
    switch (event.tag) {{
    case action_tag::decrement:
        state.counter -= 1;
        break;
    case action_tag::increment:
        state.counter += 1;
        break;
    }}

    if (state.label != nullptr) {{
        char buf[48];
        std::snprintf(buf, sizeof(buf), "%ld", static_cast<long>(state.counter));
        blusys::set_text(static_cast<lv_obj_t *>(state.label), buf);
    }}
}}

inline std::optional<action> on_event(blusys::event event, app_state &state)
{{
    (void)state;
    if (event.source != blusys::event_source::intent) {{
        return std::nullopt;
    }}

    switch (static_cast<blusys::intent>(event.kind)) {{
    case blusys::intent::increment:
        return action{{.tag = action_tag::increment}};
    case blusys::intent::decrement:
        return action{{.tag = action_tag::decrement}};
    default:
        return std::nullopt;
    }}
}}

}}  // namespace {namespace_name}
"""


def render_ui_cpp(namespace_name: str, title: str, include_header: bool = True) -> str:
    header = '#include "ui/app_ui.hpp"\n\n' if include_header else ''
    return f"""{header}#include \"blusys/framework/ui/binding/action_widgets.hpp\"

#include \"blusys/framework/ui/composition/page.hpp\"

namespace {namespace_name}::ui {{

void on_init(blusys::app_ctx &ctx, blusys::app_fx &fx, app_state &state)
{{
    (void)fx;

    auto page = blusys::page_create();
    blusys::title(page.content, \"{title}\");
    blusys::divider(page.content);

    state.label = blusys::label(page.content, \"0\");

    auto *row = blusys::row(page.content);
    blusys::button(row, \"-\", action{{.tag = action_tag::decrement}}, &ctx,
                   blusys::button_variant::secondary);
    blusys::button(row, \"+\", action{{.tag = action_tag::increment}}, &ctx,
                   blusys::button_variant::secondary);

    blusys::page_load(page);
}}

}}  // namespace {namespace_name}::ui
"""


def render_headless_logic_cpp(
    namespace_name: str, capabilities: list[str], include_header: bool = True
) -> str:
    switch_body = _headless_update_switch_body(capabilities)
    header = '#include "core/app_logic.hpp"\n\n' if include_header else ''
    return f"""{header}#include <optional>

namespace {namespace_name} {{

inline std::optional<action> on_event(blusys::event event, app_state &state)
{{
    (void)state;
    if (event.source != blusys::event_source::integration) {{
        return std::nullopt;
    }}

    blusys::capability_event ce{{}};
    if (!blusys::map_integration_event(event.id, event.kind, &ce)) {{
        return std::nullopt;
    }}
    ce.payload = event.payload;
    return action{{.tag = action_tag::capability_event, .cap_event = ce}};
}}

inline void update(blusys::app_ctx &ctx, app_state &state, const action &event)
{{
    (void)ctx;
    if (event.tag != action_tag::capability_event) {{
        return;
    }}

    switch (event.cap_event.tag) {{
{switch_body}    }}
}}

inline void on_tick(blusys::app_ctx &ctx, blusys::app_fx &fx, app_state &state, std::uint32_t now_ms)
{{
    (void)ctx;
    (void)fx;
    (void)now_ms;
    state.tick_count++;
}}

}}  // namespace {namespace_name}
"""


def render_integration_cpp(
    namespace_name: str,
    project_title: str,
    interface: str,
    capabilities: list[str],
    include_logic_header: bool = True,
    include_ui_header: bool = True,
    emit_entry: bool = True,
) -> str:
    includes = [capability_include(cap) for cap in capabilities]
    helper_blocks: list[str] = []
    instances: list[str] = []
    capability_refs: list[str] = []

    if interface == "headless":
        if "telemetry" in capabilities:
            helper_blocks.append(
                """
namespace {
bool deliver_telemetry(const blusys::telemetry_metric *metrics, std::size_t count, void *user_ctx)
{
    (void)metrics;
    (void)count;
    (void)user_ctx;
    return true;
}
}  // namespace
"""
            )
        if "lan_control" in capabilities:
            helper_blocks.append(_headless_local_ctrl_helper(namespace_name, capabilities))
    elif "lan_control" in capabilities:
        helper_blocks.append(
            """
namespace {
blusys_err_t local_ctrl_status(char *json_buf, size_t buf_len, size_t *out_len, void *user_ctx)
{
    auto *state = static_cast<%s::app_state *>(user_ctx);
    int written = std::snprintf(
        json_buf,
        buf_len,
        "{\\\"counter\\\":%%ld}",
        static_cast<long>(state != nullptr ? state->counter : 0));
    if (written < 0 || static_cast<size_t>(written) >= buf_len) {
        return BLUSYS_ERR_NO_MEM;
    }
    *out_len = static_cast<size_t>(written);
    return BLUSYS_OK;
}
}  // namespace
"""
            % namespace_name
        )

    for cap in capabilities:
        if cap == "connectivity":
            instances.append(
                """blusys::connectivity_capability connectivity(blusys::connectivity_config{
    .wifi_ssid = nullptr,
    .prov_service_name = \"%s\",
    .prov_pop = \"123456\",
});"""
                % project_title.lower().replace(" ", "-")
            )
            capability_refs.append("&connectivity")
        elif cap == "storage":
            instances.append(
                """blusys::storage_capability storage(blusys::storage_config{
    .spiffs_base_path = \"/app\",
});"""
            )
            capability_refs.append("&storage")
        elif cap == "diagnostics":
            instances.append(
                """blusys::diagnostics_capability diagnostics(blusys::diagnostics_config{
    .snapshot_interval_ms = 5000,
});"""
            )
            capability_refs.append("&diagnostics")
        elif cap == "telemetry":
            instances.append(
                """blusys::telemetry_capability telemetry(blusys::telemetry_config{
    .deliver = deliver_telemetry,
    .flush_threshold = 4,
    .flush_interval_ms = 1000,
});"""
            )
            capability_refs.append("&telemetry")
        elif cap == "ota":
            instances.append(
                """blusys::ota_capability ota(blusys::ota_config{
    .firmware_url = \"https://example.com/firmware.bin\",
    .auto_mark_valid = true,
});"""
            )
            capability_refs.append("&ota")
        elif cap == "bluetooth":
            instances.append(
                """blusys::bluetooth_capability bluetooth(blusys::bluetooth_config{
    .device_name = \"%s\",
    .auto_advertise = true,
});"""
                % project_title
            )
            capability_refs.append("&bluetooth")
        elif cap == "lan_control":
            instances.append(
                """blusys::lan_control_capability lan_control(blusys::lan_control_config{
    .device_name = \"%s\",
    .status_cb = local_ctrl_status,
    .service_name = \"%s\",
    .instance_name = \"%s\",
});"""
                % (
                    project_title,
                    project_title.lower().replace(" ", "-"),
                    project_title,
                )
            )
            capability_refs.append("&lan_control")
        elif cap == "usb":
            instances.append(
                """blusys::usb_capability usb(blusys::usb_config{
    .role = blusys::usb_role::host,
    .class_mask = static_cast<std::uint8_t>(blusys::usb_class::cdc),
    .manufacturer = \"Blusys\",
    .product = \"%s\",
});"""
                % project_title
            )
            capability_refs.append("&usb")

    cap_list = ", ".join(capability_refs)
    logic_include = '#include "core/app_logic.hpp"\n' if include_logic_header else ""
    ui_include = '#include "ui/app_ui.hpp"\n' if interface != "headless" and include_ui_header else ""
    spec_tail_fields = ""
    if interface != "headless":
        spec_tail_fields = f'    .host_title = "{project_title}",\n'
        ui_fields = (
            f"    .on_init = {namespace_name}::ui::on_init,\n"
            f"    .on_event = {namespace_name}::on_event,\n"
        )
    else:
        ui_fields = (
            f"    .on_tick = {namespace_name}::on_tick,\n"
            f"    .on_event = {namespace_name}::on_event,\n"
        )
    entry = {
        "interactive": f"BLUSYS_APP({namespace_name}::system::spec)",
        "headless": f"BLUSYS_APP_HEADLESS({namespace_name}::system::spec)",
    }[interface]
    header_block = f"{logic_include}{ui_include}"
    if header_block:
        header_block += "\n"
    entry_block = f"\n{entry}\n" if emit_entry else "\n"
    return f"""{header_block}#include \"blusys/framework/app/app.hpp\"\n{os.linesep.join(includes)}\n
#include <cstddef>\n+#include <cstdint>\n+#include <cstdio>\n
namespace {namespace_name}::system {{

{os.linesep.join(helper_blocks)}

{os.linesep.join(instances)}

blusys::capability_list_storage capabilities{{{cap_list}}};

const blusys::app_spec<app_state, action> spec{{
    .initial_state = {{}},
    .update = update,
{ui_fields}    .tick_period_ms = 100,
    .capabilities = &capabilities,
{spec_tail_fields}}};

}}  // namespace {namespace_name}::system

{entry_block}"""


def render_main_cmakelists(build_ui: bool) -> str:
    ui_include = 'include("${CMAKE_CURRENT_LIST_DIR}/ui/CMakeLists.txt")\n\n' if build_ui else ''
    ui_sources = '        ${BLUSYS_PRODUCT_UI_SRCS}\n' if build_ui else ''
    return f"""set(BLUSYS_GENERATED_SPEC_DIR "${{CMAKE_BINARY_DIR}}/generated")
set(BLUSYS_GENERATED_SPEC "${{BLUSYS_GENERATED_SPEC_DIR}}/blusys_app_spec.h")

add_custom_command(
    OUTPUT "${{BLUSYS_GENERATED_SPEC}}"
    COMMAND "${{CMAKE_COMMAND}}" -E make_directory "${{BLUSYS_GENERATED_SPEC_DIR}}"
    COMMAND "${{BLUSYS_REPO_ROOT}}/blusys" gen-spec
        --manifest "${{CMAKE_CURRENT_LIST_DIR}}/../blusys.project.yml"
        --output "${{BLUSYS_GENERATED_SPEC}}"
    DEPENDS
        "${{CMAKE_CURRENT_LIST_DIR}}/../blusys.project.yml"
        "${{BLUSYS_REPO_ROOT}}/blusys"
        "${{BLUSYS_REPO_ROOT}}/scripts/lib/blusys/scaffold_generator.py"
        "${{BLUSYS_REPO_ROOT}}/scripts/lib/blusys/manifest_validator.py"
        "${{BLUSYS_REPO_ROOT}}/scripts/scaffold/catalog.yml"
    VERBATIM
)
add_custom_target(blusys_gen_spec DEPENDS "${{BLUSYS_GENERATED_SPEC}}")

{ui_include}idf_component_register(
    SRCS
        "app_main.cpp"
{ui_sources}    INCLUDE_DIRS "."
    REQUIRES blusys
)

target_include_directories(${{COMPONENT_LIB}} PRIVATE "${{CMAKE_BINARY_DIR}}")
add_dependencies(${{COMPONENT_LIB}} blusys_gen_spec)
"""


def render_host_cmakelists(project_name: str, build_ui: bool, repo_root: Path) -> str:
    if build_ui:
        return f"""cmake_minimum_required(VERSION 3.16)
project({project_name}_host LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BLUSYS_REPO_ROOT "{repo_root}")

include("${{BLUSYS_REPO_ROOT}}/cmake/blusys_host_bridge.cmake")
blusys_host_bridge_setup_lvgl()
blusys_host_bridge_add_library(interactive)
blusys_host_bridge_resolve_build_version(BLUSYS_GIT_VERSION)

set(BLUSYS_GENERATED_SPEC_DIR "${{CMAKE_BINARY_DIR}}/generated")
set(BLUSYS_GENERATED_SPEC "${{BLUSYS_GENERATED_SPEC_DIR}}/blusys_app_spec.h")

add_custom_command(
    OUTPUT "${{BLUSYS_GENERATED_SPEC}}"
    COMMAND "${{CMAKE_COMMAND}}" -E make_directory "${{BLUSYS_GENERATED_SPEC_DIR}}"
    COMMAND "${{BLUSYS_REPO_ROOT}}/blusys" gen-spec
        --manifest "${{CMAKE_CURRENT_LIST_DIR}}/../blusys.project.yml"
        --output "${{BLUSYS_GENERATED_SPEC}}"
    DEPENDS
        "${{CMAKE_CURRENT_LIST_DIR}}/../blusys.project.yml"
        "${{BLUSYS_REPO_ROOT}}/blusys"
        "${{BLUSYS_REPO_ROOT}}/scripts/lib/blusys/scaffold_generator.py"
        "${{BLUSYS_REPO_ROOT}}/scripts/lib/blusys/manifest_validator.py"
        "${{BLUSYS_REPO_ROOT}}/scripts/scaffold/catalog.yml"
    VERBATIM
)
add_custom_target(blusys_gen_spec DEPENDS "${{BLUSYS_GENERATED_SPEC}}")

include("${{CMAKE_CURRENT_LIST_DIR}}/../main/ui/CMakeLists.txt")

add_executable({project_name}_host
    "${{CMAKE_CURRENT_LIST_DIR}}/../main/app_main.cpp"
    ${{BLUSYS_PRODUCT_UI_SRCS}}
    "${{BLUSYS_REPO_ROOT}}/scripts/host/src/app_host_platform.cpp"
)
target_include_directories({project_name}_host PRIVATE
    "${{CMAKE_BINARY_DIR}}"
)
target_link_libraries({project_name}_host PRIVATE
    blusys_framework_host PkgConfig::SDL2 m
)
blusys_host_bridge_apply_exe_compile_options({project_name}_host)
target_compile_definitions({project_name}_host PRIVATE
    BLUSYS_APP_BUILD_VERSION="${{BLUSYS_GIT_VERSION}}")
add_dependencies({project_name}_host blusys_gen_spec)
"""
    return f"""cmake_minimum_required(VERSION 3.16)
project({project_name}_host LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BLUSYS_REPO_ROOT "{repo_root}")

include("${{BLUSYS_REPO_ROOT}}/cmake/blusys_host_bridge.cmake")
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
blusys_host_bridge_add_library(headless)

set(BLUSYS_GENERATED_SPEC_DIR "${{CMAKE_BINARY_DIR}}/generated")
set(BLUSYS_GENERATED_SPEC "${{BLUSYS_GENERATED_SPEC_DIR}}/blusys_app_spec.h")

add_custom_command(
    OUTPUT "${{BLUSYS_GENERATED_SPEC}}"
    COMMAND "${{CMAKE_COMMAND}}" -E make_directory "${{BLUSYS_GENERATED_SPEC_DIR}}"
    COMMAND "${{BLUSYS_REPO_ROOT}}/blusys" gen-spec
        --manifest "${{CMAKE_CURRENT_LIST_DIR}}/../blusys.project.yml"
        --output "${{BLUSYS_GENERATED_SPEC}}"
    DEPENDS
        "${{CMAKE_CURRENT_LIST_DIR}}/../blusys.project.yml"
        "${{BLUSYS_REPO_ROOT}}/blusys"
        "${{BLUSYS_REPO_ROOT}}/scripts/lib/blusys/scaffold_generator.py"
        "${{BLUSYS_REPO_ROOT}}/scripts/lib/blusys/manifest_validator.py"
        "${{BLUSYS_REPO_ROOT}}/scripts/scaffold/catalog.yml"
    VERBATIM
)
add_custom_target(blusys_gen_spec DEPENDS "${{BLUSYS_GENERATED_SPEC}}")

add_executable({project_name}_host
    "${{CMAKE_CURRENT_LIST_DIR}}/../main/app_main.cpp"
    "${{BLUSYS_REPO_ROOT}}/scripts/host/src/app_headless_platform.cpp"
)
target_include_directories({project_name}_host PRIVATE
    "${{CMAKE_BINARY_DIR}}"
)
target_link_libraries({project_name}_host PRIVATE
    blusys_framework_core_host PkgConfig::SDL2 m
)
blusys_host_bridge_apply_exe_compile_options({project_name}_host)
add_dependencies({project_name}_host blusys_gen_spec)
"""


def render_app_main_cpp(
    namespace_name: str,
    project_title: str,
    interface: str,
    capabilities: list[str],
) -> str:
    return """#include "generated/blusys_app_spec.h"

extern "C" void app_main(void)
{
    // --- product-specific init (NVS, logging, hardware) ---
    blusys::run(blusys::generated::kAppSpec);
}

#if !BLUSYS_DEVICE_BUILD
int main(void)
{
    app_main();
    return 0;
}
#endif
"""


def render_app_ui_cpp(namespace_name: str, title: str) -> str:
    prelude = '#include "generated/blusys_app_spec.h"\n\n'
    return f"{prelude}{render_ui_cpp('blusys::generated', title, include_header=False)}"


def render_generated_spec_header(
    project_title: str,
    interface: str,
    capabilities: list[str],
    policies: list[str],
    profile: str | None = None,
) -> str:
    capabilities = list(dict.fromkeys(capabilities))
    policies = list(dict.fromkeys(policies))
    includes = [capability_include(cap) for cap in capabilities]
    includes.extend(
        [
            '#include "blusys/framework/app/app.hpp"',
            '#include "blusys/framework/capabilities/event.hpp"',
            '#include "blusys/framework/events/event.hpp"',
            '#include "blusys/framework/ui/binding/bindings.hpp"',
            "#include <cstddef>",
            "#include <cstdio>",
            "#include <cstdint>",
        ]
    )
    profile_include_line = ""
    profile_expr = ""
    if profile is not None:
        profile_include, profile_expr = _PROFILE_HEADERS[profile]
        includes.append(f'#include "{profile_include}"')

    if interface == "headless":
        state_block = render_headless_types("blusys::generated", capabilities)
        logic_block = render_headless_logic_cpp("blusys::generated", capabilities, include_header=False)
    else:
        state_block = render_ui_types("blusys::generated")
        logic_block = render_ui_logic_cpp("blusys::generated", include_header=False)

    helper_blocks: list[str] = []
    instances: list[str] = []
    capability_refs: list[str] = []

    if interface == "headless":
        if "telemetry" in capabilities:
            helper_blocks.append(
                """
namespace {
inline bool deliver_telemetry(const blusys::telemetry_metric *metrics, std::size_t count, void *user_ctx)
{
    (void)metrics;
    (void)count;
    (void)user_ctx;
    return true;
}
}  // namespace
"""
            )
        if "lan_control" in capabilities:
            helper_blocks.append(_headless_local_ctrl_helper("blusys::generated", capabilities))
    elif "lan_control" in capabilities:
        helper_blocks.append(
            """
namespace {
inline blusys_err_t local_ctrl_status(char *json_buf, size_t buf_len, size_t *out_len, void *user_ctx)
{
    auto *state = static_cast<blusys::generated::app_state *>(user_ctx);
    int written = std::snprintf(
        json_buf,
        buf_len,
        "{\\\"counter\\\":%%ld}",
        static_cast<long>(state != nullptr ? state->counter : 0));
    if (written < 0 || static_cast<size_t>(written) >= buf_len) {
        return BLUSYS_ERR_NO_MEM;
    }
    *out_len = static_cast<size_t>(written);
    return BLUSYS_OK;
}
}  // namespace
"""
        )

    for cap in capabilities:
        if cap == "connectivity":
            instances.append(
                """inline blusys::connectivity_capability connectivity(blusys::connectivity_config{
    .wifi_ssid = nullptr,
    .prov_service_name = \"%s\",
    .prov_pop = \"123456\",
});"""
                % project_title.lower().replace(" ", "-")
            )
            capability_refs.append("&connectivity")
        elif cap == "storage":
            instances.append(
                """inline blusys::storage_capability storage(blusys::storage_config{
    .spiffs_base_path = \"/app\",
});"""
            )
            capability_refs.append("&storage")
        elif cap == "diagnostics":
            instances.append(
                """inline blusys::diagnostics_capability diagnostics(blusys::diagnostics_config{
    .snapshot_interval_ms = 5000,
});"""
            )
            capability_refs.append("&diagnostics")
        elif cap == "telemetry":
            instances.append(
                """inline blusys::telemetry_capability telemetry(blusys::telemetry_config{
    .deliver = deliver_telemetry,
    .flush_threshold = 4,
    .flush_interval_ms = 1000,
});"""
            )
            capability_refs.append("&telemetry")
        elif cap == "ota":
            instances.append(
                """inline blusys::ota_capability ota(blusys::ota_config{
    .firmware_url = \"https://example.com/firmware.bin\",
    .auto_mark_valid = true,
});"""
            )
            capability_refs.append("&ota")
        elif cap == "bluetooth":
            instances.append(
                """inline blusys::bluetooth_capability bluetooth(blusys::bluetooth_config{
    .device_name = \"%s\",
    .auto_advertise = true,
});"""
                % project_title
            )
            capability_refs.append("&bluetooth")
        elif cap == "lan_control":
            instances.append(
                """inline blusys::lan_control_capability lan_control(blusys::lan_control_config{
    .device_name = \"%s\",
    .status_cb = local_ctrl_status,
    .service_name = \"%s\",
    .instance_name = \"%s\",
});"""
                % (
                    project_title,
                    project_title.lower().replace(" ", "-"),
                    project_title,
                )
            )
            capability_refs.append("&lan_control")
        elif cap == "usb":
            instances.append(
                """inline blusys::usb_capability usb(blusys::usb_config{
    .role = blusys::usb_role::host,
    .class_mask = static_cast<std::uint8_t>(blusys::usb_class::cdc),
    .manufacturer = \"Blusys\",
    .product = \"%s\",
});"""
                % project_title
            )
            capability_refs.append("&usb")

    if capability_refs:
        capability_storage = f"inline auto kCapabilities = blusys::make_capability_list({', '.join(capability_refs)});"
        capabilities_ref = "&kCapabilities"
    else:
        capability_storage = ""
        capabilities_ref = "nullptr"

    policy_flags = ["inline constexpr std::uint32_t kPolicyLowPower = 1u << 0;"]
    policy_flags.append(
        f"inline constexpr std::uint32_t kPolicyFlags = {'kPolicyLowPower' if 'low_power' in policies else '0u'};"
    )

    namespace_sections: list[str] = []
    if interface != "headless":
        namespace_sections.append(
            "namespace ui {\nvoid on_init(blusys::app_ctx &ctx, blusys::app_fx &fx, app_state &state);\n}\n"
        )
        profile_include_line = f"inline const auto kProfile = {profile_expr};" if profile is not None else ""
        namespace_sections.append(profile_include_line)
    if helper_blocks:
        namespace_sections.extend(helper_blocks)
    namespace_sections.append("\n".join(policy_flags))
    if instances:
        namespace_sections.extend(instances)
    if capability_storage:
        namespace_sections.append(capability_storage)

    spec_lines: list[str] = [
        "inline const blusys::app_spec<app_state, action> kAppSpec{",
        "    .initial_state = {},",
        "    .update = update,",
    ]
    if interface != "headless":
        spec_lines.append("    .on_init = ui::on_init,")
        spec_lines.append("    .on_event = on_event,")
    else:
        spec_lines.append("    .on_tick = on_tick,")
        spec_lines.append("    .on_event = on_event,")
    spec_lines.append("    .tick_period_ms = 100,")
    spec_lines.append(f"    .capabilities = {capabilities_ref},")
    if interface != "headless":
        spec_lines.append(f"    .profile = {('&kProfile' if profile is not None else 'nullptr')},")
        spec_lines.append(f'    .host_title = "{project_title}",')
    spec_lines.append("};")
    namespace_sections.append("\n".join(spec_lines))

    return """#pragma once

{includes}

{state_block}
{logic_block}

namespace blusys::generated {{

{body}

}}  // namespace blusys::generated
""".format(
        includes="\n".join(includes),
        state_block=state_block,
        logic_block=logic_block,
        body="\n\n".join(section for section in namespace_sections if section),
    )


def render_readme(
    project_name: str, interface: str, capabilities: list[str], policies: list[str]
) -> str:
    caps = ", ".join(capabilities) if capabilities else "none"
    pols = ", ".join(policies) if policies else "none"
    with_arg = f" --with {caps}" if capabilities else ""
    pol_arg = f" --policy {pols}" if policies else ""
    layout = [
        "- `main/app_main.cpp` thin entrypoint",
        "- `build/generated/blusys_app_spec.h` manifest-derived wiring",
    ]
    if interface != "headless":
        layout.append("- `main/ui/CMakeLists.txt` interactive UI source list fragment")
        layout.append("- `main/ui/app_ui.cpp` small sample component tree")
    return f"""# {project_name}

Generated with:

```bash
blusys create --interface {interface}{with_arg}{pol_arg} .
```

## Shape

- Interface: `{interface}`
- Capabilities: `{caps}`
- Profile: `null`
- Policies: `{pols}`

## Layout

{os.linesep.join(layout)}

## Building with platform components

The generated top-level `CMakeLists.txt` sets `EXTRA_COMPONENT_DIRS` to the
`components/` directory of the Blusys tree used when `blusys create` ran (embedded
absolute path). Adjust it if you move the project, use another checkout, or vendor
`blusys` for a standalone tree.

Builds also regenerate `build/generated/blusys_app_spec.h` from `blusys.project.yml`.

## Commands

- `blusys host-build`
- `blusys build . esp32s3`
- `blusys build . esp32`
"""


def generate_project(
    repo_root: Path,
    out_dir: Path,
    interface: str,
    capabilities: list[str],
    policies: list[str],
) -> None:
    catalog = load_catalog(repo_root)
    validate_model(catalog, interface, capabilities, policies)
    manifest_text = render_project_manifest(interface, capabilities, policies)
    manifest_errors = validate_manifest_text(manifest_text, catalog)
    if manifest_errors:
        for error in manifest_errors:
            print(f"error: generated manifest failed validation: {error}", file=sys.stderr)
        raise SystemExit(1)

    if out_dir.exists():
        out_dir.mkdir(parents=True, exist_ok=True)
    else:
        out_dir.mkdir(parents=True)

    project_name = out_dir.name.replace("-", "_")
    namespace_name = project_name
    interface_meta = catalog["interfaces"][interface]
    build_ui = interface_meta["build_ui"]
    write_file(
        out_dir / "CMakeLists.txt",
        render_top_cmakelists(project_name, build_ui, repo_root),
    )
    write_file(
        out_dir / "sdkconfig.defaults",
        render_sdkconfig(catalog, capabilities, policies),
    )
    write_file(
        out_dir / "sdkconfig.qemu",
        (repo_root / "scripts" / "scaffold" / "sdkconfig.qemu").read_text(),
    )
    write_file(out_dir / "blusys.project.yml", manifest_text)
    write_file(out_dir / "README.md", render_readme(project_name, interface, capabilities, policies))

    write_file(out_dir / "main" / "CMakeLists.txt", render_main_cmakelists(build_ui))
    write_file(
        out_dir / "main" / "idf_component.yml",
        render_idf_component_yml(catalog, interface, capabilities),
    )
    write_file(
        out_dir / "main" / "app_main.cpp",
        render_app_main_cpp(namespace_name, interface_meta["title"], interface, capabilities),
    )

    if build_ui:
        write_file(out_dir / "main" / "ui" / "CMakeLists.txt", render_main_ui_cmakelists())
        write_file(
            out_dir / "main" / "ui" / "app_ui.cpp",
            render_app_ui_cpp(namespace_name, interface_meta["title"]),
        )

    write_file(
        out_dir / "host" / "CMakeLists.txt",
        render_host_cmakelists(project_name, build_ui, repo_root),
    )


def generate_app_spec(repo_root: Path, manifest_path: Path, output_path: Path) -> None:
    catalog = load_catalog(repo_root)

    try:
        manifest_text = manifest_path.read_text(encoding="utf-8")
    except OSError as exc:
        print(f"error: {manifest_path}: {exc}", file=sys.stderr)
        raise SystemExit(1)

    manifest_errors = validate_manifest_text(manifest_text, catalog)
    if manifest_errors:
        for error in manifest_errors:
            print(f"error: manifest validation failed: {error}", file=sys.stderr)
        raise SystemExit(1)

    manifest = yaml.safe_load(manifest_text)
    if not isinstance(manifest, dict):
        raise SystemExit(f"error: {manifest_path}: manifest must be a mapping")

    interface = manifest.get("interface")
    capabilities = list(dict.fromkeys(manifest.get("capabilities") or []))
    policies = list(dict.fromkeys(manifest.get("policies") or []))
    profile = manifest.get("profile")

    validate_model(catalog, interface, capabilities, policies)

    header_text = render_generated_spec_header(
        catalog["interfaces"][interface]["title"],
        interface,
        capabilities,
        policies,
        profile,
    )
    write_file(output_path, header_text)


def render_integration_cpp(
    namespace_name: str,
    project_title: str,
    interface: str,
    capabilities: list[str],
    include_logic_header: bool = True,
    include_ui_header: bool = True,
    emit_entry: bool = True,
) -> str:
    includes = [capability_include(cap) for cap in capabilities]
    helper_blocks: list[str] = []
    instances: list[str] = []
    capability_refs: list[str] = []

    if interface == "headless":
        if "telemetry" in capabilities:
            helper_blocks.append(
                """
namespace {
bool deliver_telemetry(const blusys::telemetry_metric *metrics, std::size_t count, void *user_ctx)
{
    (void)metrics;
    (void)count;
    (void)user_ctx;
    return true;
}
}  // namespace
"""
            )
        if "lan_control" in capabilities:
            helper_blocks.append(_headless_local_ctrl_helper(namespace_name, capabilities))
    elif "lan_control" in capabilities:
        helper_blocks.append(
            """
namespace {
blusys_err_t local_ctrl_status(char *json_buf, size_t buf_len, size_t *out_len, void *user_ctx)
{
    auto *state = static_cast<%s::app_state *>(user_ctx);
    int written = std::snprintf(
        json_buf,
        buf_len,
        "{\\\"counter\\\":%%ld}",
        static_cast<long>(state != nullptr ? state->counter : 0));
    if (written < 0 || static_cast<size_t>(written) >= buf_len) {
        return BLUSYS_ERR_NO_MEM;
    }
    *out_len = static_cast<size_t>(written);
    return BLUSYS_OK;
}
}  // namespace
"""
            % namespace_name
        )

    for cap in capabilities:
        if cap == "connectivity":
            instances.append(
                """blusys::connectivity_capability connectivity(blusys::connectivity_config{
    .wifi_ssid = nullptr,
    .prov_service_name = \"%s\",
    .prov_pop = \"123456\",
});"""
                % project_title.lower().replace(" ", "-")
            )
            capability_refs.append("&connectivity")
        elif cap == "storage":
            instances.append(
                """blusys::storage_capability storage(blusys::storage_config{
    .spiffs_base_path = \"/app\",
});"""
            )
            capability_refs.append("&storage")
        elif cap == "diagnostics":
            instances.append(
                """blusys::diagnostics_capability diagnostics(blusys::diagnostics_config{
    .snapshot_interval_ms = 5000,
});"""
            )
            capability_refs.append("&diagnostics")
        elif cap == "telemetry":
            instances.append(
                """blusys::telemetry_capability telemetry(blusys::telemetry_config{
    .deliver = deliver_telemetry,
    .flush_threshold = 4,
    .flush_interval_ms = 1000,
});"""
            )
            capability_refs.append("&telemetry")
        elif cap == "ota":
            instances.append(
                """blusys::ota_capability ota(blusys::ota_config{
    .firmware_url = \"https://example.com/firmware.bin\",
    .auto_mark_valid = true,
});"""
            )
            capability_refs.append("&ota")
        elif cap == "bluetooth":
            instances.append(
                """blusys::bluetooth_capability bluetooth(blusys::bluetooth_config{
    .device_name = \"%s\",
    .auto_advertise = true,
});"""
                % project_title
            )
            capability_refs.append("&bluetooth")
        elif cap == "lan_control":
            instances.append(
                """blusys::lan_control_capability lan_control(blusys::lan_control_config{
    .device_name = \"%s\",
    .status_cb = local_ctrl_status,
    .service_name = \"%s\",
    .instance_name = \"%s\",
});"""
                % (
                    project_title,
                    project_title.lower().replace(" ", "-"),
                    project_title,
                )
            )
            capability_refs.append("&lan_control")
        elif cap == "usb":
            instances.append(
                """blusys::usb_capability usb(blusys::usb_config{
    .role = blusys::usb_role::host,
    .class_mask = static_cast<std::uint8_t>(blusys::usb_class::cdc),
    .manufacturer = \"Blusys\",
    .product = \"%s\",
});"""
                % project_title
            )
            capability_refs.append("&usb")

    cap_list = ", ".join(capability_refs)
    logic_include = '#include "core/app_logic.hpp"\n' if include_logic_header else ""
    ui_include = '#include "ui/app_ui.hpp"\n' if interface != "headless" and include_ui_header else ""
    spec_tail_fields = ""
    if interface != "headless":
        spec_tail_fields = f'    .host_title = "{project_title}",\n'
        ui_fields = (
            f"    .on_init = {namespace_name}::ui::on_init,\n"
            f"    .on_event = {namespace_name}::on_event,\n"
        )
    else:
        ui_fields = (
            f"    .on_tick = {namespace_name}::on_tick,\n"
            f"    .on_event = {namespace_name}::on_event,\n"
        )
    entry = {
        "interactive": f"BLUSYS_APP({namespace_name}::system::spec)",
        "headless": f"BLUSYS_APP_HEADLESS({namespace_name}::system::spec)",
    }[interface]
    header_block = f"{logic_include}{ui_include}"
    if header_block:
        header_block += "\n"
    entry_block = f"\n{entry}\n" if emit_entry else "\n"
    return f"""{header_block}#include \"blusys/framework/app/app.hpp\"\n{os.linesep.join(includes)}\n
#include <cstddef>\n#include <cstdint>\n#include <cstdio>\n
namespace {namespace_name}::system {{

{os.linesep.join(helper_blocks)}

{os.linesep.join(instances)}

blusys::capability_list_storage capabilities{{{cap_list}}};

const blusys::app_spec<app_state, action> spec{{
    .initial_state = {{}},
    .update = update,
{ui_fields}    .tick_period_ms = 100,
    .capabilities = &capabilities,
{spec_tail_fields}}};

}}  // namespace {namespace_name}::system

{entry_block}"""


def render_app_main_cpp(
    namespace_name: str,
    project_title: str,
    interface: str,
    capabilities: list[str],
) -> str:
    return """#include "generated/blusys_app_spec.h"

extern "C" void app_main(void)
{
    // --- product-specific init (NVS, logging, hardware) ---
    blusys::run(blusys::generated::kAppSpec);
}

#if !BLUSYS_DEVICE_BUILD
int main(void)
{
    app_main();
    return 0;
}
#endif
"""


def print_list(catalog: dict) -> None:
    def emit_names(values: list[str], per_line: int) -> None:
        for i in range(0, len(values), per_line):
            print("  " + ", ".join(values[i : i + per_line]))

    print("Interfaces:")
    for name, meta in catalog["interfaces"].items():
        print(f"  {name:10s} {meta['description']}")
    print("\nStarter presets:")
    presets = [
        ("ii", "interactive blank", "no capabilities"),
        ("is", "interactive storage", "storage"),
        ("ib", "interactive bluetooth storage", "bluetooth, storage"),
        ("id", "interactive connected operator", "connectivity, diagnostics"),
        ("ht", "headless connected telemetry", "connectivity, telemetry, ota, diagnostics"),
        ("hl", "headless lan control", "connectivity, lan_control, ota"),
        ("hu", "headless usb host", "usb"),
        ("hp", "headless low-power telemetry", "connectivity, telemetry + low_power"),
    ]
    for code, label, details in presets:
        print(f"  {code:2s}  {label:29s} {details}")

    print("\nCapabilities:")
    emit_names(list(catalog["capabilities"].keys()), 4)
    print("\nProfiles:")
    emit_names(list(catalog.get("profiles", {}).keys()), 4)
    print("\nPolicies:")
    for name, meta in catalog["policies"].items():
        print(f"  {name:12s} {meta['description']}")


def main() -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--repo-root", required=True)
    parser.add_argument("--interface", default="interactive")
    parser.add_argument("--with", dest="with_caps", default="")
    parser.add_argument("--policy", default="")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--emit-spec", action="store_true")
    parser.add_argument("--manifest")
    parser.add_argument("--output")
    parser.add_argument("paths", nargs="*")
    args = parser.parse_args()

    repo_root = Path(args.repo_root)
    catalog = load_catalog(repo_root)
    if args.list:
        print_list(catalog)
        return 0

    if args.emit_spec:
        if len(args.paths) > 2:
            print("error: gen-spec accepts at most manifest and output paths", file=sys.stderr)
            return 1

        manifest_path = Path(args.manifest) if args.manifest else None
        output_path = Path(args.output) if args.output else None
        if manifest_path is None and len(args.paths) >= 1:
            manifest_path = Path(args.paths[0])
        if output_path is None and len(args.paths) >= 2:
            output_path = Path(args.paths[1])

        if manifest_path is None:
            manifest_path = Path.cwd() / "blusys.project.yml"
        if output_path is None:
            output_path = Path.cwd() / "build" / "generated" / "blusys_app_spec.h"

        generate_app_spec(repo_root, manifest_path, output_path)
        print(f"Generated app spec: {output_path}")
        return 0

    if len(args.paths) > 1:
        print("error: create accepts at most one output path", file=sys.stderr)
        return 1

    out_dir = Path(args.paths[0]).resolve() if args.paths else Path.cwd()
    capabilities = parse_csv_arg(args.with_caps)
    policies = parse_csv_arg(args.policy)
    generate_project(repo_root, out_dir, args.interface, capabilities, policies)
    print(f"Created blusys project: {out_dir}")
    print(f"Interface: {args.interface}")
    print(f"Capabilities: {', '.join(capabilities) if capabilities else 'none'}")
    print("Profile: null")
    print(f"Policies: {', '.join(policies) if policies else 'none'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
