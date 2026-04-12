#!/usr/bin/env python3
"""Verify cmake/blusys_framework_ui_sources.cmake matches on-disk UI sources."""

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


def _literal_cpp_paths_in_set(content: str, var_name: str) -> list[str]:
    """Literal src/...cpp lines inside set(VAR ...) — excludes CMake ${...} references."""
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
        if not line or "${" in line:
            continue
        if re.match(r"^src/.+\.cpp$", line):
            out.append(line)
    return out


def main() -> None:
    repo = Path(__file__).resolve().parents[1]
    cmake_path = repo / "cmake" / "blusys_framework_ui_sources.cmake"
    text = cmake_path.read_text(encoding="utf-8")

    shared_literals = _literal_cpp_paths_in_set(text, "BLUSYS_FRAMEWORK_UI_SRC_SHARED")
    widgetkit_lines = _parse_cmake_list(text, "BLUSYS_FRAMEWORK_HOST_WIDGETKIT_REL")
    device_only = _parse_cmake_list(text, "BLUSYS_FRAMEWORK_UI_SRC_DEVICE_ONLY")

    fw = repo / "components" / "blusys_framework"

    widgets_on_disk = sorted(
        str(p.relative_to(fw)).replace("\\", "/")
        for p in fw.glob("src/ui/widgets/*/*.cpp")
    )

    for rel in shared_literals + device_only:
        if not (fw / rel).is_file():
            print(f"check-framework-ui-sources: missing file {rel}", file=sys.stderr)
            sys.exit(1)

    for line in widgetkit_lines:
        if line.startswith("src/") and "${" not in line:
            if not (fw / line).is_file():
                print(f"check-framework-ui-sources: missing file {line}", file=sys.stderr)
                sys.exit(1)

    shared_ui = sorted(
        [p for p in shared_literals if p.startswith("src/ui/")] + widgets_on_disk
    )
    kit_ui = sorted(
        [p for p in widgetkit_lines if p.startswith("src/ui/") and "${" not in p]
        + widgets_on_disk
    )

    if shared_ui != kit_ui:
        print(
            "check-framework-ui-sources: src/ui/ entries must match between "
            "BLUSYS_FRAMEWORK_UI_SRC_SHARED and BLUSYS_FRAMEWORK_HOST_WIDGETKIT_REL "
            "(widgets come from file(GLOB ...))",
            file=sys.stderr,
        )
        print(f"  shared ui: {shared_ui}", file=sys.stderr)
        print(f"  kit ui:    {kit_ui}", file=sys.stderr)
        sys.exit(1)

    cmake_widgets = sorted(
        p for p in shared_literals if p.startswith("src/ui/widgets/")
    )
    if cmake_widgets:
        print(
            "check-framework-ui-sources: remove explicit widget .cpp lines from SHARED; "
            f"use glob only (found {cmake_widgets})",
            file=sys.stderr,
        )
        sys.exit(1)

    if widgets_on_disk != sorted(
        p for p in shared_ui if p.startswith("src/ui/widgets/")
    ):
        print("check-framework-ui-sources: widget path mismatch vs disk", file=sys.stderr)
        sys.exit(1)

    if "file(GLOB _blusys_ui_widget_abs" not in text:
        print(
            "check-framework-ui-sources: expected widget file(GLOB in blusys_framework_ui_sources.cmake",
            file=sys.stderr,
        )
        sys.exit(1)

    print("check-framework-ui-sources: OK")


if __name__ == "__main__":
    main()
