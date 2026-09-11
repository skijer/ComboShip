/*
 * form_garo.cpp - Garo moveset and hybrid body (Skijer's NEI). Port of soh's garo_form.cpp +
 * garo_hybrid_render.cpp: B = free dual-sword spin (hold = elemental rod orb), R = guard / counter,
 * A = dash, Z+A = hops / banish / jump-slash chain; the ghost body is MM's native object_jso rig
 * driven bone-by-bone by Link's animation.
 */
#include "forms_internal.h"
#include <stdio.h>

extern "C" {
#include "objects/object_jso/object_jso.h"
#include "objects/gameplay_keep/gameplay_keep.h"
}

#define GARO_ANIM(n) "__OTR__objects/forms/garo/gPlayerAnim_garo_" n

#define GARO_SPIN_FRAMES 14
#define GARO_SPIN_YAW_RATE 0x1600
#define GARO_SPIN_STEER_SPEED 13.5f
#define GARO_SPIN_DAMAGE 2
#define GARO_GUARD_REFLECT_RANGE 130.0f
#define GARO_GUARD_MELEE_RANGE 160.0f
#define GARO_GUARD_HOLD_FRACTION 0.5f
#define GARO_GUARD_RETURN_SPEED 1.5f
#define GARO_DASH_SPEED 14.0f
#define GARO_DASH_DAMAGE 4
#define GARO_DASH_TURN 0x400
#define GARO_BANISH_COOLDOWN 300
#define GARO_BANISH_OFFSET 50.0f
#define GARO_BANISH_VANISH_END 8
#define GARO_BANISH_STUN_FRAMES 60
#define GARO_BANISH_SHADOW_LEN 9
#define GARO_BANISH_STUN_RADIUS 80.0f
#define GARO_RIPOSTE_OFFSET 45.0f
#define GARO_RIPOSTE_HIT_FRAME 6
#define GARO_SHADOW_BALL_DAMAGE 8
#define GARO_SHADOW_BALL_HIT_F 12
#define GARO_SHADOW_BALL_PLAYSPEED 2.5f
#define GARO_LAND_STRIKE_DAMAGE 8
#define GARO_LAND_STRIKE_HIT_F 12
#define GARO_LAND_STRIKE_TAIL_F 3
#define GARO_ROD_CHARGE_MAX 120
#define GARO_ROD_CHARGE_TIER2 35
#define GARO_ROD_CHARGE_TIER3 85
#define GARO_ROD_L2_DAMAGE 4
#define GARO_ROD_L3_DAMAGE 3
#define GARO_ROD_MAGIC_COST 4
#define GARO_ROD_RELEASE_CD 10
#define GARO_B_HOLD_THRESHOLD 9
#define GARO_LAUGH_CHANCE 0.20f
#define GARO_ROD_ELEMENT_COUNT 6
#define GARO_ORB_MAX 12
#define GARO_ORB_SEEKERS 4
#define GARO_ORB_SEEKER_DAMAGE 2
#define GARO_ORB_SEEKER_LIFETIME 70
#define GARO_ORB_SEEKER_SPEED 16.0f
#define GARO_ORB_SEEKER_TURN 0x1200
#define GARO_ORB_SEEKER_FAN 0x2000
#define GARO_ORB_HOME_RANGE 700.0f
#define GARO_ORB_HOME_CONE 0.2f
#define GARO_ORB_TURN_RATE 0x0700
#define GARO_ROD_ORB_SPEED 12.0f
#define GARO_ROD_ORB_LIFETIME 60
#define GARO_ROD_BURST_LIFETIME 30
#define GARO_ORB_QUAD_HALF 12.0f
#define GARO_REFLECT_HALF 10.0f
#define GARO_REFLECT_DAMAGE 4
#define GARO_REFLECT_LIFE 60
#define GARO_DEATH_FLAME_COUNT 9
#define GARO_DEATH_FLAME_RADIUS 20.0f
#define GARO_HOP_SPEED 12.75f
#define GARO_BACKFLIP_SPEED 12.0f
#define GARO_SUBTREE_SCALE 3.5f
#define GARO_LEG_THICK 1.3f
#define GARO_TRAIL_LEN 900.0f
#define GARO_STRIKE_DMG DMG_SWORD

typedef enum GaroState {
    GARO_IDLE,
    GARO_SPIN,
    GARO_ROD_AIM,
    GARO_PARRY_GUARD,
    GARO_GUARD_RETURN,
    GARO_PARRY_RIPOSTE,
    GARO_DASH_ATTACK,
    GARO_BANISH_VANISH,
    GARO_BANISH_SHADOW,
    GARO_SHADOW_BALL,
    GARO_LAUGH_TAUNT,
    GARO_SIDEHOP_L,
    GARO_SIDEHOP_R,
    GARO_BACKFLIP,
    GARO_JUMP_ATTACK,
    GARO_AIR_SLASH,
    GARO_LAND_STRIKE
} GaroState;

typedef enum GaroElement {
    GARO_EL_FIRE,
    GARO_EL_ICE,
    GARO_EL_LIGHT,
    GARO_EL_DARK,
    GARO_EL_SOUL,
    GARO_EL_WIND
} GaroElement;

typedef struct GaroOrb {
    u8 active;
    Vec3f pos;
    s16 yaw;
    s16 pitch;
    s32 timer;
    u8 element;
    u8 damage;
    u32 dmgFlag;
    u8 bursts;
    u8 isSeeker;
} GaroOrb;

enum {
    A_GUARD,
    A_SPIN,
    A_DASH,
    A_COLLAPSE,
    A_APPEAR,
    A_APPEAR_DRAW,
    A_DRAW,
    A_SLASH_LOOP,
    A_BOUNCE,
    A_JUMP_BACK,
    A_TAKE_OUT,
    A_LAUGH,
    A_COUNT
};
static PlayerAnimationHeader* sAnims[A_COUNT];
static u8 sAnimsLoaded;

#define GARO_STATE2_DISABLE_DRAW PLAYER_STATE2_20000000

static struct {
    GaroState state;
    s32 timer;
    s32 bHoldDetect;
    s16 spinEntryYaw;
    Actor* parryAttacker;
    Actor* banishTarget;
    Vec3f shadowStart;
    Vec3f shadowEnd;
    Vec3f shadowPos;
    s32 shadowTimer;
    s32 banishCooldown;
    s32 rodReleaseCd;
    s32 rodChargeTimer;
    u8 rodElement;
    u8 rodSfxPlayed;
    f32 rodBallScale;
    u8 laughPending;
    s32 hopAirTimer;
    s32 landStrikeTail;
    u8 strikeSfxPlayed;
    u8 trailActive;
    s32 trailIndex[2];
    s8 trailAxis[2];
    Actor* reflectShot;
    s32 reflectTimer;
    u8 deathFlamesSpawned;
    s32 enemyCount;
    GaroOrb orbs[GARO_ORB_MAX];
} sGaro;

static ColliderQuad sOrbQuads[GARO_ORB_MAX];
static ColliderQuad sReflectQuad;
static u8 sQuadsInited;

// 0xFFCFFFFF = "accepts everything except reflection", OR'd with the orb's own element flag so
// restrictive-AC enemies still take the hit while element-vulnerable bosses get routed.
static ColliderQuadInit sOrbQuadInit = {
    { COL_MATERIAL_NONE, AT_ON | AT_TYPE_PLAYER, AC_NONE, OC1_NONE, OC2_TYPE_PLAYER, COLSHAPE_QUAD },
    { ELEM_MATERIAL_UNK2,
      { 0xFFCFFFFF, 0x00, 0x10 },
      { 0x00000000, 0x00, 0x00 },
      ATELEM_ON | ATELEM_NEAREST | ATELEM_SFX_NORMAL,
      ACELEM_NONE,
      OCELEM_NONE },
    { { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } } },
};

static EffectBlureInit2 sTrailInit = {
    0,
    0,
    0,
    { 120, 60, 200, 200 },
    { 60, 30, 140, 100 },
    { 60, 30, 140, 0 },
    { 60, 30, 140, 0 },
    8,
    0,
    EFF_BLURE_DRAW_MODE_SIMPLE,
    0,
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
};

static const Color_RGBA8 sVioletPrim = { 140, 80, 220, 255 };
static const Color_RGBA8 sVioletEnv = { 40, 10, 100, 0 };

static u8 GaroForm_IsActive(void) {
    return CustomForms_ActiveForm() == CUSTOM_FORM_GARO;
}

// ---- green robe ---------------------------------------------------------------------------------
// The tunic-coloured retextures live at objects/forms/garo/object_jso/ (relocated so MM's own Garo
// enemies stay purple). The ghost DLs reference their textures by CRC hash (G_SETTIMG_OTR_HASH,
// two Gfx), so each match is patched in place to a plain G_SETTIMG on the green path, and
// unpatched when the form ends.
#define GARO_GREEN_BASE "__OTR__objects/forms/garo/object_jso/"
#define GARO_JSO_BASE "__OTR__objects/object_jso/"
#define GARO_OP_SETTIMG_OTR_HASH 0x20
#define GARO_GREEN_PATCH_MAX 64

static const char* sGreenTexNames[6] = {
    "gGaroLegWrappingTex", "gGaroRobeFrontTex", "gGaroRobeStitchingTex",
    "gGaroRobeTex",        "gGaroRobeTopTex",   "gGaroThighTex",
};
static const char* sGreenTexPaths[6] = {
    GARO_GREEN_BASE "gGaroLegWrappingTex", GARO_GREEN_BASE "gGaroRobeFrontTex", GARO_GREEN_BASE "gGaroRobeStitchingTex",
    GARO_GREEN_BASE "gGaroRobeTex",        GARO_GREEN_BASE "gGaroRobeTopTex",   GARO_GREEN_BASE "gGaroThighTex",
};
static const char* sGhostDLs[] = {
    GARO_JSO_BASE "gGaroHeadDL",       GARO_JSO_BASE "gGaroLeftArmDL",    GARO_JSO_BASE "gGaroLeftFootDL",
    GARO_JSO_BASE "gGaroLeftShinDL",   GARO_JSO_BASE "gGaroLeftSwordDL",  GARO_JSO_BASE "gGaroLeftThighDL",
    GARO_JSO_BASE "gGaroRightArmDL",   GARO_JSO_BASE "gGaroRightFootDL",  GARO_JSO_BASE "gGaroRightShinDL",
    GARO_JSO_BASE "gGaroRightSwordDL", GARO_JSO_BASE "gGaroRightThighDL", GARO_JSO_BASE "gGaroRobeBackDL",
    GARO_JSO_BASE "gGaroRobeFrontDL",  GARO_JSO_BASE "gGaroRobeLeftDL",   GARO_JSO_BASE "gGaroRobeRightDL",
    GARO_JSO_BASE "gGaroRobeTopDL",    GARO_JSO_BASE "gGaroTorsoDL",
};

