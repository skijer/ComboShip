/**
 * cryonis_rune.c — Cryonis, the Sheikah Slate's third rune. Skijer's NEI.
 *
 * A platform of ice rises out of a water surface and can be stood on and climbed. Only one exists
 * at a time: raising the next one shatters the last, and aiming at the one already standing breaks
 * it instead of placing another.
 *
 * The C press opens an aiming mode that owns the frame the way Ultrahand's does — D-up/D-down
 * choose the distance, A commits, B cancels.
 *
 * SoH builds the pillar out of the Ice Cavern block and has to drive the rise, the collider and the
 * shatter itself. This game already owns the exact actor: Bg_Icefloe is the platform the Ice Arrow
 * leaves on water — a DynaPoly that grows out of the surface on its own, bobs with it, and melts
 * when the water goes. So the rune places one and keeps the aiming layer; the pillar's own life is
 * left to the engine.
 *
 * Consumed via #include from item_sheikah_slate.c, after master_cycle.c. No header.
 */

#include "z64.h"
#include "macros.h"
#include "functions.h"
#include "variables.h"
#include <math.h>
#include "overlays/actors/ovl_Bg_Icefloe/z_bg_icefloe.h"
#include "../items/helpers/equip_helper.h"

// item_sheikah_slate.c, same TU but defined further down: is the tablet actually in Link's hand?
u8 Slate_IsDrawn(void);

// ── Public API ───────────────────────────────────────────────────────────────
s32 Cryonis_Cast(PlayState* play, Player* player);
void Cryonis_Tick(PlayState* play);
u8 Cryonis_ModeUpdate(PlayState* play, Player* player);
u8 Cryonis_ModeActive(void);
u8 Cryonis_IsClimbableBgId(s32 bgId);
void Cryonis_DrawGhost(PlayState* play);

// ── Tuning ───────────────────────────────────────────────────────────────────
#define CRYONIS_DIST_MIN 90.0f
#define CRYONIS_DIST_MAX 420.0f
#define CRYONIS_DIST_START 170.0f
#define CRYONIS_DIST_RATE 7.0f // units per frame with the pad held

// Bg_Icefloe reads its own lifetime out of params. The arrow's floes are meant to expire; a rune
// the player is standing on is not, so this is long enough to be a placed structure.
#define CRYONIS_LIFETIME 4000

// The floe grows to roughly this half-extent, measured off gIcefloePlatformCol's own footprint.
// Only the aim test and the ghost use it, so it does not have to be exact.
#define CRYONIS_HALF_EXTENT 60.0f
#define CRYONIS_GHOST_HEIGHT 40.0f
#define CRYONIS_BREAK_SLACK 20.0f // how far off the platform the aim may sit and still mean "break it"

typedef struct {
    Actor* pillar;
    s32 bgId;
    u8 modeActive;
    f32 dist;
    Vec3f ghostPos;
    u8 ghostValid;
    u8 ghostBreaks; // the aim is on the standing platform: A breaks instead of placing
} CryonisState;

static CryonisState sCryonis = { NULL, BGCHECK_SCENE, 0, CRYONIS_DIST_START, { 0.0f, 0.0f, 0.0f }, 0, 0 };

