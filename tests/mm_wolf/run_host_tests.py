#!/usr/bin/env python3
"""Compile production host/core/ext-button accessors on real MM headers and execute lifecycle fixtures."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_mm_nei_tests import flags
from run_time_pedestal_tests import functions
subprocess.run([sys.executable, str(ROOT/'tests/mm_wolf/run_equipment_policy_tests.py')], check=True)
def run(command):
    result=subprocess.run(command,capture_output=True,text=True)
    if result.returncode:
        print(result.stdout+result.stderr)
        raise SystemExit(result.returncode)
    if result.stdout: print(result.stdout,end='')
with tempfile.TemporaryDirectory(prefix='mm-wolf-host-') as td:
    # Compile these exact production bodies rather than recreating the native freeze state machine.
    player_source=(ROOT/'mm/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
    def native_body(signature):
        start=player_source.index(signature)
        brace=player_source.index('{',start)
        depth=1; end=brace+1
        while depth:
            depth+=(player_source[end]=='{')-(player_source[end]=='}'); end+=1
        return player_source[start:end]
    graph=functions((ROOT/'mm/src/code/graph.c').read_text())
    (Path(td)/'wolf-native-graph.inc').write_text(graph['Graph_OpenDisps']+'\n'+graph['Graph_CloseDisps'])
    gbi=functions((ROOT/'soh/soh/GbiWrap.cpp').read_text().replace('extern "C" ', ''))
    (Path(td)/'wolf-native-scroll-gbi.inc').write_text(gbi['gDPSetTileSizeLerp']+'\nGfx gEffIceFragment3DL[1];\n')
    rcp=functions((ROOT/'mm/src/code/z_rcp.c').read_text())
    draw_start=player_source.index('    Matrix_Push();\n    wolfDrawn = WolfLinkHost_Draw')
    draw_end=player_source.index('\n    // OoT "adult mode"',draw_start)
    native_draw=('void WolfFixture_DrawNativePrefix(PlayState* play, Player* this) {\n'
                 's32 wolfDrawn;\n'+player_source[draw_start:draw_end]+'\n}\n')
    native_actions=Path(td)/'native-freeze.c'
    input_start=player_source.index('    if (play->actorCtx.isOverrideInputOn && (this == GET_PLAYER(play))) {')
    input_end=player_source.index('\n    GameInteractor_ExecuteOnPassPlayerInputs(&input);',input_start)
    input_body=player_source[input_start:input_end]
    native_actions.write_text('#include "global.h"\n'
                              '#include "mods/forms/wolf_link_host.h"\n'
                              '#include "mods/items/custom_items.h"\n'
                              '#include "mods/extended_equipment.h"\n'
                              'extern Gfx gEffIceFragment3DL[];\n'
                              'void GameInteractor_ExecuteOnPassPlayerInputs(Input*);\n'
                              'extern Input* sPlayerControlInput;\n'
                              'extern f32 sControlStickMagnitude;\n'
                              'extern s16 sControlStickAngle;\n'
                              'u8 VanillaTB_IsIronBoots(void);\n'
                              'void Player_AnimReplace_SetupLedgeClimb(Player*, s32);\n'
                              'void func_80839E74(Player*, PlayState*);\n'
                              'void func_8084C124(PlayState*, Player*);\n'
                              's32 Player_InflictDamage(PlayState*, s32);\n'
                              'void func_80836988(Player*, PlayState*);\n'
                              'void func_808339B4(Player*, s32);\n'
                              'void func_80834104(PlayState*, Player*);\n'+
                              native_body('s32 func_8082DE88(Player* this, s32 arg1, s32 arg2) {')+'\n'+
                              native_body('void Player_Action_82(Player* this, PlayState* play) {')+'\n'+
                              native_body('bool func_8082DA90(PlayState* play) {')+'\n'+
                              native_body('u16 Player_GetFloorSfxByAge(Player* this, u16 sfxId) {')+'\n'+
                              native_body('void Player_AnimSfx_PlayFloorWalk(Player* this, f32 freqVolumeLerp) {')+'\n'+
                              native_body('void Player_AnimSfx_PlayFloorJump(Player* this) {')+'\n'+
                              native_body('void Player_AnimSfx_PlayFloorLand(Player* this) {')+'\n'+
                              native_body('void Player_AnimSfx_PlayVoice(Player* this, u16 sfxId) {')+'\n'+
                              native_body('void Player_Action_49(Player* this, PlayState* play) {')+'\n'+
                              'void WolfFixture_SelectNativeInput(PlayState* play, Player* this, Input* out) {\n'
                              'Input input;\n'+input_body+'\n'
                              'GameInteractor_ExecuteOnPassPlayerInputs(&input);\n*out=input;\n}\n'+
                              rcp['Gfx_TwoTexScrollEx']+'\n'+native_draw)
    objects=[]
    for i,path in enumerate(['mm/expansions/ssbb/ssbb_character.c','mm/expansions/ssbb/ssbb_skin.c',
                             'mm/src/code/z_skin_matrix.c','mm/src/code/z_lib.c','mm/mods/ext_buttons/ext_buttons.cpp',
                             str(native_actions)]):
        cpp=path.endswith('.cpp'); obj=str(Path(td)/f'{i}.o')
        run([os.environ.get('CXX' if cpp else 'CC','c++' if cpp else 'cc'),'-std=c++20' if cpp else '-std=gnu11',
             *flags(),'-include',str(ROOT/'mm/expansions/ssbb/ssbb_anim.h'),
             *([] if cpp else ['-include',str(ROOT/'mm/include/z64malloc.h')]),
             '-include',str(ROOT/'mm/include/functions.h'),'-include',str(ROOT/'mm/include/variables.h'),
             '-include',str(ROOT/'libultraship/include/libultraship/bridge/consolevariablebridge.h'),
             '-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all','-ffunction-sections','-fdata-sections',
             '-c',str(ROOT/path),'-o',obj]); objects.append(obj)
    binary=str(Path(td)/'host')
    options=['-DMM_WOLF_HOST','-I'+td] if (ROOT/'mm/mods/forms/wolf_link_host.cpp').exists() else []
    run([os.environ.get('CXX','c++'),'-std=c++20',*flags(),*options,'-DMM_WOLF_NATIVE_HANDOFF','-DFMT_HEADER_ONLY',
         '-DWOLF_IMPLEMENTATION="'+str(ROOT/'mm/mods/transformation_masks/wolf_link_form.cpp')+'"',
         '-O1','-g','-fsanitize=undefined','-fno-sanitize-recover=all','-ffunction-sections','-fdata-sections',
         str(ROOT/'tests/mm_wolf/host_runtime_test.cpp'),*objects,'-Wl,--gc-sections','-o',binary])
    run([binary,td])