typedef struct GaroGreenPatch {
    const char* dlPath;
    char name[24];
} GaroGreenPatch;
static GaroGreenPatch sGreenPatches[GARO_GREEN_PATCH_MAX];
static s32 sGreenPatchCount;
static u8 sGreenApplied;

static s32 GaroForm_GreenIndexFor(const char* resourceName) {
    size_t len = strlen(resourceName);
    for (s32 t = 0; t < 6; t++) {
        size_t tl = strlen(sGreenTexNames[t]);
        if (len >= tl && strcmp(resourceName + len - tl, sGreenTexNames[t]) == 0) {
            return t;
        }
    }
    return -1;
}

static void GaroForm_PatchGreenTexture(const char* dlPath, Gfx* dl, s32 index, s32 tex) {
    if (sGreenPatchCount + 2 > GARO_GREEN_PATCH_MAX) {
        return;
    }
    Gfx settimg;
    settimg.words.w0 = (dl[index].words.w0 & 0x00FFFFFF) | ((uintptr_t)G_SETTIMG << 24);
    settimg.words.w1 = (uintptr_t)sGreenTexPaths[tex];
    Gfx noop;
    noop.words.w0 = (uintptr_t)G_NOOP << 24;
    noop.words.w1 = 0;
    GaroGreenPatch* a = &sGreenPatches[sGreenPatchCount++];
    GaroGreenPatch* b = &sGreenPatches[sGreenPatchCount++];
    a->dlPath = dlPath;
    b->dlPath = dlPath;
    snprintf(a->name, sizeof(a->name), "garoGreen%d", index);
    snprintf(b->name, sizeof(b->name), "garoGreen%d", index + 1);
    ResourceMgr_PatchGfxByName(dlPath, a->name, index, settimg);
    ResourceMgr_PatchGfxByName(dlPath, b->name, index + 1, noop);
}

static void GaroForm_ApplyGreen(void) {
    if (sGreenApplied) {
        return;
    }
    sGreenApplied = 1;
    for (s32 t = 0; t < 6; t++) {
        if (!ResourceMgr_FileExists(sGreenTexPaths[t] + 7)) {
            SPDLOG_WARN("[Garo] green texture missing: {}", sGreenTexPaths[t]);
            return;
        }
    }
    for (s32 d = 0; d < (s32)(sizeof(sGhostDLs) / sizeof(sGhostDLs[0])); d++) {
        if (!ResourceMgr_FileExists(sGhostDLs[d] + 7)) {
            continue;
        }
        Gfx* dl = ResourceMgr_LoadGfxByName(sGhostDLs[d]);
        if (dl == NULL) {
            continue;
        }
        for (s32 i = 0; i < 1024; i++) {
            u8 op = (u8)((dl[i].words.w0 >> 24) & 0xFF);
            if (op == G_ENDDL) {
                break;
            }
            if (op != GARO_OP_SETTIMG_OTR_HASH) {
                continue;
            }
            uint64_t hash = ((uint64_t)(u32)dl[i + 1].words.w0 << 32) | (u32)dl[i + 1].words.w1;
            const char* name = MmAssets_HashToPath(hash);
            s32 tex = (name != NULL) ? GaroForm_GreenIndexFor(name) : -1;
            if (tex >= 0) {
                GaroForm_PatchGreenTexture(sGhostDLs[d], dl, i, tex);
            }
            i++;
        }
    }
    SPDLOG_INFO("[Garo] green robe: {} texture references patched", sGreenPatchCount / 2);
}

static void GaroForm_RemoveGreen(void) {
    for (s32 i = 0; i < sGreenPatchCount; i++) {
        ResourceMgr_UnpatchGfxByName(sGreenPatches[i].dlPath, sGreenPatches[i].name);
    }
    sGreenPatchCount = 0;
    sGreenApplied = 0;
}

static void GaroForm_LoadAnims(void) {
    if (sAnimsLoaded) {
        return;
    }
    static const char* sPaths[A_COUNT] = {
        GARO_ANIM("guard"),  GARO_ANIM("spinAttack"),       GARO_ANIM("dashAttack"),  GARO_ANIM("collapse"),
        GARO_ANIM("appear"), GARO_ANIM("appearDrawSwords"), GARO_ANIM("drawSwords"),  GARO_ANIM("slashLoop"),
        GARO_ANIM("bounce"), GARO_ANIM("jumpBack"),         GARO_ANIM("takeOutBomb"), GARO_ANIM("laugh"),
    };
    for (s32 i = 0; i < A_COUNT; i++) {
        sAnims[i] = Forms_LoadAnim(sPaths[i]);
    }
    sAnimsLoaded = 1;
}

static PlayerAnimationHeader* GaroForm_Anim(s32 id) {
    return sAnims[id];
}

// ---- clip runner ----------------------------------------------------------------------------------
static void GaroForm_StartAnim(PlayState* play, Player* player, s32 id, f32 speed, u8 loop) {
    PlayerAnimationHeader* anim = GaroForm_Anim(id);
    if (anim == NULL) {
        return;
    }
    f32 last = Animation_GetLastFrame(anim);
    if (speed < 0.0f) {
        Forms_PlayClip(play, player, anim, speed, last * GARO_GUARD_HOLD_FRACTION, 0.0f, loop, -2.0f);
        return;
    }
    Forms_PlayClip(play, player, anim, speed, 0.0f, last, loop, -2.0f);
}

static void GaroForm_StartGuardRaise(PlayState* play, Player* player) {
    PlayerAnimationHeader* anim = GaroForm_Anim(A_GUARD);
    if (anim == NULL) {
        return;
    }
    Forms_PlayClip(play, player, anim, 1.0f, 0.0f, Animation_GetLastFrame(anim) * GARO_GUARD_HOLD_FRACTION, 0, -2.0f);
}

// PlayerAnimation_Update clamps at the end frame, which IS "hold the last frame".
static u8 GaroForm_Advance(PlayState* play, Player* player) {
    return PlayerAnimation_Update(play, &player->skelAnime) != 0;
}

// ---- trails ---------------------------------------------------------------------------------------
static void GaroForm_KillTrail(PlayState* play) {
    if (!sGaro.trailActive) {
        return;
    }
    Effect_Delete(play, sGaro.trailIndex[0]);
    Effect_Delete(play, sGaro.trailIndex[1]);
    sGaro.trailActive = 0;
}

static void GaroForm_SpawnTrail(PlayState* play) {
    GaroForm_KillTrail(play);
    Effect_Add(play, &sGaro.trailIndex[0], EFFECT_BLURE2, 0, 0, &sTrailInit);
    Effect_Add(play, &sGaro.trailIndex[1], EFFECT_BLURE2, 0, 0, &sTrailInit);
    sGaro.trailAxis[0] = -1;
    sGaro.trailAxis[1] = -1;
    sGaro.trailActive = 1;
}

// ---- quads on the player's melee quad 0 -----------------------------------------------------------
static void GaroForm_DisableStrikeQuad(Player* player) {
    player->meleeWeaponQuads[0].base.atFlags &= ~AT_ON;
}

static void GaroForm_SubmitStrikeQuad(PlayState* play, Player* player, Vec3f* a, Vec3f* b, Vec3f* c, Vec3f* d,
                                      s32 damage) {
    ColliderQuad* quad = &player->meleeWeaponQuads[0];
    func_80833728(player, 0, GARO_STRIKE_DMG, damage);
    Collider_SetQuadVertices(quad, a, b, c, d);
    quad->base.atFlags |= AT_ON;
    CollisionCheck_SetAT(play, &play->colChkCtx, &quad->base);
}

// Forward slab: near/far along the facing, a slanted plane from floor+yBottom at near to yTop at far.
static void GaroForm_EnableSlabQuad(PlayState* play, Player* player, f32 near, f32 far, f32 halfW, f32 yBottom,
                                    f32 yTop, s32 damage) {
    f32 sinYaw = Math_SinS(player->actor.shape.rot.y);
    f32 cosYaw = Math_CosS(player->actor.shape.rot.y);
    Vec3f base = player->actor.world.pos;
    Vec3f right = { cosYaw * halfW, 0.0f, -sinYaw * halfW };
    Vec3f a = { base.x + sinYaw * far - right.x, base.y + yTop, base.z + cosYaw * far - right.z };
    Vec3f b = { base.x + sinYaw * far + right.x, base.y + yTop, base.z + cosYaw * far + right.z };
    Vec3f c = { base.x + sinYaw * near - right.x, base.y + yBottom, base.z + cosYaw * near - right.z };
    Vec3f d = { base.x + sinYaw * near + right.x, base.y + yBottom, base.z + cosYaw * near + right.z };
    GaroForm_SubmitStrikeQuad(play, player, &a, &b, &c, &d, damage);
}

static void GaroForm_EnableSpinQuad(PlayState* play, Player* player) {
    GaroForm_EnableSlabQuad(play, player, 10.0f, 60.0f, 35.0f, 0.0f, 55.0f, GARO_SPIN_DAMAGE);
}

static void GaroForm_EnableSwingQuad(PlayState* play, Player* player, s32 damage) {
    GaroForm_EnableSlabQuad(play, player, 10.0f, 60.0f, 25.0f, 0.0f, 60.0f, damage);
}

// Vertical rectangle perpendicular to the facing, marching one step out per live frame — the swing
// quad is one thin slanted line at any given distance and kept missing the enemy just behind.
static void GaroForm_EnableFrontSweepQuad(PlayState* play, Player* player, s32 liveFrame, s32 damage) {
    f32 dist = 15.0f + liveFrame * 12.0f;
    f32 sinYaw = Math_SinS(player->actor.shape.rot.y);
    f32 cosYaw = Math_CosS(player->actor.shape.rot.y);
    Vec3f center = player->actor.world.pos;
    center.x += sinYaw * dist;
    center.z += cosYaw * dist;
    Vec3f right = { cosYaw * 45.0f, 0.0f, -sinYaw * 45.0f };
    Vec3f a = { center.x - right.x, center.y + 75.0f, center.z - right.z };
    Vec3f b = { center.x + right.x, center.y + 75.0f, center.z + right.z };
    Vec3f c = { center.x - right.x, center.y - 10.0f, center.z - right.z };
    Vec3f d = { center.x + right.x, center.y - 10.0f, center.z + right.z };
    GaroForm_SubmitStrikeQuad(play, player, &a, &b, &c, &d, damage);
}

