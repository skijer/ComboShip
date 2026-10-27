// Native weather remains available to songs and the Rod of Seasons, independently of the Storm wand.
#include "global.h"
#include "din_fire_sword.h"
#include "GameInteractor/GameInteractor.h"
#include "overlays/actors/ovl_En_Okarina_Effect/z_en_okarina_effect.h"
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <vector>

extern ActorProfile En_Okarina_Effect_Profile;
int currentActorListIndex = -1;
ActorOverlay gActorOverlayTable[ACTOR_ID_MAX]{};
uintptr_t gSegments[NUM_SEGMENTS]{};
LightningStrike gLightningStrike{};
std::vector<void*> allocations;
PlayState* gPlayState;
int finishFatalCalls;
Actor* finishActor;
PlayState* finishPlay;
// Elemental presentation is a boundary; native actor rejection remains real.
void DinFireSword_FinishFatal(PlayState* play, Actor* actor) {
    ++finishFatalCalls; finishPlay = play; finishActor = actor;
}

#define FLAGS (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_UPDATE_DURING_OCARINA)

ActorProfile* Actor_LoadOverlay(ActorContext*, s16 id) {
    assert(id == ACTOR_EN_OKARINA_EFFECT);
    return &En_Okarina_Effect_Profile;
}
void Actor_FreeOverlay(ActorOverlay*) {}
void* ZeldaArena_Malloc(size_t size) {
    void* allocation = std::calloc(1, size);
    allocations.push_back(allocation);
    return allocation;
}
void SetActorListIndex(Actor*, s32) {}
bool GameInteractor_Should(GIVanillaBehavior, uint32_t result, ...) { return result; }
bool GameInteractor_ShouldActorInit(Actor*) { return true; }
void GameInteractor_ExecuteOnActorInit(Actor*) {}
void GameInteractor_ExecuteOnActorKill(Actor*) {}
void Math_Vec3f_Copy(Vec3f* dest, Vec3f* src) { *dest = *src; }
void CollisionCheck_InitInfo(CollisionCheckInfo*) {}
void ActorShape_Init(ActorShape* shape, f32 offset, ActorShadowFunc shadow, f32 scale) {
    shape->yOffset = offset;
    shape->shadowDraw = shadow;
    shape->shadowScale = scale;
}
s32 Flags_GetClear(PlayState*, s32) { return false; }
void Environment_PlayStormNatureAmbience(PlayState*) {}
void Environment_StopStormNatureAmbience(PlayState*) {}
s32 FrameAdvance_IsEnabled(PlayState*) { return false; }

#include "production.inc"

int main() {
    Player player{};
    PlayState play{};
    gPlayState = &play;
    play.objectCtx.numEntries = 1;
    play.objectCtx.slots[0].id = GAMEPLAY_KEEP;

    Actor* spawned = Actor_Spawn(&play.actorCtx, &play, ACTOR_EN_OKARINA_EFFECT, 0, -30, 0, 0, 0, 0, 1);
    Actor* accepted = play.actorCtx.actorLists[ACTORCAT_ITEMACTION].first;
    assert(spawned == accepted && accepted->init == nullptr && accepted->update != nullptr);
    accepted->update(accepted, &play);
    assert(play.envCtx.precipitation[PRECIP_SOS_MAX] == 60);

    play.envCtx.precipitation[PRECIP_RAIN_CUR] = 1;
    spawned = Actor_Spawn(&play.actorCtx, &play, ACTOR_EN_OKARINA_EFFECT, 0, -30, 0, 0, 0, 0, 1);
    Actor* rejected = accepted->next;
    assert(rejected != nullptr && rejected->update == nullptr);
    assert(spawned == rejected && "native weather still rejects a second rain owner during Init");
    assert(finishFatalCalls == 1 && finishPlay == &play && finishActor == rejected);

    for (void* allocation : allocations) std::free(allocation);
    std::cout << "PASS native weather spawn success and Init rejection, independent of wand dispatch\n";
}
