/**
 * wand_storm.c — Storm Rod (Skijer's NEI).
 *
 * Two casts in one rod, picked by whether Link has something locked on:
 *
 *   no lock-on  — the Song of Storms without the song. En_Okarina_Effect is what rains, thunders
 *                 and pulses the env flag the grottos and spots watch, so that half is one spawn.
 *   locked on   — a thunder ray, the yellow beam Barinade strings between its Baris.
 *
 * Barinade is an OoT boss and object_bv is not in this game, so the beam is loaded out of the
 * companion oot.o2r the same way the Light Rod loads its orb. Without that archive the ray still
 * flies and still hits — it just has nothing to draw.
 */

#include "../../helpers/combat_helper.h" // the ray's AT cylinder
#include "mods/oot_asset_loader/oot_asset_loader.h"

// Defined in z_player.c further down this same translation unit, and in no header — every consumer
// declares it for itself (cane_pacci.c, equip_byrna.c, the elemental rods).
extern bool Player_IsZTargeting(Player* this);

// Oceff_Storm is deliberately NOT spawned alongside the weather: its Destroy calls Magic_Reset,
// which would wipe the very meter this rod was just charged against.
#define WAND_STORM_OKARINA_PARAMS 1
#define WAND_STORM_SPAWN_Y_OFFSET -30.0f

#define STORM_RAY_SPEED 18.0f
#define STORM_RAY_LIFE 40
#define STORM_RAY_SPAWN_HEIGHT 30.0f
#define STORM_RAY_SCALE 0.9f
#define STORM_RAY_RADIUS 22.0f
#define STORM_RAY_HEIGHT 30.0f
#define STORM_RAY_DAMAGE 2

// Barinade's own zap-charge colours: white core, yellow rim.
#define STORM_RAY_ENV_R 255
#define STORM_RAY_ENV_G 255
#define STORM_RAY_ENV_B 50

static struct {
    Vec3f pos;
    Vec3f vel;
    s16 yaw;
    s16 pitch;
    s16 life;
    u8 active;
} sStormRay;

static ColliderCylinder sStormRayCol;
static u8 sStormRayColBuilt = 0;

static Gfx* sStormBeamMaterialDL = NULL;
static Gfx* sStormBeamModelDL = NULL;
static u8 sStormBeamTried = 0;

static u8 WandStorm_LoadBeam(void) {
    if (!sStormBeamTried) {
        sStormBeamTried = 1;
        sStormBeamMaterialDL = (Gfx*)OotAssets_LoadGfx("__OTR__objects/object_bv/gBarinadeDL_0135B0");
        sStormBeamModelDL = (Gfx*)OotAssets_LoadGfx("__OTR__objects/object_bv/gBarinadeDL_013638");
    }
    return (sStormBeamMaterialDL != NULL) && (sStormBeamModelDL != NULL);
}

static void WandStorm_RayConfig(CombatColliderConfig* cfg) {
    cfg->dmgFlags = DMG_ZORA_BOOMERANG;
    cfg->damage = STORM_RAY_DAMAGE;
    cfg->effect = 0;
    cfg->radius = STORM_RAY_RADIUS;
    cfg->height = STORM_RAY_HEIGHT;
}

void WandStorm_Forget(void) {
    sStormRay.active = 0;
}

void WandStorm_Tick(PlayState* play, Player* player) {
    CombatColliderConfig cfg;

    if (!sStormRay.active) {
        return;
    }
    if ((--sStormRay.life <= 0) || (sStormRayCol.base.atFlags & AT_HIT)) {
        sStormRayCol.base.atFlags &= ~AT_HIT;
        sStormRay.active = 0;
        return;
    }

    sStormRay.pos.x += sStormRay.vel.x;
    sStormRay.pos.y += sStormRay.vel.y;
    sStormRay.pos.z += sStormRay.vel.z;

    WandStorm_RayConfig(&cfg);
    if (!sStormRayColBuilt) {
        sStormRayColBuilt = 1;
        Combat_InitCylinder(play, &sStormRayCol, &player->actor, &cfg);
    }
    Combat_UpdateCylinder(&sStormRayCol, &sStormRay.pos, &cfg);
    Combat_RegisterCollider(play, &sStormRayCol);
}