// A 260-unit line slab swept ~30 degrees per live frame; a line covers both directions so 180 degrees
// of sweep is full coverage.
static void GaroForm_EnableLandStrikeQuad(PlayState* play, Player* player) {
    s32 live = sGaro.timer - GARO_LAND_STRIKE_HIT_F;
    if (live < 0) {
        live = 0;
    }
    s16 sweepYaw = (s16)(player->actor.shape.rot.y + live * 0x1555);
    f32 sinYaw = Math_SinS(sweepYaw);
    f32 cosYaw = Math_CosS(sweepYaw);
    Vec3f base = player->actor.world.pos;
    Vec3f a = { base.x + sinYaw * 130.0f, base.y + 130.0f, base.z + cosYaw * 130.0f };
    Vec3f b = { base.x - sinYaw * 130.0f, base.y + 130.0f, base.z - cosYaw * 130.0f };
    Vec3f c = { base.x + sinYaw * 130.0f, base.y - 20.0f, base.z + cosYaw * 130.0f };
    Vec3f d = { base.x - sinYaw * 130.0f, base.y - 20.0f, base.z - cosYaw * 130.0f };
    GaroForm_SubmitStrikeQuad(play, player, &a, &b, &c, &d, GARO_LAND_STRIKE_DAMAGE);
}

// ---- stun ring ------------------------------------------------------------------------------------
static void GaroForm_StunTarget(PlayState* play, Actor* target) {
    target->freezeTimer = GARO_BANISH_STUN_FRAMES;
    Actor_SetColorFilter(target, COLORFILTER_COLORFLAG_BLUE, 0xF8, COLORFILTER_BUFFLAG_OPA, GARO_BANISH_STUN_FRAMES);
    for (s32 i = 0; i < 8; i++) {
        f32 ang = i * (2.0f * (f32)M_PI / 8.0f);
        Vec3f pos = { target->world.pos.x + cosf(ang) * GARO_BANISH_STUN_RADIUS, target->world.pos.y + 30.0f,
                      target->world.pos.z + sinf(ang) * GARO_BANISH_STUN_RADIUS };
        Vec3f vel = { cosf(ang) * 0.5f, 1.5f, sinf(ang) * 0.5f };
        Vec3f accel = { 0.0f, -0.05f, 0.0f };
        Color_RGBA8 prim = sVioletPrim;
        Color_RGBA8 env = sVioletEnv;
        EffectSsKirakira_SpawnSmall(play, &pos, &vel, &accel, &prim, &env);
    }
    Actor_PlaySfx(target, NA_SE_IT_SHIELD_REFLECT_SW);
}

// ---- orbs -----------------------------------------------------------------------------------------
static void GaroForm_EnsureQuads(PlayState* play, Player* player) {
    if (sQuadsInited) {
        return;
    }
    for (s32 i = 0; i < GARO_ORB_MAX; i++) {
        Collider_InitQuad(play, &sOrbQuads[i]);
        Collider_SetQuad(play, &sOrbQuads[i], &player->actor, &sOrbQuadInit);
    }
    Collider_InitQuad(play, &sReflectQuad);
    Collider_SetQuad(play, &sReflectQuad, &player->actor, &sOrbQuadInit);
    sQuadsInited = 1;
}

static void GaroForm_StampSquareQuad(PlayState* play, ColliderQuad* quad, Vec3f* center, f32 half, u32 dmgFlags,
                                     u8 damage) {
    s16 camYaw = Camera_GetCamDirYaw(GET_ACTIVE_CAM(play));
    Vec3f right = { Math_CosS(camYaw) * half, 0.0f, -Math_SinS(camYaw) * half };
    Vec3f a = { center->x - right.x, center->y + half, center->z - right.z };
    Vec3f b = { center->x + right.x, center->y + half, center->z + right.z };
    Vec3f c = { center->x - right.x, center->y - half, center->z - right.z };
    Vec3f d = { center->x + right.x, center->y - half, center->z + right.z };
    quad->elem.atDmgInfo.dmgFlags = dmgFlags;
    quad->elem.atDmgInfo.damage = damage;
    Collider_SetQuadVertices(quad, &a, &b, &c, &d);
    quad->base.atFlags |= AT_ON;
    CollisionCheck_SetAT(play, &play->colChkCtx, &quad->base);
}

static u32 GaroForm_ElementDmgFlag(u8 element) {
    switch (element) {
        case GARO_EL_FIRE:
            return DMG_FIRE_ARROW;
        case GARO_EL_ICE:
            return DMG_ICE_ARROW;
        case GARO_EL_LIGHT:
            return DMG_LIGHT_ARROW;
        default:
            return DMG_NORMAL_ARROW;
    }
}

static u8 GaroForm_ElementLightBall(u8 element) {
    static const u8 sColors[GARO_ROD_ELEMENT_COUNT] = { 2, 1, 7, 5, 3, 0 };
    return sColors[element % GARO_ROD_ELEMENT_COUNT];
}

static GaroOrb* GaroForm_SpawnOrb(void) {
    for (s32 i = 0; i < GARO_ORB_MAX; i++) {
        if (!sGaro.orbs[i].active) {
            memset(&sGaro.orbs[i], 0, sizeof(GaroOrb));
            sGaro.orbs[i].active = 1;
            return &sGaro.orbs[i];
        }
    }
    return NULL;
}

static void GaroForm_BurstOrb(GaroOrb* orb) {
    for (s32 i = 0; i < GARO_ORB_SEEKERS; i++) {
        GaroOrb* seeker = GaroForm_SpawnOrb();
        if (seeker == NULL) {
            return;
        }
        seeker->pos = orb->pos;
        seeker->yaw = (s16)(orb->yaw + (i - (GARO_ORB_SEEKERS - 1) * 0.5f) * GARO_ORB_SEEKER_FAN);
        seeker->pitch = orb->pitch;
        seeker->timer = GARO_ORB_SEEKER_LIFETIME;
        seeker->element = orb->element;
        seeker->damage = GARO_ORB_SEEKER_DAMAGE;
        seeker->dmgFlag = orb->dmgFlag;
        seeker->isSeeker = 1;
    }
}

// The target must be roughly ahead, or a shot would turn round and chase what it already passed.
static void GaroForm_HomeOrb(PlayState* play, GaroOrb* orb) {
    Actor* best = NULL;
    f32 bestDist = GARO_ORB_HOME_RANGE;
    f32 dirX = Math_SinS(orb->yaw);
    f32 dirZ = Math_CosS(orb->yaw);
    Actor* it = play->actorCtx.actorLists[ACTORCAT_ENEMY].first;
    while (it != NULL) {
        if (it->update != NULL) {
            f32 dx = it->world.pos.x - orb->pos.x;
            f32 dz = it->world.pos.z - orb->pos.z;
            f32 dist = sqrtf(dx * dx + dz * dz);
            if (dist > 1.0f && dist < bestDist && (dx * dirX + dz * dirZ) / dist > GARO_ORB_HOME_CONE) {
                best = it;
                bestDist = dist;
            }
        }
        it = it->next;
    }
    if (best == NULL) {
        return;
    }
    s16 wantYaw = Math_Vec3f_Yaw(&orb->pos, &best->focus.pos);
    s16 wantPitch = -Math_Vec3f_Pitch(&orb->pos, &best->focus.pos);
    Math_ScaledStepToS(&orb->yaw, wantYaw, orb->isSeeker ? GARO_ORB_SEEKER_TURN : GARO_ORB_TURN_RATE);
    Math_ScaledStepToS(&orb->pitch, wantPitch, orb->isSeeker ? GARO_ORB_SEEKER_TURN : GARO_ORB_TURN_RATE);
}

static void GaroForm_UpdateOrbs(PlayState* play, Player* player) {
    GaroForm_EnsureQuads(play, player);
    for (s32 i = 0; i < GARO_ORB_MAX; i++) {
        GaroOrb* orb = &sGaro.orbs[i];
        ColliderQuad* quad = &sOrbQuads[i];
        if (!orb->active) {
            quad->base.atFlags &= ~AT_ON;
            continue;
        }
        if (quad->base.atFlags & AT_HIT) {
            quad->base.atFlags &= ~AT_HIT;
            if (orb->bursts) {
                GaroForm_BurstOrb(orb);
                orb->active = 0;
                continue;
            }
        }
        GaroForm_HomeOrb(play, orb);
        f32 speed = orb->isSeeker ? GARO_ORB_SEEKER_SPEED : GARO_ROD_ORB_SPEED;
        f32 cosP = Math_CosS(orb->pitch);
        orb->pos.x += Math_SinS(orb->yaw) * cosP * speed;
        orb->pos.z += Math_CosS(orb->yaw) * cosP * speed;
        orb->pos.y += -Math_SinS(orb->pitch) * speed;
        if ((play->gameplayFrames & 3) == 0) {
            Vec3f zero = { 0.0f, 0.0f, 0.0f };
            EffectSsFhgFlash_SpawnLightBall(play, &orb->pos, &zero, &zero, 85, GaroForm_ElementLightBall(orb->element));
        }
        if (--orb->timer <= 0) {
            if (orb->bursts) {
                GaroForm_BurstOrb(orb);
            }
            orb->active = 0;
            continue;
        }
        GaroForm_StampSquareQuad(play, quad, &orb->pos, GARO_ORB_QUAD_HALF, 0xFFCFFFFF | orb->dmgFlag, orb->damage);
    }
}

// The charge ball forms in front of the face along the aim, like the Deku bubble at the mouth.
static void GaroForm_ChargeBallPos(Player* player, Vec3f* out);

static s32 GaroForm_RodLevel(void) {
    if (sGaro.rodChargeTimer >= GARO_ROD_CHARGE_TIER3) {
        return 3;
    }
    return (sGaro.rodChargeTimer >= GARO_ROD_CHARGE_TIER2) ? 2 : 1;
}

// Lock-on aims at the target; otherwise the camera is the sight (pitch positive = downward here).
static void GaroForm_AimAngles(Player* player, s16* yaw, s16* pitch) {
    Actor* target = player->focusActor;
    if (target != NULL && target->update != NULL) {
        Vec3f from = player->actor.focus.pos;
        *yaw = Math_Vec3f_Yaw(&from, &target->focus.pos);
        *pitch = -Math_Vec3f_Pitch(&from, &target->focus.pos);
        return;
    }
    Camera* cam = GET_ACTIVE_CAM(gPlayState);
    *yaw = Camera_GetCamDirYaw(cam);
    *pitch = -Camera_GetCamDirPitch(cam);
}

