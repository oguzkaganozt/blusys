#!/usr/bin/env bash
# The former interactive_dashboard example was removed. Use a supported tree + `blusys qemu`, e.g.:
#   blusys qemu examples/quickstart/handheld_starter qemu32s3 --graphics
set -euo pipefail
printf '%s\n' "error: interactive_dashboard was removed; see scripts/run-interactive-dashboard-qemu.sh header for alternatives." >&2
exit 2
