#!/usr/bin/env python3
"""Rule 7 — Every framework .hpp/.cpp must be wrapped in namespace blusys.

Verifies that every .hpp and .cpp under:
  components/blusys/src/framework/
  components/blusys/include/blusys/framework/

contains at least one top-level namespace blusys declaration of the form:
  namespace blusys {
  namespace blusys::sub {

Allowlisted exceptions (header-only type/macro files with no namespace body):
  framework/app/entry.hpp          — macro-only entry point
  framework/ui/widgets.hpp         — umbrella include
  framework/ui/primitives.hpp      — umbrella include
  framework/ui/screens/screens.hpp — umbrella include
  framework/capabilities/capabilities.hpp — umbrella include
  framework/flows/flows.hpp        — umbrella include
  framework/framework.hpp          — umbrella include
  framework/services/{net,session,storage,system}.h — umbrella forwards
                                    to sibling public services APIs
  framework/platform/platform.hpp  — platform umbrella include
  framework/test/fake_hal.h        — C inject API (extern "C")
  framework/test/test.hpp          — test-surface umbrella
"""

from __future__ import annotations

import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
BLUSYS_COMP = REPO_ROOT / "components" / "blusys"

SEARCH_DIRS = [
    BLUSYS_COMP / "src" / "framework",
    BLUSYS_COMP / "include" / "blusys" / "framework",
]

# Files that legitimately have no namespace blusys body
# (relative to the framework root within their respective search dir).
ALLOWLIST_NAMES: frozenset[str] = frozenset(
    [
        "entry.hpp",           # macro-only file
        "app.hpp",             # umbrella include
        "view.hpp",            # umbrella include (ui/composition/view.hpp)
        "widgets.hpp",         # umbrella include
        "primitives.hpp",      # umbrella include
        "screens.hpp",         # umbrella include (inside screens/)
        "capabilities.hpp",    # umbrella include (inside capabilities/)
        "flows.hpp",           # umbrella include (inside flows/)
        "framework.hpp",       # top-level umbrella include
        "inline.hpp",          # capabilities/inline.hpp — macro-only inline defs
        "dashboard_display_dims.hpp",  # platform/ — preprocessor-only dimension macros
        "callbacks.hpp",       # type aliases only (may or may not have namespace)
        "framework_init.cpp",  # placeholder translation unit (no code body)
        # Framework umbrella includes (forward to sibling public service APIs).
        "net.h",               # framework/services/net.h
        "session.h",           # framework/services/session.h
        "storage.h",           # framework/services/storage.h
        "system.h",            # framework/services/system.h
        "platform.hpp",        # framework/platform/platform.hpp
        # Test-surface headers: pure C inject API (extern "C"), or umbrella.
        "fake_hal.h",          # framework/test/fake_hal.h — C inject API
        "test.hpp",            # framework/test/test.hpp — umbrella
    ]
)

# Matches:  namespace blusys {
#           namespace blusys::foo {
#           namespace blusys::foo::bar {
_NS_PATTERN = re.compile(r"\bnamespace\s+blusys\s*(?:::[A-Za-z_][A-Za-z0-9_:]*\s*)?\{")


def _has_blusys_namespace(text: str) -> bool:
    return bool(_NS_PATTERN.search(text))


def main() -> None:
    violations: list[str] = []

    for search_dir in SEARCH_DIRS:
        if not search_dir.is_dir():
            continue
        for f in sorted(search_dir.rglob("*")):
            if not f.is_file():
                continue
            if f.suffix not in {".hpp", ".cpp", ".h"}:
                continue
            if f.name in ALLOWLIST_NAMES:
                continue
            text = f.read_text(encoding="utf-8", errors="replace")
            if not _has_blusys_namespace(text):
                # Determine a clean relative path for reporting.
                try:
                    rel = f.relative_to(BLUSYS_COMP)
                except ValueError:
                    rel = f
                violations.append(str(rel))

    if violations:
        print("check-cpp-namespace: FAIL", file=sys.stderr)
        print(
            "  The following files are missing a 'namespace blusys { ... }' declaration:",
            file=sys.stderr,
        )
        for v in violations:
            print(f"  {v}", file=sys.stderr)
        sys.exit(1)

    print("check-cpp-namespace: OK")


if __name__ == "__main__":
    main()
