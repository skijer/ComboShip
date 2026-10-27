#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"
python3 -B scripts/diagnostics/run_sword_element_state_tests.py --combat --sanitize
python3 -B scripts/diagnostics/run_sword_element_save_tests.py --sanitize
python3 -B scripts/diagnostics/run_sword_element_hud_tests.py --sanitize
python3 -B scripts/diagnostics/sword_element_kaleido_tests.py
python3 -B scripts/assets/build_sword_element_ui.py --check
python3 -B scripts/assets/build_elemental_sword_aura.py --check
python3 -B scripts/diagnostics/run_elemental_sword_aura_tests.py --sanitize
python3 -B scripts/diagnostics/run_biggoron_aura_tests.py --sanitize
python3 -B scripts/diagnostics/run_din_fire_sword_tests.py
python3 -B scripts/diagnostics/run_din_fire_sword_damage_tests.py
python3 -B scripts/diagnostics/run_mm_din_fire_tests.py
python3 -B scripts/diagnostics/run_mm_din_fire_damage_tests.py
python3 -B scripts/diagnostics/run_sword_element_finish_tests.py --sanitize
python3 -B scripts/diagnostics/run_mm_ice_arrows_tests.py --sanitize
python3 -B scripts/diagnostics/run_mm_ice_octorok_tests.py --sanitize
python3 -B tests/item_receipts/run_sword_element_receipt_tests.py --sanitize
