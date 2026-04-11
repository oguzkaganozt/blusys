#!/usr/bin/env python3
"""Validate canonical main/core, main/ui, main/integration layout for product-shaped examples.

Product-shaped: under main/, at least one source file references blusys::app entry (detected via
``blusys/app/app.hpp`` include or BLUSYS_APP_MAIN*), unless the example opts in with
``product_layout: true`` in inventory.yml.

Skipped:
  - category == validation (HAL/service smoke) unless product_layout: true
  - layout_exempt: true
  - no product signal and product_layout not true

CI: run after check-inventory.py (requires PyYAML).
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    print("ERROR: PyYAML is required. Install with: pip install pyyaml")
    sys.exit(2)

REPO_ROOT = Path(__file__).resolve().parent.parent
CATEGORIES = {"quickstart", "reference", "validation"}

PRODUCT_INCLUDE = re.compile(r'#\s*include\s*[<"]blusys/app/app\.hpp[>"]')
PRODUCT_ENTRY = re.compile(r"\bBLUSYS_APP_MAIN_")
MAIN_ROOT_SOURCES = re.compile(r"\.(c|cpp|cc|cxx)$", re.IGNORECASE)


def load_inventory():
    path = REPO_ROOT / "inventory.yml"
    with open(path, encoding="utf-8") as f:
        return yaml.safe_load(f)


def find_example_dir(name: str) -> Path | None:
    for cat in CATEGORIES:
        d = REPO_ROOT / "examples" / cat / name
        if d.is_dir():
            return d
    d = REPO_ROOT / "examples" / name
    if d.is_dir():
        return d
    return None


def iter_main_sources(main_dir: Path):
    if not main_dir.is_dir():
        return
    for p in main_dir.rglob("*"):
        if p.is_file() and p.suffix.lower() in (
            ".c",
            ".cpp",
            ".cc",
            ".cxx",
            ".h",
            ".hpp",
        ):
            yield p


def file_has_product_signal(text: str) -> bool:
    return bool(PRODUCT_INCLUDE.search(text) or PRODUCT_ENTRY.search(text))


def main_has_product_signal(main_dir: Path) -> bool:
    for p in iter_main_sources(main_dir):
        try:
            txt = p.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        if file_has_product_signal(txt):
            return True
    return False


def main_has_root_sources(main_dir: Path) -> list[Path]:
    bad: list[Path] = []
    for p in main_dir.iterdir():
        if p.is_file() and MAIN_ROOT_SOURCES.search(p.name):
            bad.append(p)
    return sorted(bad)


def check_layout(main_dir: Path) -> tuple[list[str], list[str]]:
    """Returns (errors, warnings)."""
    errors: list[str] = []
    warnings: list[str] = []

    for sub in ("core", "ui", "integration"):
        p = main_dir / sub
        if not p.is_dir():
            errors.append(f"missing directory main/{sub}/")

    entry = main_dir / "integration" / "app_main.cpp"
    if not entry.is_file():
        errors.append("missing main/integration/app_main.cpp")

    root_src = main_has_root_sources(main_dir)
    if root_src:
        names = ", ".join(x.name for x in root_src)
        warnings.append(f"sources directly under main/ (move into core/ui/integration): {names}")

    return errors, warnings


def main():
    inventory = load_inventory()
    examples = inventory.get("examples", [])
    all_errors: list[str] = []
    all_warnings: list[str] = []

    for ex in examples:
        name = ex.get("name", "")
        category = ex.get("category", "")
        layout_exempt = ex.get("layout_exempt", False)
        force_layout = ex.get("product_layout", False)

        example_dir = find_example_dir(name)
        if example_dir is None:
            continue

        main_dir = example_dir / "main"

        if layout_exempt:
            continue

        if category == "validation" and not force_layout:
            continue

        has_signal = main_has_product_signal(main_dir) or force_layout
        if not has_signal:
            continue

        errs, warns = check_layout(main_dir)
        prefix = f"{name}: "
        for w in warns:
            all_warnings.append(prefix + w)
        for e in errs:
            all_errors.append(prefix + e)

    print("PRODUCT LAYOUT CHECK")
    print(f"  examples scanned: {len(examples)}")
    print(f"  errors:   {len(all_errors)}")
    print(f"  warnings: {len(all_warnings)}")

    if all_warnings:
        print("\nWARNINGS:")
        for w in all_warnings:
            print(f"  - {w}")

    if all_errors:
        print("\nERRORS:")
        for e in all_errors:
            print(f"  - {e}")
        sys.exit(1)

    print("\nProduct layout check passed.")
    sys.exit(0)


if __name__ == "__main__":
    main()
