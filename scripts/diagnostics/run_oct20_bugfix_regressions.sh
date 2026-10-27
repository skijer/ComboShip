#!/usr/bin/env bash
# Production entry/decoder/render regressions run before full platform builds.
set -euo pipefail
cd "$(dirname "$0")/../.."
export ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}"

python3 -B tests/custom_messages/run_decoder_tests.py
python3 -B tests/custom_messages/run_cutscene_actor_tests.py
python3 -B scripts/diagnostics/run_forest_gi_particles_tests.py
python3 -B tests/black_market/run_entry_access_tests.py
python3 -B tests/black_market/run_scene_spawn_tests.py
python3 -B tests/black_market/check_exit_door_resources.py --prepared
python3 -B tests/black_market/run_exit_door_actor_tests.py
python3 -B tests/black_market/run_native_tests.py --case door-exit
python3 -B tests/black_market/run_oot_native_tests.py --case door-exit
python3 -B tests/black_market/run_interior_effects_native_tests.py
python3 -B tests/black_market/run_text_codec_tests.py
python3 -B tests/native_market/run_title_tests.py
python3 -B tests/native_market/run_title_draw_tests.py
python3 -B tests/native_market/test_title_asset.py
python3 -B tests/lost_woods_entrance/run_mm_title_card_tests.py --sanitize
python3 -B tests/black_market/run_collision_pool_tests.py
python3 -B tests/black_market/run_resource_entry_tests.py --bundled-only
python3 -B tests/native_market/run_runtime_readiness_tests.py
python3 -B -m unittest discover -s tests/native_market -p 'test_interior.py'
python3 -B -m unittest discover -s tests/native_market -p 'test_platform_overlay.py'
python3 -B tests/native_market/run_legacy_cache_tests.py
python3 -B scripts/diagnostics/run_shield_particle_lifetime_tests.py
python3 -B scripts/diagnostics/run_saria_palette_tests.py
python3 -B -m unittest discover -s tests/native_forest -p 'test_*.py'
