#!/usr/bin/env python3
"""Check MM wand pose handoff and Meteor motion using native structs and bodies.

Animation decoding, world collision queries and actor allocation are boundaries.
The item driver, upper-body handoff, bomb movement and hop wrapper execute unchanged.
"""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_nei_tests import flags
from run_tests import functions, bodies

def read(path):
    return (ROOT / path).read_text()

def strip_includes(text):
    return re.sub(r'^#include[^\n]*\n', '', text, flags=re.M)

def main():
    player_path = 'mm/src/overlays/actors/ovl_player_actor/z_player.c'
    player = read(player_path)
    wand = strip_includes(read('mm/mods/items/logic/item_elemental_wand.c'))
    wand = wand.replace(functions(wand)['Wand_Draw'], '')
    wand = re.sub(r'\bthis\b', 'self', wand)
    box = re.search(r'typedef struct \{\s*const char\* iconPath;.*?} BoxMenuEntry;',
                    read('mm/mods/items/helpers/box_menu.c'), re.S)[0]
    copy_map = re.search(r'u8 sPlayerUpperBodyLimbCopyMap\[PLAYER_LIMB_MAX\] = \{.*?\};', player, re.S)[0]
    pose = box + '\ntypedef void (*BoxMenuConfirmFn)(s32);\n' + read('tests/wand_modes/cast_pose_test.cpp')
    upper = bodies(player_path, ['Player_UpdateUpperBody'])
    upper = re.sub(r'&gPlayerAnim_(\w+)', r'(PlayerAnimationHeader*)gPlayerAnim_\1', upper)
    pose = pose.replace('// PRODUCTION_BODIES', copy_map + '\n' + wand + '\n' + upper)

    meteor_path = 'mm/mods/items/logic/wand/wand_meteor.c'
    meteor = strip_includes(read(meteor_path))
    meteor = meteor.replace(functions(meteor)['WandMeteor_TintDraw'], '')
    motion = bodies('mm/src/code/z_actor.c', ['Actor_UpdatePos', 'Actor_UpdateVelocityWithGravity',
                                             'Actor_MoveWithGravity', 'Actor_SetScale'])
    motion += '\n' + bodies('mm/src/overlays/actors/ovl_En_Bom/z_en_bom.c', ['EnBom_Move'])
    motion += '\n' + bodies('mm/src/code/z_lib.c', ['Math_StepToF'])
    projectile = read('tests/wand_modes/meteor_motion_test.cpp').replace('// PRODUCTION_BODIES', motion + '\n' + meteor)
    failures = 0
    with tempfile.TemporaryDirectory(prefix='mm-wand-cast-') as td:
        for name, source in [('cast-pose', pose), ('meteor-motion', projectile)]:
            path = Path(td) / (name + '.cpp')
            path.write_text(source)
            binary = Path(td) / name
            command = ['c++', '-std=c++20', '-fpermissive', '-O1', '-g', '-fsanitize=undefined',
                       '-fno-sanitize-recover=all', *flags(), '-ffunction-sections',
                       '-fdata-sections', str(path), '-Wl,--gc-sections', '-o', str(binary)]
            result = subprocess.run(command, capture_output=True, text=True)
            if result.returncode:
                print(result.stderr)
                return result.returncode
            result = subprocess.run([str(binary)], capture_output=True, text=True)
            print(result.stdout + result.stderr, end='')
            failures += result.returncode != 0
    return 1 if failures else 0

if __name__ == '__main__':
    raise SystemExit(main())