// Staying in the aim after a shot = rapid fire, like the Deku bubble.
static void GaroForm_FireRodOrb(Player* player, PlayState* play) {
    s32 level = GaroForm_RodLevel();
    s16 yaw;
    s16 pitch;
    GaroForm_AimAngles(player, &yaw, &pitch);
    if (level < 2) {
        Player_PlaySfx(player, NA_SE_IT_BOW_FLICK);
        sGaro.rodChargeTimer = 0;
        sGaro.rodSfxPlayed = 0;
        return;
    }
    u8 burst = level >= 3;
    u8 damage = burst ? GARO_ROD_L3_DAMAGE : GARO_ROD_L2_DAMAGE;
    s16 cost = GARO_ROD_MAGIC_COST * (burst ? 2 : 1);
    if (ItemMagic_HasEnough(play, cost)) {
        ItemMagic_Consume(play, cost);
    } else {
        damage = 1;
    }
    GaroOrb* orb = GaroForm_SpawnOrb();
    if (orb != NULL) {
        orb->pos = player->bodyPartsPos[PLAYER_BODYPART_LEFT_HAND];
        orb->yaw = yaw;
        orb->pitch = pitch;
        orb->timer = burst ? GARO_ROD_BURST_LIFETIME : GARO_ROD_ORB_LIFETIME;
        orb->element = sGaro.rodElement;
        orb->damage = damage;
        orb->dmgFlag = GaroForm_ElementDmgFlag(sGaro.rodElement);
        orb->bursts = burst;
    }
    Player_PlaySfx(player, NA_SE_IT_ARROW_SHOT);
    Player_PlaySfx(player, NA_SE_IT_MAGIC_ARROW_SHOT);
    sGaro.rodChargeTimer = 0;
    sGaro.rodSfxPlayed = 0;
}

// ---- reflect ------------------------------------------------------------------------------------
// A projectile can re-categorise itself (the Octorok rock moves to PROP), so every category but
// PLAYER/BG/DOOR/CHEST is swept.
static u8 GaroForm_IsRangedAttacker(Actor* actor) {
    return actor->category == ACTORCAT_EXPLOSIVE || !(actor->flags & ACTOR_FLAG_ATTENTION_ENABLED);
}

// Projectiles kill themselves on touching anything including Garo, so a bounce made while still
// overlapping just pops: shove it clear first.
static void GaroForm_ReflectShot(PlayState* play, Player* player, Actor* shot) {
    shot->world.rot.y += 0x8000;
    shot->shape.rot.y = shot->world.rot.y;
    shot->velocity.x = -shot->velocity.x;
    shot->velocity.z = -shot->velocity.z;
    if (shot->speed < 8.0f) {
        shot->speed = 8.0f;
    }
    shot->world.pos.x += Math_SinS(shot->world.rot.y) * 35.0f;
    shot->world.pos.z += Math_CosS(shot->world.rot.y) * 35.0f;
    sGaro.reflectShot = shot;
    sGaro.reflectTimer = GARO_REFLECT_LIFE;
    player->invincibilityTimer = 12;
    Audio_PlaySfx_AtPos(&player->actor.world.pos, NA_SE_IT_SHIELD_REFLECT_SW);
}

static u8 GaroForm_TryReflectIncoming(PlayState* play, Player* player) {
    Vec3f center = player->actor.world.pos;
    center.y += 40.0f;
    for (s32 cat = 0; cat < ACTORCAT_MAX; cat++) {
        if (cat == ACTORCAT_PLAYER || cat == ACTORCAT_BG || cat == ACTORCAT_DOOR || cat == ACTORCAT_CHEST) {
            continue;
        }
        Actor* it = play->actorCtx.actorLists[cat].first;
        while (it != NULL) {
            Actor* actor = it;
            it = it->next;
            if (actor->update == NULL || !GaroForm_IsRangedAttacker(actor)) {
                continue;
            }
            Vec3f toGaro;
            f32 dist = Math_Vec3f_DistXYZAndStoreDiff(&actor->world.pos, &center, &toGaro);
            if (dist > GARO_GUARD_REFLECT_RANGE) {
                continue;
            }
            f32 velX = actor->velocity.x + Math_SinS(actor->world.rot.y) * actor->speed;
            f32 velZ = actor->velocity.z + Math_CosS(actor->world.rot.y) * actor->speed;
            if (velX * velX + velZ * velZ < 1.0f || velX * toGaro.x + velZ * toGaro.z <= 0.0f) {
                continue;
            }
            GaroForm_ReflectShot(play, player, actor);
            return 1;
        }
    }
    return 0;
}

static u8 GaroForm_ShotAlive(PlayState* play, Actor* shot) {
    if (shot == NULL) {
        return 0;
    }
    Actor* it = play->actorCtx.actorLists[shot->category].first;
    while (it != NULL) {
        if (it == shot) {
            return shot->update != NULL;
        }
        it = it->next;
    }
    return 0;
}

// The stance that started the escort is over by the next frame; the shot keeps hurting all the way out.
static void GaroForm_UpdateReflect(PlayState* play, Player* player) {
    if (sGaro.reflectTimer <= 0) {
        return;
    }
    sGaro.reflectTimer--;
    if (!GaroForm_ShotAlive(play, sGaro.reflectShot)) {
        sGaro.reflectShot = NULL;
        sGaro.reflectTimer = 0;
        return;
    }
    GaroForm_EnsureQuads(play, player);
    GaroForm_StampSquareQuad(play, &sReflectQuad, &sGaro.reflectShot->world.pos, GARO_REFLECT_HALF, GARO_STRIKE_DMG,
                             GARO_REFLECT_DAMAGE);
}

// ---- reset ----------------------------------------------------------------------------------------
static void GaroForm_ResetToIdle(Player* player) {
    if (player == NULL) {
        sGaro.state = GARO_IDLE;
        return;
    }
    if (sGaro.state == GARO_SPIN) {
        player->actor.shape.rot.y = (player->speedXZ > 0.5f) ? player->actor.world.rot.y : sGaro.spinEntryYaw;
        player->yaw = player->actor.shape.rot.y;
    }
    sGaro.state = GARO_IDLE;
    sGaro.timer = 0;
    sGaro.parryAttacker = NULL;
    sGaro.banishTarget = NULL;
    player->stateFlags2 &= ~GARO_STATE2_DISABLE_DRAW;
    sGaro.hopAirTimer = 0;
    sGaro.landStrikeTail = 0;
    sGaro.strikeSfxPlayed = 0;
    sGaro.rodChargeTimer = 0;
    sGaro.rodBallScale = 0.0f;
    player->actor.world.rot.y = player->actor.shape.rot.y;
    Forms_Release(player);
    GaroForm_DisableStrikeQuad(player);
}

void GaroForm_Reset(Player* player) {
    if (gPlayState != NULL) {
        GaroForm_KillTrail(gPlayState);
    }
    if (!GaroForm_IsActive()) {
        GaroForm_RemoveGreen();
    }
    GaroForm_ResetToIdle(player);
    memset(sGaro.orbs, 0, sizeof(sGaro.orbs));
    sGaro.reflectShot = NULL;
    sGaro.reflectTimer = 0;
    sGaro.banishCooldown = 0;
    sGaro.rodReleaseCd = 0;
    sGaro.laughPending = 0;
    sGaro.deathFlamesSpawned = 0;
    sQuadsInited = 0;
}

u8 GaroForm_OwnsAction(void) {
    return sGaro.state != GARO_IDLE && sGaro.state != GARO_ROD_AIM && sGaro.state != GARO_LAUGH_TAUNT;
}

// Mirrors the doActionA ladder in Player_Update: doors, talk/check, climb prompts and item pickups keep
// their A; the grab prompt is left out on purpose (Garo cannot lift objects).
u8 GaroForm_VanillaWantsAButton(Player* player) {
    if (player->doorType != PLAYER_DOORTYPE_NONE) {
        return 1;
    }
    if ((player->stateFlags2 & PLAYER_STATE2_CAN_ACCEPT_TALK_OFFER) && player->talkActor != NULL) {
        return 1;
    }
    if ((player->stateFlags2 & PLAYER_STATE2_4) || player->rideActor != NULL) {
        return 1;
    }
    return player->interactRangeActor != NULL && player->getItemId < GI_NONE;
}

// B and R belong to Garo; A too unless vanilla needs it. Grab is deliberately not passed through.
void GaroForm_FilterInput(Player* player, Input* input) {
    input->cur.button &= ~(BTN_B | BTN_R);
    input->press.button &= ~(BTN_B | BTN_R);
    if (!GaroForm_VanillaWantsAButton(player)) {
        input->cur.button &= ~BTN_A;
        input->press.button &= ~BTN_A;
    }
}

// ---- states ---------------------------------------------------------------------------------------
static void GaroForm_Plant(Player* player) {
    Forms_Pause(player);
    player->speedXZ = 0.0f;
    player->actor.speed = 0.0f;
}

static void GaroForm_StartHop(Player* player, PlayState* play, GaroState state, s32 animId, s16 yawOffset, f32 lift,
                              f32 speed) {
    GaroForm_StartAnim(play, player, animId, 1.0f, 0);
    player->actor.world.rot.y = player->actor.shape.rot.y + yawOffset;
    player->actor.velocity.y = lift;
    player->speedXZ = speed;
    player->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
    sGaro.state = state;
    sGaro.timer = 0;
    sGaro.hopAirTimer = 0;
    Player_AnimSfx_PlayVoice(player, NA_SE_VO_LI_AUTO_JUMP);
    SPDLOG_INFO("[Garo] hop state {} anim {}", (int)state, (void*)GaroForm_Anim(animId));
}

static void GaroForm_StartSpin(PlayState* play, Player* player) {
    GaroForm_StartAnim(play, player, A_SPIN, 1.5f, 0);
    sGaro.state = GARO_SPIN;
    sGaro.timer = 0;
    sGaro.bHoldDetect = 0;
    sGaro.spinEntryYaw = player->actor.shape.rot.y;
    GaroForm_SpawnTrail(play);
    Player_PlaySfx(player, NA_SE_IT_SWORD_SWING_HARD);
}

static void GaroForm_StartBanish(PlayState* play, Player* player, Actor* target, u8 targetIsEnemy) {
    GaroForm_StartAnim(play, player, A_COLLAPSE, 1.0f, 0);
    sGaro.state = GARO_BANISH_VANISH;
    sGaro.timer = 0;
    sGaro.banishTarget = target;
    sGaro.banishCooldown = GARO_BANISH_COOLDOWN;
    Player_PlaySfx(player, NA_SE_PL_MAGIC_WIND_WARP);
    if (targetIsEnemy && target != NULL) {
        GaroForm_StunTarget(play, target);
    }
}

