#!/usr/bin/env python3
"""Validate inventory.yml against the actual repo state.

Checks:
  - Every examples/ directory has an inventory entry
  - Every inventory example entry points to an existing directory
  - No duplicate example names
  - All required fields present with valid enum values
  - Every module has valid tier, support, and target fields
  - Every doc entry has a valid section and visibility
  - Quickstart examples must be public and ci_pr=true
  - Validation examples must be internal
"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "lib"))
from blusys.inventory_lib import REPO_ROOT, load_inventory  # noqa: E402

# Valid enum values
VALID_TIERS = {"hal", "driver", "service", "framework"}
VALID_SUPPORT = {"supported", "advanced", "internal", "deprecated"}
VALID_TARGETS = {"esp32", "esp32c3", "esp32s3"}
VALID_CATEGORIES = {"quickstart", "reference", "validation"}
VALID_VISIBILITY = {"public", "internal"}
VALID_SECTIONS = {"start", "app", "services", "hal_drivers", "internals"}
VALID_INTERFACES = {"handheld", "surface", "headless", "none"}


def find_example_dirs():
    """Find all example directories (supports both flat and categorized layouts).

    Only directories with a root CMakeLists.txt are treated as example roots.
    This keeps local build outputs such as build-*/ from being mistaken for
    source examples.
    """
    examples_dir = REPO_ROOT / "examples"
    if not examples_dir.exists():
        return set()
    dirs = set()
    for entry in examples_dir.iterdir():
        if not entry.is_dir() or entry.name.startswith("."):
            continue
        # Check if this is a category directory (quickstart/reference/validation)
        if entry.name in VALID_CATEGORIES:
            for sub in entry.iterdir():
                if sub.is_dir() and not sub.name.startswith(".") and (sub / "CMakeLists.txt").is_file():
                    dirs.add(sub.name)
        else:
            # Flat layout (pre-reorganization) — shared headers, not an example app
            if entry.name == "include":
                continue
            if (entry / "CMakeLists.txt").is_file():
                dirs.add(entry.name)
    return dirs


def validate_modules(modules, errors):
    names = set()
    for i, mod in enumerate(modules):
        prefix = f"modules[{i}]"

        name = mod.get("name")
        if not name:
            errors.append(f"{prefix}: missing 'name'")
            continue

        if name in names:
            errors.append(f"{prefix}: duplicate module name '{name}'")
        names.add(name)

        # Tier
        tier = mod.get("tier")
        if tier not in VALID_TIERS:
            errors.append(
                f"{prefix} ({name}): invalid tier '{tier}', expected one of {VALID_TIERS}"
            )

        # Support
        support = mod.get("support")
        if support not in VALID_SUPPORT:
            errors.append(
                f"{prefix} ({name}): invalid support '{support}', expected one of {VALID_SUPPORT}"
            )

        # Targets
        targets = mod.get("targets", [])
        if not targets:
            errors.append(f"{prefix} ({name}): missing or empty 'targets'")
        for t in targets:
            if t not in VALID_TARGETS:
                errors.append(f"{prefix} ({name}): invalid target '{t}'")

        # Doc (optional but recommended for supported modules)
        doc = mod.get("doc")
        if not doc:
            errors.append(f"{prefix} ({name}): missing 'doc'")

        # Examples (optional but recommended for supported modules)
        if "examples" not in mod:
            errors.append(f"{prefix} ({name}): missing 'examples' list")


def validate_examples(examples, errors):
    names = set()
    actual_dirs = find_example_dirs()
    inventory_names = set()

    for i, ex in enumerate(examples):
        prefix = f"examples[{i}]"

        name = ex.get("name")
        if not name:
            errors.append(f"{prefix}: missing 'name'")
            continue

        if name in names:
            errors.append(f"{prefix}: duplicate example name '{name}'")
        names.add(name)
        inventory_names.add(name)

        # Category
        category = ex.get("category")
        if category not in VALID_CATEGORIES:
            errors.append(
                f"{prefix} ({name}): invalid category '{category}', expected one of {VALID_CATEGORIES}"
            )

        # Visibility
        visibility = ex.get("visibility")
        if visibility not in VALID_VISIBILITY:
            errors.append(
                f"{prefix} ({name}): invalid visibility '{visibility}', expected one of {VALID_VISIBILITY}"
            )

        # Tier
        tier = ex.get("tier")
        if tier not in VALID_TIERS:
            errors.append(
                f"{prefix} ({name}): invalid tier '{tier}', expected one of {VALID_TIERS}"
            )

        # Targets
        targets = ex.get("targets", [])
        if not targets:
            errors.append(f"{prefix} ({name}): missing or empty 'targets'")
        for t in targets:
            if t not in VALID_TARGETS:
                errors.append(f"{prefix} ({name}): invalid target '{t}'")

        # CI flags
        if "ci_pr" not in ex:
            errors.append(f"{prefix} ({name}): missing 'ci_pr'")
        if "ci_nightly" not in ex:
            errors.append(f"{prefix} ({name}): missing 'ci_nightly'")

        interface = ex.get("interface")
        if interface is not None and interface not in VALID_INTERFACES:
            errors.append(
                f"{prefix} ({name}): invalid interface '{interface}', expected one of {VALID_INTERFACES}"
            )

        capabilities = ex.get("capabilities")
        if capabilities is not None:
            if not isinstance(capabilities, list) or not all(
                isinstance(c, str) for c in capabilities
            ):
                errors.append(
                    f"{prefix} ({name}): 'capabilities' must be a list of strings"
                )

        policies = ex.get("policies")
        if policies is not None:
            if not isinstance(policies, list) or not all(
                isinstance(p, str) for p in policies
            ):
                errors.append(
                    f"{prefix} ({name}): 'policies' must be a list of strings"
                )

        # Consistency rules
        if category == "quickstart" and visibility != "public":
            errors.append(
                f"{prefix} ({name}): quickstart examples must have visibility=public"
            )
        if category == "quickstart" and not ex.get("ci_pr"):
            errors.append(
                f"{prefix} ({name}): quickstart examples must have ci_pr=true"
            )
        if category == "validation" and visibility != "internal":
            errors.append(
                f"{prefix} ({name}): validation examples must have visibility=internal"
            )

    # Check for orphaned directories (exist on disk but not in inventory)
    orphans = actual_dirs - inventory_names
    for orphan in sorted(orphans):
        errors.append(
            f"orphan example directory '{orphan}' exists on disk but has no inventory entry"
        )

    # Check for missing directories (in inventory but not on disk)
    missing = inventory_names - actual_dirs
    for m in sorted(missing):
        errors.append(f"inventory example '{m}' has no matching directory on disk")


def validate_docs(docs, errors):
    paths = set()

    for i, doc in enumerate(docs):
        prefix = f"docs[{i}]"

        path = doc.get("path")
        if not path:
            errors.append(f"{prefix}: missing 'path'")
            continue

        if path in paths:
            errors.append(f"{prefix}: duplicate doc path '{path}'")
        paths.add(path)

        # Section
        section = doc.get("section")
        if section not in VALID_SECTIONS:
            errors.append(
                f"{prefix} ({path}): invalid section '{section}', expected one of {VALID_SECTIONS}"
            )

        # Visibility
        visibility = doc.get("visibility")
        if visibility not in VALID_VISIBILITY:
            errors.append(
                f"{prefix} ({path}): invalid visibility '{visibility}', expected one of {VALID_VISIBILITY}"
            )


def main():
    inventory = load_inventory()
    errors = []

    # Validate modules
    modules = inventory.get("modules", [])
    if not modules:
        errors.append("no modules defined in inventory")
    else:
        validate_modules(modules, errors)

    # Validate examples
    examples = inventory.get("examples", [])
    if not examples:
        errors.append("no examples defined in inventory")
    else:
        validate_examples(examples, errors)

    # Validate docs
    docs = inventory.get("docs", [])
    if not docs:
        errors.append("no docs defined in inventory")
    else:
        validate_docs(docs, errors)

    # Report
    if errors:
        print(f"INVENTORY CHECK FAILED ({len(errors)} error(s)):\n")
        for err in errors:
            print(f"  - {err}")
        sys.exit(1)
    else:
        mod_count = len(modules)
        ex_count = len(examples)
        doc_count = len(docs)
        pr_count = sum(1 for e in examples if e.get("ci_pr"))
        print(f"INVENTORY CHECK PASSED")
        print(f"  modules:  {mod_count}")
        print(f"  examples: {ex_count} ({pr_count} in PR CI)")
        print(f"  docs:     {doc_count}")
        sys.exit(0)


if __name__ == "__main__":
    main()
