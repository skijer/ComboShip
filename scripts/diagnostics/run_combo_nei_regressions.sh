#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
# Commands live in the shared CI manifest so local and hosted coverage agree.
python3 -B scripts/ci/run_regressions.py --suite all --source nei
