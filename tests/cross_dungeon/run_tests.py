#!/usr/bin/env python3
import argparse
import pathlib
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
root = pathlib.Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='cross-dungeon-') as tmp:
    for source in sorted((root / 'tests/cross_dungeon').glob('*_test.cpp')):
        binary = pathlib.Path(tmp) / source.stem
        cmd = ['g++', '-std=c++20', '-Wall', '-Wextra', '-Werror', '-I', str(root), '-I', str(root / 'combo'),
               '-I', str(root.parent / 'build-deps/usr/include'), str(source), '-o', str(binary)]
        if args.sanitize:
            cmd += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g']
        subprocess.run(cmd, check=True)
        subprocess.run([str(binary)], check=True)

# Execute the production host adapters, transition bodies and recovery boundaries.
for name in ['mm_boss_warp_behavior_test.py', 'cross_boss_host_behavior_test.py',
             'oot_cross_boss_behavior_test.py', 'oot_cross_boss_world_behavior_test.py',
             'mm_cross_boss_behavior_test.py', 'mm_catalogue_behavior_test.py',
             'mm_recovery_behavior_test.py', 'mm_adapter_behavior_test.py',
             'boss_launcher_behavior_test.py', 'oot_catalogue_behavior_test.py',
             'oot_pool_runtime_behavior_test.py', 'oot_exit_scope_behavior_test.py',
             'mm_boss_escape_logic_behavior_test.py', 'mm_arrival_time_behavior_test.py',
             'oot_cross_boss_spawn_behavior_test.py',
             'oot_blue_warp_hook_test.py', 'oot_cold_blue_warp_behavior_test.py',
             'oot_entrance_metadata_test.py', 'generation_reset_behavior_test.py',
             'transition_fade_behavior_test.py', 'interior_settings_behavior_test.py',
             'boss_recovery_behavior_test.py', 'shop_catalogue_behavior_test.py',
             'shop_native_pool_behavior_test.py', 'shop_return_behavior_test.py',
             'shop_cli_options_behavior_test.py']:
    subprocess.run([sys.executable, str(root / 'tests/cross_dungeon' / name)] +
                   (['--sanitize'] if args.sanitize else []), check=True)
subprocess.run([sys.executable, str(root / 'tests/cross_dungeon/affordability_behavior_test.py')], check=True)
