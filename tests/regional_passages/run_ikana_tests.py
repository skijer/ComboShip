#!/usr/bin/env python3
"""Production OoT travel + actual native grave/grotto actions; no runtime claim."""
from pathlib import Path
import os, re, subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from

def fn(source, signature):
    return re.sub(r'\bthis\b','self',block_from(source,source.index(signature)))

def build(name, source, native=False):
    with tempfile.TemporaryDirectory(prefix='ikana-'+name+'-') as d:
        cpp=Path(d)/'test.cpp'; cpp.write_text(source); binary=Path(d)/'test'
        paths=['combo']
        if native: paths += ['soh','soh/soh','soh/include','soh/include/PR','soh/src','soh/assets','libultraship/include','combo/menu','soh/soh/Enhancements/Gameplay']
        args=[os.environ.get('CXX','c++'),'-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DFMT_HEADER_ONLY','-ffunction-sections','-fdata-sections','-Wl,--gc-sections',*['-I'+str(ROOT/p) for p in paths]]
        if '--sanitize' in sys.argv:args += ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-no-pie']
        env=dict(os.environ)
        extra=env.get('COMBO_NEI_FIXTURE_CPATH','')
        if extra:env['CPATH']=os.pathsep.join((extra,env.get('CPATH','')))
        subprocess.run([*args,str(cpp),'-o',str(binary)],check=True,env=env)
        subprocess.run([str(binary)],check=True,env={**env,'ASAN_OPTIONS':'detect_leaks=0'})

# Contract assertions run first so baseline fails on behavior before new headers exist.
oot=(ROOT/'soh/soh/Enhancements/Gameplay/ComboRegionalPassages.cpp').read_text()
assert 'FindMouth(' not in oot and 'FindGrave(' not in oot, 'regional endpoints must use fixed injected providers and exact existing grave ownership'
build('placement',(ROOT/'tests/regional_passages/ikana_grave_test.cpp').read_text())
haka=(ROOT/'soh/src/overlays/actors/ovl_Bg_Haka/z_bg_haka.c').read_text()
door=(ROOT/'soh/src/overlays/actors/ovl_Door_Ana/z_door_ana.c').read_text()
player=(ROOT/'soh/src/overlays/actors/ovl_player_actor/z_player.c').read_text()
native='extern "C" {\n'+haka[haka.index('static InitChainEntry sInitChain[]'):haka.index('void BgHaka_Init(Actor* thisx, PlayState* play) {')]
native+=door[door.index('static ColliderCylinderInit sCylinderInit'):door.index('// array of entrance table entries')]
for source, signatures in [(haka,['void BgHaka_Init(Actor* thisx, PlayState* play) {','void BgHaka_Destroy(Actor* thisx, PlayState* play) {','void BgHaka_CheckPlayerOnDirtPatch(BgHaka* this, Player* player) {','void BgHaka_IdleClosed(BgHaka* this, PlayState* play) {','void BgHaka_Pull(BgHaka* this, PlayState* play) {','void BgHaka_IdleOpened(BgHaka* this, PlayState* play) {','void BgHaka_IdleLockedClosed(BgHaka* this, PlayState* play) {','void BgHaka_Update(Actor* thisx, PlayState* play) {']), (door,['void DoorAna_SetupAction(DoorAna* this, DoorAnaActionFunc actionFunc) {','void DoorAna_Init(Actor* thisx, PlayState* play) {','void DoorAna_Destroy(Actor* thisx, PlayState* play) {'])]:
    native+='\n'.join(fn(source,s) for s in signatures)+'\n'
native+='}\n'
scene=(ROOT/'soh/soh/z_scene_otr.cpp').read_text()
sceneCommands='\n'.join(fn(scene,sig) for sig in ['bool Scene_CommandEntranceList(', 'bool Scene_CommandSpawnList('])
respawn=player[player.index('    respawnFlag = gSaveContext.respawnFlag;',player.index('void Player_Init(')):player.index('    const s32 crossGameTitleCard',player.index('void Player_Init('))]
respawn=re.sub(r'\bthis\b','self',respawn)
production=oot[oot.index('namespace {'):oot.rindex('#endif')]
fixture=(ROOT/'tests/regional_passages/oot_native_test.cpp').read_text().replace('/* NATIVE_ACTORS */',native).replace('/* NATIVE_GROTTO */',fn(player,'void Player_StartMode_Grotto(PlayState* play, Player* this) {')).replace('/* PRODUCTION */',production).replace('/* NATIVE_SCENE_COMMANDS */',sceneCommands).replace('/* NATIVE_RESPAWN */',respawn)
build('native',fixture,True)
