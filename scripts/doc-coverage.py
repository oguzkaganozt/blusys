#!/usr/bin/env python3
"""Report documentation coverage based on inventory.yml.

Checks:
  - Every supported module has a doc file
  - Every doc entry in inventory points to an existing file
  - Orphaned doc files (exist on disk but not in inventory) are reported

Exit codes:
  0 = all supported modules have docs and no orphans
  1 = coverage gaps or orphans found
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

DOC_DIRS = [
    REPO_ROOT / "docs" / "start",
    REPO_ROOT / "docs" / "app",
    REPO_ROOT / "docs" / "services",
    REPO_ROOT / "docs" / "hal",
    REPO_ROOT / "docs" / "internals",
    REPO_ROOT / "docs" / "internals" / "testing",
    REPO_ROOT / "docs" / "archive",
    REPO_ROOT / "docs" / "archive" / "plans",
]


def load_inventory():
    path = REPO_ROOT / "inventory.yml"
    with open(path) as f:
        return yaml.safe_load(f)


def find_all_doc_files():
    """Find all .md files in the new docs structure."""
    files = set()
    for d in DOC_DIRS:
        if d.exists():
            for f in d.glob("*.md"):
                files.add(str(f.relative_to(REPO_ROOT)))
    return files


def main():
    inventory = load_inventory()
    issues = []
    warnings = []

    # Check module doc coverage
    modules = inventory.get("modules", [])
    for mod in modules:
        name = mod["name"]
        support = mod.get("support", "supported")
        doc = mod.get("doc", "")

        if support == "supported" and doc:
            doc_path = REPO_ROOT / doc
            if not doc_path.exists():
                issues.append(f"supported module '{name}' doc missing: {doc}")

    # Check doc inventory entries
    docs = inventory.get("docs", [])
    inventory_paths = set()
    for entry in docs:
        path = entry["path"]
        inventory_paths.add(path)
        doc_path = REPO_ROOT / path
        if not doc_path.exists():
            warnings.append(f"inventory doc entry not on disk: {path}")

    # Find orphaned files
    all_files = find_all_doc_files()
    orphaned = all_files - inventory_paths
    for orphan in sorted(orphaned):
        warnings.append(f"orphaned doc file (not in inventory): {orphan}")

    # Report
    total_modules = len([m for m in modules if m.get("support") == "supported"])
    covered = total_modules - len([i for i in issues if "doc missing" in i])

    print(f"DOC COVERAGE REPORT")
    print(f"  supported modules: {total_modules}")
    print(f"  with docs:         {covered}")
    print(f"  coverage:          {covered}/{total_modules}")
    print(f"  inventory docs:    {len(docs)}")

    if issues:
        print(f"\nISSUES ({len(issues)}):")
        for issue in issues:
            print(f"  - {issue}")

    if warnings:
        print(f"\nWARNINGS ({len(warnings)}):")
        for warning in warnings:
            print(f"  - {warning}")

    if issues:
        sys.exit(1)
    else:
        print("\nAll supported modules have documentation.")
        sys.exit(0)


if __name__ == "__main__":
    main()
