#!/usr/bin/env python3
"""Rule 5 — src/ must mirror include/blusys/ (and vice versa).

For every source at src/X/Y/Z.{c,cpp} there must be a corresponding
include/blusys/X/Y/Z.{h,hpp}.

The reverse (every public header has a src/) is enforced for C layers only
(hal/, drivers/, services/).  The framework layer has many legitimate
header-only files (type definitions, templates, umbrella includes) that are
exempt from the reverse direction.

Allowlisted src-only exceptions (no public header counterpart required):
  src/hal/targets/**              — per-MCU capability tables
  src/hal/ulp/**                  — ULP-only sources
  src/hal/internal/lock_freertos.c — internal FreeRTOS lock impl (header: lock.h)
  src/drivers/lcd_panel_ili.c     — panel draw-fn bundle included by lcd.c
  src/framework/**/*_host.cpp     — host-only capability stubs
  src/framework/**/framework_init.cpp
  src/framework/**/app_device_platform.cpp
  src/framework/**/app_headless_platform_esp.cpp

Allowlisted include-only exceptions for C layers (no src/ counterpart):
  drivers/panels/*.h              — data-only panel constant headers
  hal/internal/                   — headers shared among multiple .c files
"""

from __future__ import annotations

import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[1]
BLUSYS_COMP = REPO_ROOT / "components" / "blusys"
SRC_ROOT = BLUSYS_COMP / "src"
INC_ROOT = BLUSYS_COMP / "include" / "blusys"

# Filenames that are src-only (no public header required).
SRC_ONLY_NAME_ALLOWLIST: frozenset[str] = frozenset(
    [
        "lcd_panel_ili.c",
        "lock_freertos.c",
        "framework_init.cpp",
        "app_device_platform.cpp",
        "app_headless_platform_esp.cpp",
        # HAL symmetric pairs whose header is shared with the TX side.
        "i2s_rx.c",    # covered by hal/i2s.h
        "rmt_rx.c",    # covered by hal/rmt.h
        # Internal capability bookkeeping — no standalone public header.
        "capability_event_map.cpp",
    ]
)

# Capability host stubs: *_host.cpp — the public header is capabilities/X.hpp
def _is_capability_host_stub(rel: Path) -> bool:
    return (
        rel.parts[0] == "framework"
        and rel.parts[1] == "capabilities"
        and rel.stem.endswith("_host")
        and rel.suffix == ".cpp"
    )


def _is_src_only_exempt(rel: Path) -> bool:
    """Return True if this src/ file does not require a public header counterpart."""
    parts = rel.parts
    # src/hal/targets/**
    if len(parts) >= 2 and parts[0] == "hal" and parts[1] == "targets":
        return True
    # src/hal/ulp/**
    if len(parts) >= 2 and parts[0] == "hal" and parts[1] == "ulp":
        return True
    # Named exceptions
    if rel.name in SRC_ONLY_NAME_ALLOWLIST:
        return True
    # Framework capability host stubs
    if _is_capability_host_stub(rel):
        return True
    return False


# Directories in include/blusys/ where header-only files are legitimate
# (no src/ counterpart needed).  Framework is fully excluded from the
# reverse check; only C layers (hal, drivers, services) are checked.
def _is_include_c_layer(rel: Path) -> bool:
    """Return True if this header is in a C layer (hal, drivers, services)."""
    return rel.parts[0] in {"hal", "drivers", "services"}


def _is_include_only_exempt(rel: Path) -> bool:
    """Return True if this include/blusys/ file is exempt from needing a src/ counterpart."""
    # Framework headers: header-only is fine (templates, type defs, umbrella includes).
    if rel.parts[0] == "framework":
        return True
    # Root-level umbrella headers.
    if len(rel.parts) == 1 and rel.name in {"blusys.h", "blusys.hpp"}:
        return True
    # drivers/panels/*.h — data-only panel constants, no implementation.
    if (
        rel.parts[0] == "drivers"
        and len(rel.parts) >= 2
        and rel.parts[1] == "panels"
    ):
        return True
    # hal/internal/*.h — shared internal headers included by multiple .c files.
    if rel.parts[0] == "hal" and len(rel.parts) >= 2 and rel.parts[1] == "internal":
        return True
    # hal/log.h — macro-only header wrapping esp_log; no .c counterpart.
    # hal/ulp.h — ULP API header; implementation lives in src/hal/ulp/ subdir.
    if rel.parts[0] == "hal" and rel.name in {"log.h", "ulp.h"}:
        return True
    return False


def _src_to_inc(src_rel: Path) -> list[Path]:
    """Given a src/ relative path, return candidate include/blusys/ paths."""
    stem = src_rel.stem
    parent = src_rel.parent
    if src_rel.suffix == ".c":
        return [parent / (stem + ".h")]
    if src_rel.suffix == ".cpp":
        return [parent / (stem + ".hpp"), parent / (stem + ".h")]
    return []


def _inc_to_src(inc_rel: Path) -> list[Path]:
    """Given an include/blusys/ relative path, return candidate src/ paths."""
    stem = inc_rel.stem
    parent = inc_rel.parent
    if inc_rel.suffix == ".h":
        return [parent / (stem + ".c"), parent / (stem + ".cpp")]
    if inc_rel.suffix == ".hpp":
        return [parent / (stem + ".cpp")]
    return []


def main() -> None:
    errors: list[str] = []

    # --- Check src/ → include/blusys/ (all layers) ---
    if SRC_ROOT.is_dir():
        for src_file in sorted(SRC_ROOT.rglob("*")):
            if not src_file.is_file():
                continue
            if src_file.suffix not in {".c", ".cpp"}:
                continue
            rel = src_file.relative_to(SRC_ROOT)
            if _is_src_only_exempt(rel):
                continue
            candidates = _src_to_inc(rel)
            if not any((INC_ROOT / c).is_file() for c in candidates):
                errors.append(
                    f"src/{rel}  →  missing include/blusys/{rel.parent}/{rel.stem}.{{h,hpp}}"
                )

    # --- Check include/blusys/ → src/ (C layers only) ---
    if INC_ROOT.is_dir():
        for inc_file in sorted(INC_ROOT.rglob("*")):
            if not inc_file.is_file():
                continue
            if inc_file.suffix not in {".h", ".hpp"}:
                continue
            rel = inc_file.relative_to(INC_ROOT)
            if not _is_include_c_layer(rel):
                continue
            if _is_include_only_exempt(rel):
                continue
            candidates = _inc_to_src(rel)
            if not any((SRC_ROOT / c).is_file() for c in candidates):
                errors.append(
                    f"include/blusys/{rel}  →  missing src/{rel.parent}/{rel.stem}.{{c,cpp}}"
                )

    if errors:
        print("check-src-include-mirror: FAIL", file=sys.stderr)
        for e in errors:
            print(f"  {e}", file=sys.stderr)
        sys.exit(1)

    print("check-src-include-mirror: OK")


if __name__ == "__main__":
    main()
