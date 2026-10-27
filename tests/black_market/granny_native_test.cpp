#include "global.h"
#include "combo/BlackMarketGranny.h"
#include <cassert>
#include <cstdio>

namespace SOH { class Scene; }
namespace BlackMarketGranny {
struct Decoration { s16 id,x,y,z,rx,ry,rz,params; };
constexpr char kErrorCvar[] = "error";
int16_t MmFlameParams(int16_t value) { return (value & 0x3ff) | ((value & 0xc00)<<1); }
struct Resources {
    bool available=true;
    std::array<std::array<uint8_t,22>,4> lights{};
    std::array<Decoration,2> decorations{{{8,130,76,-8,0,0,0,0x7f5},{8,-139,76,-8,0,0,0,0x7f5}}};
    bool Prepare(const char*) { return available; }
    const char* Route() { return "test"; }
    const auto& Lights() { return lights; }
    const auto& Decorations() { return decorations; }
    Gfx* Opaque() { static Gfx list[1]{}; return list; }
    Gfx* Translucent() { return Opaque(); }
};
void Wrap(Gfx*,Gfx*,Gfx*,Gfx*,Gfx*,Gfx*,const char*) {}
}
PlayState* gPlayState;
static bool inside=true, requested=false;
bool Interior() { return inside; }
int CVarGetInteger(const char*,int) { return requested; }
void CVarSetString(const char*,const char*) {}
static int spawned;
static Actor actors[2]{};
static Actor* Spawn(ActorContext*,PlayState*,s16 id,f32 x,f32 y,f32 z,s16,s16,s16,s16 params) {
    assert(id==ACTOR_EN_LIGHT && y==76 && z==-8);
    assert(x==(spawned%2 ? -139 : 130));
#ifdef OOT_GRANNY_TEST
    assert(params==0x7f5);
#else
    assert(params==0xbf5);
#endif
    return &actors[(spawned++)%2];
}
#define Actor_Spawn Spawn
#define Actor_Kill(...) ((void)0)
static Gfx arena[64]{};
#define Graph_Alloc(...) arena
#undef GRAPH_ALLOC
#define GRAPH_ALLOC(...) arena
#define Gfx_TexScrollEx(...) arena
#define Gfx_TwoTexScrollEx(...) arena
/* PRODUCTION */
int main() {
    PlayState play{}; gPlayState=&play;
#ifdef OOT_GRANNY_TEST
    MeshHeader original{}; play.roomCtx.curRoom.meshHeader=&original;
#else
    RoomShape original{}; play.roomCtx.curRoom.roomShape=&original;
#endif
    play.roomCtx.curRoom.num=0;
    GrannySceneBegin(); ApplyGrannyInterior(); assert(!sGrannyActive && spawned==0);
    requested=true; GrannySceneBegin(); sGrannyResources.available=false;
    ApplyGrannyInterior(); assert(!sGrannyActive && spawned==0);
    sGrannyResources.available=true; GrannySceneBegin(); ApplyGrannyInterior();
    assert(sGrannyActive && spawned==2 && play.envCtx.numLightSettings==4);
    assert(play.envCtx.lightSettingsList==sGrannyLights.data());
    ApplyGrannyInterior(); assert(spawned==2);
    requested=false; ApplyGrannyInterior(); assert(sGrannyActive); // next entry, not a live mutation
    DrawGrannyRoom(); assert(spawned==2);
#ifdef OOT_GRANNY_TEST
    assert(play.roomCtx.curRoom.meshHeader==&sGrannyShape);
    assert(original.polygon0.start==nullptr && sGrannyShape.polygon0.num==1);
    assert(sGrannyEntry.opa && sGrannyEntry.xlu);
#else
    assert(play.roomCtx.curRoom.roomShape==&sGrannyShape);
    assert(original.normal.entries==nullptr && sGrannyShape.normal.numEntries==1);
    assert(sGrannyEntry.opa && sGrannyEntry.xlu);
#endif
    inside=false; GrannySceneBegin(); ApplyGrannyInterior(); assert(!sGrannyActive && spawned==2);
    inside=true; GrannySceneBegin(); ApplyGrannyInterior(); assert(!sGrannyActive && spawned==2);
    std::puts("PASS: real native room shape; four runtime lights; exactly two mapped flames; next-entry latch; missing-pack fallback");
}
