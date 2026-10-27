#!/usr/bin/env python3
"""Exercise the actual stump return bounds and legacy-pack compatibility."""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
source = r'''
#include "global.h"
#include "2s2h/Enhancements/Gameplay/ComboOpeningForest.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
SaveContext gSaveContext{};
static void require(bool condition, const char* reason) {
    if (!condition) { std::fprintf(stderr,"FAIL: %s\n",reason); std::exit(1); }
}
int main() {
    PlayState play{}; play.sceneId=SCENE_LOST_WOODS; play.roomCtx.curRoom.num=0;
    ActorEntry spawn{}; spawn.pos={-417,0,-122}; spawn.rot.y=11520;
    play.linkActorEntry=&spawn;
    RoomShape shape{}; RoomShapeDListsEntry entry{};
    shape.normal.base.type=ROOM_SHAPE_TYPE_NORMAL; shape.normal.numEntries=1;
    shape.normal.entries=&entry; play.roomCtx.curRoom.roomShape=&shape;
    alignas(Gfx) static const char current[]="__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_centered_v8";
    alignas(Gfx) static const char previousRecentered[]="__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_centered_v7";
    alignas(Gfx) static const char previousCentered[]="__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_centered_v6";
    alignas(Gfx) static const char previousAligned[]="__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_aligned_v5";
    alignas(Gfx) static const char previousEdge[]="__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_edge_v4";
    alignas(Gfx) static const char previousCorner[]="__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_corner_v3";
    alignas(Gfx) static const char previous[]="__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_stump_v2";
    alignas(Gfx) static const char legacy[]="__OTR__custom/cor_mm3d/lost_woods_poc1/room_0/opa_combo_return_v1";
    alignas(Gfx) static const char native[]="__OTR__scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_00DL_005788";
    gSaveContext.gameMode=GAMEMODE_NORMAL; gSaveContext.fileNum=0;
    gSaveContext.save.entrance=ENTRANCE(LOST_WOODS,0);
    Vec3f stump={-695,0,-313}, approach={-651,0,-295}, oldArrival={-417,0,-122};
    Vec3f edge={-660,0,-350};
    Vec3f cutStump={-390,0,120};
    Vec3f middleTree={120,0,240};
    entry.opa=(Gfx*)current;
    require(ComboOpeningForest_ReturnPackVersion(&play)==8,"the right-nudge archive must be recognized as v8");
    require(ComboOpeningForest_ReturnYaw(&play)==12288,"the trigger and camera must follow the 22.5-degree doorway turn");
    require(ComboOpeningForest_AtReturn(&play,&stump),"the current doorway must return at the marked room-edge trunk");
    require(ComboOpeningForest_UsesPackTunnel(&play),"the stump pack must suppress the temporary arch");
    require(!ComboOpeningForest_AtReturn(&play,&approach),"walking beside the stump must not teleport");
    require(!ComboOpeningForest_AtReturn(&play,&oldArrival),"the retained native arrival must not bounce");
    require(!ComboOpeningForest_AtReturn(&play,&middleTree),"the abandoned middle doorway must not leave a trigger");
    require(!ComboOpeningForest_AtReturn(&play,&cutStump),"the rejected small cut-stump position must not leave a trigger");
    require(!ComboOpeningForest_AtReturn(&play,&edge),"the former unaligned anchor must not leave an extra trigger");
    const auto original=gSaveContext.save;
    for (int day=1;day<=3;++day) for(int form=0;form<PLAYER_FORM_MAX;++form) {
        gSaveContext.save.day=day; gSaveContext.save.playerForm=form;
        require(ComboOpeningForest_AtReturn(&play,&stump),"the return must remain available on every day/form");
        for(auto point : {Vec3f{-695,150,-313},Vec3f{-695,-50,-313},Vec3f{-668,0,-378},Vec3f{-762,0,-341}})
            require(!ComboOpeningForest_AtReturn(&play,&point),"stump bounds must exclude its wall, roof and ground below");
    }
    gSaveContext.save=original;
    entry.opa=(Gfx*)legacy;
    require(ComboOpeningForest_UsesPackTunnel(&play) && ComboOpeningForest_AtReturn(&play,&oldArrival),
            "an existing v1 pack must retain its visible return location");
    require(!ComboOpeningForest_AtReturn(&play,&stump),"a legacy visible doorway must not leave an invisible stump trigger");
    entry.opa=(Gfx*)previous;
    require(ComboOpeningForest_UsesPackTunnel(&play) && ComboOpeningForest_AtReturn(&play,&middleTree),
            "the previous v2 pack must retain its visible middle-tree trigger");
    require(!ComboOpeningForest_AtReturn(&play,&stump),"v2 must not use an invisible corner trigger");
    entry.opa=(Gfx*)previousCorner;
    require(ComboOpeningForest_ReturnPackVersion(&play)==3 && ComboOpeningForest_AtReturn(&play,&cutStump),
            "an older v3 archive must keep its visible small-stump trigger");
    require(!ComboOpeningForest_AtReturn(&play,&stump),"v3 must not leave an invisible room-edge trigger");
    entry.opa=(Gfx*)previousEdge;
    require(ComboOpeningForest_ReturnPackVersion(&play)==4 && ComboOpeningForest_AtReturn(&play,&edge),
            "the older v4 archive must retain its original edge position");
    require(ComboOpeningForest_ReturnYaw(&play)==16384,"v4 must retain its original doorway angle");
    entry.opa=(Gfx*)previousAligned;
    const Vec3f aligned=ComboOpeningForest_ReturnAnchor(&play);
    require(ComboOpeningForest_ReturnPackVersion(&play)==5 && aligned.x==-677 && aligned.z==-357 &&
            ComboOpeningForest_ReturnYaw(&play)==12288,"v5 must retain its prior aligned pose");
    entry.opa=(Gfx*)previousCentered;
    const Vec3f centered=ComboOpeningForest_ReturnAnchor(&play);
    require(ComboOpeningForest_ReturnPackVersion(&play)==6 && centered.x==-684 && centered.z==-340 &&
            ComboOpeningForest_ReturnYaw(&play)==12288,"v6 must retain its prior centered pose");
    entry.opa=(Gfx*)previousRecentered;
    const Vec3f recentered=ComboOpeningForest_ReturnAnchor(&play);
    require(ComboOpeningForest_ReturnPackVersion(&play)==7 && recentered.x==-698 && recentered.z==-306 &&
            ComboOpeningForest_ReturnYaw(&play)==12288,"v7 must retain its previous pose");
    entry.opa=(Gfx*)native;
    require(!ComboOpeningForest_UsesPackTunnel(&play) && ComboOpeningForest_AtReturn(&play,&stump),
            "native assets must use the temporary arch at the same stump");
    alignas(Gfx) static const char nativePortal[]="__OTR__custom/comboship/native_forest/portal_root";
    RoomShapeCullableEntry nativeEntries[3]{};
    shape.cullable.base.type=ROOM_SHAPE_TYPE_CULLABLE;
    shape.cullable.numEntries=3; shape.cullable.entries=nativeEntries;
    nativeEntries[0].opa=(Gfx*)native;
    require(ComboOpeningForest_ReturnPackVersion(&play)==0,
            "ordinary stock culling entries must retain the procedural fallback");
    nativeEntries[2].opa=(Gfx*)nativePortal;
    require(ComboOpeningForest_ReturnPackVersion(&play)==9 && ComboOpeningForest_UsesPackTunnel(&play),
            "the native scene portal must suppress the duplicate procedural arch");
    const Vec3f nativeAnchor=ComboOpeningForest_ReturnAnchor(&play);
    require(nativeAnchor.x==-695 && nativeAnchor.y==0 && nativeAnchor.z==-313 &&
            ComboOpeningForest_ReturnYaw(&play)==12288 && ComboOpeningForest_AtReturn(&play,&stump),
            "the native cullable portal must retain the accepted doorway and trigger pose");
    const float nativeAngle=12288*(6.28318530718f/65536.0f);
    Vec3f nativeJamb={-695+52*std::cos(nativeAngle)-20*std::sin(nativeAngle),0,
                      -313-52*std::sin(nativeAngle)-20*std::cos(nativeAngle)};
    require(!ComboOpeningForest_AtReturn(&play,&nativeJamb),
            "the native arch jamb must not teleport outside its 44-unit mouth");
    nativeEntries[2].opa=nullptr; nativeEntries[1].xlu=(Gfx*)nativePortal;
    require(ComboOpeningForest_ReturnPackVersion(&play)==0,
            "a portal name in an unrelated translucent slot must not suppress the fallback");
    nativeEntries[2].opa=(Gfx*)nativePortal;
    shape.cullable.entries=nullptr;
    require(ComboOpeningForest_ReturnPackVersion(&play)==0,"an incomplete cullable mesh must retain its fallback");
    shape.cullable.entries=nativeEntries;
    play.roomCtx.curRoom.num=1;
    require(!ComboOpeningForest_AtReturn(&play,&stump),"the second room must retain its native onward route");
    play.roomCtx.curRoom.num=0; gSaveContext.sceneLayer=1;
    require(!ComboOpeningForest_AtReturn(&play,&stump),"flashbacks must not receive a return trigger");
    require(std::memcmp(&original,&gSaveContext.save,sizeof(original))==0,"checking the return cannot change save progress");
    std::puts("PASS right-nudge return: native cullable portal, v8 active mesh/yaw, exact rotated bounds, stock fallback, v1-v7 compatibility, every day/form, progress preserved");
}
'''
with tempfile.TemporaryDirectory(prefix='forest-stump-') as temporary:
    path=Path(temporary); cpp=path/'test.cpp'; binary=path/'test'; cpp.write_text(source)
    includes=['mm','mm/2s2h','mm/include','mm/include/PR','mm/src','mm/assets','libultraship/include','combo']
    command=[os.environ.get('CXX','c++'),'-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2',
             *['-I'+str(ROOT/p) for p in includes]]
    if args.sanitize: command+=['-g','-fsanitize=address,undefined','-fno-omit-frame-pointer','-no-pie']
    subprocess.run([*command,str(cpp),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True,env=dict(os.environ,ASAN_OPTIONS='detect_leaks=0'))
