// Native actor spawn/init/draw/destroy and dynapoly registration stay real.
// Archive decoding, heap allocation and the graphics interpreter are boundaries.
#include "global.h"
#include "din_fire_sword.h"
#include "GameInteractor/GameInteractor.h"
#include "overlays/actors/ovl_Obj_Hunsui/z_obj_hunsui.h"
#include "objects/object_hunsui/object_hunsui.h"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <vector>

static_assert(std::is_same_v<decltype(ObjHunsui::unk_174), f32>);
static_assert(std::is_same_v<decltype(ObjHunsui::unk_178), f32>);
static_assert(std::is_same_v<decltype(ObjHunsui::unk_172), u16>);
static_assert(offsetof(ObjHunsui, dyna) == 0);

extern ActorProfile Obj_Hunsui_Profile;
int currentActorListIndex = -1;
ActorOverlay gActorOverlayTable[ACTOR_ID_MAX]{};
uintptr_t gSegments[NUM_SEGMENTS]{};
std::vector<void*> allocations;
CollisionHeader spoutCollision{};
AnimatedMaterial spoutMaterial{}, unusedMaterial{};
int materialDraws, meshDraws, switchWrites, cutsceneRequests, objectRequests;
bool heapFails, initAllowed = true;
PlayState* gPlayState;
int finishFatalCalls, finishForgetCalls;
Actor* finishActor;
PlayState* finishPlay;
// Elemental presentation is a boundary; native kill/destroy remains real.
void DinFireSword_FinishFatal(PlayState* play, Actor* actor) {
    ++finishFatalCalls; finishPlay = play; finishActor = actor;
}
void DinFireSword_FinishForget(PlayState* play, Actor* actor) {
    ++finishForgetCalls; finishPlay = play; finishActor = actor;
}

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED)
ActorProfile* Actor_LoadOverlay(ActorContext*, s16 id) {
    assert(id == ACTOR_OBJ_HUNSUI && "Water must spawn MM's native water spout, not a stone lift");
    return &Obj_Hunsui_Profile;
}
void Actor_FreeOverlay(ActorOverlay*) {}
void* ZeldaArena_Malloc(size_t size) {
    if (heapFails) return nullptr;
    void* allocation = std::calloc(1, size); allocations.push_back(allocation); return allocation;
}
void SetActorListIndex(Actor*, s32) {}
bool GameInteractor_Should(GIVanillaBehavior, uint32_t result, ...) { return result; }
bool GameInteractor_ShouldActorInit(Actor*) { return initAllowed; }
void GameInteractor_ExecuteOnActorInit(Actor*) {}
void GameInteractor_ExecuteOnActorKill(Actor*) {}
void GameInteractor_ExecuteOnSceneFlagSet(s16, FlagType, u32) { ++switchWrites; }
void GameInteractor_ExecuteOnSceneFlagUnset(s16, FlagType, u32) { ++switchWrites; }
void Math_Vec3f_Copy(Vec3f* dest, Vec3f* src) { *dest = *src; }
void CollisionCheck_InitInfo(CollisionCheckInfo*) {}
void ActorShape_Init(ActorShape* shape, f32 offset, ActorShadowFunc shadow, f32 scale) {
    shape->yOffset = offset; shape->shadowDraw = shadow; shape->shadowScale = scale;
}
s32 Flags_GetClear(PlayState*, s32) { return false; }
s32 Object_SpawnPersistent(ObjectContext* ctx, s16 id) {
    assert(ctx->numEntries < ARRAY_COUNT(ctx->slots));
    assert(id == OBJECT_HUNSUI); ++objectRequests;
    ctx->slots[ctx->numEntries].id = id; return ctx->numEntries++;
}
void CollisionHeader_GetVirtual(CollisionHeader* path, CollisionHeader** header) {
    assert(!std::strcmp((const char*)path, object_hunsui_Colheader_000C74));
    *header = &spoutCollision;
}
// Native C accepts archive path pointers at these resource decoding boundaries.
// Explicit overloads keep that ABI call intact when this fixture compiles as C++.
template <size_t N> void DynaPolyActor_LoadMesh(PlayState* play, DynaPolyActor* actor, const char (*path)[N]) {
    DynaPolyActor_LoadMesh(play, actor, (CollisionHeader*)*path);
}
void* Lib_SegmentedToVirtual(void* path) {
    if (!std::strcmp((const char*)path, object_hunsui_Matanimheader_001888)) return &spoutMaterial;
    assert(!std::strcmp((const char*)path, object_hunsui_Matanimheader_000BF0));
    return &unusedMaterial;
}
s16 CutsceneManager_GetAdditionalCsId(s16) { ++cutsceneRequests; return CS_ID_NONE; }
void ObjHunsui_Init(Actor*, PlayState*);
void ObjHunsui_Destroy(Actor*, PlayState*);
void ObjHunsui_Draw(Actor*, PlayState*);
void ObjHunsui_Reset();
void func_80B9DA60(Actor*, PlayState*);
void ObjHunsui_Update(Actor*, PlayState*) { assert(false && "summoned Water must not run native switch/cutscene updates"); }
void func_80B9CE64(ObjHunsui*, PlayState*) { assert(false); }
void func_80B9D714(ObjHunsui*, PlayState*) { assert(false); }
void func_80B9D2BC(ObjHunsui*, PlayState*) { assert(false); }
void func_80B9D4D0(ObjHunsui*, PlayState*) { assert(false); }
void func_80B9D0FC(ObjHunsui*, PlayState*) { assert(false); }
static void WandWater_GeyserDraw(Actor*, PlayState*) {} // old proxy draw, only for RED against baseline
void Actor_PlaySfx_Flagged(Actor*, u16) {}
void Audio_PlaySfx_AtPosWithFreq(Vec3f*, u16 id, f32 frequency) {
    assert(id == NA_SE_EV_WATER_PILLAR - SFX_FLAG && std::isfinite(frequency));
}
void AnimatedMat_Draw(PlayState*, AnimatedMaterial*) { assert(false); }
void AnimatedMat_DrawXlu(PlayState*, AnimatedMaterial* material) {
    assert(material == &spoutMaterial); ++materialDraws;
}
void Gfx_DrawDListXlu(PlayState*, Gfx* path) {
    assert(!std::strcmp((const char*)path, object_hunsui_DL_000EC0)); ++meshDraws;
}
void Gfx_DrawDListXlu(PlayState* play, const char* path) { Gfx_DrawDListXlu(play, (Gfx*)path); }
Gfx* Gfx_TwoTexScrollEx(GraphicsContext*, s32, u32, u32, s32, s32, s32, u32, u32, s32, s32, s32, s32, s32, s32) {
    assert(false && "mode 1 does not use mode 5/6 scrolling state"); return nullptr;
}
void gSPSegment(void*, int, uintptr_t) { assert(false); }
void FrameInterpolation_RecordOpenChild(const void*, int) {}
void FrameInterpolation_RecordCloseChild() {}
f32 Math_SinS(s16 angle) { return std::sin(angle * M_PI / 32768.0); }
f32 Math_CosS(s16 angle) { return std::cos(angle * M_PI / 32768.0); }
f32 Math_SmoothStepToF(f32* value, f32 target, f32 scale, f32 maxStep, f32 minStep) {
    f32 step = (target - *value) * scale;
    if (step > maxStep) step = maxStep;
    if (step < -maxStep) step = -maxStep;
    if (std::fabs(step) < minStep) step = target - *value;
    *value += step; return target - *value;
}