static void GaroForm_TickIdle(Player* player, PlayState* play, const Input* in) {
    u8 bPress = CHECK_BTN_ALL(in->press.button, BTN_B);
    u8 bHold = CHECK_BTN_ALL(in->cur.button, BTN_B);
    u8 rPress = CHECK_BTN_ALL(in->press.button, BTN_R);
    u8 aPress = CHECK_BTN_ALL(in->press.button, BTN_A);
    u8 onGround = Forms_OnGround(player);
    Actor* zTarget = (Player_IsZTargeting(player) && player->focusActor != NULL) ? player->focusActor : NULL;
    u8 zEnemy = zTarget != NULL && zTarget->category == ACTORCAT_ENEMY;
    u8 zEngaged = CHECK_BTN_ALL(in->cur.button, BTN_Z) || Player_IsZTargeting(player);
    s32 stickDir = player->controlStickDirections[player->controlStickDataIndex];
    u8 stickActive = Forms_StickMagnitude(in) >= 10.0f;
    u8 aForGaro = aPress && !GaroForm_VanillaWantsAButton(player);
    u8 rodReady = sGaro.rodReleaseCd == 0;

    player->stateFlags2 &= ~PLAYER_STATE2_1;
    player->interactRangeActor = NULL;

    if (rPress && !bHold) {
        GaroForm_StartGuardRaise(play, player);
        sGaro.state = GARO_PARRY_GUARD;
        sGaro.timer = 0;
        sGaro.parryAttacker = NULL;
        player->cylinder.base.ac = NULL;
        return;
    }
    if (bPress && !onGround && rodReady) {
        GaroForm_StartAnim(play, player, A_SLASH_LOOP, 1.0f, 0);
        sGaro.state = GARO_AIR_SLASH;
        sGaro.timer = 0;
        GaroForm_SpawnTrail(play);
        Player_PlaySfx(player, NA_SE_IT_SWORD_SWING);
        return;
    }
    if (aForGaro && zEngaged && onGround && rodReady) {
        if (stickActive && stickDir == PLAYER_STICK_DIR_BACKWARD) {
            GaroForm_StartHop(player, play, GARO_BACKFLIP, A_JUMP_BACK, 0x8000, 5.8f, GARO_BACKFLIP_SPEED);
            return;
        }
        if (stickActive && stickDir == PLAYER_STICK_DIR_LEFT) {
            GaroForm_StartHop(player, play, GARO_SIDEHOP_L, A_BOUNCE, 0x4000, 4.5f, GARO_HOP_SPEED);
            return;
        }
        if (stickActive && stickDir == PLAYER_STICK_DIR_RIGHT) {
            GaroForm_StartHop(player, play, GARO_SIDEHOP_R, A_BOUNCE, -0x4000, 4.5f, GARO_HOP_SPEED);
            return;
        }
        if (stickActive && stickDir == PLAYER_STICK_DIR_FORWARD && player->speedXZ > 0.5f) {
            f32 keepFwd = (player->speedXZ > 10.0f) ? player->speedXZ : 10.0f;
            GaroForm_StartHop(player, play, GARO_JUMP_ATTACK, A_APPEAR, 0, 7.5f, keepFwd);
            player->skelAnime.playSpeed = 1.5f;
            return;
        }
        if (!stickActive && sGaro.banishCooldown == 0) {
            GaroForm_StartBanish(play, player, zTarget, zEnemy);
            return;
        }
    }
    if (aForGaro && !zEngaged && onGround && rodReady) {
        sGaro.state = GARO_DASH_ATTACK;
        sGaro.timer = 0;
        return;
    }
    if (bPress && onGround && rodReady) {
        GaroForm_StartSpin(play, player);
        return;
    }
    if (sGaro.laughPending) {
        sGaro.laughPending = 0;
        if (Rand_ZeroOne() < GARO_LAUGH_CHANCE) {
            GaroForm_StartAnim(play, player, A_LAUGH, 1.0f, 0);
            sGaro.state = GARO_LAUGH_TAUNT;
            sGaro.timer = 0;
            Player_PlaySfx(player, NA_SE_EN_BOSU_LAUGH);
        }
    }
}

// shape.rot.y is the SPIN, written absolutely (Player_UpdateShapeYaw already dragged it toward the
// lock-on this frame); player->yaw is the STEERING field Player_UpdateCommon copies into world.rot.y.
static void GaroForm_TickSpin(Player* player, PlayState* play, const Input* in) {
    u8 bHold = CHECK_BTN_ALL(in->cur.button, BTN_B);
    if (bHold && ++sGaro.bHoldDetect >= GARO_B_HOLD_THRESHOLD) {
        GaroForm_DisableStrikeQuad(player);
        player->actor.shape.rot.y = (player->speedXZ > 0.5f) ? player->actor.world.rot.y : sGaro.spinEntryYaw;
        player->yaw = player->actor.shape.rot.y;
        sGaro.rodChargeTimer = 0;
        sGaro.rodBallScale = 0.0f;
        sGaro.rodSfxPlayed = 0;
        GaroForm_KillTrail(play);
        GaroForm_StartAnim(play, player, A_TAKE_OUT, 1.0f, 0);
        sGaro.state = GARO_ROD_AIM;
        sGaro.timer = 0;
        return;
    }
    sGaro.timer++;
    player->actor.shape.rot.y = (s16)(sGaro.spinEntryYaw + sGaro.timer * GARO_SPIN_YAW_RATE);
    f32 stickMag = Forms_StickMagnitude(in);
    f32 stickNorm = (stickMag > 80.0f) ? 1.0f : stickMag / 80.0f;
    if (stickNorm > 0.1f) {
        s16 moveYaw = Forms_StickWorldYaw(play, in);
        player->yaw = moveYaw;
        player->actor.world.rot.y = moveYaw;
        player->speedXZ = stickNorm * GARO_SPIN_STEER_SPEED;
    } else {
        player->speedXZ = 0.0f;
    }
    Forms_Pause(player);
    GaroForm_EnableSpinQuad(play, player);
    if (GaroForm_Advance(play, player)) {
        GaroForm_StartAnim(play, player, A_SPIN, 1.5f, 0);
    }
    if (sGaro.timer >= GARO_SPIN_FRAMES) {
        GaroForm_DisableStrikeQuad(player);
        GaroForm_ResetToIdle(player);
    }
}

// The charge SIZE steps by tier so what you see is what the shot will do; going silent at max is
// the "full" tell.
static void GaroForm_TickRodAim(Player* player, PlayState* play, const Input* in) {
    u8 bHold = CHECK_BTN_ALL(in->cur.button, BTN_B);
    if (CHECK_BTN_ALL(in->press.button, BTN_A)) {
        GaroForm_ResetToIdle(player);
        sGaro.rodReleaseCd = GARO_ROD_RELEASE_CD;
        return;
    }
    if (!bHold) {
        GaroForm_FireRodOrb(player, play);
        sGaro.rodReleaseCd = GARO_ROD_RELEASE_CD;
        GaroForm_ResetToIdle(player);
        return;
    }
    sGaro.timer++;
    if (sGaro.rodChargeTimer < GARO_ROD_CHARGE_MAX) {
        sGaro.rodChargeTimer++;
    }
    static const f32 sLevelScale[3] = { 0.35f, 0.7f, 1.0f };
    Math_ApproachF(&sGaro.rodBallScale, 7.0f * sLevelScale[GaroForm_RodLevel() - 1], 0.3f, 1.0f);
    // Stock light-ball sparks so the charge reads even where the Ganon orb DLs are unavailable.
    if ((sGaro.timer & 1) == 0) {
        Vec3f ballPos;
        Vec3f zero = { 0.0f, 0.0f, 0.0f };
        GaroForm_ChargeBallPos(player, &ballPos);
        EffectSsFhgFlash_SpawnLightBall(play, &ballPos, &zero, &zero, (s16)(40.0f * sGaro.rodBallScale / 7.0f + 20.0f),
                                        GaroForm_ElementLightBall(sGaro.rodElement));
    }
    if (sGaro.rodChargeTimer < GARO_ROD_CHARGE_MAX) {
        u16 chargeSfx = NA_SE_IT_SWORD_CHARGE;
        if (sGaro.rodElement == GARO_EL_FIRE) {
            chargeSfx = NA_SE_PL_ARROW_CHARGE_FIRE;
        } else if (sGaro.rodElement == GARO_EL_ICE) {
            chargeSfx = NA_SE_PL_ARROW_CHARGE_ICE;
        } else if (sGaro.rodElement == GARO_EL_LIGHT) {
            chargeSfx = NA_SE_PL_ARROW_CHARGE_LIGHT;
        }
        Actor_PlaySfx_Flagged(&player->actor, chargeSfx);
    }
    if (sGaro.rodChargeTimer >= GARO_ROD_CHARGE_TIER3 && !sGaro.rodSfxPlayed) {
        sGaro.rodSfxPlayed = 1;
        Player_PlaySfx(player, NA_SE_SY_SYNTH_MAGIC_ARROW);
    }
    if (CHECK_BTN_ALL(in->press.button, BTN_L)) {
        sGaro.rodElement = (sGaro.rodElement + GARO_ROD_ELEMENT_COUNT - 1) % GARO_ROD_ELEMENT_COUNT;
        Player_PlaySfx(player, NA_SE_SY_DECIDE);
    }
    if (CHECK_BTN_ALL(in->press.button, BTN_R)) {
        sGaro.rodElement = (sGaro.rodElement + 1) % GARO_ROD_ELEMENT_COUNT;
        Player_PlaySfx(player, NA_SE_SY_DECIDE);
    }
    GaroForm_Plant(player);
    // He turns with the sight so the shot leaves along where he looks.
    s16 aimYaw;
    s16 aimPitch;
    GaroForm_AimAngles(player, &aimYaw, &aimPitch);
    player->actor.shape.rot.y = aimYaw;
    player->actor.world.rot.y = aimYaw;
    player->yaw = aimYaw;
    GaroForm_Advance(play, player);
}

static void GaroForm_StartGuardReturn(PlayState* play, Player* player) {
    GaroForm_StartAnim(play, player, A_GUARD, -GARO_GUARD_RETURN_SPEED, 0);
    sGaro.state = GARO_GUARD_RETURN;
    sGaro.timer = 0;
}

