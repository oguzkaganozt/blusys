#!/usr/bin/env python3
"""Verify that cmake/blusys_host_bridge.cmake stays in sync with the
blusys single-component source list (components/blusys/CMakeLists.txt).

Two invariants checked:

  1. Every framework src/ file listed in _BLUSYS_HOST_BRIDGE_COMMON_SOURCES
     or _BLUSYS_HOST_BRIDGE_INTERACTIVE_SOURCES also appears in the
     blusys_sources set in components/blusys/CMakeLists.txt.
     (Catches bridge entries that reference removed/renamed sources.)

  2. For each non-host device capability source in blusys_sources
     (src/framework/capabilities/<name>.cpp, excluding *_host.cpp and
     telemetry.cpp/mqtt_host.cpp which are host-safe), the bridge's
     COMMON_SOURCES must contain a corresponding *_host.cpp entry.
     (Catches capability host stubs that were forgotten in the bridge.)
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


def _parse_cmake_sources(text: str, var_name: str) -> list[str]:
    """Extract relative src/... paths from a CMake variable block.

    Supports both:
      set(VAR
          src/...
      )
    and:
      set(_VAR
          ${PREFIX}/src/...
      )
    stripping the ${...}/ cmake variable prefix where present.
    """
    pattern = rf"set\s*\(\s*{re.escape(var_name)}\b(.*?)\n\s*\)"
    m = re.search(pattern, text, re.DOTALL)
    if not m:
        return []
    out: list[str] = []
    for line in m.group(1).splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        # Strip cmake variable prefix like ${BLUSYS_COMPONENT_DIR}/
        rel = re.sub(r"^\$\{[^}]+\}/", "", line)
        if rel.startswith("src/") and (rel.endswith(".cpp") or rel.endswith(".c")):
            out.append(rel)
    return out


def _parse_all_cmake_src(cmake_text: str) -> set[str]:
    """Collect all src/... paths from a CMakeLists.txt file.

    Handles both set(blusys_sources ...) and target_sources(... PRIVATE ...)
    blocks by scanning every line for src/-prefixed paths.
    """
    out: set[str] = set()
    for line in cmake_text.splitlines():
        line = line.split("#", 1)[0].strip()
        # Strip cmake variable prefix like ${COMPONENT_LIB}  or other variables.
        rel = re.sub(r"^\$\{[^}]+\}/", "", line).strip()
        if rel.startswith("src/") and (rel.endswith(".cpp") or rel.endswith(".c")):
            out.add(rel)
    return out


def _parse_blusys_sources(cmake_text: str) -> set[str]:
    """Collect all src/ paths from components/blusys/CMakeLists.txt."""
    sources = _parse_all_cmake_src(cmake_text)
    if not sources:
        print(
            "check-host-bridge-spine: could not parse any sources in CMakeLists.txt",
            file=sys.stderr,
        )
        sys.exit(1)
    return sources


def _parse_bridge_sources(bridge_text: str, var_name: str) -> list[str]:
    sources = _parse_cmake_sources(bridge_text, var_name)
    if not sources and var_name == "_BLUSYS_HOST_BRIDGE_COMMON_SOURCES":
        print(
            f"check-host-bridge-spine: could not parse {var_name} in blusys_host_bridge.cmake",
            file=sys.stderr,
        )
        sys.exit(1)
    return sources


def main() -> None:
    repo = Path(__file__).resolve().parents[1]
    blusys_cmake = (
        repo / "components" / "blusys" / "CMakeLists.txt"
    ).read_text(encoding="utf-8")
    bridge_text = (
        repo / "cmake" / "blusys_host_bridge.cmake"
    ).read_text(encoding="utf-8")

    blusys_sources = _parse_blusys_sources(blusys_cmake)

    common_sources = _parse_bridge_sources(bridge_text, "_BLUSYS_HOST_BRIDGE_COMMON_SOURCES")
    interactive_sources = _parse_bridge_sources(bridge_text, "_BLUSYS_HOST_BRIDGE_INTERACTIVE_SOURCES")
    all_bridge_sources = common_sources + interactive_sources

    errors: list[str] = []

    # --- Invariant 1: every bridge framework src/ entry exists in blusys_sources ---
    for src in all_bridge_sources:
        if not src.startswith("src/framework/") and not src.startswith("src/hal/"):
            continue
        if src not in blusys_sources:
            errors.append(
                f"bridge lists '{src}' but it is not in components/blusys/CMakeLists.txt"
            )

    # --- Invariant 2: every device capability has a host stub in bridge COMMON ---
    # Device-only capabilities: src/framework/capabilities/<name>.cpp
    # where <name> does NOT end in _host, and is not telemetry or mqtt_host.
    HOST_SAFE = {"telemetry.cpp", "mqtt_host.cpp", "capability_event_map.cpp"}
    device_caps: list[str] = []
    for src in blusys_sources:
        if not src.startswith("src/framework/capabilities/"):
            continue
        name = Path(src).name
        if name.endswith("_host.cpp"):
            continue
        if name in HOST_SAFE:
            continue
        device_caps.append(src)

    bridge_host_stubs = {
        Path(s).name
        for s in common_sources
        if s.startswith("src/framework/capabilities/") and Path(s).name.endswith("_host.cpp")
    }

    for dev_src in sorted(device_caps):
        stem = Path(dev_src).stem  # e.g. "connectivity"
        expected_host = f"{stem}_host.cpp"
        if expected_host not in bridge_host_stubs:
            errors.append(
                f"device capability '{dev_src}' has no '{expected_host}' in bridge COMMON_SOURCES"
            )

    if errors:
        print("check-host-bridge-spine: FAIL", file=sys.stderr)
        for e in errors:
            print(f"  {e}", file=sys.stderr)
        sys.exit(1)

    print("check-host-bridge-spine: OK")


if __name__ == "__main__":
    main()
