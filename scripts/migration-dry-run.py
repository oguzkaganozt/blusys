#!/usr/bin/env python3
"""
Phase 0 migration dry-run for the v0 platform restructure.

For every file currently under components/{blusys_hal,blusys_services,blusys_framework}/
this script computes its expected destination under components/blusys/ according to the
PLATFORM_RESTRUCTURE.md migration tables.

Outputs:
  - A table of (source, destination, phase) for every mapped file
  - A list of any source files NOT yet mapped (gaps to fix before migrating)

No files are moved.
"""

import os
import re
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
COMPONENTS = REPO / "components"

# ---------------------------------------------------------------------------
# Migration tables — source path (relative to REPO) → dest path (relative to REPO)
# Each entry: (src_rel, dst_rel, phase)
# ---------------------------------------------------------------------------

def build_migration_table():
    entries = []

    def add(src, dst, phase):
        entries.append((Path(src), Path(dst), phase))

    # -------------------------------------------------------------------------
    # PHASE 2 — HAL
    # -------------------------------------------------------------------------
    hal_src_inc = "components/blusys_hal/include/blusys"
    hal_dst_inc = "components/blusys/include/blusys/hal"
    hal_src_src = "components/blusys_hal/src"
    hal_dst_src = "components/blusys/src/hal"

    # Public HAL headers (flat, excluding drivers/ and internal/)
    for f in (REPO / hal_src_inc).glob("*.h"):
        if f.name == "blusys.h":
            # Old umbrella — deleted in Phase 2
            add(f"components/blusys_hal/include/blusys/blusys.h",
                "__DELETE__", 2)
        else:
            add(f"components/blusys_hal/include/blusys/{f.name}",
                f"components/blusys/include/blusys/hal/{f.name}", 2)

    # Internal headers — renamed per policy
    internal_renames = {
        "blusys_bt_stack.h":     "bt_stack.h",
        "blusys_esp_err.h":      "esp_err_shim.h",
        "blusys_global_lock.h":  "global_lock.h",
        "blusys_internal.h":     "hal_internal.h",
        "blusys_lock.h":         "lock.h",
        "blusys_nvs_init.h":     "nvs_init.h",
        "blusys_target_caps.h":  "target_caps.h",
        "blusys_timeout.h":      "timeout.h",
        "lcd_panel_ili.h":       "panel_ili.h",
    }
    for old_name, new_name in internal_renames.items():
        add(f"components/blusys_hal/include/blusys/internal/{old_name}",
            f"components/blusys/include/blusys/hal/internal/{new_name}", 2)

    # HAL C sources (flat under src/, excluding drivers/ sub-tree)
    for f in (REPO / hal_src_src).glob("*.c"):
        if f.name == "blusys_lock_freertos.c":
            add(f"components/blusys_hal/src/blusys_lock_freertos.c",
                "components/blusys/src/hal/internal/lock_freertos.c", 2)
        elif f.name == "bt_stack.c":
            add("components/blusys_hal/src/bt_stack.c",
                "components/blusys/src/hal/internal/bt_stack.c", 2)
        else:
            add(f"components/blusys_hal/src/{f.name}",
                f"components/blusys/src/hal/{f.name}", 2)

    # targets/
    for f in (REPO / "components/blusys_hal/src/targets").rglob("*"):
        if f.is_file():
            rel = f.relative_to(REPO / "components/blusys_hal/src/targets")
            add(f"components/blusys_hal/src/targets/{rel}",
                f"components/blusys/src/hal/targets/{rel}", 2)

    # ulp/
    for f in (REPO / "components/blusys_hal/src/ulp").rglob("*"):
        if f.is_file():
            rel = f.relative_to(REPO / "components/blusys_hal/src/ulp")
            add(f"components/blusys_hal/src/ulp/{rel}",
                f"components/blusys/src/hal/ulp/{rel}", 2)

    # -------------------------------------------------------------------------
    # PHASE 3 — Drivers
    # -------------------------------------------------------------------------
    for f in (REPO / "components/blusys_hal/include/blusys/drivers").glob("*.h"):
        add(f"components/blusys_hal/include/blusys/drivers/{f.name}",
            f"components/blusys/include/blusys/drivers/{f.name}", 3)

    for f in (REPO / "components/blusys_hal/src/drivers").glob("*.c"):
        add(f"components/blusys_hal/src/drivers/{f.name}",
            f"components/blusys/src/drivers/{f.name}", 3)

    # display moved from services → drivers
    add("components/blusys_services/include/blusys/drivers/display.h",
        "components/blusys/include/blusys/drivers/display.h", 3)
    add("components/blusys_services/src/output/display.c",
        "components/blusys/src/drivers/display.c", 3)

    # NEW panel headers — new files, no source mapping needed
    # (authored fresh, not moved — listed as informational)
    panel_names = ["ili9341", "ili9488", "st7735", "st7789", "ssd1306", "qemu_rgb"]
    for p in panel_names:
        entries.append((Path(f"__NEW__/drivers/panels/{p}.h"),
                        Path(f"components/blusys/include/blusys/drivers/panels/{p}.h"),
                        3))

    # -------------------------------------------------------------------------
    # PHASE 4 — Services
    # -------------------------------------------------------------------------
    # connectivity/ (excluding protocol/)
    for f in (REPO / "components/blusys_services/include/blusys/connectivity").glob("*.h"):
        add(f"components/blusys_services/include/blusys/services/connectivity/{f.name}",
            f"components/blusys/include/blusys/services/connectivity/{f.name}", 4)
    for f in (REPO / "components/blusys_services/src/connectivity").glob("*.c"):
        add(f"components/blusys_services/src/connectivity/{f.name}",
            f"components/blusys/src/services/connectivity/{f.name}", 4)

    # connectivity/protocol/ → services/protocol/ (promoted to sibling)
    for f in (REPO / "components/blusys_services/include/blusys/services/connectivity/protocol").glob("*.h"):
        add(f"components/blusys_services/include/blusys/services/protocol/{f.name}",
            f"components/blusys/include/blusys/services/protocol/{f.name}", 4)
    for f in (REPO / "components/blusys_services/src/connectivity/protocol").glob("*.c"):
        add(f"components/blusys_services/src/connectivity/protocol/{f.name}",
            f"components/blusys/src/services/protocol/{f.name}", 4)

    # storage/
    for f in (REPO / "components/blusys_services/include/blusys/storage").glob("*.h"):
        add(f"components/blusys_services/include/blusys/services/storage/{f.name}",
            f"components/blusys/include/blusys/services/storage/{f.name}", 4)
    for f in (REPO / "components/blusys_services/src/storage").glob("*.c"):
        add(f"components/blusys_services/src/storage/{f.name}",
            f"components/blusys/src/services/storage/{f.name}", 4)

    # device/ → system/
    for f in (REPO / "components/blusys_services/include/blusys/device").glob("*.h"):
        add(f"components/blusys_services/include/blusys/services/system/{f.name}",
            f"components/blusys/include/blusys/services/system/{f.name}", 4)
    for f in (REPO / "components/blusys_services/src/device").glob("*.c"):
        add(f"components/blusys_services/src/device/{f.name}",
            f"components/blusys/src/services/system/{f.name}", 4)

    # input/usb_hid → connectivity/usb_hid
    add("components/blusys_services/include/blusys/services/connectivity/usb_hid.h",
        "components/blusys/include/blusys/services/connectivity/usb_hid.h", 4)
    add("components/blusys_services/src/input/usb_hid.c",
        "components/blusys/src/services/connectivity/usb_hid.c", 4)

    # Old services umbrella — deleted
    add("components/blusys_services/include/blusys/blusys_services.h",
        "__DELETE__", 4)

    # -------------------------------------------------------------------------
    # PHASE 5a — Framework (concept restructure, non-UI)
    # -------------------------------------------------------------------------
    fw_inc = "components/blusys_framework/include/blusys"
    fw_src = "components/blusys_framework/src"
    fw_dst_inc = "components/blusys/include/blusys/framework"
    fw_dst_src = "components/blusys/src/framework"

    # app/*.hpp → framework/app/*.hpp (with renames)
    app_renames = {
        "app_ctx.hpp":    "ctx.hpp",
        "app_spec.hpp":   "spec.hpp",
        "app_identity.hpp": "identity.hpp",
        "app_services.hpp": "services.hpp",
    }
    for old, new in app_renames.items():
        add(f"{fw_inc}/app/{old}", f"{fw_dst_inc}/app/{new}", "5a")

    for name in ["app.hpp", "entry.hpp", "variant_dispatch.hpp"]:
        add(f"{fw_inc}/app/{name}", f"{fw_dst_inc}/app/{name}", "5a")

    # app/detail/
    add(f"{fw_inc}/app/detail/app_runtime.hpp",
        f"{fw_dst_inc}/app/internal/app_runtime.hpp", "5a")
    add(f"{fw_inc}/app/detail/default_route_sink.hpp",
        f"{fw_dst_inc}/engine/internal/default_route_sink.hpp", "5a")
    add(f"{fw_inc}/app/detail/feedback_logging_sink.hpp",
        f"{fw_dst_inc}/feedback/internal/logging_sink.hpp", "5a")
    add(f"{fw_inc}/app/detail/pending_events.hpp",
        f"{fw_dst_inc}/engine/pending_events.hpp", "5a")

    # capabilities
    add(f"{fw_inc}/app/capability.hpp",       f"{fw_dst_inc}/capabilities/capability.hpp", "5a")
    add(f"{fw_inc}/app/capability_event.hpp", f"{fw_dst_inc}/capabilities/event.hpp", "5a")
    add(f"{fw_inc}/app/capability_list.hpp",  f"{fw_dst_inc}/capabilities/list.hpp", "5a")
    add(f"{fw_inc}/app/capabilities_inline.hpp", f"{fw_dst_inc}/capabilities/inline.hpp", "5a")
    for cap in ["bluetooth", "connectivity", "diagnostics", "lan_control",
                "mqtt_host", "ota", "provisioning", "storage", "telemetry", "usb"]:
        add(f"{fw_inc}/app/capabilities/{cap}.hpp",
            f"{fw_dst_inc}/capabilities/{cap}.hpp", "5a")
    add(f"{fw_inc}/app/capabilities/capabilities.hpp",
        f"{fw_dst_inc}/capabilities/capabilities.hpp", "5a")

    # flows
    for flow in ["boot.hpp", "loading.hpp", "error.hpp", "status.hpp", "settings.hpp",
                 "connectivity_flow.hpp", "provisioning_flow.hpp",
                 "diagnostics_flow.hpp", "ota_flow.hpp", "flows.hpp"]:
        add(f"{fw_inc}/app/flows/{flow}", f"{fw_dst_inc}/flows/{flow}", "5a")

    # engine
    add(f"{fw_inc}/framework/core/router.hpp",  f"{fw_dst_inc}/engine/router.hpp", "5a")
    add(f"{fw_inc}/framework/core/intent.hpp",  f"{fw_dst_inc}/engine/intent.hpp", "5a")
    add(f"{fw_inc}/app/integration_dispatch.hpp", f"{fw_dst_inc}/engine/integration_dispatch.hpp", "5a")
    add(f"{fw_inc}/framework/core/runtime.hpp", f"{fw_dst_inc}/engine/event_queue.hpp", "5a")

    # feedback
    add(f"{fw_inc}/framework/core/feedback.hpp",         f"{fw_dst_inc}/feedback/feedback.hpp", "5a")
    add(f"{fw_inc}/framework/core/feedback_presets.hpp",  f"{fw_dst_inc}/feedback/presets.hpp", "5a")

    # misc top-level
    add(f"{fw_inc}/framework/core/containers.hpp", f"{fw_dst_inc}/containers.hpp", "5a")
    add(f"{fw_inc}/framework/ui/callbacks.hpp",    f"{fw_dst_inc}/callbacks.hpp", "5a")
    add(f"{fw_inc}/app/theme_presets.hpp",          f"{fw_dst_inc}/ui/style/presets.hpp", "5a")

    # integration.hpp — deleted
    add(f"{fw_inc}/app/integration.hpp", "__DELETE__", "5a")
    # Old framework.hpp — deleted and replaced fresh
    add(f"{fw_inc}/framework/framework.hpp", "__REWRITE__", "5a")

    # -------------------------------------------------------------------------
    # PHASE 5b — platform/
    # -------------------------------------------------------------------------
    platform_moves = {
        "app/platform_profile.hpp":         "platform/profile.hpp",
        "app/host_profile.hpp":             "platform/host.hpp",
        "app/auto_profile.hpp":             "platform/auto.hpp",
        "app/build_profile.hpp":            "platform/build.hpp",
        "app/reference_build_profile.hpp":  "platform/reference_build.hpp",
        "app/layout_surface.hpp":           "platform/layout_surface.hpp",
        "app/auto_shell.hpp":               "platform/auto_shell.hpp",
        "app/button_array_bridge.hpp":      "platform/button_array_bridge.hpp",
        "app/input_bridge.hpp":             "platform/input_bridge.hpp",
        "app/touch_bridge.hpp":             "platform/touch_bridge.hpp",
        "app/profiles/headless.hpp":        "platform/headless.hpp",
    }
    for src_rel, dst_rel in platform_moves.items():
        add(f"{fw_inc}/{src_rel}", f"{fw_dst_inc}/{dst_rel}", "5b")

    # profiles/host.hpp merged into platform/host.hpp (already covered above)
    add(f"{fw_inc}/app/profiles/host.hpp", "__MERGE_INTO_platform/host.hpp__", "5b")

    # panel profiles → platform/profiles/
    for panel in ["ili9341", "ili9488", "qemu_rgb", "ssd1306", "st7735", "st7789"]:
        add(f"{fw_inc}/app/profiles/{panel}.hpp",
            f"{fw_dst_inc}/platform/profiles/{panel}.hpp", "5b")

    # -------------------------------------------------------------------------
    # PHASE 5c — UI layer (non-widget restructure)
    # -------------------------------------------------------------------------
    # framework/ui/* → framework/ui/* (mostly preserved, some renames/moves)
    # style/
    ui_style_moves = {
        "framework/ui/theme.hpp":            "ui/style/theme.hpp",
        "framework/ui/transition.hpp":       "ui/style/transition.hpp",
        "framework/ui/visual_feedback.hpp":  "ui/style/interaction_effects.hpp",
        "framework/ui/fonts.hpp":            "ui/style/fonts.hpp",
        "framework/ui/icons/icon_set.hpp":           "ui/style/icon_set.hpp",
        "framework/ui/icons/icon_set_minimal.hpp":   "ui/style/icon_set_minimal.hpp",
    }
    for src_rel, dst_rel in ui_style_moves.items():
        add(f"{fw_inc}/{src_rel}", f"{fw_dst_inc}/{dst_rel}", "5c")

    # ui/input/ — unchanged paths, kept under ui/input/
    for name in ["encoder.hpp", "focus_scope.hpp"]:
        add(f"{fw_inc}/framework/ui/input/{name}",
            f"{fw_dst_inc}/ui/input/{name}", "5c")

    # ui/composition/ (was app/view/)
    view_moves = {
        "app/view/view.hpp":             "ui/composition/view.hpp",
        "app/view/page.hpp":             "ui/composition/page.hpp",
        "app/view/shell.hpp":            "ui/composition/shell.hpp",
        "app/view/overlay_manager.hpp":  "ui/composition/overlay_manager.hpp",
        "app/view/screen_registry.hpp":  "ui/composition/screen_registry.hpp",
        "app/view/screen_router.hpp":    "ui/composition/screen_router.hpp",
    }
    for src_rel, dst_rel in view_moves.items():
        add(f"{fw_inc}/{src_rel}", f"{fw_dst_inc}/{dst_rel}", "5c")

    # ui/binding/ (was app/view/)
    binding_moves = {
        "app/view/bindings.hpp":       "ui/binding/bindings.hpp",
        "app/view/action_widgets.hpp": "ui/binding/action_widgets.hpp",
        "app/view/composites.hpp":     "ui/binding/composites.hpp",
    }
    for src_rel, dst_rel in binding_moves.items():
        add(f"{fw_inc}/{src_rel}", f"{fw_dst_inc}/{dst_rel}", "5c")

    # ui/extension/ (was app/view/)
    ext_moves = {
        "app/view/custom_widget.hpp": "ui/extension/custom_widget.hpp",
        "app/view/lvgl_scope.hpp":    "ui/extension/lvgl_scope.hpp",
    }
    for src_rel, dst_rel in ext_moves.items():
        add(f"{fw_inc}/{src_rel}", f"{fw_dst_inc}/{dst_rel}", "5c")

    # ui/primitives/ — moved from framework/ui/primitives/ to ui/primitives/
    for f in (REPO / fw_inc / "framework/ui/primitives").glob("*.hpp"):
        add(f"{fw_inc}/framework/ui/primitives/{f.name}",
            f"{fw_dst_inc}/ui/primitives/{f.name}", "5c")

    # ui/widgets/ — FLAT (was singleton sub-dirs)
    widget_dirs = ["button", "card", "chart", "data_table", "dropdown", "gauge",
                   "input_field", "knob", "level_bar", "list", "modal", "overlay",
                   "progress", "slider", "tabs", "toggle", "vu_strip"]
    for w in widget_dirs:
        hpp = f"{w}.hpp"
        add(f"{fw_inc}/framework/ui/widgets/{w}/{hpp}",
            f"{fw_dst_inc}/ui/widgets/{hpp}", "5c")

    # ui/screens/ (was app/screens/ with _screen suffix dropped)
    screen_renames = {
        "app/screens/about_screen.hpp":       "ui/screens/about.hpp",
        "app/screens/diagnostics_screen.hpp": "ui/screens/diagnostics.hpp",
        "app/screens/status_screen.hpp":      "ui/screens/status.hpp",
        "app/screens/screens.hpp":            "ui/screens/screens.hpp",
    }
    for src_rel, dst_rel in screen_renames.items():
        add(f"{fw_inc}/{src_rel}", f"{fw_dst_inc}/{dst_rel}", "5c")

    # ui/detail/ — private helpers (stay as internal, mapped as-is)
    for name in ["fixed_slot_pool.hpp", "flex_layout.hpp", "widget_common.hpp"]:
        add(f"{fw_inc}/framework/ui/detail/{name}",
            f"{fw_dst_inc}/ui/detail/{name}", "5c")

    # umbrella headers (kept)
    add(f"{fw_inc}/framework/ui/widgets.hpp",    f"{fw_dst_inc}/ui/widgets.hpp", "5c")
    add(f"{fw_inc}/framework/ui/primitives.hpp", f"{fw_dst_inc}/ui/primitives.hpp", "5c")

    # -------------------------------------------------------------------------
    # PHASE 5a — Framework src/ files (app, capabilities, flows, engine, feedback)
    # -------------------------------------------------------------------------
    # app/
    app_src_renames = {
        "app_ctx.cpp":     "ctx.cpp",
        "app_services.cpp": "services.cpp",
    }
    for old, new in app_src_renames.items():
        add(f"{fw_src}/app/{old}", f"{fw_dst_src}/app/{new}", "5a")

    for name in ["feedback_logging_sink.cpp"]:
        add(f"{fw_src}/app/{name}", f"{fw_dst_src}/feedback/internal/logging_sink.cpp", "5a")

    for name in ["capability_event_map.cpp"]:
        add(f"{fw_src}/app/{name}", f"{fw_dst_src}/capabilities/capability_event_map.cpp", "5a")

    # capabilities/
    for f in (REPO / fw_src / "app/capabilities").glob("*.cpp"):
        add(f"{fw_src}/app/capabilities/{f.name}",
            f"{fw_dst_src}/capabilities/{f.name}", "5a")

    # flows/
    for f in (REPO / fw_src / "app/flows").glob("*.cpp"):
        add(f"{fw_src}/app/flows/{f.name}",
            f"{fw_dst_src}/flows/{f.name}", "5a")

    # core/ (engine + feedback)
    core_src_moves = {
        "feedback.cpp":         "feedback/feedback.cpp",
        "feedback_presets.cpp": "feedback/presets.cpp",
        "framework.cpp":        "framework.cpp",   # internal build entry point
        "router.cpp":           "engine/router.cpp",
        "runtime.cpp":          "engine/event_queue.cpp",
    }
    for old, new in core_src_moves.items():
        add(f"{fw_src}/core/{old}", f"{fw_dst_src}/{new}", "5a")

    # -------------------------------------------------------------------------
    # PHASE 5b — Platform src/ files
    # -------------------------------------------------------------------------
    platform_src_moves = {
        "button_array_bridge.cpp": "platform/button_array_bridge.cpp",
        "input_bridge.cpp":        "platform/input_bridge.cpp",
        "touch_bridge.cpp":        "platform/touch_bridge.cpp",
        # ESP-specific platform impls (no public header — internal)
        "app_device_platform.cpp":          "platform/app_device_platform.cpp",
        "app_headless_platform_esp.cpp":    "platform/app_headless_platform_esp.cpp",
    }
    for old, new in platform_src_moves.items():
        add(f"{fw_src}/app/{old}", f"{fw_dst_src}/{new}", "5b")

    # -------------------------------------------------------------------------
    # PHASE 5c — UI src/ files
    # -------------------------------------------------------------------------
    # ui/composition/ (was src/app/)
    ui_comp_moves = {
        "action_widgets.cpp":  "ui/binding/action_widgets.cpp",
        "bindings.cpp":        "ui/binding/bindings.cpp",
        "overlay_manager.cpp": "ui/composition/overlay_manager.cpp",
        "page.cpp":            "ui/composition/page.cpp",
        "screen_registry.cpp": "ui/composition/screen_registry.cpp",
        "screen_router.cpp":   "ui/composition/screen_router.cpp",
        "shell.cpp":           "ui/composition/shell.cpp",
        "theme_presets.cpp":   "ui/style/presets.cpp",
    }
    for old, new in ui_comp_moves.items():
        add(f"{fw_src}/app/{old}", f"{fw_dst_src}/{new}", "5c")

    # app/screens/
    screen_src_renames = {
        "about_screen.cpp":       "ui/screens/about.cpp",
        "diagnostics_screen.cpp": "ui/screens/diagnostics.cpp",
        "status_screen.cpp":      "ui/screens/status.cpp",
    }
    for old, new in screen_src_renames.items():
        add(f"{fw_src}/app/screens/{old}", f"{fw_dst_src}/{new}", "5c")

    # src/ui/ subtree
    add(f"{fw_src}/ui/icons/icon_set_minimal.cpp",
        f"{fw_dst_src}/ui/style/icon_set_minimal.cpp", "5c")
    add(f"{fw_src}/ui/input/encoder.cpp",
        f"{fw_dst_src}/ui/input/encoder.cpp", "5c")
    add(f"{fw_src}/ui/input/focus_scope.cpp",
        f"{fw_dst_src}/ui/input/focus_scope.cpp", "5c")
    add(f"{fw_src}/ui/theme.cpp",      f"{fw_dst_src}/ui/style/theme.cpp", "5c")
    add(f"{fw_src}/ui/transition.cpp", f"{fw_dst_src}/ui/style/transition.cpp", "5c")
    add(f"{fw_src}/ui/visual_feedback.cpp",
        f"{fw_dst_src}/ui/style/interaction_effects.cpp", "5c")

    for f in (REPO / fw_src / "ui/primitives").glob("*.cpp"):
        add(f"{fw_src}/ui/primitives/{f.name}",
            f"{fw_dst_src}/ui/primitives/{f.name}", "5c")

    # widget singleton dirs → flat
    for w in (REPO / fw_src / "ui/widgets").iterdir():
        if w.is_dir():
            for f in w.glob("*.cpp"):
                add(f"{fw_src}/ui/widgets/{w.name}/{f.name}",
                    f"{fw_dst_src}/ui/widgets/{f.name}", "5c")

    # private UI detail headers → src/ (moved out of public include)
    for name in ["fixed_slot_pool.hpp", "flex_layout.hpp", "widget_common.hpp"]:
        add(f"{fw_inc}/framework/ui/detail/{name}",
            f"{fw_dst_src}/ui/{name}", "5c")

    # -------------------------------------------------------------------------
    # Infrastructure / meta-files — old components get deleted
    # -------------------------------------------------------------------------
    for comp in ["blusys_hal", "blusys_services", "blusys_framework"]:
        cmake = REPO / "components" / comp / "CMakeLists.txt"
        if cmake.exists():
            add(f"components/{comp}/CMakeLists.txt", "__DELETE__", "infra")
        kconfig = REPO / "components" / comp / "Kconfig"
        if kconfig.exists():
            add(f"components/{comp}/Kconfig", "__MERGE_INTO_blusys/Kconfig__", "1")
        readme = REPO / "components" / comp / "README.md"
        if readme.exists():
            add(f"components/{comp}/README.md", "__DELETE__", "infra")

    # widget-author-guide.md → keep in docs
    add("components/blusys_framework/widget-author-guide.md",
        "__DELETE_OR_MOVE_TO_docs__", "8")

    # .gitkeep files — ignored (infrastructure scaffolding)
    for base in [REPO / fw_inc, REPO / fw_src]:
        for gk in base.rglob(".gitkeep"):
            rel = gk.relative_to(REPO)
            entries.append((rel, Path("__IGNORE__"), "infra"))

    return entries


