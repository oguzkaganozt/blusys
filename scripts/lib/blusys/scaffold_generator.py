#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path


def load_catalog(repo_root: Path) -> dict:
    catalog_path = repo_root / "scripts" / "scaffold" / "catalog.yml"
    return json.loads(catalog_path.read_text())


def parse_csv_arg(value: str) -> list[str]:
    if not value:
        return []
    return [item.strip() for item in value.split(",") if item.strip()]


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
    interface: str, capabilities: list[str], policies: list[str]
) -> str:
    caps = ", ".join(capabilities)
    pols = ", ".join(policies)
    return (
        "version: 1\n"
        f"interface: {interface}\n"
        f"capabilities: [{caps}]\n"
        f"policies: [{pols}]\n"
    )


def render_headless_logic_hpp(namespace_name: str, capabilities: list[str]) -> str:
    fields = _headless_app_state_fields(capabilities)
    return f"""#pragma once

#include \"blusys/framework/app/app.hpp\"
#include \"blusys/framework/capabilities/event.hpp\"

#include <cstdint>

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
void on_tick(blusys::app_ctx &ctx, blusys::app_services &svc, app_state &state, std::uint32_t now_ms);

}}  // namespace {namespace_name}
"""


def render_headless_logic_cpp(namespace_name: str, capabilities: list[str]) -> str:
    switch_body = _headless_update_switch_body(capabilities)
    return f"""#include \"core/app_logic.hpp\"

namespace {namespace_name} {{

void update(blusys::app_ctx &ctx, app_state &state, const action &event)
{{
    (void)ctx;
    if (event.tag != action_tag::capability_event) {{
        return;
    }}

    switch (event.cap_event.tag) {{
{switch_body}    }}
}}

void on_tick(blusys::app_ctx &ctx, blusys::app_services &svc, app_state &state, std::uint32_t now_ms)
{{
    (void)ctx;
    (void)svc;
    (void)now_ms;
    state.tick_count++;
}}

}}  // namespace {namespace_name}
"""


def render_ui_logic_hpp(namespace_name: str) -> str:
    return f"""#pragma once

#include \"blusys/framework/app/app.hpp\"

#include <cstdint>

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
bool map_intent(blusys::app_services &svc, blusys::intent intent, action *out);

}}  // namespace {namespace_name}
"""


def render_ui_logic_cpp(namespace_name: str) -> str:
    return f"""#include \"core/app_logic.hpp\"

#include \"blusys/framework/ui/binding/action_widgets.hpp\"

#include <cstdio>

namespace {namespace_name} {{

void update(blusys::app_ctx &ctx, app_state &state, const action &event)
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

bool map_intent(blusys::app_services &svc, blusys::intent intent, action *out)
{{
    (void)svc;
    switch (intent) {{
    case blusys::intent::increment:
        *out = action{{.tag = action_tag::increment}};
        return true;
    case blusys::intent::decrement:
        *out = action{{.tag = action_tag::decrement}};
        return true;
    default:
        return false;
    }}
}}

}}  // namespace {namespace_name}
"""


def render_ui_hpp(namespace_name: str) -> str:
    return f"""#pragma once

#include \"core/app_logic.hpp\"

namespace {namespace_name}::ui {{

void on_init(blusys::app_ctx &ctx, blusys::app_services &svc, app_state &state);

}}  // namespace {namespace_name}::ui
"""


