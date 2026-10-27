#!/usr/bin/env python3
"""Execute wand modes against native MM headers and production function bodies.

The fixture replaces resource/archive, heap and presentation boundaries. Native
spawn/init/kill, mode dispatch, input/magic gates and each tested mode stay real.
"""
import re
import argparse
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE_REF = None
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_mm_nei_tests import flags


def source(path):
    if SOURCE_REF:
        return subprocess.check_output(['git', 'show', SOURCE_REF + ':' + path], cwd=ROOT, text=True)
    return (ROOT / path).read_text()


def functions(text, names=None):
    # Native actor spawn contains an old commented-out opening brace. Ignore
    # comments and string literals for balancing, while retaining original code.
    masked = re.sub(r'/\*.*?\*/|//[^\n]*|"(?:\\.|[^"\\])*"',
        lambda match: re.sub(r'[^\n]', ' ', match[0]), text, flags=re.S)
    pattern = r'^(?:static[ \t]+)?[A-Za-z_][\w *]*[ \t]+(\w+)\s*\([^;{}]*\)\s*\{'
    found = {}
    for match in re.finditer(pattern, masked, re.M):
        if names is not None and match[1] not in names:
            continue
        end, depth = match.end(), 1
        while depth:
            depth += (masked[end] == '{') - (masked[end] == '}')
            end += 1
        found[match[1]] = text[match.start():end]
    return found


def bodies(path, names):
    found = functions(source(path), names)
    return '\n'.join(re.sub(r'\bthis\b', 'self', found[name]) for name in names)


def declaration(path, start):
    text = source(path)
    begin = text.index(start)
    end = begin + re.search(r'\n}[^;\n]*;', text[begin:]).end()
    return text[begin:end]