// The guard is a straight trade: he keeps the health he just lost, only DAMAGED is cleared because
// the knockback would drag him out of the counter.
static void GaroForm_TickParryGuard(Player* player, PlayState* play, const Input* in) {
    GaroForm_Plant(player);
    GaroForm_Advance(play, player);
    if (GaroForm_TryReflectIncoming(play, player)) {
        GaroForm_StartGuardReturn(play, player);
        return;
    }
    Actor* attacker = player->cylinder.base.ac;
    u8 gotHit = (attacker != NULL && attacker->update != NULL) || (player->stateFlags1 & FORMS_STATE1_DAMAGED);
    if (gotHit) {
        player->cylinder.base.ac = NULL;
        player->stateFlags1 &= ~FORMS_STATE1_DAMAGED;
        if (attacker == NULL) {
            attacker = player->focusActor;
        }
        if (attacker == NULL) {
            GaroForm_StartGuardReturn(play, player);
            return;
        }
        Vec3f diff;
        f32 dist = Math_Vec3f_DistXYZAndStoreDiff(&attacker->world.pos, &player->actor.world.pos, &diff);
        if (dist > GARO_GUARD_MELEE_RANGE || GaroForm_IsRangedAttacker(attacker)) {
            if (GaroForm_IsRangedAttacker(attacker)) {
                GaroForm_ReflectShot(play, player, attacker);
            }
            GaroForm_StartGuardReturn(play, player);
            return;
        }
        GaroForm_StunTarget(play, attacker);
        s16 aYaw = attacker->shape.rot.y;
        player->actor.world.pos.x = attacker->world.pos.x - Math_SinS(aYaw) * GARO_RIPOSTE_OFFSET;
        player->actor.world.pos.y = attacker->world.pos.y;
        player->actor.world.pos.z = attacker->world.pos.z - Math_CosS(aYaw) * GARO_RIPOSTE_OFFSET;
        player->actor.world.rot.y = aYaw;
        player->actor.shape.rot.y = aYaw;
        player->yaw = aYaw;
        sGaro.parryAttacker = attacker;
        GaroForm_StartAnim(play, player, A_DRAW, 1.0f, 0);
        sGaro.state = GARO_PARRY_RIPOSTE;
        sGaro.timer = 0;
        sGaro.strikeSfxPlayed = 0;
        GaroForm_SpawnTrail(play);
        Player_PlaySfx(player, NA_SE_PL_MAGIC_WIND_WARP);
        return;
    }
    sGaro.timer++;
    if (!CHECK_BTN_ALL(in->cur.button, BTN_R)) {
        GaroForm_StartGuardReturn(play, player);
    }
}

static void GaroForm_TickGuardReturn(Player* player, PlayState* play) {
    GaroForm_Plant(player);
    sGaro.timer++;
    if (GaroForm_Advance(play, player)) {
        GaroForm_ResetToIdle(player);
    }
}

// A frozen actor never re-registers its AC collider, so the target thaws a couple of frames before
// the strike goes live; the colour filter carries the stunned look.
static void GaroForm_TickRiposte(Player* player, PlayState* play) {
    GaroForm_Plant(player);
    if (sGaro.timer >= GARO_RIPOSTE_HIT_FRAME - 2 && sGaro.parryAttacker != NULL) {
        sGaro.parryAttacker->freezeTimer = 0;
    }
    if (sGaro.timer >= GARO_RIPOSTE_HIT_FRAME) {
        GaroForm_EnableLandStrikeQuad(play, player);
        player->meleeWeaponQuads[0].elem.atDmgInfo.damage = GARO_DASH_DAMAGE;
        if (!sGaro.strikeSfxPlayed) {
            sGaro.strikeSfxPlayed = 1;
            Player_PlaySfx(player, NA_SE_IT_SWORD_SWING_HARD);
        }
    } else {
        GaroForm_DisableStrikeQuad(player);
    }
    sGaro.timer++;
    if (GaroForm_Advance(play, player)) {
        GaroForm_DisableStrikeQuad(player);
        GaroForm_ResetToIdle(player);
    }
}

// Turn rate is Pegasus-boots-heavy so the player commits to a direction (Goron-roll feel).
static void GaroForm_TickDash(Player* player, PlayState* play, const Input* in) {
    if (player->skelAnime.animation != GaroForm_Anim(A_DASH) && GaroForm_Anim(A_DASH) != NULL) {
        PlayerAnimationHeader* anim = GaroForm_Anim(A_DASH);
        Forms_PlayClip(play, player, anim, 1.0f, 0.0f, Animation_GetLastFrame(anim), 1, -4.0f);
    }
    Forms_Pause(player);
    f32 stickMag = Forms_StickMagnitude(in);
    if (stickMag / 80.0f > 0.3f) {
        Math_ScaledStepToS(&player->actor.world.rot.y, Forms_StickWorldYaw(play, in), GARO_DASH_TURN);
    }
    player->actor.shape.rot.y = player->actor.world.rot.y;
    player->yaw = player->actor.world.rot.y;
    player->speedXZ = GARO_DASH_SPEED;
    GaroForm_EnableSwingQuad(play, player, GARO_DASH_DAMAGE);
    GaroForm_Advance(play, player);
    if ((player->actor.bgCheckFlags & BGCHECKFLAG_WALL) || !CHECK_BTN_ALL(in->cur.button, BTN_A)) {
        player->speedXZ = 0.0f;
        GaroForm_ResetToIdle(player);
    }
    sGaro.timer++;
}

static void GaroForm_TickBanishVanish(Player* player, PlayState* play) {
    GaroForm_Plant(player);
    u8 done = GaroForm_Advance(play, player);
    if (sGaro.timer < GARO_BANISH_VANISH_END) {
        player->actor.world.pos.y -= 1.0f;
    }
    if (sGaro.timer == GARO_BANISH_VANISH_END) {
        player->stateFlags2 |= GARO_STATE2_DISABLE_DRAW;
    }
    sGaro.timer++;
    if (!done && sGaro.timer < GARO_BANISH_VANISH_END) {
        return;
    }
    Actor* target = sGaro.banishTarget;
    u8 valid = target != NULL && target->update != NULL;
    sGaro.shadowStart = player->actor.world.pos;
    sGaro.shadowStart.y += 30.0f;
    if (valid) {
        sGaro.shadowEnd = target->world.pos;
    } else {
        sGaro.shadowEnd = player->actor.world.pos;
        sGaro.shadowEnd.x += Math_SinS(player->actor.shape.rot.y) * 60.0f;
        sGaro.shadowEnd.z += Math_CosS(player->actor.shape.rot.y) * 60.0f;
    }
    sGaro.shadowEnd.y += 30.0f;
    sGaro.shadowTimer = 0;
    sGaro.state = GARO_BANISH_SHADOW;
    Player_PlaySfx(player, NA_SE_PL_MAGIC_WIND_WARP);
}

static void GaroForm_TickBanishShadow(Player* player, PlayState* play) {
    GaroForm_Plant(player);
    player->stateFlags2 |= GARO_STATE2_DISABLE_DRAW;
    f32 t = sGaro.shadowTimer / (f32)GARO_BANISH_SHADOW_LEN;
    if (t > 1.0f) {
        t = 1.0f;
    }
    sGaro.shadowPos.x = sGaro.shadowStart.x + (sGaro.shadowEnd.x - sGaro.shadowStart.x) * t;
    sGaro.shadowPos.y = sGaro.shadowStart.y + (sGaro.shadowEnd.y - sGaro.shadowStart.y) * t;
    sGaro.shadowPos.z = sGaro.shadowStart.z + (sGaro.shadowEnd.z - sGaro.shadowStart.z) * t;
    Vec3f back = { sGaro.shadowStart.x - sGaro.shadowEnd.x, 0.0f, sGaro.shadowStart.z - sGaro.shadowEnd.z };
    f32 len = sqrtf(back.x * back.x + back.z * back.z);
    if (len > 0.001f) {
        back.x = back.x / len * 10.0f;
        back.z = back.z / len * 10.0f;
    }
    for (s32 i = 0; i < 2; i++) {
        Vec3f pos = { sGaro.shadowPos.x + back.x + Rand_CenteredFloat(8.0f),
                      sGaro.shadowPos.y + Rand_CenteredFloat(8.0f),
                      sGaro.shadowPos.z + back.z + Rand_CenteredFloat(8.0f) };
        Vec3f zero = { 0.0f, 0.0f, 0.0f };
        Color_RGBA8 prim = { 110, 40, 190, 220 };
        Color_RGBA8 env = { 25, 0, 60, 0 };
        EffectSsDust_Spawn(play, 0, &pos, &zero, &zero, &prim, &env, 90, -6, 8, 0);
    }
    sGaro.shadowTimer++;
    if (sGaro.shadowTimer < GARO_BANISH_SHADOW_LEN) {
        return;
    }
    Actor* target = sGaro.banishTarget;
    if (target != NULL && target->update != NULL) {
        s16 tYaw = target->shape.rot.y;
        player->actor.world.pos.x = target->world.pos.x - Math_SinS(tYaw) * GARO_BANISH_OFFSET;
        player->actor.world.pos.y = target->world.pos.y;
        player->actor.world.pos.z = target->world.pos.z - Math_CosS(tYaw) * GARO_BANISH_OFFSET;
        player->actor.world.rot.y = tYaw;
        player->actor.shape.rot.y = tYaw;
        player->yaw = tYaw;
    }
    GaroForm_StartAnim(play, player, A_APPEAR_DRAW, GARO_SHADOW_BALL_PLAYSPEED, 0);
    player->stateFlags2 &= ~GARO_STATE2_DISABLE_DRAW;
    Player_PlaySfx(player, NA_SE_PL_MAGIC_WIND_WARP);
    sGaro.state = GARO_SHADOW_BALL;
    sGaro.timer = 0;
    sGaro.strikeSfxPlayed = 0;
    GaroForm_SpawnTrail(play);
}

// Exit waits for the hit window: played faster, the arrival ends before the strike frame.
static void GaroForm_TickShadowBall(Player* player, PlayState* play) {
    GaroForm_Plant(player);
    player->stateFlags2 &= ~GARO_STATE2_DISABLE_DRAW;
    if (player->invincibilityTimer < 5) {
        player->invincibilityTimer = 5;
    }
    if (sGaro.timer >= GARO_SHADOW_BALL_HIT_F - 3 && sGaro.banishTarget != NULL && sGaro.banishTarget->update != NULL) {
        sGaro.banishTarget->freezeTimer = 0;
    }
    if (sGaro.timer >= GARO_SHADOW_BALL_HIT_F && sGaro.timer <= GARO_SHADOW_BALL_HIT_F + 4) {
        GaroForm_EnableFrontSweepQuad(play, player, sGaro.timer - GARO_SHADOW_BALL_HIT_F, GARO_SHADOW_BALL_DAMAGE);
        if (!sGaro.strikeSfxPlayed) {
            sGaro.strikeSfxPlayed = 1;
            Player_PlaySfx(player, NA_SE_IT_SWORD_SWING_HARD);
        }
    } else {
        GaroForm_DisableStrikeQuad(player);
    }
    u8 done = GaroForm_Advance(play, player);
    sGaro.timer++;
    if (done && sGaro.timer > GARO_SHADOW_BALL_HIT_F + 4) {
        GaroForm_DisableStrikeQuad(player);
        GaroForm_ResetToIdle(player);
    }
}

static void GaroForm_TickLaugh(Player* player, PlayState* play, const Input* in) {
    u8 done = GaroForm_Advance(play, player);
    sGaro.timer++;
    if (done || CHECK_BTN_ALL(in->press.button, BTN_A | BTN_B | BTN_R)) {
        GaroForm_ResetToIdle(player);
    }
}

