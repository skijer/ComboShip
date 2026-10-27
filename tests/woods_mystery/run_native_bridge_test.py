#!/usr/bin/env python3
"""Execute native-wall ownership, fail-closed patching and restoration in production."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
header = root / 'mm/2s2h/Enhancements/Graphics/WoodsMysteryNativeBridge.h'
assert header.exists(), 'The native entrance must verify and restore its owned wall patch'
source = r'''
#include "2s2h/Enhancements/Graphics/WoodsMysteryNativeBridge.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <sanitizer/asan_interface.h>
SaveContext gSaveContext{};
constexpr char kMockCanopy[]="__OTR__scenes/nonmq/Z2_26SARUNOMORI/Z2_26SARUNOMORI_room_01DL_002968";
constexpr char kMockCanopyVertices[]="__OTR__custom/cor_woods_bridge/canopy_vtx";
static bool altEnabled=false, resources=true, patchAllowed=true, customWall=false, bakedLoads=true;
static bool customCanopy=false;
static bool shortenOnNextLoad=false;
static int rejectedIndex=-1;
static const char* missing=nullptr;
static int patchCalls=0, unpatchCalls=0;
static Gfx nativeA[1]={{}}, nativeB[1]={{}}, xmlWall[1]={{}}, baked[1]={{}};
static Gfx *current=nativeA, *saved=nullptr;
static Gfx original{};
static Gfx canopy[57]{},canopyReloaded[57]{},shortCanopy[57]{},canopyOriginal[57]{};
static Gfx* currentCanopy=canopy;
static size_t canopySlots=57,vertexCount=38;
struct CanopyPatch {Gfx* resource;int index;Gfx original;};
static std::map<std::string,CanopyPatch> canopyPatches;
extern "C" bool ResourceMgr_IsAltAssetsEnabled() { return altEnabled; }
extern "C" uint8_t ResourceMgr_FileExists(const char* path) {
    return resources && (!missing || std::strcmp(path,missing)!=0);
}
extern "C" uint8_t ResourceMgr_FileAltExists(const char*) { return 0; }
extern "C" Gfx* ResourceMgr_LoadGfxByName(const char* path) {
    if(std::strcmp(path,WoodsMysteryNativeBridge::kNativeWall)==0) return current;
    if(std::strcmp(path,kMockCanopy)==0) {
        if(shortenOnNextLoad) {currentCanopy=shortCanopy;canopySlots=2;shortenOnNextLoad=false;}
        return currentCanopy;
    }
    return bakedLoads ? baked : nullptr;
}
extern "C" size_t ResourceGetSizeByName(const char* path) {
    if(!ResourceMgr_FileExists(path)) return 0;
    if(std::strcmp(path,kMockCanopy)==0) return canopySlots*sizeof(Gfx);
    if(std::strcmp(path,kMockCanopyVertices)==0) return bakedLoads?vertexCount*sizeof(Vtx):0;
    return 0;
}
extern "C" uint8_t ResourceGetIsCustomByName(const char* path) {
    return std::strcmp(path,kMockCanopy)==0?customCanopy:customWall;
}
extern "C" void ResourceMgr_PatchGfxByName(const char* path,const char* name,int index,Gfx instruction) {
    ++patchCalls;
    if(std::strcmp(path,kMockCanopy)==0) {
        if(!patchAllowed || customCanopy || !currentCanopy || index<0 || size_t(index)>=canopySlots || index==rejectedIndex) return;
        auto found=canopyPatches.find(name);
        if(found==canopyPatches.end()) canopyPatches.emplace(name,CanopyPatch{currentCanopy,index,currentCanopy[index]});
        else if(found->second.resource!=currentCanopy || found->second.index!=index) {
            found->second.resource[found->second.index]=found->second.original;
            found->second={currentCanopy,index,currentCanopy[index]};
        }
        currentCanopy[index]=instruction;return;
    }
    assert(std::strcmp(path,WoodsMysteryNativeBridge::kNativeWall)==0 && index==0);
    if(!patchAllowed || customWall || !current) return;
    if(saved!=current) {
        if(saved) saved[0]=original;
        saved=current;original=current[0];
    }
    current[0]=instruction;
}
extern "C" void ResourceMgr_UnpatchGfxByName(const char* path,const char* name) {
    ++unpatchCalls;
    if(std::strcmp(path,kMockCanopy)==0) {
        auto found=canopyPatches.find(name);
        if(found!=canopyPatches.end()) {
            found->second.resource[found->second.index]=found->second.original;canopyPatches.erase(found);
        }
        return;
    }
    assert(std::strcmp(path,WoodsMysteryNativeBridge::kNativeWall)==0);
    if(saved) saved[0]=original;
    saved=nullptr;
}
static void AssertCanopyRestored() {
    assert(canopyPatches.empty() && std::memcmp(canopy,canopyOriginal,sizeof(canopy))==0);
}
static void AssertCanopyPatched() {
    for(int index : {28,49}) {
        assert(currentCanopy[index].words.w0==(uintptr_t(G_VTX_OTR_FILEPATH)<<24));
        assert(std::strcmp(reinterpret_cast<const char*>(currentCanopy[index].words.w1),kMockCanopyVertices)==0);
    }
    assert(currentCanopy[29].words.w0==12 && currentCanopy[29].words.w1==0);
    assert(currentCanopy[50].words.w0==18 && currentCanopy[50].words.w1==12);
    assert(canopyPatches.size()==4);
    for(int i=0;i<57;++i) if(i!=28&&i!=29&&i!=49&&i!=50)
        assert(std::memcmp(&currentCanopy[i],&canopyOriginal[i],sizeof(Gfx))==0);
}
int main() {
    using namespace WoodsMysteryNativeBridge;
    PlayState play{};RoomShape shape{};RoomShapeCullableEntry entries[2]{};
    play.sceneId=SCENE_26SARUNOMORI;play.roomCtx.curRoom.num=1;
    play.roomCtx.curRoom.roomShape=&shape;shape.base.type=ROOM_SHAPE_TYPE_CULLABLE;
    shape.cullable.numEntries=2;shape.cullable.entries=entries;
    entries[0].opa=(Gfx*)kNativeWall;
    gSaveContext.gameMode=GAMEMODE_NORMAL;
    const SaveContext save=gSaveContext;
    const PlayState originalPlay=play;
    // The declared short resource owns only two commands. Poison its backing
    // guard area so a fixed-index read cannot accidentally land in another global.
    __asan_poison_memory_region(shortCanopy+2,sizeof(shortCanopy)-2*sizeof(Gfx));
    for(int i=0;i<57;++i) {canopy[i].words.w0=0xAB000000+i;canopy[i].words.w1=i+1000;}
    // The hash interpreter has already cached these native vertex pointers.
    canopy[28].words.w0=0x3200C018;canopy[28].words.w1=0x12345678;
    canopy[49].words.w0=0x32012024;canopy[49].words.w1=0x23456789;
    for(int i : {29,50}) {canopy[i].words.w0=0x6D7AC1D5;canopy[i].words.w1=0xC35928A3;}
    std::memcpy(canopyOriginal,canopy,sizeof(canopy));
    std::memcpy(canopyReloaded,canopy,sizeof(canopy));
    nativeA[0].words.w0=101;nativeB[0].words.w0=202;xmlWall[0].words.w0=303;
    assert(!Active(&play));
    resources=false;Update(&play,true);assert(!Active(&play) && nativeA[0].words.w0==101);
    resources=true;Update(&play,true);assert(Active(&play) && saved==nativeA);
    assert(current[0].words.w1==(uintptr_t)kBakedWall);
    AssertCanopyPatched();
    // Alt on/off without a reload must both work with native-layout walls.
    altEnabled=true;assert(!Active(&play));Update(&play,true);assert(Active(&play));
    altEnabled=false;Update(&play,true);assert(Active(&play));
    // Reloading the same path rebinds the patch and restores the old instance.
    current=nativeB;currentCanopy=canopyReloaded;Update(&play,true);assert(Active(&play));
    assert(nativeA[0].words.w0==101 && std::memcmp(canopy,canopyOriginal,sizeof(canopy))==0);
    AssertCanopyPatched();
    // XML Alt walls are rejected by the patch API and never enable the effect.
    current=xmlWall;customWall=true;altEnabled=true;
    Update(&play,true);assert(!Active(&play));assert(nativeB[0].words.w0==202 && xmlWall[0].words.w0==303);
    assert(std::memcmp(canopyReloaded,canopyOriginal,sizeof(canopyReloaded))==0);
    customWall=false;current=nativeA;currentCanopy=canopy;altEnabled=false;
    AssertCanopyRestored();
    // Bounds checks precede every read of the four patch command indices.
    shortenOnNextLoad=true;Update(&play,true);assert(!Active(&play));AssertCanopyRestored();
    currentCanopy=shortCanopy;canopySlots=2;
    Update(&play,true);assert(!Active(&play) && nativeA[0].words.w0==101);AssertCanopyRestored();
    customCanopy=true;Update(&play,true);assert(!Active(&play));AssertCanopyRestored();
    currentCanopy=canopy;canopySlots=57;Update(&play,true);assert(!Active(&play));AssertCanopyRestored();
    customCanopy=false;
    currentCanopy=nullptr;Update(&play,true);assert(!Active(&play));AssertCanopyRestored();currentCanopy=canopy;
    canopy[28].words.w0=0;Update(&play,true);assert(!Active(&play) && nativeA[0].words.w0==101);
    canopy[28]=canopyOriginal[28];AssertCanopyRestored();
    for(int index : {28,29,49,50}) {
        rejectedIndex=index;Update(&play,true);
        assert(!Active(&play) && nativeA[0].words.w0==101);AssertCanopyRestored();
    }
    rejectedIndex=-1;
    vertexCount=37;Update(&play,true);assert(!Active(&play));AssertCanopyRestored();vertexCount=38;
    patchAllowed=false;Update(&play,true);assert(!Active(&play) && nativeA[0].words.w0==101);
    patchAllowed=true;bakedLoads=false;Update(&play,true);assert(!Active(&play));
    bakedLoads=true;
    // Every generated resource is required before any native wall is changed.
    for(const char* path : {kBakedWall,kBakedWallVertices,kBridge,kBridgeVertices,kBark,kPlanks,kBlack,kMockCanopyVertices,kMockCanopy}) {
        missing=path;int before=patchCalls;Update(&play,true);
        assert(!Active(&play) && patchCalls==before && nativeA[0].words.w0==101);
    }
    missing=nullptr;Update(&play,true);assert(Active(&play));
    canopy[50].words.w0=333;assert(!Active(&play));Update(&play,true);
    assert(!Active(&play) && nativeA[0].words.w0==101);AssertCanopyRestored();
    Update(&play,true);assert(Active(&play));AssertCanopyPatched();
    resources=false;Update(&play,true);assert(!Active(&play) && nativeA[0].words.w0==101);resources=true;
    // A loaded makeover room owns the bridge even during an Alt toggle.
    entries[1].opa=(Gfx*)WoodsMysteryMakeover::kBridgeMesh;
    for(bool alt : {false,true}) {
        altEnabled=alt;Update(&play,true);assert(!Active(&play) && nativeA[0].words.w0==101);
    }
    entries[1].opa=nullptr;altEnabled=false;Update(&play,true);assert(Active(&play));
    Update(&play,false);assert(!Active(&play) && nativeA[0].words.w0==101);
    Update(&play,true);assert(Active(&play));
    play.roomCtx.curRoom.num=2;Update(&play,true);assert(!Active(&play) && nativeA[0].words.w0==101);
    play.roomCtx.curRoom.num=1;play.sceneId=SCENE_LOST_WOODS;Update(&play,true);assert(!Active(&play));
    play.sceneId=SCENE_26SARUNOMORI;gSaveContext.gameMode=GAMEMODE_TITLE_SCREEN;Update(&play,true);assert(!Active(&play));
    gSaveContext.gameMode=GAMEMODE_NORMAL;
    shape.base.type=ROOM_SHAPE_TYPE_NORMAL;Update(&play,true);assert(!Active(&play));
    shape.base.type=ROOM_SHAPE_TYPE_CULLABLE;entries[0].opa=(Gfx*)"__OTR__other";Update(&play,true);assert(!Active(&play));
    entries[0].opa=(Gfx*)kNativeWall;shape.cullable.numEntries=0;Update(&play,true);assert(!Active(&play));
    shape.cullable.numEntries=2;Update(&play,true);assert(Active(&play));
    Reset();assert(!Active(&play) && nativeA[0].words.w0==101);
    AssertCanopyRestored();
    Update(nullptr,true);assert(!Active(nullptr));
    assert(std::memcmp(&save,&gSaveContext,sizeof(save))==0);
    assert(std::memcmp(&originalPlay,&play,sizeof(play))==0);
    assert(shape.cullable.entries==entries && shape.cullable.numEntries==2);
    assert(unpatchCalls>0);
    __asan_unpoison_memory_region(shortCanopy,sizeof(shortCanopy));
    std::puts("PASS: production wall/canopy patches redirect cached native batches, preserve cull/material commands, bound short lists, fail closed and restore as one unit across every missing/custom/failed/Alt/reload/pack/mode/room/scene path");
}
'''
with tempfile.TemporaryDirectory(prefix='woods-native-bridge-') as td:
    directory = Path(td)
    (directory / 'test.cpp').write_text(source)
    command = [os.environ.get('CXX', 'c++'), '-std=c++20', '-w', '-DCOMBO_BUILD', '-DF3DEX_GBI_2', '-DFMT_HEADER_ONLY', '-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
    command += ['-I' + str(root / path) for path in ['mm', 'mm/2s2h', 'mm/include', 'mm/include/PR', 'mm/src', 'mm/assets', 'libultraship/include', 'combo']]
    subprocess.run([*command, str(directory / 'test.cpp'), '-o', str(directory / 'test')], check=True)
    subprocess.run([str(directory / 'test')], check=True, env=dict(os.environ, ASAN_OPTIONS='detect_leaks=0'))
