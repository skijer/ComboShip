/**
 * wand_storm.c — Storm Rod (Skijer's NEI).
 *
 * A thunder ray, aimed at a live lock-on or fired straight ahead. Weather belongs to the Song of
 * Storms and the Rod of Seasons; this rod never claims it.
 *
 * Simulation retains the existing MM projectile. Private medallion-derived
 * materials add surface detail; bounded geometry remains when they are absent.
 */

#include "../../helpers/combat_helper.h" // the ray's AT cylinder
#include "2s2h/Rando/NeiAirMagicPresentation.h"

// Defined in z_player.c further down this same translation unit, and in no header — every consumer
// declares it for itself (cane_pacci.c, equip_byrna.c, the elemental rods).
extern bool Player_IsZTargeting(Player* this);

#define STORM_RAY_SPEED 18.0f
#define STORM_RAY_LIFE 40
#define STORM_RAY_SPAWN_HEIGHT 30.0f
#define STORM_RAY_RADIUS 22.0f
#define STORM_RAY_HEIGHT 30.0f
#define STORM_RAY_DAMAGE 2

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
    // The ray position is its center. A cylinder based there only hits above
    // the bolt: a landing Chuchu is 26 units tall and free shots fly at 30.
    sStormRayCol.dim.yShift = -(s16)(STORM_RAY_HEIGHT * 0.5f);
    Combat_RegisterCollider(play, &sStormRayCol);
}

void WandStorm_Draw(PlayState* play) {
    if (!sStormRay.active) {
        return;
    }
    NeiAirMagic_DrawLightning(play, &sStormRay.pos, &sStormRay.vel);
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

u8 WandStorm_Cast(Player* player, PlayState* play) {
    // Same lock-on test the fire rod uses, update included: a focusActor mid-Actor_Kill is a
    // dangling aim point.
    if (Player_IsZTargeting(player) && (player->focusActor != NULL) && (player->focusActor->update != NULL)) {
        return WandStorm_FireRay(player, play, player->focusActor);
    }
    return WandStorm_FireRay(player, play, NULL);
}
