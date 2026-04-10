#!/usr/bin/env python3
"""Validate example health based on inventory.yml.

Checks:
  - Every example directory has CMakeLists.txt and main/ subdirectory
  - Every quickstart and reference example has a corresponding doc page
  - Example directory structure matches the expected taxonomy layout

Exit codes:
  0 = all checks pass
  1 = one or more checks fail
"""

import os
import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    print("ERROR: PyYAML is required. Install with: pip install pyyaml")
    sys.exit(2)

REPO_ROOT = Path(__file__).resolve().parent.parent
CATEGORIES = {"quickstart", "reference", "validation"}


def load_inventory():
    path = REPO_ROOT / "inventory.yml"
    with open(path) as f:
        return yaml.safe_load(f)


def find_example_dir(name):
    """Find an example directory by name (supports flat and categorized)."""
    for cat in CATEGORIES:
        d = REPO_ROOT / "examples" / cat / name
        if d.is_dir():
            return d
    d = REPO_ROOT / "examples" / name
    if d.is_dir():
        return d
    return None


def main():
    inventory = load_inventory()
    errors = []
    warnings = []

    examples = inventory.get("examples", [])
    modules = inventory.get("modules", [])

    # Build a set of module docs for cross-reference
    module_docs = {}
    for mod in modules:
        for ex in mod.get("examples", []):
            module_docs[ex] = mod.get("doc", "")

    for ex in examples:
        name = ex["name"]
        category = ex.get("category", "")
        visibility = ex.get("visibility", "")

        # Find the directory
        example_dir = find_example_dir(name)
        if not example_dir:
            errors.append(f"'{name}': directory not found")
            continue

        # Check CMakeLists.txt
        if not (example_dir / "CMakeLists.txt").exists():
            errors.append(f"'{name}': missing CMakeLists.txt")

        # Check main/ directory
        if not (example_dir / "main").is_dir():
            errors.append(f"'{name}': missing main/ directory")

        # Check that quickstart and reference examples have docs
        if category in ("quickstart", "reference"):
            doc = module_docs.get(name, "")
            if doc:
                doc_path = REPO_ROOT / doc
                if not doc_path.exists():
                    warnings.append(f"'{name}' ({category}): module doc missing: {doc}")

        # Check that the example lives under the correct category directory
        expected_parent = REPO_ROOT / "examples" / category / name
        if expected_parent.is_dir():
            pass  # Correct
        elif (REPO_ROOT / "examples" / name).is_dir():
            warnings.append(f"'{name}': in flat layout, expected under examples/{category}/")

    # Check for directories not in inventory
    for cat_dir in (REPO_ROOT / "examples").iterdir():
        if not cat_dir.is_dir() or cat_dir.name.startswith("."):
            continue
        if cat_dir.name in CATEGORIES:
            for sub in cat_dir.iterdir():
                if sub.is_dir() and not sub.name.startswith("."):
                    inv_names = {e["name"] for e in examples}
                    if sub.name not in inv_names:
                        errors.append(f"'{sub.name}': exists on disk under {cat_dir.name}/ but not in inventory")

    # Report
    total = len(examples)
    print(f"EXAMPLE HEALTH REPORT")
    print(f"  total examples: {total}")
    print(f"  quickstart:     {sum(1 for e in examples if e.get('category') == 'quickstart')}")
    print(f"  reference:      {sum(1 for e in examples if e.get('category') == 'reference')}")
    print(f"  validation:     {sum(1 for e in examples if e.get('category') == 'validation')}")

    if errors:
        print(f"\nERRORS ({len(errors)}):")
        for err in errors:
            print(f"  - {err}")

    if warnings:
        print(f"\nWARNINGS ({len(warnings)}):")
        for warning in warnings:
            print(f"  - {warning}")

    if errors:
        sys.exit(1)
    else:
        print("\nAll examples are healthy.")
        sys.exit(0)


if __name__ == "__main__":
    main()