static void Cryonis_Sfx(u16 sfxId, Vec3f* pos) {
    Audio_PlaySoundGeneral(sfxId, pos, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
}

// ── The pillar ───────────────────────────────────────────────────────────────

static void Cryonis_Forget(void) {
    sCryonis.pillar = NULL;
    sCryonis.bgId = BGCHECK_SCENE;
}

// A pillar killed from elsewhere — a scene change, a room unload, its own melt — leaves a dangling
// pointer and, worse, a bgId the engine will hand to somebody else's collision. Both have to go
// together.
static void Cryonis_Prune(void) {
    if ((sCryonis.pillar != NULL) && (sCryonis.pillar->update == NULL)) {
        Cryonis_Forget();
    }
}

static void Cryonis_Shatter(PlayState* play) {
    Cryonis_Prune();
    if (sCryonis.pillar == NULL) {
        return;
    }

    Vec3f pos = sCryonis.pillar->world.pos;
    Cryonis_Sfx(NA_SE_EV_ICE_BROKEN, &pos);

    // The same shards the floe throws while it grows, so a break reads as the reverse of a raise.
    static Vec3f sShardAccel = { 0.0f, -0.5f, 0.0f };
    for (s32 i = 0; i < 12; i++) {
        Vec3f burst = pos;
        Vec3f velocity;

        burst.x += Rand_CenteredFloat(CRYONIS_HALF_EXTENT);
        burst.y += Rand_ZeroOne() * CRYONIS_GHOST_HEIGHT;
        burst.z += Rand_CenteredFloat(CRYONIS_HALF_EXTENT);
        velocity.x = Rand_CenteredFloat(4.0f);
        velocity.y = 1.0f + (Rand_ZeroOne() * 2.0f);
        velocity.z = Rand_CenteredFloat(4.0f);
        EffectSsIceBlock_Spawn(play, &burst, &velocity, &sShardAccel, Rand_S16Offset(10, 10));
    }

    Actor_Kill(sCryonis.pillar);
    Cryonis_Forget();
}

static u8 Cryonis_Raise(PlayState* play, Vec3f* pos) {
    Actor* pillar =
        Actor_Spawn(&play->actorCtx, play, ACTOR_BG_ICEFLOE, pos->x, pos->y, pos->z, 0, 0, 0, CRYONIS_LIFETIME);

    if (pillar == NULL) {
        return 0;
    }

    sCryonis.pillar = pillar;
    sCryonis.bgId = ((DynaPolyActor*)pillar)->bgId;
    return 1;
}

/**
 * The one climbable-surface question the engine asks. The rune may not touch the collision header
 * it borrows — those are shared and cached — so the answer is by bgId. Comparing the pointer is
 * safe; reading through it is not.
 */
u8 Cryonis_IsClimbableBgId(s32 bgId) {
    return (sCryonis.pillar != NULL) && (bgId == sCryonis.bgId);
}

/**
 * Every frame from the slate's tick, ahead of its early returns. The pillar is a real actor and the
 * scene unload takes it with no word to us, so the scene number is the only reliable notice that
 * the pointer — and the bgId, which the engine hands straight back out — has gone stale.
 */
void Cryonis_Tick(PlayState* play) {
    static s16 sLastScene = -1;

    if (sLastScene != play->sceneId) {
        sLastScene = play->sceneId;
        sCryonis.modeActive = 0;
        Cryonis_Forget();
        return;
    }
    Cryonis_Prune();
}

// ── Aiming ───────────────────────────────────────────────────────────────────

// Placement follows the CAMERA, not Link's body, so the platform lands where the player is looking.
static s16 Cryonis_CameraYaw(PlayState* play, Player* player) {
    Vec3f eye = play->view.eye;
    Vec3f at = play->view.at;

    if ((fabsf(at.x - eye.x) < 0.001f) && (fabsf(at.z - eye.z) < 0.001f)) {
        return player->actor.shape.rot.y;
    }
    return Math_Vec3f_Yaw(&eye, &at);
}

static u8 Cryonis_AimIsOnPillar(Vec3f* pos) {
    if (sCryonis.pillar == NULL) {
        return 0;
    }

    f32 dx = pos->x - sCryonis.pillar->world.pos.x;
    f32 dz = pos->z - sCryonis.pillar->world.pos.z;
    f32 reach = CRYONIS_HALF_EXTENT + CRYONIS_BREAK_SLACK;

    return ((dx * dx) + (dz * dz)) < (reach * reach);
}

// Is the water at `pos` reachable, or is there a wall between Link and it?
static u8 Cryonis_PathIsClear(PlayState* play, Player* player, Vec3f* pos) {
    Vec3f from = player->actor.world.pos;
    Vec3f to = *pos;
    Vec3f hit;
    CollisionPoly* poly = NULL;
    s32 bgId = BGCHECK_SCENE;

    from.y += 40.0f;
    to.y += 20.0f;
    return !BgCheck_EntityLineTest1(&play->colCtx, &from, &to, &hit, &poly, true, false, false, true, &bgId);
}

static void Cryonis_UpdateGhost(PlayState* play, Player* player) {
    s16 yaw = Cryonis_CameraYaw(play, player);
    Vec3f pos;
    f32 waterY;
    WaterBox* waterBox = NULL;

    pos.x = player->actor.world.pos.x + (Math_SinS(yaw) * sCryonis.dist);
    pos.y = player->actor.world.pos.y;
    pos.z = player->actor.world.pos.z + (Math_CosS(yaw) * sCryonis.dist);

    if (!WaterBox_GetSurface1(play, &play->colCtx, pos.x, pos.z, &waterY, &waterBox)) {
        sCryonis.ghostPos = pos;
        sCryonis.ghostValid = 0;
        sCryonis.ghostBreaks = 0;
        return;
    }

    pos.y = waterY;
    sCryonis.ghostPos = pos;
    sCryonis.ghostBreaks = Cryonis_AimIsOnPillar(&pos);
    sCryonis.ghostValid = sCryonis.ghostBreaks || Cryonis_PathIsClear(play, player, &pos);
}

// ── The aiming mode ──────────────────────────────────────────────────────────

u8 Cryonis_ModeActive(void) {
    return sCryonis.modeActive;
}

static void Cryonis_ModeExit(u8 cancelled) {
    if (!sCryonis.modeActive) {
        return;
    }
    sCryonis.modeActive = 0;
    sCryonis.ghostValid = 0;
    sCryonis.ghostBreaks = 0;
    if (cancelled) {
        Cryonis_Sfx(NA_SE_SY_CANCEL, &gSfxDefaultPos);
    }
}

/**
 * The rune's cast: it opens the aiming mode rather than placing anything. Returns 1 so the slate
 * tick treats the press as a real cast and skips its error cue.
 */
s32 Cryonis_Cast(PlayState* play, Player* player) {
    sCryonis.modeActive = 1;
    sCryonis.dist = CRYONIS_DIST_START;
    Cryonis_UpdateGhost(play, player);
    Cryonis_Sfx(NA_SE_SY_GET_ITEM, &gSfxDefaultPos);
    return 1;
}

/**
 * One frame of the mode. Returns 1 while it owns the input, so the slate tick stops.
 *
 * Called from Slate_TickInput ABOVE its blocking checks and its stow paths, which is why the ones
 * that matter are repeated here: with the mode below them, the A that commits had already gone
 * through the unequip and the tablet was gone by the time the press arrived.
 */
u8 Cryonis_ModeUpdate(PlayState* play, Player* player) {
    if (!sCryonis.modeActive) {
        return 0;
    }
    if (!Slate_IsDrawn() || ItemInput_IsBlocked(player, play) || (player->stateFlags1 & PLAYER_STATE1_IN_WATER) ||
        (player->meleeWeaponState != 0)) {
        Cryonis_ModeExit(1);
        return 0;
    }

    Input* input = &play->state.input[0];
    u16 cur = input->cur.button;

    if (CHECK_BTN_ALL(input->press.button, BTN_B)) {
        Cryonis_ModeExit(1);
        return 1;
    }

    // Read from cur.button, never press: the player actor consumes the D-pad press bits for its own
    // item handling long before this runs. A held pad is a continuous move here, so the state is
    // what we want anyway.
    if (cur & BTN_DUP) {
        sCryonis.dist += CRYONIS_DIST_RATE;
    }
    if (cur & BTN_DDOWN) {
        sCryonis.dist -= CRYONIS_DIST_RATE;
    }
    sCryonis.dist = CLAMP(sCryonis.dist, CRYONIS_DIST_MIN, CRYONIS_DIST_MAX);

    Cryonis_UpdateGhost(play, player);

    if (CHECK_BTN_ALL(input->press.button, BTN_A)) {
        if (sCryonis.ghostBreaks) {
            Cryonis_Shatter(play);
            Cryonis_ModeExit(0);
            return 1;
        }
        if (!sCryonis.ghostValid) {
            Cryonis_Sfx(NA_SE_SY_ERROR, &player->actor.world.pos);
            return 1; // stay in the mode: a bad aim is a miss, not a cancel
        }
        Cryonis_Shatter(play); // only one platform stands at a time
        if (!Cryonis_Raise(play, &sCryonis.ghostPos)) {
            Cryonis_Sfx(NA_SE_SY_ERROR, &player->actor.world.pos);
        }
        Cryonis_ModeExit(0);
    }
    return 1;
}

// ── Ghost ────────────────────────────────────────────────────────────────────

// A self-contained unit cube, so the preview needs nothing out of any object bank.
static Vtx sCryonisGhostVtx[] = {
    VTX(-1, 0, -1, 0, 0, 0, 0, 0, 255), VTX(1, 0, -1, 0, 0, 0, 0, 0, 255),  VTX(1, 0, 1, 0, 0, 0, 0, 0, 255),
    VTX(-1, 0, 1, 0, 0, 0, 0, 0, 255),  VTX(-1, 1, -1, 0, 0, 0, 0, 0, 255), VTX(1, 1, -1, 0, 0, 0, 0, 0, 255),
    VTX(1, 1, 1, 0, 0, 0, 0, 0, 255),   VTX(-1, 1, 1, 0, 0, 0, 0, 0, 255),
};

static Gfx sCryonisGhostDL[] = {
    gsSPVertex(sCryonisGhostVtx, 8, 0),     gsSP2Triangles(0, 1, 2, 0, 0, 2, 3, 0),
    gsSP2Triangles(4, 6, 5, 0, 4, 7, 6, 0), gsSP2Triangles(0, 5, 1, 0, 0, 4, 5, 0),
    gsSP2Triangles(1, 6, 2, 0, 1, 5, 6, 0), gsSP2Triangles(2, 7, 3, 0, 2, 6, 7, 0),
    gsSP2Triangles(3, 4, 0, 0, 3, 7, 4, 0), gsSPEndDisplayList(),
};

void Cryonis_DrawGhost(PlayState* play) {
    if (!sCryonis.modeActive) {
        return;
    }

    f32 pulse = 0.94f + (0.06f * Math_SinS((s16)(play->gameplayFrames * 1500)));

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL_25Xlu(play->state.gfxCtx);

    Matrix_Translate(sCryonis.ghostPos.x, sCryonis.ghostPos.y, sCryonis.ghostPos.z, MTXMODE_NEW);
    Matrix_Scale(CRYONIS_HALF_EXTENT * pulse, CRYONIS_GHOST_HEIGHT * pulse, CRYONIS_HALF_EXTENT * pulse, MTXMODE_APPLY);

    // Components spelled out: MSVC hands a multi-value #define to a function-like macro as ONE
    // argument, so gDPSetPrimColor would not expand.
    if (sCryonis.ghostBreaks) {
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 150, 90, 120);
        gDPSetEnvColor(POLY_XLU_DISP++, 180, 60, 0, 120);
    } else if (sCryonis.ghostValid) {
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 150, 215, 255, 110);
        gDPSetEnvColor(POLY_XLU_DISP++, 20, 90, 180, 110);
    } else {
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 70, 70, 110);
        gDPSetEnvColor(POLY_XLU_DISP++, 150, 0, 0, 110);
    }
    gDPSetCombineLERP(POLY_XLU_DISP++, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE, 0, 0, 0, PRIMITIVE);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_LIGHTING | G_CULL_BACK);

    gSPMatrix(POLY_XLU_DISP++, MATRIX_NEWMTX(play->state.gfxCtx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gSPDisplayList(POLY_XLU_DISP++, sCryonisGhostDL);

    CLOSE_DISPS(play->state.gfxCtx);
}
