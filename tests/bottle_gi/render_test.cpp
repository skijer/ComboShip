#include <cassert>
// Black Market reporting is an observer outside this rendering fixture.
void NeiGi_ReportBlackMarketModel(const char*, const char*, const char*) {}
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "combo/menu/ComboFairyBottle.h"

using s8 = int8_t;
using s16 = int16_t;
using s32 = int32_t;
using u8 = uint8_t;
using f32 = float;
struct Gfx { struct { uintptr_t w0 = 0, w1 = 0; } words; int stream = 0; };
struct GraphicsContext {};
struct PlayState { struct { GraphicsContext* gfxCtx; uint32_t frames = 0; } state; };
constexpr int G_DL_OTR_FILEPATH = 0x27;
#include "combo/menu/ComboBottleGi.h"

static bool selected = true, otherMod = false, missing = false;
static int partial;
static const char marker[] = "objects/combo_bottle_gi/PotionMarker";
static const char another[] = "objects/some_other_pack/potion";
static Gfx root[3];
Gfx* ResourceMgr_LoadGfxByName(const char* path) {
    const int slot=!std::strcmp(path,"pot")?0:!std::strcmp(path,"liquid")?1:!std::strcmp(path,"shell")?2:-1;
    assert(slot>=0);
    const char* targets[]={marker,"objects/combo_bottle_gi/Liquid","objects/combo_bottle_gi/BottleShell"};
    root[slot].words.w0 = selected ? uintptr_t(G_DL_OTR_FILEPATH) << 24 : uintptr_t(0xDF) << 24;
    root[slot].words.w1 = reinterpret_cast<uintptr_t>(otherMod || (partial&(1<<slot)) ? another : targets[slot]);
    return missing ? nullptr : &root[slot];
}
static const char* selectedEmptyPath = nullptr;
static bool emptyOpaqueAvailable = true, emptyGlassAvailable = true;
int ResourceMgr_IsModAsset(const char* path) {
    return selectedEmptyPath && (!std::strcmp(path, selectedEmptyPath) ||
        (!std::strcmp(selectedEmptyPath,"whole") && (!std::strcmp(path,"cork") || !std::strcmp(path,"glass"))));
}
int ResourceMgr_IsModAssetForGame(const char*,const char*) { return 0; }
int ResourceMgr_IsCustomAssetForGame(const char*,const char*) { return 0; }
int ResourceMgr_FileExists(const char* path) {
    if (!std::strcmp(path,"__OTR__objects/combo_bottle_gi/EmptyOpaque")) return emptyOpaqueAvailable;
    if (!std::strcmp(path,"__OTR__objects/combo_bottle_gi/EmptyXlu")) return emptyGlassAvailable;
    return 0;
}
static GraphicsContext gfx;
static PlayState play{{&gfx, 37}};
static Gfx opa[64], xlu[64];
static Gfx *opaPtr, *xluPtr;
static std::vector<std::pair<int,std::string>> draws;
static int scrolls, matrices;
static void DrawList(Gfx* cmd,const void* path) {
    draws.emplace_back(cmd->stream,static_cast<const char*>(path));
}
static Gfx* Gfx_TwoTexScrollEx(...) { ++scrolls; return root; }
constexpr int G_MTX_MODELVIEW=1, G_MTX_LOAD=2, G_TX_RENDERTILE=0;
constexpr int GID_SONG_GENERIC=0x75;
#define ARRAY_COUNT(x) (sizeof(x)/sizeof((x)[0]))
#define OPEN_DISPS(...) ((void)0)
#define CLOSE_DISPS(...) ((void)0)
#define POLY_OPA_DISP opaPtr
#define POLY_XLU_DISP xluPtr
#define Gfx_SetupDL_25Opa(...) ((void)0)
#define Gfx_SetupDL_25Xlu(...) ((void)0)
#define Gfx_SetupDL25_Opa(...) ((void)0)
#define Gfx_SetupDL25_Xlu(...) ((void)0)
#define MATRIX_NEWMTX(...) nullptr
#define MATRIX_FINALIZE_AND_LOAD(p,ctx) ((void)(p), ++matrices)
#define gSPMatrix(p,m,f) ((void)(p), ++matrices)
#define gSPDisplayList(p,dl) DrawList(p,dl)
#define gSPSegment(p,s,dl) ((void)(p), (void)(dl))

/* PRODUCTION_NATIVE */