def render_ui_cpp(namespace_name: str, title: str) -> str:
    return f"""#include \"ui/app_ui.hpp\"

#include \"blusys/framework/ui/binding/action_widgets.hpp\"
#include \"blusys/framework/ui/composition/page.hpp\"

namespace {namespace_name}::ui {{

void on_init(blusys::app_ctx &ctx, blusys::app_services &svc, app_state &state)
{{
    (void)svc;

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


def render_host_cmakelists(project_name: str, build_ui: bool) -> str:
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
                """blusys::connectivity_capability connectivity{{
    .wifi_ssid = nullptr,
    .prov_service_name = \"%s\",
    .prov_pop = \"123456\",
}};"""
                % project_title.lower().replace(" ", "-")
            )
            capability_refs.append("&connectivity")
        elif cap == "storage":
            instances.append(
                """blusys::storage_capability storage{{
    .init_nvs = true,
    .spiffs_base_path = \"/app\",
}};"""
            )
            capability_refs.append("&storage")
        elif cap == "diagnostics":
            instances.append(
                """blusys::diagnostics_capability diagnostics{{
    .snapshot_interval_ms = 5000,
}};"""
            )
            capability_refs.append("&diagnostics")
        elif cap == "telemetry":
            instances.append(
                """blusys::telemetry_capability telemetry{{
    .deliver = deliver_telemetry,
    .flush_threshold = 4,
    .flush_interval_ms = 1000,
}};"""
            )
            capability_refs.append("&telemetry")
        elif cap == "ota":
            instances.append(
                """blusys::ota_capability ota{{
    .firmware_url = \"https://example.com/firmware.bin\",
    .auto_mark_valid = true,
}};"""
            )
            capability_refs.append("&ota")
        elif cap == "bluetooth":
            instances.append(
                """blusys::bluetooth_capability bluetooth{{
    .device_name = \"%s\",
    .auto_advertise = true,
}};"""
                % project_title
            )
            capability_refs.append("&bluetooth")
        elif cap == "lan_control":
            instances.append(
                """blusys::lan_control_capability lan_control{{
    .device_name = \"%s\",
    .status_cb = local_ctrl_status,
    .service_name = \"%s\",
    .instance_name = \"%s\",
}};"""
                % (
                    project_title,
                    project_title.lower().replace(" ", "-"),
                    project_title,
                )
            )
            capability_refs.append("&lan_control")
        elif cap == "usb":
            instances.append(
                """blusys::usb_capability usb{{
    .role = blusys::usb_role::host,
    .class_mask = static_cast<std::uint8_t>(blusys::usb_class::cdc),
    .manufacturer = \"Blusys\",
    .product = \"%s\",
}};"""
                % project_title
            )
            capability_refs.append("&usb")

    cap_list = ", ".join(capability_refs)
    ui_include = '#include "ui/app_ui.hpp"\n' if interface != "headless" else ""
    ui_fields = (
        f"    .on_init = {namespace_name}::ui::on_init,\n    .map_intent = {namespace_name}::map_intent,\n"
        if interface != "headless"
        else "    .on_tick = %s::on_tick,\n    .capability_event_discriminant = static_cast<std::uint32_t>(%s::action_tag::capability_event),\n"
        % (namespace_name, namespace_name)
    )
    entry = {
        "handheld": f"BLUSYS_APP({namespace_name}::system::spec)",
        "surface": f'BLUSYS_APP_DASHBOARD({namespace_name}::system::spec, "{project_title}")',
        "headless": f"BLUSYS_APP_MAIN_HEADLESS({namespace_name}::system::spec)",
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
}};

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
    catalog = load_catalog(repo_root)
    validate_model(catalog, interface, capabilities, policies)
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
        render_project_manifest(interface, capabilities, policies),
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


def print_list(catalog: dict) -> None:
    print("Interfaces:")
    for name, meta in catalog["interfaces"].items():
        print(f"  {name:10s} {meta['description']}")
    print("\nCapabilities:")
    for name, meta in catalog["capabilities"].items():
        print(f"  {name:12s} {meta['description']}")
    print("\nPolicies:")
    for name, meta in catalog["policies"].items():
        print(f"  {name:12s} {meta['description']}")


def main() -> int:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--repo-root", required=True)
    parser.add_argument("--interface", default="handheld")
    parser.add_argument("--with", dest="with_caps", default="")
    parser.add_argument("--policy", default="")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("path", nargs="?")
    args = parser.parse_args()

    repo_root = Path(args.repo_root)
    catalog = load_catalog(repo_root)
    if args.list:
        print_list(catalog)
        return 0

    out_dir = Path(args.path).resolve() if args.path else Path.cwd()
    capabilities = parse_csv_arg(args.with_caps)
    policies = parse_csv_arg(args.policy)
    generate_project(repo_root, out_dir, args.interface, capabilities, policies)
    print(f"Created blusys project: {out_dir}")
    print(f"Interface: {args.interface}")
    print(f"Capabilities: {', '.join(capabilities) if capabilities else 'none'}")
    print(f"Policies: {', '.join(policies) if policies else 'none'}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