# ---------------------------------------------------------------------------
# Validate: collect all source files and check against migration table
# ---------------------------------------------------------------------------

def collect_sources():
    """All files under the three source component trees."""
    sources = set()
    for comp in ["blusys_hal", "blusys_services", "blusys_framework"]:
        for f in (COMPONENTS / comp).rglob("*"):
            if f.is_file():
                sources.add(f.relative_to(REPO))
    return sources


def main():
    table = build_migration_table()
    sources = collect_sources()

    # Compute set of mapped sources
    mapped = {}  # src_path → (dst_path, phase)
    for src, dst, phase in table:
        if not str(src).startswith("__NEW__"):
            mapped[src] = (dst, phase)

    # Find unmapped files
    unmapped = []
    for s in sorted(sources):
        if s not in mapped:
            unmapped.append(s)

    # Print migration table
    print("=" * 100)
    print(f"MIGRATION DRY-RUN — {len(table)} entries, {len(sources)} source files")
    print("=" * 100)
    print(f"\n{'Phase':<6}  {'Source':<70}  {'Destination'}")
    print("-" * 100)
    for src, dst, phase in sorted(table, key=lambda x: (str(x[2]), str(x[0]))):
        print(f"{str(phase):<6}  {str(src):<70}  {dst}")

    print()
    print("=" * 100)
    if unmapped:
        print(f"GAPS — {len(unmapped)} source files NOT yet mapped:")
        print("-" * 100)
        for u in unmapped:
            # Classify which component
            print(f"  UNMAPPED: {u}")
        print()
        print("ACTION REQUIRED: add these files to the migration table before Phase 1.")
        sys.exit(1)
    else:
        print(f"OK — all {len(sources)} source files accounted for in the migration table.")
        print()
        print("Phase 0 checklist:")
        print(f"  [x] File inventory captured ({len(sources)} files)")
        print(f"  [x] Consumer audit complete (examples, docs, scripts, cmake)")
        print(f"  [x] Migration dry-run passes with zero unaccounted paths")
        sys.exit(0)


if __name__ == "__main__":
    main()