static void Reset() {
    opaPtr=opa; xluPtr=xlu; draws.clear(); scrolls=matrices=0;
    for (auto& cmd:opa) cmd.stream=0;
    for (auto& cmd:xlu) cmd.stream=1;
}
static void TestEmptyBottles() {
    for (int host=0;host<2;++host) for(int availability:{3,0,1,2})
        for(int replacement=0;replacement<4;++replacement) {
            emptyOpaqueAvailable=availability&1; emptyGlassAvailable=availability&2;
            selectedEmptyPath=replacement==1?"cork":replacement==2?"glass":replacement==3?"whole":nullptr;
            const bool bundled=availability==3 && !replacement;
            const std::vector<std::pair<int,std::string>> expected={
                {0,bundled?"__OTR__objects/combo_bottle_gi/EmptyOpaque":"cork"},
                {1,bundled?"__OTR__objects/combo_bottle_gi/EmptyXlu":"glass"}};
            Reset();
            if(host)mm::GetItem_DrawOpa0Xlu1(&play,3); else oot::GetItem_DrawOpa0Xlu1(&play,3);
            assert(draws==expected && "empty bottle bypassed authored casing or selected replacement");
            assert(scrolls==0 && matrices==2);
            void* resources[8]{};s32 xs=-1,kind=-1,scroll=-1;float scale=-1;u8 colors[16]{};
            int count=host?mm::GetItem_GetDrawTableEntry(3,resources,8,&xs,&scale,&scroll,&kind):
                           oot::GetItem_GetDrawTableEntry(3,resources,8,&xs,&scale,&kind,colors);
            assert(count==2 && xs==1 && kind==0 && scale==0);
            for(int i=0;i<2;++i)assert(expected[i].second==(const char*)resources[i]);
            for(int capacity=0;capacity<2;++capacity)
                assert((host?mm::GetItem_GetDrawTableEntry(3,resources,capacity,&xs,&scale,&scroll,&kind):
                             oot::GetItem_GetDrawTableEntry(3,resources,capacity,&xs,&scale,&kind,colors))==0);
        }
    selectedEmptyPath=nullptr;
    puts("PASS both native empty drawers and foreign exports: direct private casing despite ROM roots; selected whole/partial mod priority; missing-resource fallback; complete two-pass recipe");
}
int main() {
    TestEmptyBottles();
    for (int host=0;host<2;++host) {
        for (bool alternate:{false,true}) for (bool competing:{false,true}) for(int mask=0;mask<8;++mask) {
            selected=alternate; otherMod=competing; missing=false; partial=mask;
            for (int potion=0;potion<3;++potion) {
                Reset();
                if (host) mm::GetItem_DrawPotion(&play,potion);
                else oot::GetItem_DrawPotion(&play,potion);
                const bool custom=alternate&&!competing&&mask==0;
                if (custom) {
                    assert((draws==std::vector<std::pair<int,std::string>>{
                        {1,"palette"+std::to_string(potion)},{1,"liquid"},{1,"shell"}}) &&
                        "custom liquid must draw in XLU using live owning palette before neutral glass");
                    assert(scrolls==0 && matrices==1 && opaPtr==opa);
                } else {
                    assert(draws.size()==6 && draws[0].first==0 && draws[4].first==1);
                    assert(scrolls==1 && matrices==2);
                }
                void* resources[8]{};
                s32 xs=-1, kind=-1, scroll=-1; float scale=-1; u8 colors[16]{};
                int count=host?mm::GetItem_GetDrawTableEntry(potion,resources,8,&xs,&scale,&scroll,&kind):
                               oot::GetItem_GetDrawTableEntry(potion,resources,8,&xs,&scale,&kind,colors);
                assert(count==(custom?3:6) && xs==(custom?0:4) && kind==(custom?0:5));
                if (custom) {
                    assert(std::string((const char*)resources[0])=="palette"+std::to_string(potion));
                    assert(!std::strcmp((const char*)resources[1],"liquid"));
                    assert(!std::strcmp((const char*)resources[2],"shell"));
                    for(int capacity=0;capacity<3;++capacity)
                        assert((host?mm::GetItem_GetDrawTableEntry(potion,resources,capacity,&xs,&scale,&scroll,&kind):
                                     oot::GetItem_GetDrawTableEntry(potion,resources,capacity,&xs,&scale,&kind,colors))==0);
                }
            }
        }
        missing=true; selected=true; otherMod=false; partial=0;
        Reset();
        if(host) mm::GetItem_DrawPotion(&play,0); else oot::GetItem_DrawPotion(&play,0);
        assert(draws.size()==6); // A missing marker must not switch another pack's recipe.
    }
    puts("PASS both native potion paths and foreign exports: live palette/liquid/glass XLU order, complete recipes, vanilla/competing Alt preservation and missing-marker fallback");
}