void WandStorm_Draw(PlayState* play) {
    if (!sStormRay.active || !WandStorm_LoadBeam()) {
        return;
    }

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, sStormBeamMaterialDL);
    gDPPipeSync(POLY_XLU_DISP++);
    gDPSetEnvColor(POLY_XLU_DISP++, STORM_RAY_ENV_R, STORM_RAY_ENV_G, STORM_RAY_ENV_B, 255);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255);

    Matrix_Translate(sStormRay.pos.x, sStormRay.pos.y, sStormRay.pos.z, MTXMODE_NEW);
    Matrix_RotateZYX(sStormRay.pitch, sStormRay.yaw, 0, MTXMODE_APPLY);
    Matrix_Scale(STORM_RAY_SCALE, STORM_RAY_SCALE, STORM_RAY_SCALE, MTXMODE_APPLY);

    gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(POLY_XLU_DISP++, sStormBeamModelDL);

    CLOSE_DISPS(play->state.gfxCtx);
}

// Aimed at the lock-on when there is one, straight ahead otherwise.
static u8 WandStorm_FireRay(Player* player, PlayState* play, Actor* target) {
    Vec3f localVel = { 0.0f, 0.0f, STORM_RAY_SPEED };

    if (sStormRay.active) {
        return 0; // one ray at a time
    }

    sStormRay.pos = player->actor.world.pos;
    sStormRay.pos.y += STORM_RAY_SPAWN_HEIGHT;

    if (target != NULL) {
        // focus.pos, not world.pos: that is the point the game itself considers "where you aimed",
        // and it is what the fire/ice/light rods lock onto.
        sStormRay.yaw = Math_Vec3f_Yaw(&sStormRay.pos, &target->focus.pos);
        sStormRay.pitch = Math_Vec3f_Pitch(&sStormRay.pos, &target->focus.pos);
    } else {
        sStormRay.yaw = player->actor.shape.rot.y;
        sStormRay.pitch = 0;
    }

    // The elemental rods' own conversion: a +Z vector taken through RotateY(yaw) then RotateX(pitch).
    // Rebuilding it from Math_SinS by hand gets the pitch sign backwards, which is why it is done
    // with the matrix here too.
    Matrix_Push();
    Matrix_RotateY(BINANG_TO_RAD(sStormRay.yaw), MTXMODE_NEW);
    Matrix_RotateX(BINANG_TO_RAD(sStormRay.pitch), MTXMODE_APPLY);
    Matrix_MultVec3f(&localVel, &sStormRay.vel);
    Matrix_Pop();

    sStormRay.life = STORM_RAY_LIFE;
    sStormRay.active = 1;

    Audio_PlaySoundGeneral(NA_SE_IT_MAGIC_ARROW_SHOT, &player->actor.world.pos, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    return 1;
}

// En_Okarina_Effect kills itself on Init when a scene weather tag already owns the weather, so a
// NULL there is a legitimate "not now" and the cast reports failure rather than eating the magic.
static u8 WandStorm_CallStorm(Player* player, PlayState* play) {
    Actor* storm = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_OKARINA_EFFECT, player->actor.world.pos.x,
                               player->actor.world.pos.y + WAND_STORM_SPAWN_Y_OFFSET, player->actor.world.pos.z, 0, 0,
                               0, WAND_STORM_OKARINA_PARAMS);

    return (storm != NULL);
}

u8 WandStorm_Cast(Player* player, PlayState* play) {
    // Same lock-on test the fire rod uses, update included: a focusActor mid-Actor_Kill is a
    // dangling aim point.
    if (Player_IsZTargeting(player) && (player->focusActor != NULL) && (player->focusActor->update != NULL)) {
        return WandStorm_FireRay(player, play, player->focusActor);
    }
    return WandStorm_CallStorm(player, play);
}