static void GaroForm_TickHop(Player* player, PlayState* play) {
    Forms_Pause(player);
    GaroForm_DisableStrikeQuad(player);
    GaroForm_Advance(play, player);
    sGaro.timer++;
    sGaro.hopAirTimer++;
    if (Forms_OnGround(player) && sGaro.hopAirTimer >= 3) {
        GaroForm_ResetToIdle(player);
    }
}

static void GaroForm_StartLandStrike(PlayState* play, Player* player) {
    GaroForm_StartAnim(play, player, A_DRAW, 1.0f, 0);
    sGaro.state = GARO_LAND_STRIKE;
    sGaro.timer = 0;
    sGaro.strikeSfxPlayed = 0;
    Player_PlaySfx(player, NA_SE_PL_ROLL_DUST);
}

static void GaroForm_TickJumpAttack(Player* player, PlayState* play) {
    Forms_Pause(player);
    GaroForm_Advance(play, player);
    sGaro.timer++;
    sGaro.hopAirTimer++;
    if (sGaro.hopAirTimer >= 6) {
        GaroForm_StartAnim(play, player, A_SLASH_LOOP, 1.0f, 0);
        sGaro.state = GARO_AIR_SLASH;
        sGaro.timer = 0;
        GaroForm_SpawnTrail(play);
        Player_PlaySfx(player, NA_SE_IT_SWORD_SWING);
        return;
    }
    if (Forms_OnGround(player) && sGaro.hopAirTimer >= 3) {
        GaroForm_StartLandStrike(play, player);
    }
}

static void GaroForm_TickAirSlash(Player* player, PlayState* play) {
    Forms_Pause(player);
    GaroForm_DisableStrikeQuad(player);
    if (GaroForm_Advance(play, player)) {
        GaroForm_StartAnim(play, player, A_SLASH_LOOP, 1.0f, 0);
    }
    sGaro.timer++;
    if (Forms_OnGround(player) && sGaro.timer >= 2) {
        GaroForm_StartLandStrike(play, player);
    }
}

static void GaroForm_TickLandStrike(Player* player, PlayState* play) {
    GaroForm_Plant(player);
    if (sGaro.timer >= GARO_LAND_STRIKE_HIT_F) {
        GaroForm_EnableLandStrikeQuad(play, player);
        if (!sGaro.strikeSfxPlayed) {
            sGaro.strikeSfxPlayed = 1;
            Player_PlaySfx(player, NA_SE_IT_SWORD_SWING_HARD);
        }
    }
    u8 done = GaroForm_Advance(play, player);
    sGaro.timer++;
    if (!done) {
        return;
    }
    if (++sGaro.landStrikeTail > GARO_LAND_STRIKE_TAIL_F) {
        GaroForm_DisableStrikeQuad(player);
        GaroForm_ResetToIdle(player);
    }
}

// A vanished enemy pointer between two frames that both had enemies = a kill worth a laugh.
static void GaroForm_DetectKill(PlayState* play) {
    s32 count = 0;
    Actor* it = play->actorCtx.actorLists[ACTORCAT_ENEMY].first;
    while (it != NULL) {
        count++;
        it = it->next;
    }
    if (sGaro.enemyCount > 0 && count > 0 && count < sGaro.enemyCount) {
        sGaro.laughPending = 1;
    }
    sGaro.enemyCount = count;
}

void GaroForm_Update(Player* player, PlayState* play) {
    GaroForm_LoadAnims();
    GaroForm_ApplyGreen();
    GaroForm_UpdateOrbs(play, player);
    GaroForm_UpdateReflect(play, player);
    GaroForm_DetectKill(play);

    u8 trailWanted = sGaro.state == GARO_SPIN || sGaro.state == GARO_AIR_SLASH || sGaro.state == GARO_LAND_STRIKE ||
                     sGaro.state == GARO_SHADOW_BALL || sGaro.state == GARO_PARRY_RIPOSTE;
    if (!trailWanted) {
        GaroForm_KillTrail(play);
    }
    if (sGaro.banishCooldown > 0) {
        sGaro.banishCooldown--;
    }
    if (sGaro.rodReleaseCd > 0) {
        sGaro.rodReleaseCd--;
    }
    u32 hardBlock = FORMS_STATE1_LOADING | PLAYER_STATE1_TALKING | PLAYER_STATE1_DEAD | FORMS_STATE1_GETTING_ITEM |
                    PLAYER_STATE1_CARRYING_ACTOR | FORMS_STATE1_CLIMBING_LEDGE | FORMS_STATE1_HANGING |
                    PLAYER_STATE1_IN_CUTSCENE | PLAYER_STATE1_10000000;
    if ((player->stateFlags1 & hardBlock) || player->csAction != PLAYER_CSACTION_NONE ||
        play->msgCtx.msgMode != MSGMODE_NONE) {
        GaroForm_KillTrail(play);
        GaroForm_ResetToIdle(player);
        return;
    }
    // The guard chain is exempt: being hit is what ARMS the counter.
    if ((player->stateFlags1 & FORMS_STATE1_DAMAGED) && sGaro.state != GARO_PARRY_GUARD &&
        sGaro.state != GARO_PARRY_RIPOSTE) {
        GaroForm_KillTrail(play);
        GaroForm_ResetToIdle(player);
        return;
    }
    if (sGaro.state != GARO_ROD_AIM) {
        player->heldItemAction = PLAYER_IA_NONE;
        player->itemAction = PLAYER_IA_NONE;
    }

    const Input* in = Forms_RawInput();
    switch (sGaro.state) {
        case GARO_IDLE:
            GaroForm_TickIdle(player, play, in);
            break;
        case GARO_SPIN:
            GaroForm_TickSpin(player, play, in);
            break;
        case GARO_ROD_AIM:
            GaroForm_TickRodAim(player, play, in);
            break;
        case GARO_PARRY_GUARD:
            GaroForm_TickParryGuard(player, play, in);
            break;
        case GARO_GUARD_RETURN:
            GaroForm_TickGuardReturn(player, play);
            break;
        case GARO_PARRY_RIPOSTE:
            GaroForm_TickRiposte(player, play);
            break;
        case GARO_DASH_ATTACK:
            GaroForm_TickDash(player, play, in);
            break;
        case GARO_BANISH_VANISH:
            GaroForm_TickBanishVanish(player, play);
            break;
        case GARO_BANISH_SHADOW:
            GaroForm_TickBanishShadow(player, play);
            break;
        case GARO_SHADOW_BALL:
            GaroForm_TickShadowBall(player, play);
            break;
        case GARO_LAUGH_TAUNT:
            GaroForm_TickLaugh(player, play, in);
            break;
        case GARO_SIDEHOP_L:
        case GARO_SIDEHOP_R:
        case GARO_BACKFLIP:
            GaroForm_TickHop(player, play);
            break;
        case GARO_JUMP_ATTACK:
            GaroForm_TickJumpAttack(player, play);
            break;
        case GARO_AIR_SLASH:
            GaroForm_TickAirSlash(player, play);
            break;
        case GARO_LAND_STRIKE:
            GaroForm_TickLandStrike(player, play);
            break;
        default:
            GaroForm_ResetToIdle(player);
            break;
    }
}

// ---- death ----------------------------------------------------------------------------------------
// MM Garo Master death canon: nine flames in a ring; they live in the effect system and survive the
// death cutscene.
void GaroForm_OnDeath(Player* player, PlayState* play) {
    if (sGaro.deathFlamesSpawned) {
        return;
    }
    Vec3f center = player->actor.world.pos;
    center.y += 5.0f;
    for (s32 i = 0; i < GARO_DEATH_FLAME_COUNT; i++) {
        f32 ang = i * (2.0f * (f32)M_PI / GARO_DEATH_FLAME_COUNT);
        Vec3f pos = { center.x + cosf(ang) * GARO_DEATH_FLAME_RADIUS, center.y,
                      center.z + sinf(ang) * GARO_DEATH_FLAME_RADIUS };
        Vec3f vel = { cosf(ang) * 0.5f, 1.5f, sinf(ang) * 0.5f };
        Vec3f accel = { 0.0f, 0.1f, 0.0f };
        EffectSsDFire_Spawn(play, &pos, &vel, &accel, 100, 35, 255, 8, 12, 60);
    }
    sGaro.deathFlamesSpawned = 1;
    Actor_PlaySfx(&player->actor, NA_SE_EN_STAL_DEAD);
}

// ---- voice ----------------------------------------------------------------------------------------
// MM has no Garo player bank; Igos du Ikana's lines are paired by TONE, every action index filled so
// nothing falls through to Link's grunt.
u16 GaroForm_VoiceFor(u16 linkVoiceSfx) {
    static const u16 sByAction[0x20] = {
        NA_SE_EN_BOSU_ATTACK,     NA_SE_EN_BOSU_ATTACK_K, NA_SE_EN_BOSU_ATTACK_W, NA_SE_EN_BOSU_HAND,
        NA_SE_EN_BOSU_STAND,      NA_SE_EN_BOSU_DAMAGE,   NA_SE_EN_BOSU_SHOCK,    NA_SE_EN_BOSU_DAMAGE,
        NA_SE_EN_BOSU_DEAD_VOICE, NA_SE_EN_BOSU_SIT,      NA_SE_EN_BOSU_SIT,      NA_SE_EN_BOSU_DEAD,
        NA_SE_EN_BOSU_SHOCK,      NA_SE_EN_BOSU_HAND,     NA_SE_EN_BOSU_SHIT,     NA_SE_EN_BOSU_SIT,
        NA_SE_EN_BOSU_SIT,        NA_SE_EN_BOSU_CYNICAL,  NA_SE_EN_BOSU_SWORD,    NA_SE_EN_BOSU_DAMAGE,
        NA_SE_EN_BOSU_ATTACK_W,   NA_SE_EN_BOSU_ATTACK_K, NA_SE_EN_BOSU_SHOCK,    NA_SE_EN_BOSU_ATTACK_K,
        NA_SE_EN_BOSU_HAND,       NA_SE_EN_BOSU_HAND,     NA_SE_EN_BOSU_DAMAGE,   NA_SE_EN_BOSU_DAMAGE,
        NA_SE_EN_BOSU_ATTACK_K,   NA_SE_EN_BOSU_CYNICAL,  NA_SE_EN_BOSU_ATTACK,   NA_SE_EN_BOSU_LAUGH,
    };
    u16 action = linkVoiceSfx - NA_SE_VO_LI_SWORD_N;
    return (action < 0x20) ? sByAction[action] : NA_SE_EN_BOSU_ATTACK;
}

