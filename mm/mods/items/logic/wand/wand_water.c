/**
 * wand_water.c — Water Rod (Skijer's NEI).
 *
 * A rideable water column: cast once to raise it, cast again to lower it, and stand on it either way.
 *
 * SoH borrows the Water Temple's En_Siofuki, whose top face is already standable and whose height is
 * a field. There is no Siofuki here. MM's own geyser, Obj_Hunsui, is not a drop-in: it has seven
 * param modes, animated materials and six separate Actor_Kill paths in its init, so driving it from
 * outside means owning all of that. The column is built on Obj_Lift instead — the same DynaPoly the
 * Sand Rod and the cane's platform already ride — with the height driven here and the stone tinted
 * to water. The platform is honest at every height because the collision follows the actor.
 */

#include "objects/object_d_lift/object_d_lift.h" // gDampeGraveBrownElevatorDL

#define WATER_SPAWN_DIST 70.0f
#define WATER_RIDE_HEIGHT 240.0f
#define WATER_REST_HEIGHT 10.0f

// Obj_Lift's smoothing, matched to EnSiofuki's so the column reads as water rather than a lift.
#define WATER_STEP_SCALE 0.8f
#define WATER_STEP_MAX 3.0f
#define WATER_STEP_MIN 0.01f

// The surface never settles: without it the column reads as a block that happens to be blue.
#define WATER_BOB_RATE 0x800
#define WATER_BOB_AMPLITUDE 4.0f

// ObjLift reads its scene switch flag out of (params >> 1) & 0x7F; with params 0 that is flag 0.
#define WATER_SWITCH_FLAG 0

#define WATER_ENV_R 60
#define WATER_ENV_G 150
#define WATER_ENV_B 225

static Actor* sWaterGeyser = NULL;
static f32 sWaterBaseY = 0.0f;
static f32 sWaterCurrentHeight = 0.0f;
static f32 sWaterTargetHeight = 0.0f;

static void WandWater_GeyserUpdate(Actor* thisx, PlayState* play) {
    f32 bob = Math_SinS((s16)(play->gameplayFrames * WATER_BOB_RATE)) * WATER_BOB_AMPLITUDE;

    Math_SmoothStepToF(&sWaterCurrentHeight, sWaterTargetHeight, WATER_STEP_SCALE, WATER_STEP_MAX, WATER_STEP_MIN);
    thisx->world.pos.y = sWaterBaseY + sWaterCurrentHeight + bob;
    Actor_PlaySfx_Flagged(thisx, NA_SE_EV_FOUNTAIN - SFX_FLAG);
}

static void WandWater_GeyserDraw(Actor* thisx, PlayState* play) {
    OPEN_DISPS(play->state.gfxCtx);
    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetEnvColor(POLY_OPA_DISP++, WATER_ENV_R, WATER_ENV_G, WATER_ENV_B, 255);
    CLOSE_DISPS(play->state.gfxCtx);

    Gfx_DrawDListOpa(play, gDampeGraveBrownElevatorDL);
}

// The scene took the geyser with it. The pointer is dropped, never written through.
void WandWater_Forget(void) {
    sWaterGeyser = NULL;
}

// Compares the update pointer, not just NULL: the slot may already belong to something else.
static u8 WandWater_IsAlive(void) {
    return (sWaterGeyser != NULL) && (sWaterGeyser->update == WandWater_GeyserUpdate);
}

u8 WandWater_Cast(Player* player, PlayState* play) {
    s16 yaw = player->actor.shape.rot.y;

    // Already up: the cast is the lift control, so it just flips which way the column is going.
    if (WandWater_IsAlive()) {
        sWaterTargetHeight = (sWaterTargetHeight > WATER_REST_HEIGHT) ? WATER_REST_HEIGHT : WATER_RIDE_HEIGHT;
        return 1;
    }

    if (Object_GetSlot(&play->objectCtx, OBJECT_D_LIFT) < 0) {
        Object_SpawnPersistent(&play->objectCtx, OBJECT_D_LIFT);
        return 0; // not resident yet this frame
    }

    Vec3f pos;
    pos.x = player->actor.world.pos.x + (Math_SinS(yaw) * WATER_SPAWN_DIST);
    pos.y = player->actor.world.pos.y;
    pos.z = player->actor.world.pos.z + (Math_CosS(yaw) * WATER_SPAWN_DIST);

    // ObjLift's init kills itself when the switch flag is already set, so it is lent out and
    // restored around the spawn. Nothing else runs in between — this is the player's own update.
    u8 flagWasSet = Flags_GetSwitch(play, WATER_SWITCH_FLAG) != 0;
    if (flagWasSet) {
        Flags_UnsetSwitch(play, WATER_SWITCH_FLAG);
    }
    sWaterGeyser = Actor_Spawn(&play->actorCtx, play, ACTOR_OBJ_LIFT, pos.x, pos.y, pos.z, 0, yaw, 0, 0);
    if (flagWasSet) {
        Flags_SetSwitch(play, WATER_SWITCH_FLAG);
    }

    if ((sWaterGeyser == NULL) || (sWaterGeyser->update == NULL)) {
        sWaterGeyser = NULL;
        return 0;
    }

    // destroy is left alone: ObjLift's destroy is what unregisters the dynapoly.
    sWaterGeyser->update = WandWater_GeyserUpdate;
    sWaterGeyser->draw = WandWater_GeyserDraw;
    sWaterGeyser->room = -1;

    sWaterBaseY = pos.y;
    sWaterCurrentHeight = 0.0f;
    sWaterTargetHeight = WATER_REST_HEIGHT;
    return 1;
}
