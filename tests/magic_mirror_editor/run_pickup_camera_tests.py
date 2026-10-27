#!/usr/bin/env python3
"""Execute private forest camera preparation and native player CS assignment."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import functions, block_from
play = (ROOT / 'mm/src/code/z_play.c').read_text()
native = block_from(play, play.index('s16 sPlayerCsIdToCsCamId[]')) + ';\n'
native += functions(play, {'Play_AssignPlayerCsIdsFromScene'})['Play_AssignPlayerCsIdsFromScene']
native = native.replace('this', 'current')
header = (ROOT / 'mm/2s2h/Enhancements/Camera/ComboForestOcarina.h').read_text()
fixture = r'''
#include "global.h"
#include "ComboGameMode.h"
#include "entrances/SoloForest.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
SaveContext gSaveContext{};
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; s16 sSceneCutsceneCount; }
static ActorCutscene* list;
extern "C" void CutsceneManager_Init(PlayState*, ActorCutscene* entries, s16 count) {
 list = entries; sSceneCutsceneCount = count;
}
extern "C" ActorCutscene* CutsceneManager_GetCutsceneEntry(s16 id) {
 assert(id >= 0 && id < sSceneCutsceneCount); return &list[id];
}
/* HEADER */
/* NATIVE */
int main() {
 for (int mode: {COMBO_GAME_BOTH, COMBO_GAME_MM}) for (int form=0; form<PLAYER_FORM_MAX; ++form) {
  PlayState play{}; ActorEntry original{};
  original.rot.y = (204 << 7) | 127; original.params = 0x0EFF;
  const auto saved = original;
  gComboGameMode = mode; gSaveContext = {};
  gSaveContext.fileNum = 1; gSaveContext.gameMode = GAMEMODE_NORMAL;
  gSaveContext.save.playerForm = form;
  gSaveContext.save.entrance = mode == COMBO_GAME_BOTH ? ENTRANCE(LOST_WOODS,0) : COMBO_SOLO_FOREST_MM_ENTRANCE;
  play.sceneId = mode == COMBO_GAME_BOTH ? SCENE_LOST_WOODS : COMBO_SOLO_FOREST_MM_SCENE;
  play.curSpawn = 0; play.linkActorEntry = &original; sSceneCutsceneCount = 0;
  ComboForestOcarina_PrepareScene(&play);
  assert(sSceneCutsceneCount == 2 && play.linkActorEntry != &original);
  assert(std::memcmp(&saved, &original, sizeof(saved)) == 0);
  assert((play.linkActorEntry->rot.y & 127) == 0);
  assert((play.linkActorEntry->rot.y >> 7) == 204);
  Play_AssignPlayerCsIdsFromScene(&play, 0);
  assert(play.playerCsIds[PLAYER_CS_ID_ITEM_OCARINA] == 0);
  assert(play.playerCsIds[PLAYER_CS_ID_ITEM_GET] == 1);
  assert(list[play.playerCsIds[PLAYER_CS_ID_ITEM_GET]].csCamId == CS_CAM_ID_GLOBAL_ITEM_GET);
  assert(list[1].scriptIndex == CS_SCRIPT_ID_NONE && list[1].additionalCsId == CS_ID_NONE);
  assert(gSaveContext.save.playerForm == form);
  // Native/authored chains and non-forest scenes remain their owners.
  ActorCutscene authored{}; list = &authored; sSceneCutsceneCount = 1;
  play.linkActorEntry = &original;
  ComboForestOcarina_PrepareScene(&play);
  assert(list == &authored && sSceneCutsceneCount == 1 && play.linkActorEntry == &original);
  sSceneCutsceneCount = 0; play.sceneId = SCENE_TOWN;
  ComboForestOcarina_PrepareScene(&play);
  assert(sSceneCutsceneCount == 0 && play.linkActorEntry == &original);
 }
 puts("PASS both MM forest modes/all forms: native ocarina + item-get camera IDs, packed yaw, resource preservation and authored-chain guards");
}
'''.replace('/* HEADER */', header).replace('/* NATIVE */', native)
with tempfile.TemporaryDirectory(prefix='mirror-pickup-camera-') as temp:
    path = Path(temp); (path/'test.cpp').write_text(fixture)
    inc = [ROOT, ROOT/'mm', ROOT/'mm/include', ROOT/'mm/include/PR', ROOT/'mm/2s2h', ROOT/'mm/src',
           ROOT/'mm/assets', ROOT/'combo', ROOT/'libultraship/include']
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2',
                    '-ffunction-sections','-fdata-sections','-Wl,--gc-sections',
                    *['-I'+str(p) for p in inc],str(path/'test.cpp'),'-o',str(path/'test')],check=True)
    subprocess.run([str(path/'test')],check=True)