def main():
    global SOURCE_REF
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--case', choices=['native-storm', 'native-water', 'shadow', 'modes', 'oot-sand', 'behavior', 'sand-parity'])
    parser.add_argument('--check', choices=['sand', 'held-sand', 'water', 'storm', 'destroy', 'sand-path', 'storm-hits'],
                        help='run one focused modes or behavior acceptance check')
    parser.add_argument('--sanitize', action='store_true', help='enable AddressSanitizer and UndefinedBehaviorSanitizer')
    parser.add_argument('--source-ref', help='run the same expectations against an earlier source revision')
    args = parser.parse_args()
    if args.check:
        check_case = 'behavior' if args.check in ('sand-path', 'storm-hits') else 'modes'
        if args.case and args.case != check_case:
            parser.error('--check ' + args.check + ' requires --case ' + check_case)
        args.case = check_case
    SOURCE_REF = args.source_ref
    actor = 'mm/src/code/z_actor.c'
    storm = 'mm/src/overlays/actors/ovl_En_Okarina_Effect/z_en_okarina_effect.c'
    storm_functions = functions(source(storm))
    storm_body = '\n'.join(body[:body.index('{')] + ';' for body in storm_functions.values())
    storm_body += '\n' + re.search(r'ActorProfile En_Okarina_Effect_Profile = \{.*?\};', source(storm), re.S)[0]
    storm_body += '\n' + '\n'.join(storm_functions.values())
    storm_body = re.sub(r'\bthis\b', 'self', storm_body)

    production = bodies(actor, ['Actor_Kill', 'Actor_SetWorldToHome', 'Actor_SetShapeRotToWorld',
        'Actor_SetFocus', 'Actor_SetScale', 'Actor_SetObjectDependency', 'Actor_Init', 'Actor_AddToCategory',
        'Actor_SpawnAsChildAndCutscene', 'Actor_Spawn'])
    production += '\n' + bodies('mm/src/code/z_scene.c', ['Object_GetSlot', 'Object_IsLoaded'])
    production += '\n' + storm_body

    water_actor = 'mm/src/overlays/actors/ovl_Obj_Hunsui/z_obj_hunsui.c'
    water_text = source(water_actor)
    water_native = bodies(actor, ['Actor_Kill', 'Actor_Destroy', 'Actor_SetWorldToHome', 'Actor_SetShapeRotToWorld',
        'Actor_SetFocus', 'Actor_SetScale', 'Actor_SetObjectDependency', 'Actor_Init', 'Actor_AddToCategory',
        'Actor_SpawnAsChildAndCutscene', 'Actor_Spawn', 'Flags_GetSwitch', 'Flags_SetSwitch', 'Flags_UnsetSwitch'])
    water_native += '\n' + bodies('mm/src/code/z_scene.c', ['Object_GetSlot', 'Object_IsLoaded'])
    water_native += '\n' + bodies('mm/src/code/z_bg_item.c', ['DynaPolyActor_Init', 'DynaPolyActor_LoadMesh'])
    water_native += '\n' + bodies('mm/src/code/z_bgcheck.c', ['BgActor_SetActor', 'DynaPoly_IsBgIdBgActor',
        'DynaPoly_SetBgActor', 'DynaPoly_GetActor', 'DynaPoly_DeleteBgActor'])
    chain_names = [name for name in functions(source('mm/src/code/z_lib.c')) if name.startswith('IChain_Apply_')]
    water_native += '\n' + bodies('mm/src/code/z_lib.c', chain_names)
    water_native += '\n' + re.search(r'void \(\*sInitChainHandlers\[\]\).*?\n};', source('mm/src/code/z_lib.c'), re.S)[0]
    water_native += '\n' + bodies('mm/src/code/z_lib.c', ['Actor_ProcessInitChain'])
    water_native += '\n' + bodies('mm/src/code/z_sub_s.c', ['SubS_FillCutscenesList'])
    water_native += '\n' + bodies('mm/src/code/graph.c', ['Graph_OpenDisps', 'Graph_CloseDisps'])
    water_native += '\n' + water_text[water_text.index('AnimatedMaterial* D_80B9DED0;'):
                                     water_text.index('s32 func_80B9C450(')]
    water_native += '\n' + bodies(water_actor, ['func_80B9C450', 'ObjHunsui_Init', 'ObjHunsui_Destroy',
        'ObjHunsui_Draw', 'func_80B9DA60', 'ObjHunsui_Reset'])
    water_mode = re.sub(r'^#include[^\n]*\n', '', source('mm/mods/items/logic/wand/wand_water.c'), flags=re.M)
    # The old stone draw is irrelevant to the acceptance test. The native Hunsui draw above stays real.
    if 'WandWater_GeyserDraw' in functions(water_mode):
        water_mode = water_mode.replace(functions(water_mode)['WandWater_GeyserDraw'], '')
    water_native += '\n' + re.sub(r'\bthis\b', 'self', water_mode)

    shadow_path = 'mm/mods/items/logic/wand/wand_shadow.c'
    shadow = re.sub(r'^#include[^\n]*\n', '', source(shadow_path), flags=re.M)
    shadow = shadow.replace(functions(shadow)['WandShadow_Draw'], '')
    shadow = bodies('mm/mods/items/helpers/target_select_helper.c', ['TargetSelect_FindNearest']) + '\n' + shadow

    helper = source('mm/mods/items/helpers/equip_helper.c')
    modes = helper[helper.index('typedef struct {'):helper.index('u8 ItemEquip_GetItemOnSlot')]
    modes += '\n' + bodies('mm/mods/items/helpers/equip_helper.c', ['ItemEquip_GetItemOnSlot', 'EquipCache_Update',
        'ItemInput_GetEquippedButton', 'ItemInput_Update', 'ItemInput_CheckDamage', 'ItemInput_CheckOtherButtons',
        'ItemInput_IsBlockedEx', 'ItemInput_IsBlocked', 'ItemMagic_IsInfinite', 'ItemMagic_Consume', 'ItemMagic_HasEnough'])
    inventory = 'mm/mods/extended_inventory.c'
    modes += '\n' + re.search(r'static const uint8_t sWandQuest\[WAND_MODE_COUNT\] = \{.*?\};', source(inventory), re.S)[0]
    modes += '\n' + bodies(inventory, ['Wand_RandoMode', 'Wand_ModeOwned', 'Wand_ModeCount', 'Wand_ModeAt',
        'Wand_GetMode', 'Wand_SetMode'])
    modes += '\n' + bodies('mm/mods/items/helpers/target_select_helper.c', ['TargetSelect_FindNearest'])
    modes += '\n' + bodies('mm/src/code/z_actor.c', ['Actor_SetScale', 'Flags_GetSwitch', 'Flags_SetSwitch', 'Flags_UnsetSwitch'])
    modes += '\n' + bodies('mm/src/code/z_scene.c', ['Object_GetSlot', 'Object_IsLoaded'])
    modes += '\n' + bodies('mm/src/code/z_player_lib.c', ['Player_CheckHostileLockOn', 'Player_FriendlyLockOnOrParallel'])
    modes += '\n' + bodies('mm/src/overlays/actors/ovl_player_actor/z_player.c', ['Player_IsZTargeting'])
    modes += '\n' + bodies('mm/src/code/sys_matrix.c', ['Matrix_MtxFCopy', 'Matrix_Push', 'Matrix_Pop',
        'Matrix_RotateYF', 'Matrix_RotateXF', 'Matrix_MultVec3f'])
    modes += '\n' + bodies('mm/mods/items/helpers/combat_helper.c', ['Combat_InitCylinder',
        'Combat_UpdateCylinder', 'Combat_RegisterCollider'])

    wand = source('mm/mods/items/logic/item_elemental_wand.c')
    for slug in ['sand', 'wind', 'water', 'meteor', 'storm', 'shadow']:
        mode = source('mm/mods/items/logic/wand/wand_' + slug + '.c')
        # Drawing is a boundary: the gameplay cast/update bodies remain verbatim.
        for name, function in functions(mode).items():
            if 'Draw' in name:
                mode = mode.replace(function, '')
        wand = wand.replace('#include "wand/wand_' + slug + '.c"', mode)
    wand = wand.replace(functions(wand)['Wand_Draw'], '')
    wand = re.sub(r'^#include[^\n]*\n', '', wand, flags=re.M)
    modes += '\n' + re.sub(r'\bthis\b', 'self', wand)

    player_path = 'mm/src/overlays/actors/ovl_player_actor/z_player.c'
    lib_path = 'mm/src/code/z_player_lib.c'
    extended = 'mm/mods/extended_player.c'
    for path, start in [(player_path, 's8 sItemItemActions['), (lib_path, 'u8 sActionModelGroups['),
            (lib_path, 'PlayerModelIndices gPlayerModelTypes['), (player_path, 'typedef enum ItemChangeType {'),
            (player_path, 's8 sPlayerItemChangeTypes[')]:
        modes += '\n' + declaration(path, start)
    # This registry slice retains the actual columns consumed by native item
    # action/model dispatch. Other rows and presentation columns are unrelated.
    wand_row = re.search(r'\{ ITEM_ELEMENTAL_WAND,\s*[^,]+,\s*[^,]+,', source(extended))[0].rstrip(',') + ' }'
    modes += '\nstatic const NeiItem sNeiItems[] = {' + wand_row + '};\n#define NEI_ITEMS_COUNT 1\n'
    modes += bodies(extended, ['Nei_FindByItem', 'ExtPlayer_FindByIA', 'ExtPlayer_GetItemAction', 'ExtPlayer_GetActionModelGroup'])
    modes += '\n' + bodies(player_path, ['Player_ItemToItemAction', 'Player_UseItem', 'func_8082FDC4',
        'func_Dpad_8082FDC4', 'Player_ItemIsInUse', 'Player_ProcessItemButtons'])
    modes += '\n' + bodies(lib_path, ['Player_ActionToModelGroup', 'Player_GetItemOnButton', 'Player_Dpad_GetItemOnButton'])
    update = functions(source(player_path), ['Player_Update'])['Player_Update']
    begin = update.index('    if (play->actorCtx.isOverrideInputOn')
    end = update.index('    GameInteractor_ExecuteOnPassPlayerInputs(&input);', begin)
    end += len('    GameInteractor_ExecuteOnPassPlayerInputs(&input);')
    selection = re.sub(r'\bthis\b', 'player', update[begin:end])
    modes += '\nvoid CompleteWandFrame(Player* player,PlayState* play,u16 press,u16 held){\n' + \
        '++play->gameplayFrames;play->state.input[0].press.button=press;play->state.input[0].cur.button=held;Input input;\n' + \
        selection + '\nsPlayerControlInput=&input;Wand_TickInput(play,player);Player_ProcessItemButtons(player,play);' + \
        '\nif(player->heldItemAction==PLAYER_IA_ELEMENTAL_WAND)Player_UpperAction_ElementalWand(player,play);\n}\n'

    oot_helper = 'soh/mods/items/helpers/equip_helper.c'
    oot_text = source(oot_helper)
    oot = oot_text[oot_text.index('typedef struct {'):oot_text.index('static void EquipCache_Update')]
    oot += '\n' + bodies(oot_helper, ['EquipCache_Update', 'ItemInput_GetEquippedButton',
        'ItemInput_ButtonIsClaimed', 'ItemInput_CheckOtherButtons', 'ItemInput_Update',
        'ItemInput_IsBlockedEx', 'ItemInput_IsBlocked', 'ItemMagic_IsInfinite', 'ItemMagic_Consume', 'ItemMagic_HasEnough'])
    oot_sand = re.sub(r'^#include[^\n]*\n', '', source('soh/mods/items/logic/wand/wand_sand.c'), flags=re.M)
    oot_sand = oot_sand.replace(functions(oot_sand)['WandSand_SlabDraw'], '')
    oot += '\n' + oot_sand
    oot_wand = 'soh/mods/items/logic/item_elemental_wand.c'
    oot += '\n' + re.search(r'static const s16 sWandMagicCost\[WAND_MODE_COUNT\] = \{.*?\};', source(oot_wand), re.S)[0]
    oot += '\n' + bodies(oot_wand, ['Wand_IsDrawn', 'Wand_OnWheelConfirm', 'Wand_BuildWheel',
        'Wand_ActiveWheelIndex', 'Wand_Cast', 'Wand_TickInput'])

    # Replace the old fixture's collider boundaries with the real engine pipeline.
    # This catches a projectile that registers successfully but misses low enemies.
    behavior_fixture = (ROOT / 'tests/wand_modes/modes_test.cpp').read_text()
    fixture_functions = functions(behavior_fixture)
    for name in ['Collider_InitCylinder', 'Collider_SetCylinder', 'CollisionCheck_SetAT', 'main']:
        behavior_fixture = behavior_fixture.replace(fixture_functions[name], '')
    collision = 'mm/src/code/z_collision_check.c'
    collision_names = ['Collider_InitBase', 'Collider_SetBase', 'Collider_ResetATBase',
        'Collider_InitElementDamageInfoAT', 'Collider_SetElementDamageInfoAT', 'Collider_ResetATElementUnk',
        'Collider_InitElementDamageInfoAC', 'Collider_SetElementDamageInfoAC', 'Collider_InitElement',
        'Collider_SetElement', 'Collider_ResetATElement', 'Collider_InitCylinderDim', 'Collider_SetCylinderDim',
        'Collider_InitCylinder', 'Collider_SetCylinder', 'Collider_ResetCylinderAT',
        'CollisionCheck_GetElementATDamage', 'CollisionCheck_GetDamageAndEffectOnElementAC',
        'CollisionCheck_ApplyElementATDefense', 'CollisionCheck_IsElementNotAT', 'CollisionCheck_IsElementNotAC',
        'CollisionCheck_NoSharedFlags', 'CollisionCheck_SetBounce', 'CollisionCheck_SetATvsAC',
        'CollisionCheck_AC_CylVsCyl', 'CollisionCheck_ApplyDamage']
    behavior_native = bodies(collision, collision_names)
    behavior_native += '\ntypedef s32 (*ColChkResetFunc)(PlayState*, Collider*);\n' + \
        'ColChkResetFunc sATResetFuncs[COLSHAPE_MAX] = {nullptr, Collider_ResetCylinderAT};\n'
    behavior_native += bodies(collision, ['CollisionCheck_SetAT'])
    behavior_native += '\n' + bodies('mm/src/code/sys_math3d.c', ['Math3D_CylVsCylOverlapCenterDist'])
    behavior_native += '\n' + bodies('mm/src/code/z_actor.c', ['Actor_ApplyDamage'])
    slime = source('mm/src/overlays/actors/ovl_En_Slime/z_en_slime.c')
    behavior_native += '\n' + re.search(r'typedef enum EnSlimeDamageEffect \{.*?} EnSlimeDamageEffect;', slime, re.S)[0]
    behavior_native += '\n' + re.search(r'static ColliderCylinderInit sCylinderInit = \{.*?\n};', slime, re.S)[0]
    behavior_native += '\n' + re.search(r'static DamageTable sDamageTable = \{.*?\n};', slime, re.S)[0]
    behavior_native += '\n' + bodies('mm/src/overlays/actors/ovl_En_Slime/z_en_slime.c', ['EnSlime_UpdateDamage'])

    with tempfile.TemporaryDirectory(prefix='mm-wand-') as directory:
        temp = Path(directory)
        (temp / 'production.inc').write_text(production)
        (temp / 'native_water.inc').write_text(water_native)
        (temp / 'shadow.inc').write_text(shadow)
        (temp / 'modes.inc').write_text(modes)
        (temp / 'oot_sand.inc').write_text(oot)
        (temp / 'behavior_fixture.inc').write_text(behavior_fixture)
        (temp / 'behavior_native.inc').write_text(behavior_native)
        for name, test_path in [('native-storm', 'native_storm_test.cpp'), ('native-water', 'native_water_test.cpp'), ('shadow', 'shadow_test.cpp'),
                ('modes', 'modes_test.cpp'), ('oot-sand', 'oot_sand_test.cpp'), ('behavior', 'behavior_test.cpp')]:
            parity_case = args.case == 'sand-parity' and name in ('oot-sand', 'behavior')
            if args.case and args.case != name and not parity_case:
                continue
            binary = temp / name
            compiler_flags = flags()
            if name == 'oot-sand':
                sys.path.insert(0, str(ROOT / 'tests/nei_held'))
                from run_articulated_tests import flags as oot_flags
                compiler_flags = oot_flags()[1:]
            if args.sanitize:
                compiler_flags += ['-O1', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
                # Native C passes D-pad values beyond EquipSlot's named members. That is valid in
                # C; the C++ fixture's enum-range check is a language-translation false positive.
                compiler_flags += ['-fno-sanitize=enum']
            subprocess.run(['c++', '-std=c++20', '-fpermissive', *compiler_flags, '-I' + str(temp),
                '-ffunction-sections', '-fdata-sections', str(ROOT / 'tests/wand_modes' / test_path),
                '-Wl,--gc-sections', '-o', str(binary)], check=True)
            if not parity_case:
                subprocess.run([str(binary), *([args.check] if args.check else [])], check=True)

        if args.case in (None, 'sand-parity'):
            donor = subprocess.check_output([str(temp / 'oot-sand'), 'sand-parity'], text=True).splitlines()
            port = subprocess.check_output([str(temp / 'behavior'), 'sand-parity'], text=True).splitlines()
            if donor != port:
                for index, (expected, actual) in enumerate(zip(donor, port)):
                    if expected != actual:
                        raise AssertionError(f'Sand differs from OoT at observation {index}:\nOoT {expected}\nMM  {actual}')
                raise AssertionError(f'Sand trace lengths differ: OoT {len(donor)}, MM {len(port)}')
            print(f'PASS OoT/MM Sand parity: {len(donor)} observations, C/D-pad, four headings, two meshes, '
                  'walking/pausing/turning, placement, billing, crumble and ring overflow')

    if args.case in (None, 'modes') and SOURCE_REF is None and not args.check:
        subprocess.run([sys.executable, str(ROOT / 'tests/wand_modes/run_cast_regression_tests.py')], check=True)


if __name__ == '__main__':
    main()
