#!/usr/bin/env python3
"""Rule 6 — No blusys_ prefix in file basenames under include/blusys/.

The include path already carries the blusys namespace (one blusys/ at the
top of every include path). File names inside must never repeat the prefix.

Flags any file whose basename starts with 'blusys_'.
"""

from __future__ import annotations

import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
INC_ROOT = REPO_ROOT / "components" / "blusys" / "include" / "blusys"


def main() -> None:
    violations: list[str] = []

    if INC_ROOT.is_dir():
        for f in sorted(INC_ROOT.rglob("*")):
            if not f.is_file():
                continue
            if f.suffix not in {".h", ".hpp"}:
                continue
            if f.name.startswith("blusys_"):
                rel = f.relative_to(INC_ROOT)
                violations.append(f"include/blusys/{rel}")

    if violations:
        print("check-no-blusys-prefix: FAIL", file=sys.stderr)
        print(
            "  The following headers use a 'blusys_' basename prefix.\n"
            "  Drop the prefix — the include path already provides the namespace.",
            file=sys.stderr,
        )
        for v in violations:
            print(f"  {v}", file=sys.stderr)
        sys.exit(1)

    print("check-no-blusys-prefix: OK")


if __name__ == "__main__":
    main()
