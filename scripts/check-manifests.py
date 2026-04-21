#!/usr/bin/env python3
"""Validate blusys.project.yml manifests against the scaffold catalog."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent / "lib"))
from blusys.manifest_validator import load_catalog, validate_manifest_text  # noqa: E402


def expand_manifest_paths(path: Path) -> list[Path]:
    if path.is_file():
        return [path]
    if path.is_dir():
        return sorted(p for p in path.rglob("blusys.project.yml") if p.is_file())
    raise SystemExit(f"error: path not found: {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo-root", required=True)
    parser.add_argument("--allow-empty", action="store_true")
    parser.add_argument("paths", nargs="+")
    args = parser.parse_args()

    repo_root = Path(args.repo_root)
    catalog = load_catalog(repo_root)

    manifest_paths: list[Path] = []
    missing_paths: list[Path] = []
    for raw in args.paths:
        expanded = expand_manifest_paths(Path(raw))
        if not expanded:
            missing_paths.append(Path(raw))
            continue
        manifest_paths.extend(expanded)

    manifest_paths = list(dict.fromkeys(p.resolve() for p in manifest_paths))
    if missing_paths and not args.allow_empty:
        for missing in missing_paths:
            print(f"error: no blusys.project.yml manifests found under {missing}", file=sys.stderr)

    if not manifest_paths:
        if args.allow_empty:
            return 0
        if missing_paths:
            return 1
        print("error: no blusys.project.yml manifests found", file=sys.stderr)
        return 1

    errors: list[str] = []
    for manifest_path in manifest_paths:
        try:
            text = manifest_path.read_text(encoding="utf-8")
        except OSError as exc:
            errors.append(f"{manifest_path}: {exc}")
            continue

        manifest_errors = validate_manifest_text(text, catalog)
        for error in manifest_errors:
            errors.append(f"{manifest_path}: {error}")

    if errors or (missing_paths and not args.allow_empty):
        for error in errors:
            print(f"error: {error}", file=sys.stderr)
        return 1

    print(f"Validated {len(manifest_paths)} manifest(s).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
