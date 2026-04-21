#!/usr/bin/env python3
"""Fail if tracked files under build/generated are committed."""

from __future__ import annotations

import subprocess
import sys
from pathlib import Path


def main() -> int:
    repo_root = Path(__file__).resolve().parent.parent

    try:
        result = subprocess.run(
            ["git", "-C", str(repo_root), "ls-files", "-z"],
            check=True,
            capture_output=True,
            text=True,
        )
    except FileNotFoundError:
        print("error: git is required to check generated artifacts", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError as exc:
        stderr = exc.stderr.strip()
        if stderr:
            print(f"error: git ls-files failed: {stderr}", file=sys.stderr)
        else:
            print("error: git ls-files failed", file=sys.stderr)
        return 1

    tracked = [path for path in result.stdout.split("\0") if path]
    offenders = sorted(
        path for path in tracked if "/build/generated/" in path or path.startswith("build/generated/")
    )

    print("GENERATED ARTIFACT CHECK")
    print(f"  tracked files scanned: {len(tracked)}")
    print(f"  generated paths tracked: {len(offenders)}")

    if offenders:
        print("\nERRORS:")
        for path in offenders:
            print(f"  - {path}")
        return 1

    print("\nGenerated artifact check passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
