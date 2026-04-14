#!/usr/bin/env python3
"""Verify that cmake/blusys_host_bridge.cmake's interactive UI source list matches
on-disk UI sources under components/blusys/src/framework/ui/.

Checks:
  1. Every source listed in _BLUSYS_HOST_BRIDGE_INTERACTIVE_SOURCES exists on disk.
  2. Every .cpp under src/framework/ui/widgets/ is present in the interactive list
     (flat layout — no sub-dirs).
  3. No source is listed more than once.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


def _parse_interactive_sources(bridge_text: str) -> list[str]:
    """Extract relative src/... paths from _BLUSYS_HOST_BRIDGE_INTERACTIVE_SOURCES."""
    m = re.search(
        r"set\(_BLUSYS_HOST_BRIDGE_INTERACTIVE_SOURCES\s+(.*?)\n\)",
        bridge_text,
        re.DOTALL,
    )
    if not m:
        print(
            "check-framework-ui-sources: missing _BLUSYS_HOST_BRIDGE_INTERACTIVE_SOURCES in blusys_host_bridge.cmake",
            file=sys.stderr,
        )
        sys.exit(1)
    out: list[str] = []
    for line in m.group(1).splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        # Lines look like: ${BLUSYS_COMPONENT_DIR}/src/framework/ui/...cpp
        # Strip leading cmake variable prefix (${VAR}/).
        rel = re.sub(r"^\$\{[^}]+\}/", "", line)
        if rel.endswith(".cpp") or rel.endswith(".c"):
            out.append(rel)
    return out


def main() -> None:
    repo = Path(__file__).resolve().parents[1]
    comp = repo / "components" / "blusys"
    bridge_path = repo / "cmake" / "blusys_host_bridge.cmake"
    bridge_text = bridge_path.read_text(encoding="utf-8")

    listed = _parse_interactive_sources(bridge_text)

    # 1. Check for duplicates.
    seen: set[str] = set()
    duplicates: list[str] = []
    for src in listed:
        if src in seen:
            duplicates.append(src)
        seen.add(src)
    if duplicates:
        print(
            f"check-framework-ui-sources: duplicate entries in INTERACTIVE_SOURCES: {duplicates}",
            file=sys.stderr,
        )
        sys.exit(1)

    # 2. Check every listed source exists on disk.
    missing = [src for src in listed if not (comp / src).is_file()]
    if missing:
        print(
            "check-framework-ui-sources: listed sources not found on disk:",
            file=sys.stderr,
        )
        for m in missing:
            print(f"  {m}", file=sys.stderr)
        sys.exit(1)

    # 3. Check every widget .cpp on disk is covered.
    widgets_on_disk = sorted(
        str(p.relative_to(comp)).replace("\\", "/")
        for p in (comp / "src" / "framework" / "ui" / "widgets").glob("*.cpp")
    )
    listed_widgets = sorted(
        s for s in listed if s.startswith("src/framework/ui/widgets/")
    )
    if widgets_on_disk != listed_widgets:
        only_disk = sorted(set(widgets_on_disk) - set(listed_widgets))
        only_list = sorted(set(listed_widgets) - set(widgets_on_disk))
        if only_disk:
            print(
                f"check-framework-ui-sources: widgets on disk not in bridge list: {only_disk}",
                file=sys.stderr,
            )
        if only_list:
            print(
                f"check-framework-ui-sources: bridge list has widgets not on disk: {only_list}",
                file=sys.stderr,
            )
        sys.exit(1)

    print("check-framework-ui-sources: OK")


if __name__ == "__main__":
    main()
