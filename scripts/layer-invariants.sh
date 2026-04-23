#!/usr/bin/env bash
# Layer invariants and refactor guards — same checks as CI job
# "blusys-layer-invariants" (.github/workflows/ci.yml).
#
# Usage: from repo root,
#   bash scripts/layer-invariants.sh
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

bash scripts/lint-layering.sh
python3 scripts/check-src-include-mirror.py
python3 scripts/check-no-blusys-prefix.py
python3 scripts/check-cpp-namespace.py
bash scripts/check-no-service-locator.sh
bash scripts/check-no-platform-ifdef-above-hal.sh
bash scripts/check-public-api.sh
bash scripts/check-capability-shape.sh
bash scripts/check-service-access.sh

echo "layer-invariants: OK"
