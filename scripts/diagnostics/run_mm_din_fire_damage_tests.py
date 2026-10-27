#!/usr/bin/env python3
"""Compile MM Din with native strike/collision/drop functions and real actor tables.

The unrelated engine entry points are boundaries, not copies of gameplay logic.
Function bodies are extracted unchanged from current production source; actor
tables and grass masks are compiled from the complete actor translation units.
"""
import os
from pathlib import Path
import re
import tempfile
import sys

from run_mm_din_fire_tests import FLAGS, ROOT, run
from run_time_pedestal_tests import functions


def native_functions():
    chunks = ['MtxF* sCurrentMatrix = &currentMatrix;']
    source = (ROOT / 'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    chunks.append(re.search(r'typedef struct MeleeWeaponDamageInfo \{.*?\} MeleeWeaponDamageInfo;', source, re.S)[0])
    chunks.append(re.search(r'MeleeWeaponDamageInfo D_8085D09C\[.*?^\};', source, re.M | re.S)[0])
    mine = (ROOT / 'mm/src/overlays/actors/ovl_Obj_Mine/z_obj_mine.c').read_text()
    chunks.append(re.search(r'static ObjMineMtxF3 sStandardBasis = \{.*?^\};', mine, re.M | re.S)[0])
    paths = {
        'code/z_lib.c': ['Math_Vec3s_ToVec3f', 'Math_Vec3f_Copy', 'Math_Vec3f_Yaw', 'Math_SinS', 'Math_CosS'],
        'code/sys_math3d.c': ['Math3D_Vec3fMagnitudeSq', 'Math3D_Vec3fMagnitude'],
        'code/sys_matrix.c': ['Matrix_RotateXS', 'Matrix_RotateYS', 'Matrix_MultVecZ'],
        'code/z_player_lib.c': ['Player_MeleeWeaponFromIA', 'Player_GetMeleeWeaponHeld'],
        'overlays/actors/ovl_player_actor/z_player.c': ['func_80833728', 'func_8083375C'],
        'code/z_collision_check.c': ['CollisionCheck_GetElementATDamage',
            'CollisionCheck_GetDamageAndEffectOnElementAC', 'CollisionCheck_ApplyElementATDefense',
            'CollisionCheck_NoSharedFlags', 'CollisionCheck_ApplyDamage',
            'CollisionCheck_SetBounce', 'CollisionCheck_SetATvsAC'],
        'code/z_actor.c': ['Actor_SetDropFlag', 'Actor_SetDropFlagJntSph', 'Actor_WorldYawTowardActor', 'Actor_ApplyDamage'],
        'overlays/actors/ovl_Boss_Hakugin/z_boss_hakugin.c': ['BossHakugin_FrozenBeforeFight'],
        'overlays/actors/ovl_Obj_Ice_Poly/z_obj_ice_poly.c': ['func_80931A38'],
        'overlays/actors/ovl_Obj_Mine/z_obj_mine.c': ['ObjMine_GetUnitVec3f', 'ObjMine_Air_CheckAC',
                                                   'ObjMine_Water_CheckAC'],
    }
    for path, names in paths.items():
        native = functions((ROOT / 'mm/src' / path).read_text(), names)
        chunks.extend(native[name] for name in names)
    return '\n\n'.join(chunks) + '\n'


def native_reactions(build):
    # Verbatim native damage consumption and freeze setup. Animation/audio,
    # rendering and scene actor spawning are the only inert boundaries.
    source = (ROOT / 'mm/src/overlays/actors/ovl_En_Wf/z_en_wf.c').read_text()
    code = '''#include "global.h"
#include "din_fire_sword.h"
#include "overlays/actors/ovl_En_Wf/z_en_wf.h"
#include "overlays/actors/ovl_En_Clear_Tag/z_en_clear_tag.h"
static int Test_facing;
static Actor* Test_Spawn(void) { return NULL; }
#define Actor_IsFacingPlayer(...) Test_facing
#define Actor_SetColorFilter(...) ((void)0)
#define Actor_PlaySfx(...) ((void)0)
#define Enemy_StartFinishingBlow(...) ((void)0)
#define Actor_Spawn(...) Test_Spawn()
#define func_809907D4 Test_func_809907D4
#define func_8099386C Test_func_8099386C
static void func_80990854(EnWf* actor, PlayState* play) {}
static void func_809923B0(EnWf* actor) {}
static void func_80992D6C(EnWf* actor) {}
static void func_8099245C(EnWf* actor) {}
'''
    native = functions(source, ['func_809907D4', 'func_8099386C'])
    code += native['func_809907D4'] + '\n' + native['func_8099386C'] + '''
int Test_MM_WolfosHealth(PlayState* play, ColliderElement* toucher, u8 damage, u8 effect, int rear, int frozen, int* visual) {
    EnWf actor = { 0 };
    actor.actor.colChkInfo.health = 100;
    actor.actor.colChkInfo.damage = damage;
    actor.actor.colChkInfo.damageEffect = effect;
    actor.drawDmgEffType = frozen ? ACTOR_DRAW_DMGEFF_FROZEN_NO_SFX : ACTOR_DRAW_DMGEFF_FIRE;
    actor.collider2.base.acFlags = AC_HIT;
    actor.collider2.elem.acHitElem = toucher;
    Test_facing = !rear;
    func_8099386C(&actor, play);
    *visual = actor.drawDmgEffType;
    return actor.actor.colChkInfo.health;
}
'''
    path = build / 'WolfosElementReaction.c'
    path.write_text(code)
    paths = [str(path)]
    guards = [
        ('Wallmaster', 'En_Wallmas', 'EnWallmas', 'EnWallmas_UpdateDamage', 'collider', 'EnWallmasDamageEffect'),
        ('Floormaster', 'En_Floormas', 'EnFloormas', 'func_808D2E34', 'collider', None),
        ('DekuNuts', 'En_Dekunuts', 'EnDekunuts', 'func_808BE73C', 'collider', None),
        ('Peahat', 'En_Peehat', 'EnPeehat', 'func_8089874C', 'colliderSphere', None),
        ('Dinolfos', 'En_Dinofos', 'EnDinofos', 'EnDinofos_UpdateDamage', 'bodyAndFireCollider', 'EnDinofosDamageEffect'),
        ('DekuBaba', 'En_Dekubaba', 'EnDekubaba', 'EnDekubaba_UpdateDamage', 'collider', 'DekuBabaDamageEffect|DekuBabaHitType'),
        ('Chuchu', 'En_Slime', 'EnSlime', 'EnSlime_UpdateDamage', 'collider', 'EnSlimeDamageEffect'),
        ('IronKnuckle', 'En_Ik', 'EnIk', 'EnIk_UpdateDamage', 'colliderCylinder', 'EnIkDmgEff'),
    ]
    for name, actor, type_, damage_fn, collider, enum in guards:
        source = (ROOT / f'mm/src/overlays/actors/ovl_{actor}/z_{actor.lower()}.c').read_text()
        native = functions(source)
        code = f'''#include "global.h"
#include "din_fire_sword.h"
#include "overlays/actors/ovl_{actor}/z_{actor.lower()}.h"
#include "overlays/actors/ovl_En_Clear_Tag/z_en_clear_tag.h"
#define Actor_SetColorFilter(...) ((void)0)
#define Actor_PlaySfx(...) ((void)0)
#define Actor_Kill(...) ((void)0)
#define Enemy_StartFinishingBlow(...) ((void)0)
#define Actor_Spawn(...) ((Actor*)0)
#define Audio_RestorePrevBgm(...) ((void)0)
#define func_800BE568(...) ((void)0)
#define {damage_fn} Test_{damage_fn}
'''
        if enum:
            for enum_name in enum.split('|'):
                code += re.search(r'typedef enum(?: \w+)? \{[^{}]*\} ' + enum_name + ';', source, re.S)[0] + '\n'
        for action in set(re.findall(r'this->actionFunc\s*[!=]=\s*(\w+)', native[damage_fn])):
            code += f'static void {action}({type_}* actor, PlayState* play) {{}}\n'
        # The actor's action/animation handoff and thaw visuals are boundaries;
        # the entire native admission guard, damage, and effect choice remain.
        for called in sorted(set(re.findall(r'\b(\w+)\s*\(', native[damage_fn])) & native.keys() - {damage_fn}):
            code += f'#define {called}(...) ((void)0)\n'
        if actor == 'En_Dinofos':
            code += 'static s16 sNumAlive = 2, sCsId = CS_ID_NONE;\n'
        if actor == 'En_Ik':
            code += 'static DamageTable sDamageTableNoArmor;\n#define Animation_GetLastFrame(...) 10.0f\n'
        if actor == 'En_Dekubaba':
            code += '''static int Test_cutRoute;
#undef EnDekubaba_SetupSway
#undef EnDekubaba_SetupPrunedSomersaultDie
#define EnDekubaba_SetupSway(...) (Test_cutRoute = 1)
#define EnDekubaba_SetupPrunedSomersaultDie(...) (Test_cutRoute = 1)
int Test_MM_DekuBabaCutRoute(void) { return Test_cutRoute; }
'''
        code += native[damage_fn] + '\n'
        elem = f'actor.{collider}.elem'
        init = ''
        if actor == 'En_Dinofos':
            elem = 'actor.bodyAndFireColliderElements[0].base'
            init = '''actor.bodyAndFireCollider.elements = actor.bodyAndFireColliderElements;
    actor.bodyAndFireCollider.count = ARRAY_COUNT(actor.bodyAndFireColliderElements);
    actor.bodyAndFireColliderElements[0].base.acElemFlags = ACELEM_HIT;
'''
        if actor == 'En_Dekubaba':
            elem = 'actor.colliderElements[0].base'
            init = '''actor.collider.elements = actor.colliderElements;
    actor.collider.count = ARRAY_COUNT(actor.colliderElements);
    actor.colliderElements[0].base.acElemFlags = ACELEM_HIT;
    actor.actionFunc = EnDekubaba_StunnedVertical;
    Test_cutRoute = 0;
'''
        if actor == 'En_Slime':
            init = 'actor.actor.params = EN_SLIME_TYPE_BLUE;\n'
        code += f'''int Test_MM_{name}FrozenHealth(PlayState* play, ColliderElement* toucher, u8 damage, u8 effect, int frozen) {{
    {type_} actor = {{ 0 }};
    actor.actor.colChkInfo.health = 100;
    actor.actor.colChkInfo.mass = 50;
    actor.actor.scale.x = .01f;
    actor.actor.colChkInfo.damage = damage;
    actor.actor.colChkInfo.damageEffect = effect;
    actor.drawDmgEffType = frozen ? ACTOR_DRAW_DMGEFF_FROZEN_NO_SFX : ACTOR_DRAW_DMGEFF_FIRE;
    actor.{collider}.base.acFlags = AC_HIT;
    {init}
    {elem}.acHitElem = toucher;
    {damage_fn}(&actor, play);
    return actor.actor.colChkInfo.health;
}}
'''
        path = build / (name + 'FrozenReaction.c')
        path.write_text(code)
        paths.append(str(path))
    return paths


def main(test_source=None):
    actors = {
        'En_Slime': [('Chuchu', 'sDamageTable')],
        'En_Dekubaba': [('DekuBaba', 'sDamageTable')],
        'En_Wf': [('Wolfos', 'sDamageTable1'), ('WhiteWolfos', 'sDamageTable2')],
        'En_Am': [('Armos', 'sDamageTable')],
        'En_Firefly': [('Keese', 'sDamageTable')],
        'En_Dinofos': [('Dinolfos', 'sDamageTable')],
        'En_Rd': [('Redead', 'sDamageTable')],
        'En_Fz': [('Freezard', 'sDamageTable')],
        'En_Wallmas': [('Wallmaster', 'sDamageTable')],
        'En_Floormas': [('Floormaster', 'sDamageTable')],
        'En_Crow': [('Crow', 'sDamageTable')],
        'En_Dekunuts': [('DekuNuts', 'sDamageTable')],
        'En_Peehat': [('Peahat', 'sDamageTable')],
        'En_Ik': [('IronKnuckle', 'sDamageTableArmor')],
    }
    with tempfile.TemporaryDirectory(prefix='mm-din-damage-') as directory:
        build = Path(directory)
        (build / 'din_fire_native.inc').write_text(native_functions())
        sine = build / 'sine.c'
        sine.write_text('#include "global.h"\n#include "libultra/gu/sins.c"\n')
        wrappers = [str(sine), *native_reactions(build)]
        for actor, tables in actors.items():
            code = f'#include "overlays/actors/ovl_{actor}/z_{actor.lower()}.c"\n'
            for name, table in tables:
                code += f'DamageTable* Test_{name}Table(void) {{ return &{table}; }}\n'
            path = build / f'{actor}.c'
            path.write_text(code)
            wrappers.append(str(path))
        # Every classification-only frozen guard edit is compiled as an intact
        # native actor translation unit, including actors with no added status.
        frozen_guards = {
            'Boss_Hakugin', 'En_Bb', 'En_Bbfall', 'En_Bigslime', 'En_Dekubaba',
            'En_Dodongo', 'En_Ik', 'En_Kame', 'En_Okuta', 'En_Pametfrog', 'En_Rr',
            'En_Slime', 'En_Tite', 'En_Wf', 'En_Wallmas', 'En_Floormas',
            'En_Dekunuts', 'En_Peehat', 'En_Dinofos',
        }
        for actor in sorted(frozen_guards - actors.keys()):
            run(os.environ.get('CC', 'cc'), *FLAGS, '-ffunction-sections', '-fdata-sections', '-c',
                str(ROOT / f'mm/src/overlays/actors/ovl_{actor}/z_{actor.lower()}.c'),
                '-o', str(build / f'{actor}.o'))
        for actor, name in [('En_Kusa', 'Kusa'), ('Obj_Grass', 'Grass')]:
            path = build / f'{actor}.c'
            path.write_text(f'#include "overlays/actors/ovl_{actor}/z_{actor.lower()}.c"\n'
                            f'u32 Test_{name}Mask(void) {{ return sCylinderInit.elem.acDmgInfo.dmgFlags; }}\n')
            wrappers.append(str(path))
        # Execute the native enemy-soul hook instead of assuming every AC collision is allowed.
        souls = (ROOT / 'mm/2s2h/Rando/ActorBehavior/Souls.cpp').read_text()
        rando = (ROOT / 'mm/2s2h/Rando/Rando.h').read_text()
        soul_header = (ROOT / 'mm/2s2h/Rando/ActorBehavior/Souls.h').read_text()
        soul_code = '\n'.join(re.search(r'^#define ' + name + r'[^\n]*', text, re.M)[0]
                              for name, text in [('IS_RANDO', rando), ('RANDO_SAVE_OPTIONS', rando),
                                                 ('SOUL_RI_TO_RANDO_INF', soul_header)])
        soul_code += '\n' + re.search(r'std::unordered_map<int16_t, RandoItemId> enemySoulMap = \{.*?^\};',
                                       souls, re.M | re.S)[0]
        soul_code += '\n' + functions(souls, ['HaveEnemySoul'])['HaveEnemySoul']
        gate = re.search(r'    bool shouldEnemyInjure = .*?\n    \}\);', souls, re.S)[0]
        soul_code += '\nvoid Test_RegisterSoulGate(void) {\n' + gate + '\n}\n'
        (build / 'din_soul_native.inc').write_text(soul_code)
        soul_object = str(build / 'souls.o')
        cxx_flags = [flag for flag in FLAGS if not flag.startswith('-std=') and
                     flag not in ('-Wno-incompatible-pointer-types', '-Wno-discarded-qualifiers',
                                  '-Werror=implicit-function-declaration')]
        run(os.environ.get('CXX', 'c++'), '-std=c++20', *cxx_flags, '-I' + directory,
            '-c', 'mm/tests/din_fire_soul_gate.cpp', '-o', soul_object)
        for source in ([test_source] if test_source else ['mm/tests/din_fire_damage_test.c',
                                                         'mm/tests/elemental_sword_damage_test.c']):
            binary = str(build / Path(source).stem)
            run(os.environ.get('CC', 'cc'), *FLAGS, '-I' + directory,
                '-ffunction-sections', '-fdata-sections', source,
                'mm/src/code/din_fire_sword.c', 'mm/src/code/elemental_sword_finish.c',
                'mm/src/code/sys_math_atan.c',
                'mm/src/libultra/gu/coss.c',
                *wrappers, soul_object, '-Wl,--gc-sections', '-lstdc++', '-lm', '-o', binary)
            run(binary)


if __name__ == '__main__':
    main(sys.argv[1] if len(sys.argv) > 1 else None)
