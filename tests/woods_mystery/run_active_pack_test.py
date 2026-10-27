"""The actual loaded mesh controls the new effect and fallback-arch ownership."""
from pathlib import Path
import os,subprocess,sys,tempfile

root=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(root/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
renderer=(root/'mm/2s2h/Enhancements/Graphics/ComboLostWoodsPassage.cpp').read_text()
draw=block_from(renderer,renderer.index('static void DrawWoodsMysteryHexes('))
draw=draw[:draw.index('    Matrix_Push()')]+'\n++draws;\n}\n'
arch=block_from(renderer,renderer.index('static void DrawPassage('))
arch=arch[:arch.index('    static const bool built')]+'\n++arches;\n}\n'
source=r'''
#include "2s2h/Enhancements/Graphics/WoodsMysteryMakeover.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include "ComboGameMode.h"
#include <cassert>
#include <cstdio>
static bool altEnabled=true, visualAvailable=true, bareVisualAvailable=false;
static bool soloAvailable=false;
static bool nativeReady=false;
static int draws=0,arches=0;
SaveContext gSaveContext{};
PlayState* gPlayState;
extern "C" { int gComboGameMode=COMBO_GAME_BOTH; }
extern "C" int ComboSoloForestMm_PassageAvailable(void) { return soloAvailable; }
extern "C" bool ResourceMgr_IsAltAssetsEnabled() { return altEnabled; }
extern "C" uint8_t ResourceMgr_FileAltExists(const char*) { return visualAvailable; }
extern "C" uint8_t ResourceMgr_FileExists(const char*) { return bareVisualAvailable; }
namespace WoodsMysteryNativeBridge {
inline bool Active(const PlayState*) { return nativeReady; }
}
/* DRAW_GUARD */
/* ARCH_GUARD */
int main() {
    using namespace WoodsMysteryMakeover;
    PlayState play{};RoomShape shape{};RoomShapeCullableEntry entries[2]{};
    play.sceneId=SCENE_26SARUNOMORI;play.roomCtx.curRoom.num=1;
    play.roomCtx.curRoom.roomShape=&shape;shape.base.type=ROOM_SHAPE_TYPE_CULLABLE;
    shape.cullable.numEntries=2;shape.cullable.entries=entries;
    entries[1].opa=(Gfx*)"__OTR__scenes/nonmq/Z2_26SARUNOMORI/native";
    assert(!Active(&play));
    entries[1].opa=(Gfx*)kBridgeMesh;assert(Active(&play));
    // Resource selection changes in place; the loaded room header stays intact.
    altEnabled=false;assert(!Active(&play));
    altEnabled=true;assert(Active(&play));
    visualAvailable=false;assert(!Active(&play));
    // Prelude exports introduced custom resources at their canonical bare path.
    bareVisualAvailable=true;assert(Active(&play));
    altEnabled=false;assert(!Active(&play));altEnabled=true;
    entries[1].opa=(Gfx*)"__OTR__scenes/nonmq/Z2_26SARUNOMORI/native";
    assert(!Active(&play));entries[1].opa=(Gfx*)kBridgeMesh;
    bareVisualAvailable=false;assert(!Active(&play));
    visualAvailable=true;assert(Active(&play));
    gPlayState=&play;gSaveContext.gameMode=GAMEMODE_NORMAL;
    DrawWoodsMysteryHexes();assert(draws==1);
    DrawPassage();assert(arches==0);
    gComboGameMode=COMBO_GAME_OOT;DrawWoodsMysteryHexes();assert(draws==1);
    gComboGameMode=COMBO_GAME_MM;DrawWoodsMysteryHexes();assert(draws==1);
    soloAvailable=true;DrawWoodsMysteryHexes();assert(draws==2);
    play.csCtx.state=CS_STATE_RUN;DrawWoodsMysteryHexes();assert(draws==2);
    play.csCtx.state=CS_STATE_IDLE;gSaveContext.gameMode=GAMEMODE_TITLE_SCREEN;
    DrawWoodsMysteryHexes();assert(draws==2);
    gSaveContext.gameMode=GAMEMODE_NORMAL;altEnabled=false;
    DrawWoodsMysteryHexes();assert(draws==2);altEnabled=true;
    play.roomCtx.curRoom.num=2;assert(!Active(&play));
    play.roomCtx.curRoom.num=1;play.sceneId=SCENE_LOST_WOODS;assert(!Active(&play));
    play.sceneId=SCENE_26SARUNOMORI;shape.base.type=ROOM_SHAPE_TYPE_NORMAL;assert(!Active(&play));
    shape.base.type=ROOM_SHAPE_TYPE_CULLABLE;shape.cullable.numEntries=0;assert(!Active(&play));
    shape.cullable.numEntries=2;entries[1].opa=nullptr;assert(!Active(&play));
    char binary[16]{};entries[1].opa=(Gfx*)binary;assert(!Active(&play));
    assert(!Active(nullptr));
    // The baked entrance owns a verified wall aperture without any archive.
    entries[1].opa=(Gfx*)"__OTR__scenes/nonmq/Z2_26SARUNOMORI/native";
    visualAvailable=false;bareVisualAvailable=false;altEnabled=false;
    gComboGameMode=COMBO_GAME_BOTH;soloAvailable=false;
    nativeReady=true;DrawWoodsMysteryHexes();assert(draws==3);
    DrawPassage();assert(arches==0);
    altEnabled=true;DrawWoodsMysteryHexes();assert(draws==4);
    gComboGameMode=COMBO_GAME_MM;DrawWoodsMysteryHexes();assert(draws==4);
    soloAvailable=true;DrawWoodsMysteryHexes();assert(draws==5);
    nativeReady=false;DrawWoodsMysteryHexes();assert(draws==5);
    DrawPassage();assert(arches==1); // Missing assets retain the legacy fallback.
    entries[1].opa=(Gfx*)kBridgeMesh;altEnabled=false;
    DrawPassage();assert(arches==1); // Stale pack headers cannot double the arch.
    std::puts("PASS: verified baked entrance draws GI hex motes without any archive in combined/solo modes; native/pack ownership suppresses duplicate arches; missing assets retain fallback");
}
'''
source=source.replace('/* DRAW_GUARD */',draw).replace('/* ARCH_GUARD */',arch)
with tempfile.TemporaryDirectory() as td:
    p=Path(td);(p/'test.cpp').write_text(source)
    command=[os.environ.get('CXX','c++'),'-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DFMT_HEADER_ONLY']
    command+=['-I'+str(root/path) for path in ['mm','mm/2s2h','mm/include','mm/include/PR','mm/src','mm/assets','libultraship/include','combo']]
    subprocess.run([*command,str(p/'test.cpp'),'-o',str(p/'test')],check=True)
    subprocess.run([str(p/'test')],check=True)