// ---- hybrid body ----------------------------------------------------------------------------------
// PLAYER_LIMB index -> jso jointTable slot; -1 = no counterpart. Sword + top/back/front robe bones keep
// Garo's own animation; the side robe panels ride Link's shoulders (set after the loop).
static const s8 sLinkLimbToGaroJt[PLAYER_LIMB_MAX] = {
    -1,
    GARO_LIMB_ROOT,
    GARO_LIMB_LOWER_BODY_ROOT,
    -1,
    GARO_LIMB_RIGHT_THIGH,
    GARO_LIMB_RIGHT_SHIN,
    GARO_LIMB_RIGHT_FOOT,
    GARO_LIMB_LEFT_THIGH,
    GARO_LIMB_LEFT_SHIN,
    GARO_LIMB_LEFT_FOOT,
    GARO_LIMB_TORSO,
    GARO_LIMB_HEAD,
    -1,
    -1,
    GARO_LIMB_LEFT_ARM,
    -1,
    GARO_LIMB_LEFT_SWORD,
    GARO_LIMB_RIGHT_ARM,
    -1,
    GARO_LIMB_RIGHT_SWORD,
    -1,
    -1,
};

static u8 GaroForm_IsGaroBone(s32 jt) {
    return jt == GARO_LIMB_LEFT_SWORD || jt == GARO_LIMB_RIGHT_SWORD || jt == GARO_LIMB_ROBE_TOP ||
           jt == GARO_LIMB_ROBE_BACK || jt == GARO_LIMB_ROBE_FRONT;
}

void GaroForm_Pose(Player* player, Vec3s* garoJointTable, s32 garoLimbCount) {
    Vec3s* link = player->skelAnime.jointTable;
    if (link == NULL) {
        return;
    }
    garoJointTable[0] = link[0];
    for (s32 pl = 1; pl < PLAYER_LIMB_MAX; pl++) {
        s8 jt = sLinkLimbToGaroJt[pl];
        if (jt < 0 || jt >= garoLimbCount || GaroForm_IsGaroBone(jt)) {
            continue;
        }
        garoJointTable[jt] = link[pl];
    }
    garoJointTable[GARO_LIMB_ROBE_LEFT] = link[PLAYER_LIMB_LEFT_SHOULDER];
    garoJointTable[GARO_LIMB_ROBE_RIGHT] = link[PLAYER_LIMB_RIGHT_SHOULDER];
}

// The outer matrix is Link's 0.01 (actor sits at Link's height); Garo's enemy is drawn at 0.035, so the
// whole subtree is scaled x3.5 at the ROOT entry. Legs thickened at the thigh, undone at the foot.
s32 GaroForm_OverrideLimb(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot, Actor* actor) {
    if (limbIndex == GARO_LIMB_ROOT) {
        Matrix_TranslateRotateZYX(pos, rot);
        Matrix_Scale(GARO_SUBTREE_SCALE, GARO_SUBTREE_SCALE, GARO_SUBTREE_SCALE, MTXMODE_APPLY);
        return 1;
    }
    if (limbIndex == GARO_LIMB_RIGHT_THIGH || limbIndex == GARO_LIMB_LEFT_THIGH) {
        Matrix_Scale(1.0f, GARO_LEG_THICK, GARO_LEG_THICK, MTXMODE_APPLY);
    } else if (limbIndex == GARO_LIMB_RIGHT_FOOT || limbIndex == GARO_LIMB_LEFT_FOOT) {
        Matrix_Scale(1.0f, 1.0f / GARO_LEG_THICK, 1.0f / GARO_LEG_THICK, MTXMODE_APPLY);
    }
    return 0;
}

static const Vec3f sTrailAxisDirs[6] = {
    { 0.0f, 1.0f, 0.0f },  { 0.0f, -1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f },
    { -1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f },  { 0.0f, 0.0f, -1.0f },
};

// The blade's local axis is picked once per trail: the one whose tip points most outward-and-down.
static s8 GaroForm_PickTrailAxis(Player* player, const Vec3f* baseWorld) {
    f32 outX = baseWorld->x - player->actor.world.pos.x;
    f32 outZ = baseWorld->z - player->actor.world.pos.z;
    f32 outLen = sqrtf(outX * outX + outZ * outZ);
    if (outLen > 0.001f) {
        outX /= outLen;
        outZ /= outLen;
    }
    Vec3f want = { outX * 0.6f, -1.0f, outZ * 0.6f };
    s8 best = 0;
    f32 bestScore = -1.0e9f;
    for (s8 i = 0; i < 6; i++) {
        Vec3f cand = { sTrailAxisDirs[i].x * GARO_TRAIL_LEN, sTrailAxisDirs[i].y * GARO_TRAIL_LEN,
                       sTrailAxisDirs[i].z * GARO_TRAIL_LEN };
        Vec3f tip;
        Matrix_MultVec3f(&cand, &tip);
        Vec3f d = { tip.x - baseWorld->x, tip.y - baseWorld->y, tip.z - baseWorld->z };
        f32 dl = sqrtf(d.x * d.x + d.y * d.y + d.z * d.z);
        if (dl < 0.001f) {
            continue;
        }
        f32 score = (d.x * want.x + d.y * want.y + d.z * want.z) / dl;
        if (score > bestScore) {
            bestScore = score;
            best = i;
        }
    }
    return best;
}

static void GaroForm_FeedTrail(Player* player, s32 slot) {
    EffectBlure* trail = (EffectBlure*)Effect_GetByIndex(sGaro.trailIndex[slot]);
    if (trail == NULL) {
        return;
    }
    Vec3f baseLocal = { 0.0f, 0.0f, 0.0f };
    Vec3f baseWorld;
    Matrix_MultVec3f(&baseLocal, &baseWorld);
    if (sGaro.trailAxis[slot] < 0) {
        sGaro.trailAxis[slot] = GaroForm_PickTrailAxis(player, &baseWorld);
    }
    const Vec3f* axis = &sTrailAxisDirs[sGaro.trailAxis[slot]];
    Vec3f tipLocal = { axis->x * GARO_TRAIL_LEN, axis->y * GARO_TRAIL_LEN, axis->z * GARO_TRAIL_LEN };
    Vec3f tipWorld;
    Matrix_MultVec3f(&tipLocal, &tipWorld);
    EffectBlure_AddVertex(trail, &tipWorld, &baseWorld);
}

// The only place a blade the player can see has its matrix in scope.
void GaroForm_PostLimb(PlayState* play, s32 limbIndex, Gfx** dList, Vec3s* rot, Actor* actor) {
    Player* player = (Player*)actor;
    if (player == NULL || !sGaro.trailActive) {
        return;
    }
    if (limbIndex == GARO_LIMB_LEFT_SWORD) {
        GaroForm_FeedTrail(player, 0);
    } else if (limbIndex == GARO_LIMB_RIGHT_SWORD) {
        GaroForm_FeedTrail(player, 1);
    }
}

// ---- world draw (orbs, charge ball, shadow ball) --------------------------------------------------
static void GaroForm_DrawOrbAt(PlayState* play, Vec3f* pos, f32 scale, const Color_RGBA8* prim,
                               const Color_RGBA8* env) {
    // MM's own light orb (the one the bosses throw), so nothing here depends on oot.o2r.
    OPEN_DISPS(play->state.gfxCtx);
    Matrix_Translate(pos->x, pos->y, pos->z, MTXMODE_NEW);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    Matrix_RotateZS((s16)(play->gameplayFrames * 0x400), MTXMODE_APPLY);
    Matrix_Scale(scale, scale, 1.0f, MTXMODE_APPLY);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gLightOrbMaterial1DL);
    gDPSetEnvColor(POLY_XLU_DISP++, env->r, env->g, env->b, 128);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, prim->r, prim->g, prim->b, prim->a);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)gLightOrbModelDL);
    CLOSE_DISPS(play->state.gfxCtx);
}

static void GaroForm_ElementColors(u8 element, Color_RGBA8* prim, Color_RGBA8* env) {
    static const Color_RGBA8 sPrim[GARO_ROD_ELEMENT_COUNT] = {
        { 255, 200, 0, 255 },   { 170, 255, 255, 255 }, { 255, 255, 170, 255 },
        { 200, 120, 255, 255 }, { 255, 255, 255, 255 }, { 170, 255, 170, 255 },
    };
    static const Color_RGBA8 sEnv[GARO_ROD_ELEMENT_COUNT] = {
        { 255, 0, 0, 255 },   { 0, 150, 255, 255 },   { 255, 255, 0, 255 },
        { 100, 0, 255, 255 }, { 200, 200, 255, 255 }, { 0, 150, 0, 255 },
    };
    *prim = sPrim[element % GARO_ROD_ELEMENT_COUNT];
    *env = sEnv[element % GARO_ROD_ELEMENT_COUNT];
}

static void GaroForm_ChargeBallPos(Player* player, Vec3f* out) {
    s16 yaw;
    s16 pitch;
    GaroForm_AimAngles(player, &yaw, &pitch);
    f32 cosP = Math_CosS(pitch);
    out->x = player->actor.focus.pos.x + Math_SinS(yaw) * cosP * 55.0f;
    out->y = player->actor.focus.pos.y - Math_SinS(pitch) * 55.0f;
    out->z = player->actor.focus.pos.z + Math_CosS(yaw) * cosP * 55.0f;
}

// Two call sites can reach this on one frame; additive orbs drawn twice double their brightness.
void GaroForm_DrawWorld(PlayState* play, Player* player) {
    static u32 sLastDrawFrame = 0xFFFFFFFF;
    if (!GaroForm_IsActive() || sLastDrawFrame == play->gameplayFrames) {
        return;
    }
    sLastDrawFrame = play->gameplayFrames;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);

    for (s32 i = 0; i < GARO_ORB_MAX; i++) {
        GaroOrb* orb = &sGaro.orbs[i];
        if (!orb->active) {
            continue;
        }
        Color_RGBA8 prim;
        Color_RGBA8 env;
        GaroForm_ElementColors(orb->element, &prim, &env);
        f32 scale = orb->isSeeker ? 0.03f : (orb->bursts ? 0.07f : 0.05f);
        GaroForm_DrawOrbAt(play, &orb->pos, scale, &prim, &env);
    }
    if (sGaro.state == GARO_ROD_AIM && sGaro.rodBallScale > 0.01f) {
        Vec3f ballPos;
        GaroForm_ChargeBallPos(player, &ballPos);
        f32 pulse = 1.0f + 0.06f * Math_SinS((s16)(play->gameplayFrames * 0x1000));
        Color_RGBA8 prim;
        Color_RGBA8 env;
        GaroForm_ElementColors(sGaro.rodElement, &prim, &env);
        GaroForm_DrawOrbAt(play, &ballPos, sGaro.rodBallScale * 0.01f * pulse, &prim, &env);
    }
    if (sGaro.state == GARO_BANISH_SHADOW) {
        Color_RGBA8 prim = { 140, 80, 220, 255 };
        Color_RGBA8 env = { 40, 10, 100, 255 };
        GaroForm_DrawOrbAt(play, &sGaro.shadowPos, 0.06f, &prim, &env);
    }
}
