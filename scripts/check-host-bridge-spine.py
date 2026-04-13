#!/usr/bin/env python3
"""Ensure cmake/blusys_host_bridge.cmake lists the same framework spine + host
capability stubs implied by components/blusys_framework/CMakeLists.txt.

Device-only translation units (ESP_PLATFORM) and UI sources are not compared here;
this catches drift in the shared core + capability_host + telemetry set.
"""

from __future__ import annotations

import re
import sys
from pathlib import Path


def _parse_spine(fw_text: str) -> list[str]:
    """Collect entries from the initial set(blusys_framework_sources ...) block.

    Uses a line-by-line state machine: start collecting after the opening
    set(...) line and stop at the first bare ')' line.  Robust to blank lines,
    comment-only lines, and any following structure.
    """
    lines = fw_text.splitlines()
    collecting = False
    out: list[str] = []
    for line in lines:
        stripped = line.strip()
        if not collecting:
            if re.match(r"set\(blusys_framework_sources\b", stripped):
                collecting = True
            continue
        # A lone ')' closes the set() call.
        if stripped == ")":
            return out
        entry = stripped.split("#", 1)[0].strip()
        if entry:
            out.append(entry)
    print("check-host-bridge-spine: missing initial blusys_framework_sources block", file=sys.stderr)
    sys.exit(1)


def _parse_esp_capability_cpp(fw_text: str) -> list[str]:
    """Collect capability .cpp entries from if(ESP_PLATFORM) ... endif() blocks.

    Uses a line-by-line state machine: enter on 'if(ESP_PLATFORM)', count
    nested if/endif pairs, collect list(APPEND blusys_framework_sources ...)
    entries, exit on the matching endif().  Robust to blank lines, comments,
    and the absence of a specific following comment.
    """
    lines = fw_text.splitlines()
    depth = 0
    in_append = False
    out: list[str] = []
    for line in lines:
        stripped = line.strip()
        if depth == 0:
            if re.match(r"if\(ESP_PLATFORM\)", stripped):
                depth = 1
            continue
        # Track nested if/endif.
        if re.match(r"if\s*\(", stripped):
            depth += 1
        if re.match(r"endif\s*\(\)", stripped):
            depth -= 1
            if depth == 0:
                in_append = False
                continue
        if depth == 0:
            continue
        # Detect the start of a list(APPEND blusys_framework_sources ...) block.
        if re.match(r"list\s*\(\s*APPEND\s+blusys_framework_sources\b", stripped):
            in_append = True
            # The sources may follow on the same line after the keyword.
            rest = re.sub(r"list\s*\(\s*APPEND\s+blusys_framework_sources\b", "", stripped, count=1)
            rest = rest.split("#", 1)[0].strip().rstrip(")")
            for tok in rest.split():
                tok = tok.strip()
                if tok and tok.startswith("src/app/capabilities/") and tok.endswith(".cpp"):
                    out.append(tok)
            if ")" in stripped.split("#", 1)[0]:
                in_append = False
            continue
        if in_append:
            if ")" in stripped.split("#", 1)[0]:
                in_append = False
                stripped = stripped.split(")", 1)[0]
            entry = stripped.split("#", 1)[0].strip()
            if entry and entry.startswith("src/app/capabilities/") and entry.endswith(".cpp"):
                out.append(entry)
    if not out:
        print("check-host-bridge-spine: missing ESP_PLATFORM capability block", file=sys.stderr)
        sys.exit(1)
    return out


def _parse_bridge_framework_sources(bridge_text: str) -> tuple[list[str], list[str]]:
    m = re.search(
        r"set\(_BLUSYS_HOST_BRIDGE_COMMON_SOURCES\s+(.*?)\n\)",
        bridge_text,
        re.DOTALL,
    )
    if not m:
        print("check-host-bridge-spine: missing _BLUSYS_HOST_BRIDGE_COMMON_SOURCES", file=sys.stderr)
        sys.exit(1)
    fw: list[str] = []
    blusys: list[str] = []
    for line in m.group(1).splitlines():
        line = line.split("#", 1)[0].strip()
        if not line:
            continue
        mfw = re.match(r"\$\{BLUSYS_FRAMEWORK_DIR\}/(.+)$", line)
        if mfw:
            fw.append(mfw.group(1))
            continue
        mbc = re.match(r"\$\{BLUSYS_COMPONENT_DIR\}/(.+)$", line)
        if mbc:
            blusys.append(mbc.group(1))
            continue
        print(f"check-host-bridge-spine: unexpected line in COMMON_SOURCES: {line}", file=sys.stderr)
        sys.exit(1)
    return fw, blusys


def main() -> None:
    repo = Path(__file__).resolve().parents[1]
    fw_cmake = (repo / "components" / "blusys_framework" / "CMakeLists.txt").read_text(encoding="utf-8")
    bridge = (repo / "cmake" / "blusys_host_bridge.cmake").read_text(encoding="utf-8")

    spine = _parse_spine(fw_cmake)
    esp_caps = _parse_esp_capability_cpp(fw_cmake)
    expected_fw: set[str] = set(spine)
    expected_fw.add("src/app/capabilities/telemetry.cpp")
    for dev in esp_caps:
        stem = Path(dev).stem
        expected_fw.add(f"src/app/capabilities/{stem}_host.cpp")

    bridge_fw, bridge_blusys = _parse_bridge_framework_sources(bridge)

    expected_blusys = ["src/error.c"]
    if bridge_blusys != expected_blusys:
        print(
            f"check-host-bridge-spine: Bridge's blusys-component sources ({bridge_blusys!r}) differ "
            f"from expected ({expected_blusys!r}). If you legitimately need to add a C dependency, "
            "update the expected list in this script and ensure the bridge "
            "cmake/blusys_host_bridge.cmake is updated in sync.",
            file=sys.stderr,
        )
        sys.exit(1)

    if set(bridge_fw) != expected_fw:
        only_bridge = sorted(set(bridge_fw) - expected_fw)
        only_expected = sorted(expected_fw - set(bridge_fw))
        print(
            "check-host-bridge-spine: framework COMMON_SOURCES drift vs components/blusys_framework/CMakeLists.txt",
            file=sys.stderr,
        )
        if only_expected:
            print(f"  missing from bridge: {only_expected}", file=sys.stderr)
        if only_bridge:
            print(f"  extra in bridge: {only_bridge}", file=sys.stderr)
        sys.exit(1)

    print("check-host-bridge-spine: OK")


if __name__ == "__main__":
    main()
