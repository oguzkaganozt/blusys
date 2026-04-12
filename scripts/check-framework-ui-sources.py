#!/usr/bin/env python3
"""Verify cmake/blusys_framework_ui_sources.cmake internal consistency."""

from __future__ import annotations

import re
import sys
from pathlib import Path


def _parse_cmake_list(content: str, var_name: str) -> list[str]:
    m = re.search(
        rf"set\({re.escape(var_name)}\s+([^)]+)\)",
        content,
        re.DOTALL,
    )
    if not m:
        print(f"check-framework-ui-sources: missing {var_name}", file=sys.stderr)
        sys.exit(1)
    body = m.group(1)
    out: list[str] = []
    for line in body.splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        out.append(line)
    return out


def main() -> None:
    repo = Path(__file__).resolve().parents[1]
    cmake_path = repo / "cmake" / "blusys_framework_ui_sources.cmake"
    text = cmake_path.read_text(encoding="utf-8")

    shared = _parse_cmake_list(text, "BLUSYS_FRAMEWORK_UI_SRC_SHARED")
    widget_rel = _parse_cmake_list(text, "BLUSYS_FRAMEWORK_HOST_WIDGETKIT_REL")
    device_only = _parse_cmake_list(text, "BLUSYS_FRAMEWORK_UI_SRC_DEVICE_ONLY")

    fw = repo / "components" / "blusys_framework"
    for rel in shared + widget_rel + device_only:
        if not (fw / rel).is_file():
            print(f"check-framework-ui-sources: missing file {rel}", file=sys.stderr)
            sys.exit(1)

    ui_from_shared = sorted(p for p in shared if p.startswith("src/ui/"))
    if sorted(widget_rel) != ui_from_shared:
        print(
            "check-framework-ui-sources: BLUSYS_FRAMEWORK_HOST_WIDGETKIT_REL must match "
            "all src/ui/ paths in BLUSYS_FRAMEWORK_UI_SRC_SHARED (same set, device order in SHARED)",
            file=sys.stderr,
        )
        only_shared = set(ui_from_shared) - set(widget_rel)
        only_widget = set(widget_rel) - set(ui_from_shared)
        if only_shared:
            print(f"  only in SHARED ui: {only_shared}", file=sys.stderr)
        if only_widget:
            print(f"  only in WIDGETKIT_REL: {only_widget}", file=sys.stderr)
        sys.exit(1)

    print("check-framework-ui-sources: OK")


if __name__ == "__main__":
    main()