#include "native_water.inc"

int main() {
    Player player{}; PlayState play{}; GraphicsContext graphics{};
    gPlayState = &play;
    Gfx opa[64]{}, xlu[64]{}, overlay[64]{};
    graphics.polyOpa.p = opa; graphics.polyXlu.p = xlu; graphics.overlay.p = overlay;
    play.state.gfxCtx = &graphics;
    player.actor.world.pos = {20, 100, 40};
    Flags_SetSwitch(&play, 0); switchWrites = 0;
    play.objectCtx.numEntries = ARRAY_COUNT(play.objectCtx.slots);
    for (auto& slot : play.objectCtx.slots) slot.id = GAMEPLAY_KEEP;
    assert(!WandWater_Cast(&player, &play) && objectRequests == 0 && allocations.empty());
    play.objectCtx.numEntries = 0;
    assert(!WandWater_Cast(&player, &play) && objectRequests == 1 && allocations.empty());
    play.objectCtx.slots[0].id = -OBJECT_HUNSUI;
    assert(!WandWater_Cast(&player, &play) && objectRequests == 1 && allocations.empty());
    play.objectCtx.slots[0].id = OBJECT_HUNSUI;
    assert(WandWater_Cast(&player, &play));
    auto* water = (ObjHunsui*)sWaterGeyser; Actor* actor = &water->dyna.actor;
    assert(actor->id == ACTOR_OBJ_HUNSUI && actor->params == 0x1000 && water->unk_160 == OBJHUNSUI_F000_1);
    assert(actor->init == nullptr && actor->update != nullptr && actor->update != ObjHunsui_Update);
    assert(actor->draw == func_80B9DA60 && actor->destroy != nullptr && actor->room == -1);
    assert(actor->scale.x == .1f && actor->scale.y == .1f && actor->scale.z == .1f);
    assert(actor->home.pos.y == 100 && actor->world.pos.z == 110);
    assert(water->unk_178 == 0 && water->unk_174 == 10 && !(water->unk_172 & (2 | 0x40)));
    assert(D_80B9DED8.unk_00 == 0 && D_80B9DED8.unk_01 == 0 && D_80B9DED8.unk_02 == 0);
    const s32 bgId = water->dyna.bgId;
    assert(DynaPoly_GetActor(&play.colCtx, bgId) == &water->dyna);
    assert(play.colCtx.dyna.bgActors[bgId].colHeader == &spoutCollision);
    actor->flags |= ACTOR_FLAG_INSIDE_CULLING_VOLUME;
    actor->draw(actor, &play);
    assert(materialDraws == 1 && meshDraws == 1 && graphics.polyOpa.p == opa && graphics.overlay.p == overlay);
    bool translucent = false;
    for (Gfx* command = xlu; command < graphics.polyXlu.p; ++command) {
        if ((command->words.w0 >> 24) == G_SETPRIMCOLOR) translucent |= (command->words.w1 & 0xff) == 127;
    }
    assert(translucent && "native Water must emit its translucent spout material and mesh");
    actor->update(actor, &play);
    assert(water->unk_178 > 0 && water->unk_178 <= 10 && actor->world.pos.y > 100);
    assert(WandWater_Cast(&player, &play) && sWaterGeyser == actor && water->unk_174 == 240);
    for (int frame = 0; frame < 90; ++frame) actor->update(actor, &play);
    assert(water->unk_178 > 239 && water->unk_178 <= 240 && DynaPoly_GetActor(&play.colCtx, bgId) == &water->dyna);
    assert(WandWater_Cast(&player, &play) && water->unk_174 == 10);
    for (int frame = 0; frame < 90; ++frame) actor->update(actor, &play);
    assert(water->unk_178 >= 10 && water->unk_178 < 11);
    assert(Flags_GetSwitch(&play, 0) && switchWrites == 0 && cutsceneRequests == 0);
    Actor_Kill(actor); Actor_Destroy(actor, &play);
    assert(finishFatalCalls == 1 && finishForgetCalls == 1 && finishPlay == &play && finishActor == actor);
    assert(sWaterGeyser == nullptr && "native external destruction must clear the retained Water pointer before memory is freed");
    assert(actor->destroy == nullptr && water->dyna.bgId == -1 && DynaPoly_GetActor(&play.colCtx, bgId) == nullptr);
    // Once the background system retires the deleted entry, another actor can reuse the slot.
    // Calling the old actor's destroy again cannot unregister its successor.
    play.colCtx.dyna.bgActorFlags[bgId] = 0;
    ObjHunsui successor{}; DynaPolyActor_Init(&successor.dyna, DYNA_TRANSFORM_POS);
    successor.dyna.bgId = DynaPoly_SetBgActor(&play, &play.colCtx.dyna, &successor.dyna.actor, &spoutCollision);
    assert(successor.dyna.bgId == bgId);
    Actor_Destroy(actor, &play);
    assert(finishForgetCalls == 2 && finishPlay == &play && finishActor == actor);
    assert(DynaPoly_GetActor(&play.colCtx, bgId) == &successor.dyna);
    assert(!WandWater_IsAlive());
    WandWater_Forget(); heapFails = true;
    assert(!WandWater_Cast(&player, &play));
    heapFails = false; initAllowed = false;
    assert(!WandWater_Cast(&player, &play) && sWaterGeyser == nullptr);
    assert(Flags_GetSwitch(&play, 0) && switchWrites == 0 && cutsceneRequests == 0);
    initAllowed = true;
    for (auto& flags : play.colCtx.dyna.bgActorFlags) flags = BGACTOR_IN_USE;
    assert(!WandWater_Cast(&player, &play) && sWaterGeyser == nullptr);
    for (void* allocation : allocations) std::free(allocation);
    std::cout << "PASS native Water mode 1 init, translucent spout draw, height toggles, collision destroy and no switch/cutscene ownership\n";
}
