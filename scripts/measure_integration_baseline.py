#!/usr/bin/env python3
"""Print platform thickness metrics for V2 roadmap baselines."""

from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]

EXAMPLES = [
    "examples/reference/connected_headless/main/platform",
    "examples/reference/connected_device/main/platform",
    "examples/quickstart/handheld_starter/main/platform",
]


def git_short_hash() -> str:
    try:
        return subprocess.check_output(
            ["git", "rev-parse", "--short", "HEAD"],
            cwd=REPO_ROOT,
            text=True,
        ).strip()
    except (subprocess.CalledProcessError, FileNotFoundError):
        return "unknown"


def count_loc_py(path: Path) -> int:
    if not path.exists():
        return -1
    total = 0
    for p in path.rglob("*"):
        if p.suffix in {".cpp", ".hpp", ".h", ".c"} and p.is_file():
            total += sum(1 for _ in p.open("r", encoding="utf-8", errors="replace"))
    return total


def count_map_branches(text: str) -> tuple[int, int]:
    """Rough counts of map_event / map_intent branch keywords inside function bodies."""
    mi = len(re.findall(r"\bmap_intent\b", text))
    me = len(re.findall(r"\bmap_event\b", text))
    cases = len(re.findall(r"\bcase\b", text))
    ifs = len(re.findall(r"\bif\s*\(", text))
    return cases + ifs, mi + me


def analyze_platform_dir(platform_dir: Path) -> None:
    rel = platform_dir.relative_to(REPO_ROOT)
    loc = count_loc_py(platform_dir)
    chunks: list[str] = []
    if platform_dir.exists():
        for p in sorted(platform_dir.rglob("*")):
            if p.suffix in {".cpp", ".hpp"} and p.is_file():
                chunks.append(p.read_text(encoding="utf-8", errors="replace"))
    combined = "\n".join(chunks)
    branches, map_refs = count_map_branches(combined)

    print(f"## {rel}")
    print(f"  platform LOC (.c/.cpp/.h/.hpp): {loc if loc >= 0 else 'missing'}")
    print(f"  heuristic branch weight (case+if): {branches}")
    print(f"  map_intent/map_event references: {map_refs}")
    print()


def main() -> int:
    print(f"git HEAD: {git_short_hash()}")
    print()
    for ex in EXAMPLES:
        analyze_platform_dir(REPO_ROOT / ex)
    return 0


if __name__ == "__main__":
    sys.exit(main())
