#!/usr/bin/env python3
"""Exercise the actual finish geometry and time variation without a graphics context."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='forest-portal-finish-') as temporary:
    path = Path(temporary)
    cpp, binary = path / 'finish.cpp', path / 'finish'
    subprocess.run([sys.executable, '-B', str(ROOT / 'scripts/diagnostics/run_forest_gi_particles_tests.py'),
                    '--policy-only'], check=True)

    # Execute the production submission guards; graphics allocation/submission
    # is the boundary. A recognized pack must never draw the temporary arch.
    sys.path.insert(0, str(ROOT/'scripts/diagnostics'))
    from run_time_pedestal_tests import block_from
    renderer = (ROOT/'mm/2s2h/Enhancements/Graphics/ComboLostWoodsPassage.cpp').read_text()
    finish = block_from(renderer, renderer.index('static void DrawPortalFinish('))
    finish = finish[:finish.index('    Matrix_Push()')] + '\n++finishes;\n}\n'
    arch = block_from(renderer, renderer.index('static void DrawPassage('))
    arch = arch[:arch.index('    static const bool built')] + '\n++arches;\n}\n'
    cpp.write_text(r'''
#include "global.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "2s2h/Enhancements/Graphics/WoodsMysteryMakeover.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
extern "C" { int gComboGameMode=COMBO_GAME_BOTH; }
SaveContext gSaveContext{};
PlayState* gPlayState;
static int finishes, arches;
extern "C" bool ResourceMgr_IsAltAssetsEnabled() { return true; }
extern "C" uint8_t ResourceMgr_FileAltExists(const char*) { return 0; }
extern "C" uint8_t ResourceMgr_FileExists(const char*) { return 0; }
extern "C" int ComboSoloForestMm_PassageAvailable(void) { return 0; }
// Opening-forest fixtures contain no installed native Mystery wall patch.
namespace WoodsMysteryNativeBridge { inline bool Active(const PlayState*) { return false; } }
''' + finish + arch + r'''
int main() {
 PlayState play{}; gPlayState=&play; play.sceneId=SCENE_LOST_WOODS;
 ActorEntry spawn{}; play.linkActorEntry=&spawn;
 RoomShape shape{}; RoomShapeDListsEntry entry{};
 shape.normal.base.type=ROOM_SHAPE_TYPE_NORMAL; shape.normal.numEntries=1;
 shape.normal.entries=&entry; play.roomCtx.curRoom.roomShape=&shape;
 gSaveContext.gameMode=GAMEMODE_NORMAL; gSaveContext.fileNum=0;
 gSaveContext.save.entrance=ENTRANCE(LOST_WOODS,0);
 const auto saved=gSaveContext;
 for(const char* path : {COMBO_OPENING_FOREST_PACK_ROOT,COMBO_OPENING_FOREST_STUMP_PACK_ROOT}) {
  entry.opa=(Gfx*)path; DrawPortalFinish(); DrawPassage();
  assert(finishes==0 && arches==0);
 }
 entry.opa=(Gfx*)COMBO_OPENING_FOREST_CORNER_PACK_ROOT;
 DrawPortalFinish(); DrawPassage(); assert(finishes==1 && arches==0);
 entry.opa=(Gfx*)COMBO_OPENING_FOREST_EDGE_PACK_ROOT;
 DrawPortalFinish(); DrawPassage(); assert(finishes==2 && arches==0);
 entry.opa=(Gfx*)COMBO_OPENING_FOREST_ALIGNED_PACK_ROOT;
 DrawPortalFinish(); DrawPassage(); assert(finishes==3 && arches==0);
 entry.opa=(Gfx*)COMBO_OPENING_FOREST_CENTERED_PACK_ROOT;
 DrawPortalFinish(); DrawPassage(); assert(finishes==4 && arches==0);
 entry.opa=(Gfx*)COMBO_OPENING_FOREST_RECENTERED_PACK_ROOT;
 DrawPortalFinish(); DrawPassage(); assert(finishes==5 && arches==0);
 entry.opa=(Gfx*)COMBO_OPENING_FOREST_NUDGED_PACK_ROOT;
 DrawPortalFinish(); DrawPassage(); assert(finishes==6 && arches==0);
 entry.opa=(Gfx*)"__OTR__native";
 DrawPortalFinish(); DrawPassage(); assert(finishes==7 && arches==1);
 entry.opa=(Gfx*)COMBO_OPENING_FOREST_NUDGED_PACK_ROOT;
 play.roomCtx.curRoom.num=1; DrawPortalFinish();
 play.roomCtx.curRoom.num=0; play.csCtx.state=CS_STATE_RUN; DrawPortalFinish();
 play.csCtx.state=CS_STATE_IDLE; gComboGameMode=COMBO_GAME_OOT; DrawPortalFinish();
 gComboGameMode=COMBO_GAME_BOTH; gSaveContext.sceneLayer=1; DrawPortalFinish();
 gSaveContext.sceneLayer=0; gSaveContext.save.cutsceneIndex=0xFFF0; DrawPortalFinish();
 gSaveContext.save.cutsceneIndex=0; gPlayState=nullptr; DrawPortalFinish();
 assert(finishes==7 && arches==1);
 assert(std::memcmp(&saved,&gSaveContext,sizeof(saved))==0);
 std::puts("PASS production draw selection: one packed doorway, scoped corner finish, legacy/native fallback, solo/room/cutscene guards, save unchanged");
}
''')
    includes = ['mm','mm/2s2h','mm/include','mm/include/PR','mm/src','mm/assets','libultraship/include','combo']
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2',
                    '-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie',
                    *['-I'+str(ROOT/p) for p in includes],str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
