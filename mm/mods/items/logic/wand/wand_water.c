/**
 * wand_water.c — Water Rod (Skijer's NEI).
 *
 * MM's Obj_Hunsui mode 1 supplies the water spout, animated translucent material and rideable
 * dynapoly. The summon owns height control instead of running the native switch/cutscene action.
 * Native draw and destroy remain installed, including collision cleanup.
 */

#include "overlays/actors/ovl_Obj_Hunsui/z_obj_hunsui.h"

#define WATER_SPAWN_DIST 70.0f
#define WATER_RIDE_HEIGHT 240.0f
#define WATER_REST_HEIGHT 10.0f
#define WATER_HUNSUI_PARAMS 0x1000 // mode 1: dynapoly spout, no singleton or switch writes during Init

#define WATER_STEP_SCALE 0.8f
#define WATER_STEP_MAX 3.0f
#define WATER_STEP_MIN 0.01f
#define WATER_BOB_RATE 0x800
#define WATER_BOB_AMPLITUDE 4.0f

static Actor* sWaterGeyser = NULL;
static ActorFunc sWaterNativeDestroy = NULL;

static void WandWater_GeyserDestroy(Actor* thisx, PlayState* play) {
    if (sWaterGeyser == thisx) {
        sWaterGeyser = NULL;
    }
    if (sWaterNativeDestroy != NULL) {
        sWaterNativeDestroy(thisx, play);
    }
}

static void WandWater_GeyserUpdate(Actor* thisx, PlayState* play) {
    ObjHunsui* geyser = (ObjHunsui*)thisx;
    f32 bob = Math_SinS((s16)(play->gameplayFrames * WATER_BOB_RATE)) * WATER_BOB_AMPLITUDE;

    // Native mode-1 draw reads unk_178 for the water sound and the actor transform for its mesh.
    // Keeping both native height fields current also keeps the rideable collision at the surface.
    Math_SmoothStepToF(&geyser->unk_178, geyser->unk_174, WATER_STEP_SCALE, WATER_STEP_MAX, WATER_STEP_MIN);
    thisx->world.pos.y = thisx->home.pos.y + geyser->unk_178 + bob;
}

// The scene took the geyser with it. The pointer is dropped, never written through.
void WandWater_Forget(void) {
    sWaterGeyser = NULL;
}

static u8 WandWater_IsAlive(void) {
    return (sWaterGeyser != NULL) && (sWaterGeyser->update == WandWater_GeyserUpdate);
}

u8 WandWater_Cast(Player* player, PlayState* play) {
    s16 yaw = player->actor.shape.rot.y;

    if (WandWater_IsAlive()) {
        ObjHunsui* geyser = (ObjHunsui*)sWaterGeyser;
        geyser->unk_174 = (geyser->unk_174 > WATER_REST_HEIGHT) ? WATER_REST_HEIGHT : WATER_RIDE_HEIGHT;
        return 1;
    }

    s32 slot = Object_GetSlot(&play->objectCtx, OBJECT_HUNSUI);
    if (slot < 0) {
        if (play->objectCtx.numEntries >= ARRAY_COUNT(play->objectCtx.slots)) {
            return 0;
        }
        Object_SpawnPersistent(&play->objectCtx, OBJECT_HUNSUI);
        return 0;
    }
    if (!Object_IsLoaded(&play->objectCtx, slot)) {
        return 0; // Actor_Init must finish before native fields or collision can be used.
    }

    Vec3f pos;
    pos.x = player->actor.world.pos.x + (Math_SinS(yaw) * WATER_SPAWN_DIST);
    pos.y = player->actor.world.pos.y;
    pos.z = player->actor.world.pos.z + (Math_CosS(yaw) * WATER_SPAWN_DIST);
    Actor* actor =
        Actor_Spawn(&play->actorCtx, play, ACTOR_OBJ_HUNSUI, pos.x, pos.y, pos.z, 0, yaw, 0, WATER_HUNSUI_PARAMS);
    if ((actor == NULL) || (actor->init != NULL) || (actor->update == NULL) || (actor->draw == NULL)) {
        sWaterGeyser = NULL;
        return 0;
    }

    ObjHunsui* geyser = (ObjHunsui*)actor;
    if ((geyser->dyna.bgId < 0) || (geyser->dyna.bgId >= BG_ACTOR_MAX)) {
        Actor_Kill(actor);
        sWaterGeyser = NULL;
        return 0;
    }

    // Mode 1 initializes hidden at -30. Start visible at Link's feet and control only the native
    // height fields. Its draw's water sound stays enabled; no native switch/cutscene action runs.
    geyser->unk_174 = WATER_REST_HEIGHT;
    geyser->unk_178 = 0.0f;
    geyser->unk_172 = 0x10;
    geyser->actionFunc = NULL;
    geyser->csId = CS_ID_NONE;
    actor->update = WandWater_GeyserUpdate;
    sWaterNativeDestroy = actor->destroy;
    actor->destroy = WandWater_GeyserDestroy;
    actor->room = -1;
    sWaterGeyser = actor;
    return 1;
}
