"""Shared helpers for inventory.yml validators.

Centralises YAML loading and example-directory resolution so individual
checkers stay focused on their own rules.
"""

from __future__ import annotations

import sys
from pathlib import Path

try:
    import yaml
except ImportError:
    print("ERROR: PyYAML is required. Install with: pip install pyyaml")
    sys.exit(2)

REPO_ROOT = Path(__file__).resolve().parents[3]
CATEGORIES = ("quickstart", "reference", "validation")


def load_inventory(repo_root: Path = REPO_ROOT) -> dict:
    path = repo_root / "inventory.yml"
    if not path.exists():
        print(f"ERROR: {path} not found")
        sys.exit(1)
    with open(path, encoding="utf-8") as f:
        return yaml.safe_load(f)


def find_example_dir(name: str, repo_root: Path = REPO_ROOT) -> Path | None:
    """Return the directory for an example, searching categorised then flat layouts."""
    for cat in CATEGORIES:
        d = repo_root / "examples" / cat / name
        if d.is_dir():
            return d
    d = repo_root / "examples" / name
    return d if d.is_dir() else None
