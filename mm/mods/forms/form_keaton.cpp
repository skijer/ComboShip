/*
 * form_keaton.cpp - Keaton moveset (Skijer's NEI). Port of soh's keaton_form / keaton_reflector /
 * keaton_tails: three-hit fist combo on B, charged projectile on B held, long jump on A, air kick,
 * magic-fed wall climb, hexagonal reflector on R, three animated tails and the flute.
 */
#include "forms_internal.h"

extern "C" {
#include "objects/gameplay_keep/gameplay_keep.h"
}

namespace {
#include "keaton_tail_anim.inc.c"
}

#define ANIM_SS "__OTR__misc/link_animetion/gPlayerAnim_mhr_ss_"
#define ANIM_FIELD "__OTR__misc/link_animetion/gPlayerAnim_mhr_field_"
#define REFLECT_ANIM "__OTR__misc/link_animetion/gPlayerAnim_mhr_damage_idle03_loop"
#define TAIL_SKEL_PATH "objects/forms/keaton/object_link_boy/gLinkAdultTails"
#define FLUTE_DL_PATH "__OTR__objects/forms/keaton/object_link_boy/gKeatonFluteDL"

#define FIST_DMG_FLAGS DMG_SWORD
#define FIST_DAMAGE 4
#define FIST_REACH 1200.0f
#define KICK_BLOCK_DAMAGE 8
#define KICK_BLOCK_INVINCIBILITY 20
#define RECOIL_SPEED -18.0f
#define CHAIN_OPENS 0.55f
#define LOOP_START 13.0f
#define LOOP_END 17.0f
#define CHARGE_LEVEL1 20
#define CHARGE_LEVEL2 60
#define SHOT_SPEED 4.5f
#define RANGE_LEVEL1 600.0f
#define RANGE_LEVEL2 1000.0f
#define PULL_RADIUS_LEVEL1 280.0f
#define PULL_RADIUS_LEVEL2 400.0f
#define PULL_STEP 7.0f
#define HADOUKEN_MAGIC_LEVEL1 8
#define HADOUKEN_MAGIC_LEVEL2 16
#define FLAME_ORANGE 0
#define FLAME_BLUE 2
#define BALL_SCALE_LEVEL1 (0.0075f * 0.16f)
#define BALL_SCALE_LEVEL2 (0.0075f * 0.33f)
#define BALL_RADIUS_LEVEL1 34
#define BALL_RADIUS_LEVEL2 60
#define LONG_JUMP_SPEED 16.0f
#define LONG_JUMP_LIFT 8.0f
#define LONG_JUMP_ANIM_DELAY 2
#define AIR_KICK_SPEED 14.0f
#define CLIMB_RATE_MUL 2.0f
#define CLIMB_DRAIN_INTERVAL 10
#define REFLECTOR_POP_STEP 0.28f
#define REFLECTOR_PULSE_MIN 0.95f
#define REFLECTOR_PULSE_PERIOD 24
#define REFLECTOR_HEIGHT 20.0f
#define REFLECT_RADIUS 90.0f
#define REFLECTOR_BURST_DAMAGE 8
#define REFLECTOR_AIR_DRAG 0.35f
#define REFLECTOR_PROJECTILE_SPEED 6.0f
#define REFLECTED_MAX 8
#define REFLECTED_LIFE 40
#define TAIL_SEGMENTS 9
#define TAIL_MTX_SEGMENT 0x0B
#define TAIL_EASE_AMOUNT 0.39f

typedef enum KeatonState {
    KEATON_IDLE,
    KEATON_COMBO,
    KEATON_CHARGE,
    KEATON_THROW,
    KEATON_LONGJUMP,
    KEATON_AIRKICK,
    KEATON_RECOIL
} KeatonState;

typedef struct KeatonMove {
    const char* path;
    u8 vanilla;
    f32 hitStart;
    f32 hitEnd;
} KeatonMove;

static const KeatonMove sComboMoves[3] = {
    { gPlayerAnim_pg_punchA, 1, 4.0f, 9.0f },
    { gPlayerAnim_pg_punchB, 1, 6.0f, 13.0f },
    { ANIM_SS "dash_attack09", 0, 14.0f, 26.0f },
};
static PlayerAnimationHeader* sCombo[3];
static PlayerAnimationHeader* sRebound;
static PlayerAnimationHeader* sCharge;
static PlayerAnimationHeader* sLongJump;
static PlayerAnimationHeader* sAirKick;
static PlayerAnimationHeader* sReflectAnim;
static u8 sLoaded;

static KeatonState sState;
static s32 sStep;
static s32 sChargeTimer;
static s32 sChargeLevel;
static u8 sLoopReversing;
static u8 sBlocked;
static s32 sJumpDelay;
static s32 sClimbDrain;
static u8 sClimbActive;

typedef struct KeatonShot {
    u8 active;
    Vec3f pos;
    Vec3f dir;
    f32 travelled;
    f32 range;
    f32 pullRadius;
    s32 level;
} KeatonShot;
static KeatonShot sShot;
static Actor* sFlame;
static ColliderCylinder sShotCyl;
static u8 sShotCylInited;

static ColliderCylinderInit sShotCylInit = {
    { COL_MATERIAL_NONE, AT_ON | AT_TYPE_PLAYER, AC_NONE, OC1_NONE, OC2_TYPE_PLAYER, COLSHAPE_CYLINDER },
    { ELEM_MATERIAL_UNK2,
      { DMG_SWORD, 0x00, 0x02 },
      { 0x00000000, 0x00, 0x00 },
      ATELEM_ON | ATELEM_NEAREST | ATELEM_SFX_NORMAL,
      ACELEM_NONE,
      OCELEM_NONE },
    { 34, 46, -24, { 0, 0, 0 } },
};

// ---- loading --------------------------------------------------------------------------------------
static void KeatonForm_Load(void) {
    if (sLoaded) {
        return;
    }
    for (s32 i = 0; i < 3; i++) {
        sCombo[i] =
            sComboMoves[i].vanilla ? Forms_VanillaAnim(sComboMoves[i].path) : Forms_LoadAnim(sComboMoves[i].path);
    }
    sRebound = Forms_VanillaAnim(gPlayerAnim_link_fighter_rebound);
    sCharge = Forms_LoadAnim(ANIM_SS "attack13");
    sLongJump = Forms_LoadAnim(ANIM_FIELD "charge_attack01");
    sAirKick = Forms_LoadAnim(ANIM_FIELD "wirebug_dash02");
    sReflectAnim = Forms_LoadAnim(REFLECT_ANIM);
    sLoaded = 1;
}

static u8 KeatonForm_IsActive(void) {
    return CustomForms_ActiveForm() == CUSTOM_FORM_KEATON;
}

// ---- projectile -----------------------------------------------------------------------------------
static u8 KeatonForm_FlameAlive(PlayState* play) {
    if (sFlame == NULL) {
        return 0;
    }
    Actor* it = play->actorCtx.actorLists[sFlame->category].first;
    while (it != NULL) {
        if (it == sFlame) {
            return sFlame->update != NULL;
        }
        it = it->next;
    }
    sFlame = NULL;
    return 0;
}

static void KeatonForm_KillFlame(PlayState* play) {
    if (play != NULL && KeatonForm_FlameAlive(play)) {
        Actor_Kill(sFlame);
    }
    sFlame = NULL;
}

static f32 KeatonForm_BallScale(s32 level) {
    return (level >= 2) ? BALL_SCALE_LEVEL2 : BALL_SCALE_LEVEL1;
}

// En_Light bakes its colour into params, so a level-up respawns the actor instead of recolouring it.
static void KeatonForm_SpawnFlame(PlayState* play, Vec3f* at, s32 level) {
    KeatonForm_KillFlame(play);
    sFlame = Actor_Spawn(&play->actorCtx, play, ACTOR_EN_LIGHT, at->x, at->y, at->z, 0, 0, 0x4000,
                         (level >= 2) ? FLAME_ORANGE : FLAME_BLUE);
}

static void KeatonForm_PlaceFlame(PlayState* play, Vec3f* at, s32 level) {
    if (!KeatonForm_FlameAlive(play)) {
        return;
    }
    Math_Vec3f_Copy(&sFlame->world.pos, at);
    Actor_SetScale(sFlame, KeatonForm_BallScale(level));
}

static void KeatonForm_Aim(Player* player) {
    Actor* target = player->focusActor;
    u8 valid = (target != NULL) && (target->update != NULL) &&
               (target->category == ACTORCAT_ENEMY || target->category == ACTORCAT_BOSS);
    if (valid) {
        Vec3f diff;
        f32 dist = Math_Vec3f_DistXYZAndStoreDiff(&sShot.pos, &target->focus.pos, &diff);
        if (dist > 1.0f) {
            sShot.dir.x = diff.x * (SHOT_SPEED / dist);
            sShot.dir.y = diff.y * (SHOT_SPEED / dist);
            sShot.dir.z = diff.z * (SHOT_SPEED / dist);
            return;
        }
    }
    sShot.dir.x = Math_SinS(player->actor.shape.rot.y) * SHOT_SPEED;
    sShot.dir.y = 0.0f;
    sShot.dir.z = Math_CosS(player->actor.shape.rot.y) * SHOT_SPEED;
}

static void KeatonForm_Launch(PlayState* play, Player* player, s32 level) {
    Math_Vec3f_Copy(&sShot.pos, &player->bodyPartsPos[PLAYER_BODYPART_RIGHT_HAND]);
    KeatonForm_Aim(player);
    sShot.travelled = 0.0f;
    sShot.range = (level >= 2) ? RANGE_LEVEL2 : RANGE_LEVEL1;
    sShot.pullRadius = (level >= 2) ? PULL_RADIUS_LEVEL2 : PULL_RADIUS_LEVEL1;
    sShot.level = level;
    sShot.active = 1;
    Player_AnimSfx_PlayVoice(player, NA_SE_VO_LI_MAGIC_ATTACK);
    Player_PlaySfx(player, NA_SE_IT_MAGIC_ARROW_SHOT);
}

static void KeatonForm_PullEnemies(PlayState* play) {
    Actor* it = play->actorCtx.actorLists[ACTORCAT_ENEMY].first;
    while (it != NULL) {
        if (it->update != NULL) {
            Vec3f toBall;
            f32 dist = Math_Vec3f_DistXYZAndStoreDiff(&it->world.pos, &sShot.pos, &toBall);
            if (dist <= sShot.pullRadius && dist >= 1.0f) {
                f32 step = PULL_STEP / dist;
                it->world.pos.x += toBall.x * step;
                it->world.pos.y += toBall.y * step;
                it->world.pos.z += toBall.z * step;
            }
        }
        it = it->next;
    }
}

static void KeatonForm_SubmitShotCollider(PlayState* play, Player* player) {
    if (!sShotCylInited) {
        Collider_InitCylinder(play, &sShotCyl);
        Collider_SetCylinder(play, &sShotCyl, &player->actor, &sShotCylInit);
        sShotCylInited = 1;
    }
    sShotCyl.dim.radius = (sShot.level >= 2) ? BALL_RADIUS_LEVEL2 : BALL_RADIUS_LEVEL1;
    sShotCyl.base.atFlags |= AT_ON;
    sShotCyl.dim.pos.x = (s16)sShot.pos.x;
    sShotCyl.dim.pos.y = (s16)sShot.pos.y;
    sShotCyl.dim.pos.z = (s16)sShot.pos.z;
    CollisionCheck_SetAT(play, &play->colChkCtx, &sShotCyl.base);
}

static void KeatonForm_UpdateShot(PlayState* play, Player* player) {
    if (!sShot.active) {
        return;
    }
    sShot.pos.x += sShot.dir.x;
    sShot.pos.y += sShot.dir.y;
    sShot.pos.z += sShot.dir.z;
    sShot.travelled += SHOT_SPEED;
    KeatonForm_PlaceFlame(play, &sShot.pos, sShot.level);
    KeatonForm_PullEnemies(play);
    KeatonForm_SubmitShotCollider(play, player);
    Audio_PlaySfx_AtPos(&sShot.pos, NA_SE_EV_FIRE_PILLAR);
    if (sShot.travelled >= sShot.range || (sShotCyl.base.atFlags & AT_HIT)) {
        sShotCyl.base.atFlags &= ~(AT_ON | AT_HIT);
        sShot.active = 0;
        KeatonForm_KillFlame(play);
    }
}

// ---- state helpers --------------------------------------------------------------------------------
static void KeatonForm_ClearState(void) {
    sState = KEATON_IDLE;
    sStep = 0;
    sChargeTimer = 0;
    sChargeLevel = 0;
    sLoopReversing = 0;
    sBlocked = 0;
    sJumpDelay = 0;
}

static void KeatonForm_Play(PlayState* play, Player* player, PlayerAnimationHeader* anim, f32 start, f32 end) {
    sBlocked = 0;
    Forms_PlayClip(play, player, anim, (end >= start) ? 1.0f : -1.0f, start, end, 0, -6.0f);
}

static u8 KeatonForm_WasBlocked(void) {
    return sBlocked;
}

static void KeatonForm_ArmFists(Player* player) {
    func_80833728(player, 0, FIST_DMG_FLAGS, FIST_DAMAGE);
    func_80833728(player, 1, FIST_DMG_FLAGS, FIST_DAMAGE);
}

static void KeatonForm_StartRecoil(PlayState* play, Player* player) {
    sBlocked = 0;
    sJumpDelay = 0;
    player->meleeWeaponState = PLAYER_MELEE_WEAPON_STATE_0;
    player->meleeWeaponQuads[0].base.atFlags &= ~(AT_ON | AT_BOUNCED);
    player->meleeWeaponQuads[1].base.atFlags &= ~(AT_ON | AT_BOUNCED);
    Player_RequestRumble(play, player, 180, 20, 100, SQ(0));
    if (!Forms_OnGround(player)) {
        Forms_Release(player);
        KeatonForm_ClearState();
        player->actor.colChkInfo.damage = KICK_BLOCK_DAMAGE;
        func_80833B18(play, player, 2, 4.0f, 5.0f, player->actor.shape.rot.y + 0x8000, KICK_BLOCK_INVINCIBILITY);
        Actor_SetColorFilter(&player->actor, COLORFILTER_COLORFLAG_RED, 255, COLORFILTER_BUFFLAG_OPA, 20);
        return;
    }
    if (sRebound == NULL) {
        Forms_Release(player);
        KeatonForm_ClearState();
        return;
    }
    sState = KEATON_RECOIL;
    KeatonForm_Play(play, player, sRebound, 0.0f, Animation_GetLastFrame(sRebound));
    player->speedXZ = RECOIL_SPEED;
}

static void KeatonForm_StartAirKick(PlayState* play, Player* player) {
    sState = KEATON_AIRKICK;
    sJumpDelay = 0;
    KeatonForm_ArmFists(player);
    player->actor.shape.rot.y = player->yaw;
    player->speedXZ = AIR_KICK_SPEED;
    KeatonForm_Play(play, player, sAirKick, 0.0f, Animation_GetLastFrame(sAirKick));
    Player_PlaySfx(player, NA_SE_IT_SWORD_SWING_HARD);
}

static s32 KeatonForm_LevelForTimer(void) {
    if (sChargeTimer >= CHARGE_LEVEL2) {
        return 2;
    }
    return (sChargeTimer >= CHARGE_LEVEL1) ? 1 : 0;
}

static void KeatonForm_EnterCharge(PlayState* play, Player* player) {
    sState = KEATON_CHARGE;
    sChargeTimer = 0;
    sChargeLevel = 0;
    sLoopReversing = 0;
    player->meleeWeaponState = PLAYER_MELEE_WEAPON_STATE_0;
    KeatonForm_Play(play, player, sCharge, 0.0f, LOOP_END);
    Player_PlaySfx(player, NA_SE_IT_SWORD_CHARGE);
}

// PlayerAnimation has ONCE and LOOP but no bounce: the hold ping-pongs the window by hand.
static void KeatonForm_TickChargeLoop(PlayState* play, Player* player) {
    sLoopReversing = !sLoopReversing;
    if (sLoopReversing) {
        KeatonForm_Play(play, player, sCharge, LOOP_END, LOOP_START);
    } else {
        KeatonForm_Play(play, player, sCharge, LOOP_START, LOOP_END);
    }
}

// ---- block scan (must run before Player_UpdateCommon's AT reset) --------------------------------
void KeatonForm_ScanBlock(Player* player) {
    if (sState != KEATON_COMBO && sState != KEATON_AIRKICK) {
        return;
    }
    if ((player->meleeWeaponQuads[0].base.atFlags & AT_BOUNCED) ||
        (player->meleeWeaponQuads[1].base.atFlags & AT_BOUNCED)) {
        sBlocked = 1;
    }
}

// ---- climb ----------------------------------------------------------------------------------------
u8 KeatonForm_ClimbActive(void) {
    return sClimbActive;
}

// Wall contact only decides whether a climb may START; BGCHECKFLAG_WALL drops out between rungs, so
// re-asking it mid-climb shook him off.
static void KeatonForm_TickClimb(Player* player, PlayState* play, u8 holdingA) {
    u8 climbing = (player->stateFlags1 & PLAYER_STATE1_CLIMBING_LADDER) != 0;
    u8 canHold = climbing || ((player->actor.bgCheckFlags & BGCHECKFLAG_WALL) && (player->actor.wallPoly != NULL));
    if (!holdingA || !canHold) {
        sClimbActive = 0;
        sClimbDrain = 0;
        return;
    }
    sClimbActive = 1;
    if (player->actor.wallPoly == NULL) {
        return;
    }
    sClimbActive = 0;
    u8 freeSurface =
        (SurfaceType_GetWallFlags(&play->colCtx, player->actor.wallPoly, player->actor.wallBgId) & WALL_FLAG_3) != 0;
    sClimbActive = 1;
    if (freeSurface) {
        sClimbDrain = 0;
        return;
    }
    sClimbDrain++;
    if (sClimbDrain < CLIMB_DRAIN_INTERVAL) {
        return;
    }
    sClimbDrain = 0;
    if (!ItemMagic_HasEnough(play, 1)) {
        sClimbActive = 0;
        return;
    }
    ItemMagic_Consume(play, 1);
}

// ---- reflector ------------------------------------------------------------------------------------
typedef enum ReflectorState { REFLECTOR_OFF, REFLECTOR_POPPING, REFLECTOR_HELD } ReflectorState;
static ReflectorState sReflector;
static f32 sReflectorScale;
static s32 sReflectorPulse;
static ColliderCylinder sGuard;
static ColliderCylinder sBurst;
static u8 sGuardInited;
static u8 sBurstInited;
static Actor* sReflected[REFLECTED_MAX];
static s32 sReflectedLife[REFLECTED_MAX];

static ColliderCylinderInit sGuardInit = {
    { COL_MATERIAL_METAL, AT_NONE, AC_ON | AC_HARD | AC_TYPE_ENEMY, OC1_NONE, OC2_TYPE_PLAYER, COLSHAPE_CYLINDER },
    { ELEM_MATERIAL_UNK0, { 0x00000000, 0x00, 0x00 }, { 0xFFCFFFFF, 0x00, 0x00 }, ATELEM_NONE, ACELEM_ON, OCELEM_NONE },
    { 70, 90, -40, { 0, 0, 0 } },
};
static ColliderCylinderInit sBurstInit = {
    { COL_MATERIAL_NONE, AT_ON | AT_TYPE_PLAYER, AC_NONE, OC1_NONE, OC2_TYPE_PLAYER, COLSHAPE_CYLINDER },
    { ELEM_MATERIAL_UNK2,
      { DMG_LIGHT_ARROW, 0x00, 0x02 },
      { 0x00000000, 0x00, 0x00 },
      ATELEM_ON | ATELEM_NEAREST | ATELEM_SFX_NORMAL,
      ACELEM_NONE,
      OCELEM_NONE },
    { 70, 90, -40, { 0, 0, 0 } },
};

// Hollow hexagon, pointy left/right, four rings of six; colour rides the vertices (G_CC_SHADE, no
// lighting) and the inner ring's alpha 0 fades into the hole.
#define HEXV(x, y, r, g, b, a)          \
    {                                   \
        {                               \
            { x, y, 0 }, 0, { 0, 0 }, { \
                r, g, b, a              \
            }                           \
        }                               \
    }
static Vtx sHexVtx[24] = {
    HEXV(22, 0, 74, 148, 240, 255),   HEXV(11, 19, 74, 148, 240, 255),    HEXV(-11, 19, 74, 148, 240, 255),
    HEXV(-22, 0, 74, 148, 240, 255),  HEXV(-11, -19, 74, 148, 240, 255),  HEXV(11, -19, 74, 148, 240, 255),
    HEXV(20, 0, 168, 218, 251, 255),  HEXV(10, 17, 168, 218, 251, 255),   HEXV(-10, 17, 168, 218, 251, 255),
    HEXV(-20, 0, 168, 218, 251, 255), HEXV(-10, -17, 168, 218, 251, 255), HEXV(10, -17, 168, 218, 251, 255),
    HEXV(14, 0, 150, 206, 250, 255),  HEXV(7, 12, 150, 206, 250, 255),    HEXV(-7, 12, 150, 206, 250, 255),
    HEXV(-14, 0, 150, 206, 250, 255), HEXV(-7, -12, 150, 206, 250, 255),  HEXV(7, -12, 150, 206, 250, 255),
    HEXV(11, 0, 120, 190, 248, 0),    HEXV(6, 10, 120, 190, 248, 0),      HEXV(-6, 10, 120, 190, 248, 0),
    HEXV(-11, 0, 120, 190, 248, 0),   HEXV(-6, -10, 120, 190, 248, 0),    HEXV(6, -10, 120, 190, 248, 0),
};
#define HEX_BAND(o)                                                        \
    gsSP2Triangles(o + 0, o + 6, o + 7, 0, o + 0, o + 7, o + 1, 0),        \
        gsSP2Triangles(o + 1, o + 7, o + 8, 0, o + 1, o + 8, o + 2, 0),    \
        gsSP2Triangles(o + 2, o + 8, o + 9, 0, o + 2, o + 9, o + 3, 0),    \
        gsSP2Triangles(o + 3, o + 9, o + 10, 0, o + 3, o + 10, o + 4, 0),  \
        gsSP2Triangles(o + 4, o + 10, o + 11, 0, o + 4, o + 11, o + 5, 0), \
        gsSP2Triangles(o + 5, o + 11, o + 6, 0, o + 5, o + 6, o + 0, 0)
static Gfx sHexDL[] = {
    gsDPPipeSync(),
    gsSPTexture(0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_OFF),
    gsDPSetCombineMode(G_CC_SHADE, G_CC_SHADE),
    gsSPClearGeometryMode(G_CULL_BOTH | G_FOG | G_LIGHTING | G_TEXTURE_GEN | G_TEXTURE_GEN_LINEAR),
    gsSPSetGeometryMode(G_ZBUFFER | G_SHADE | G_SHADING_SMOOTH),
    gsSPVertex(sHexVtx, 24, 0),
    HEX_BAND(0),
    HEX_BAND(6),
    HEX_BAND(12),
    gsSPEndDisplayList(),
};

static void KeatonReflector_Reset(void) {
    sReflector = REFLECTOR_OFF;
    sReflectorScale = 0.0f;
    sReflectorPulse = 0;
    sBurst.base.atFlags &= ~(AT_ON | AT_HIT);
    sGuard.base.acFlags &= ~(AC_ON | AC_HIT);
    sGuard.base.ac = NULL;
}

static void KeatonReflector_Remember(Actor* actor) {
    s32 slot = -1;
    for (s32 i = 0; i < REFLECTED_MAX; i++) {
        if (sReflected[i] == actor) {
            sReflectedLife[i] = REFLECTED_LIFE;
            return;
        }
        if (sReflected[i] == NULL && slot < 0) {
            slot = i;
        }
    }
    if (slot >= 0) {
        sReflected[slot] = actor;
        sReflectedLife[slot] = REFLECTED_LIFE;
    }
}

static void KeatonReflector_TickRemembered(void) {
    for (s32 i = 0; i < REFLECTED_MAX; i++) {
        if (sReflected[i] == NULL) {
            continue;
        }
        if (--sReflectedLife[i] <= 0 || sReflected[i]->update == NULL) {
            sReflected[i] = NULL;
        }
    }
}

// A reflected shot still carries AT_TYPE_ENEMY and would not hurt its sender; runs right after the
// projectile's own update registered its collider.
void KeatonForm_OnActorUpdate(Actor* actor) {
    if (gPlayState == NULL) {
        return;
    }
    u8 remembered = 0;
    for (s32 i = 0; i < REFLECTED_MAX; i++) {
        if (sReflected[i] == actor) {
            remembered = 1;
        }
    }
    if (!remembered) {
        return;
    }
    CollisionCheckContext* ctx = &gPlayState->colChkCtx;
    for (s32 i = 0; i < ctx->colATCount; i++) {
        Collider* col = ctx->colAT[i];
        if (col != NULL && col->actor == actor) {
            col->atFlags &= ~AT_TYPE_ENEMY;
            col->atFlags |= AT_TYPE_PLAYER;
        }
    }
}

static void KeatonReflector_ReflectOne(Actor* actor) {
    actor->world.rot.y += 0x8000;
    actor->shape.rot.y = actor->world.rot.y;
    actor->world.rot.x = -actor->world.rot.x;
    actor->velocity.x = -actor->velocity.x;
    actor->velocity.z = -actor->velocity.z;
    if (actor->parent != NULL) {
        actor->world.rot.y = Actor_WorldYawTowardActor(actor, actor->parent);
        actor->shape.rot.y = actor->world.rot.y;
    }
}

typedef struct SweepRule {
    u8 category;
    u8 flyingOnly;
} SweepRule;
static const SweepRule sSweep[4] = {
    { ACTORCAT_ITEMACTION, 0 },
    { ACTORCAT_EXPLOSIVE, 0 },
    { ACTORCAT_PROP, 1 },
    { ACTORCAT_ENEMY, 1 },
};

// The approach test also keeps a reflected shot from flipping again every frame it stays in range.
static void KeatonReflector_Sweep(PlayState* play, Player* player) {
    for (s32 r = 0; r < 4; r++) {
        Actor* it = play->actorCtx.actorLists[sSweep[r].category].first;
        while (it != NULL) {
            Actor* actor = it;
            it = it->next;
            if (actor->update == NULL || actor == &player->actor) {
                continue;
            }
            if (sSweep[r].flyingOnly) {
                if ((actor->bgCheckFlags & BGCHECKFLAG_GROUND) || actor->speed < REFLECTOR_PROJECTILE_SPEED) {
                    continue;
                }
            } else if (actor->speed <= 0.0f) {
                continue;
            }
            Vec3f toPlayer;
            f32 dist = Math_Vec3f_DistXYZAndStoreDiff(&actor->world.pos, &player->actor.world.pos, &toPlayer);
            if (dist > REFLECT_RADIUS || dist < 0.1f) {
                continue;
            }
            f32 velX = actor->velocity.x + Math_SinS(actor->world.rot.y) * actor->speed;
            f32 velZ = actor->velocity.z + Math_CosS(actor->world.rot.y) * actor->speed;
            if (velX * toPlayer.x + velZ * toPlayer.z <= 0.0f) {
                continue;
            }
            KeatonReflector_ReflectOne(actor);
            KeatonReflector_Remember(actor);
            Player_PlaySfx(player, NA_SE_IT_SHIELD_REFLECT_MG);
        }
    }
}

static void KeatonReflector_SubmitBurst(PlayState* play, Player* player) {
    if (!sBurstInited) {
        Collider_InitCylinder(play, &sBurst);
        Collider_SetCylinder(play, &sBurst, &player->actor, &sBurstInit);
        sBurstInited = 1;
    }
    sBurst.elem.atDmgInfo.dmgFlags = DMG_LIGHT_ARROW;
    sBurst.elem.atDmgInfo.damage = REFLECTOR_BURST_DAMAGE;
    sBurst.base.atFlags |= AT_ON;
    sBurst.dim.pos.x = (s16)player->actor.world.pos.x;
    sBurst.dim.pos.y = (s16)player->actor.world.pos.y;
    sBurst.dim.pos.z = (s16)player->actor.world.pos.z;
    CollisionCheck_SetAT(play, &play->colChkCtx, &sBurst.base);
}

// The nut bit stuns as well as hurts; placed at the attacker's focus (a Beamos' eye), not its feet.
static void KeatonReflector_PunishAttacker(PlayState* play, Player* player, Actor* attacker) {
    if (!sBurstInited) {
        Collider_InitCylinder(play, &sBurst);
        Collider_SetCylinder(play, &sBurst, &player->actor, &sBurstInit);
        sBurstInited = 1;
    }
    sBurst.elem.atDmgInfo.dmgFlags = DMG_DEKU_NUT | DMG_LIGHT_ARROW;
    sBurst.elem.atDmgInfo.damage = REFLECTOR_BURST_DAMAGE;
    sBurst.base.atFlags |= AT_ON;
    sBurst.dim.pos.x = (s16)attacker->focus.pos.x;
    sBurst.dim.pos.y = (s16)attacker->focus.pos.y;
    sBurst.dim.pos.z = (s16)attacker->focus.pos.z;
    CollisionCheck_SetAT(play, &play->colChkCtx, &sBurst.base);
    Player_PlaySfx(player, NA_SE_IT_SHIELD_REFLECT_MG2);
}

// Link's own cylinder stands down for the frame: a beam reaching him would hit both and still land.
static void KeatonReflector_SubmitGuard(PlayState* play, Player* player) {
    if (!sGuardInited) {
        Collider_InitCylinder(play, &sGuard);
        Collider_SetCylinder(play, &sGuard, &player->actor, &sGuardInit);
        sGuardInited = 1;
    }
    if (sGuard.base.acFlags & AC_HIT) {
        sGuard.base.acFlags &= ~AC_HIT;
        if (sGuard.base.ac != NULL) {
            KeatonReflector_PunishAttacker(play, player, sGuard.base.ac);
        }
        sGuard.base.ac = NULL;
    }
    player->cylinder.base.acFlags &= ~(AC_ON | AC_HIT);
    sGuard.base.acFlags |= AC_ON;
    sGuard.dim.pos.x = (s16)player->actor.world.pos.x;
    sGuard.dim.pos.y = (s16)player->actor.world.pos.y;
    sGuard.dim.pos.z = (s16)player->actor.world.pos.z;
    CollisionCheck_SetAC(play, &play->colChkCtx, &sGuard.base);
}

static void KeatonReflector_Open(PlayState* play, Player* player) {
    sReflector = REFLECTOR_POPPING;
    sReflectorScale = 0.0f;
    sReflectorPulse = 0;
    Forms_PlayClip(play, player, sReflectAnim, 1.0f, 0.0f, Animation_GetLastFrame(sReflectAnim), 1, -6.0f);
    if (!Forms_OnGround(player)) {
        player->speedXZ *= REFLECTOR_AIR_DRAG;
        player->actor.velocity.y *= REFLECTOR_AIR_DRAG;
    }
    KeatonReflector_SubmitBurst(play, player);
    Player_AnimSfx_PlayVoice(player, NA_SE_VO_LI_SWORD_N);
    Player_PlaySfx(player, NA_SE_IT_SHIELD_REFLECT_MG);
}

// Re-issued in a terminal state so the vanilla action takes the pose back without a snap.
static void KeatonReflector_Close(PlayState* play, Player* player) {
    KeatonReflector_Reset();
    Forms_Release(player);
    f32 last = Animation_GetLastFrame(sReflectAnim);
    PlayerAnimation_Change(play, &player->skelAnime, sReflectAnim, 1.0f, last, last, ANIMMODE_ONCE, -4.0f);
}

static u8 KeatonReflector_Update(Player* player, PlayState* play) {
    const Input* in = Forms_RawInput();
    u8 holdingR = CHECK_BTN_ALL(in->cur.button, BTN_R);
    KeatonReflector_TickRemembered();
    if (sReflector == REFLECTOR_OFF) {
        if (sReflectAnim == NULL || !holdingR ||
            (player->stateFlags1 & (FORMS_STATE1_GETTING_ITEM | FORMS_STATE1_DAMAGED | PLAYER_STATE1_IN_CUTSCENE))) {
            return 0;
        }
        KeatonReflector_Open(play, player);
        return 1;
    }
    if (!holdingR || (player->stateFlags1 & FORMS_STATE1_DAMAGED)) {
        KeatonReflector_Close(play, player);
        return 0;
    }
    if (sReflector == REFLECTOR_POPPING) {
        sReflectorScale += REFLECTOR_POP_STEP;
        if (sReflectorScale >= 1.0f) {
            sReflectorScale = 1.0f;
            sReflector = REFLECTOR_HELD;
        }
    } else {
        sReflectorPulse = (sReflectorPulse + 1) % REFLECTOR_PULSE_PERIOD;
        f32 wave = 0.5f + 0.5f * Math_CosS((s16)(sReflectorPulse * (0x10000 / REFLECTOR_PULSE_PERIOD)));
        sReflectorScale = REFLECTOR_PULSE_MIN + (1.0f - REFLECTOR_PULSE_MIN) * wave;
    }
    KeatonReflector_SubmitGuard(play, player);
    KeatonReflector_Sweep(play, player);
    PlayerAnimation_Update(play, &player->skelAnime);
    // damage_idle03 walks its root: pin both the frame and the previous translation or the body drifts.
    player->skelAnime.jointTable[0] = player->skelAnime.baseTransl;
    player->skelAnime.prevTransl = player->skelAnime.baseTransl;
    Forms_Pause(player);
    player->speedXZ = 0.0f;
    return 1;
}

static void KeatonReflector_Draw(PlayState* play, Player* player) {
    if (sReflector == REFLECTOR_OFF) {
        return;
    }
    OPEN_DISPS(play->state.gfxCtx);
    Matrix_Push();
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    Matrix_Translate(player->actor.world.pos.x, player->actor.world.pos.y + REFLECTOR_HEIGHT, player->actor.world.pos.z,
                     MTXMODE_NEW);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    Matrix_Scale(sReflectorScale, sReflectorScale, sReflectorScale, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, sHexDL);
    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
}

// ---- tails ----------------------------------------------------------------------------------------
static FlexSkeletonHeader* sTailSkel;
static u8 sTailProbed;
static Vec3s sTail[TAIL_SEGMENTS];
static s32 sTailFrame;
static s32 sTailPose;

static FlexSkeletonHeader* KeatonTails_Skel(void) {
    if (sTailProbed) {
        return sTailSkel;
    }
    sTailProbed = 1;
    if (!ResourceMgr_FileExists(TAIL_SKEL_PATH)) {
        return NULL;
    }
    FlexSkeletonHeader* skel = (FlexSkeletonHeader*)ResourceMgr_LoadSkeletonByName(TAIL_SKEL_PATH, NULL);
    // A missing resource can hand back an unrelated pointer; validate the shape before trusting it.
    if (skel != NULL && skel->sh.limbCount > 0 && skel->sh.limbCount <= TAIL_SEGMENTS && skel->sh.segment != NULL) {
        sTailSkel = skel;
    }
    return sTailSkel;
}

static void KeatonTails_Reset(void) {
    memset(sTail, 0, sizeof(sTail));
    sTailFrame = 0;
    sTailPose = 0;
}

// The animation-driven face lives in joint 22's low nibble, one-based; shock -> chuckle, the pained
// pair -> celebrate, everything else -> the idle sway.
static s32 KeatonTails_PoseFor(Player* player) {
    s32 eye = (player->skelAnime.jointTable[22].x & 0xF) - 1;
    if (eye < 0) {
        return sTailPose;
    }
    if (eye == 5) {
        return 2;
    }
    return (eye >= 6) ? 1 : 0;
}

static void KeatonTails_Update(Player* player) {
    FlexSkeletonHeader* skel = KeatonTails_Skel();
    if (skel == NULL) {
        return;
    }
    s32 want = KeatonTails_PoseFor(player);
    if (want != sTailPose) {
        sTailPose = want;
        sTailFrame = 0;
    }
    sTailFrame = (sTailFrame + 1) % kTailClipFrames[sTailPose];
    const s16(*clip)[3] = kTailClip[sTailPose][sTailFrame];
    // The clips do not share a start pose: ease 1/4 per frame or a switch snaps.
    for (s32 i = 0; i < skel->sh.limbCount && i < TAIL_SEGMENTS; i++) {
        s16 tx = (s16)(clip[i][0] * TAIL_EASE_AMOUNT);
        s16 ty = (s16)(clip[i][1] * TAIL_EASE_AMOUNT);
        s16 tz = (s16)(clip[i][2] * TAIL_EASE_AMOUNT);
        sTail[i].x += (tx - sTail[i].x) >> 2;
        sTail[i].y += (ty - sTail[i].y) >> 2;
        sTail[i].z += (tz - sTail[i].z) >> 2;
    }
}

static void KeatonTails_WalkChain(PlayState* play, StandardLimb** limbs, s32 limbCount, s32 i, Mtx* mtx) {
    OPEN_DISPS(play->state.gfxCtx);
    while (i != 0xFF && i < limbCount) {
        StandardLimb* limb = limbs[i];
        Matrix_Push();
        Matrix_Translate(limb->jointPos.x, limb->jointPos.y, limb->jointPos.z, MTXMODE_APPLY);
        Matrix_RotateZYX(sTail[i].x, sTail[i].y, sTail[i].z, MTXMODE_APPLY);
        Matrix_ToMtx(&mtx[i]);
        if (limb->dList != NULL) {
            gSPDisplayList(POLY_OPA_DISP++, limb->dList);
        }
        if (limb->child != 0xFF) {
            KeatonTails_WalkChain(play, limbs, limbCount, limb->child, mtx);
        }
        Matrix_Pop();
        i = limb->sibling;
    }
    CLOSE_DISPS(play->state.gfxCtx);
}

// Runs at the WAIST limb, the one moment its matrix is current; the tail DLs read their matrices
// from segment 0x0B (0x0D stays bound to the player skeleton).
static void KeatonTails_Draw(PlayState* play, Player* player) {
    FlexSkeletonHeader* skel = KeatonTails_Skel();
    if (skel == NULL) {
        return;
    }
    OPEN_DISPS(play->state.gfxCtx);
    Mtx* mtx = (Mtx*)GRAPH_ALLOC(play->state.gfxCtx, skel->sh.limbCount * sizeof(Mtx));
    gSPSegment(POLY_OPA_DISP++, TAIL_MTX_SEGMENT, (uintptr_t)mtx);
    KeatonTails_WalkChain(play, (StandardLimb**)skel->sh.segment, skel->sh.limbCount, 0, mtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

// Placement tuned in-game on the NEI sliders, then baked (Skull Kid's hand space != Keaton's).
static void KeatonForm_DrawFlute(PlayState* play) {
    if (!ResourceMgr_FileExists(FLUTE_DL_PATH + 7)) {
        return;
    }
    Gfx* flute = ResourceMgr_LoadGfxByName(FLUTE_DL_PATH);
    if (flute == NULL) {
        return;
    }
    OPEN_DISPS(play->state.gfxCtx);
    Matrix_Push();
    Matrix_Translate(-600.0f, 164.29f, -102.38f, MTXMODE_APPLY);
    Matrix_RotateZYX(30556, 28606, 3120, MTXMODE_APPLY);
    Matrix_Scale(0.73f, 0.73f, 0.73f, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, flute);
    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
}

void KeatonForm_PostLimb(PlayState* play, Player* player, s32 limbIndex) {
    if (limbIndex == PLAYER_LIMB_WAIST) {
        KeatonTails_Draw(play, player);
        KeatonReflector_Draw(play, player);
        return;
    }
    if (limbIndex == PLAYER_LIMB_RIGHT_HAND && (player->stateFlags2 & PLAYER_STATE2_USING_OCARINA)) {
        KeatonForm_DrawFlute(play);
    }
    if ((limbIndex == PLAYER_LIMB_LEFT_HAND || limbIndex == PLAYER_LIMB_RIGHT_HAND) &&
        (sState == KEATON_AIRKICK || sState == KEATON_COMBO)) {
        Forms_StampFistQuads(play, player, limbIndex, FIST_REACH, FIST_DMG_FLAGS, FIST_DAMAGE);
    }
}

// ---- lifecycle ------------------------------------------------------------------------------------
void KeatonForm_Reset(PlayState* play) {
    KeatonForm_ClearState();
    KeatonForm_KillFlame(play);
    sShot.active = 0;
    sShotCyl.base.atFlags &= ~(AT_ON | AT_HIT);
    sClimbActive = 0;
    sClimbDrain = 0;
    KeatonReflector_Reset();
    KeatonTails_Reset();
    memset(sReflected, 0, sizeof(sReflected));
}

u8 KeatonForm_OwnsAction(void) {
    return sState != KEATON_IDLE || sReflector != REFLECTOR_OFF;
}

// B and R belong to the form; A becomes the long jump unless vanilla needs it (doors, talk, climb,
// ledge), he is against a wall (the climb reads it) or a lock-on hop is being asked for.
static u8 KeatonForm_WantsVanillaHop(Player* player) {
    s32 stickDir = player->controlStickDirections[player->controlStickDataIndex];
    return Player_IsZTargeting(player) && stickDir > PLAYER_STICK_DIR_FORWARD;
}

void KeatonForm_FilterInput(Player* player, Input* input) {
    input->cur.button &= ~(BTN_B | BTN_R);
    input->press.button &= ~(BTN_B | BTN_R);
    if (!GaroForm_VanillaWantsAButton(player) && !KeatonForm_WantsVanillaHop(player) &&
        !(player->actor.bgCheckFlags & BGCHECKFLAG_WALL) && Forms_OnGround(player)) {
        input->cur.button &= ~BTN_A;
        input->press.button &= ~BTN_A;
    }
}

static u8 KeatonForm_TickIdle(Player* player, PlayState* play, const Input* input) {
    if (player->stateFlags1 & (FORMS_STATE1_GETTING_ITEM | PLAYER_STATE1_IN_CUTSCENE | FORMS_STATE1_DAMAGED |
                               PLAYER_STATE1_CLIMBING_LADDER | FORMS_STATE1_CLIMBING_LEDGE | FORMS_STATE1_HANGING)) {
        return 0;
    }
    if (!Forms_OnGround(player)) {
        if (CHECK_BTN_ALL(input->press.button, BTN_B) && sAirKick != NULL) {
            KeatonForm_StartAirKick(play, player);
            return 1;
        }
        return 0;
    }
    if (sLongJump != NULL && CHECK_BTN_ALL(input->press.button, BTN_A) && !GaroForm_VanillaWantsAButton(player) &&
        !KeatonForm_WantsVanillaHop(player) && !(player->actor.bgCheckFlags & BGCHECKFLAG_WALL)) {
        sState = KEATON_LONGJUMP;
        sJumpDelay = LONG_JUMP_ANIM_DELAY;
        player->speedXZ = LONG_JUMP_SPEED;
        player->actor.velocity.y = LONG_JUMP_LIFT;
        player->actor.bgCheckFlags &= ~BGCHECKFLAG_GROUND;
        Player_AnimSfx_PlayVoice(player, NA_SE_VO_LI_AUTO_JUMP);
        return 0;
    }
    if (CHECK_BTN_ALL(input->press.button, BTN_B) && sCombo[0] != NULL) {
        sState = KEATON_COMBO;
        sStep = 0;
        KeatonForm_ArmFists(player);
        KeatonForm_Play(play, player, sCombo[0], 0.0f, Animation_GetLastFrame(sCombo[0]));
        return 1;
    }
    return 0;
}

static u8 KeatonForm_TickCombo(Player* player, PlayState* play, const Input* input) {
    if (KeatonForm_WasBlocked()) {
        KeatonForm_StartRecoil(play, player);
        return 1;
    }
    f32 frame = player->skelAnime.curFrame;
    f32 last = Animation_GetLastFrame(sCombo[sStep]);
    if ((sStep + 1 < 3) && (frame >= last * CHAIN_OPENS) && CHECK_BTN_ALL(input->press.button, BTN_B) &&
        sCombo[sStep + 1] != NULL) {
        sStep++;
        KeatonForm_Play(play, player, sCombo[sStep], 0.0f, Animation_GetLastFrame(sCombo[sStep]));
        return 1;
    }
    const KeatonMove* move = &sComboMoves[sStep];
    player->meleeWeaponState =
        (frame >= move->hitStart && frame <= move->hitEnd) ? PLAYER_MELEE_WEAPON_STATE_1 : PLAYER_MELEE_WEAPON_STATE_0;
    if (PlayerAnimation_Update(play, &player->skelAnime)) {
        player->meleeWeaponState = PLAYER_MELEE_WEAPON_STATE_0;
        if (CHECK_BTN_ALL(input->cur.button, BTN_B) && sCharge != NULL) {
            KeatonForm_EnterCharge(play, player);
            return 1;
        }
        Forms_Release(player);
        KeatonForm_ClearState();
        return 0;
    }
    Forms_Pause(player);
    player->speedXZ = 0.0f;
    return 1;
}

static u8 KeatonForm_TickCharge(Player* player, PlayState* play, const Input* input) {
    sChargeTimer++;
    s32 level = KeatonForm_LevelForTimer();
    if (level != sChargeLevel) {
        sChargeLevel = level;
        KeatonForm_SpawnFlame(play, &player->bodyPartsPos[PLAYER_BODYPART_RIGHT_HAND], level);
        Player_PlaySfx(player, NA_SE_IT_SWORD_CHARGE);
    }
    KeatonForm_PlaceFlame(play, &player->bodyPartsPos[PLAYER_BODYPART_RIGHT_HAND], level);
    if (!CHECK_BTN_ALL(input->cur.button, BTN_B)) {
        s32 cost = (level >= 2) ? HADOUKEN_MAGIC_LEVEL2 : HADOUKEN_MAGIC_LEVEL1;
        if (level == 0 || !ItemMagic_HasEnough(play, cost)) {
            KeatonForm_KillFlame(play);
            Forms_Release(player);
            KeatonForm_ClearState();
            return 0;
        }
        ItemMagic_Consume(play, cost);
        KeatonForm_Launch(play, player, level);
        sState = KEATON_THROW;
        KeatonForm_Play(play, player, sCharge, LOOP_END, Animation_GetLastFrame(sCharge));
        return 1;
    }
    if (PlayerAnimation_Update(play, &player->skelAnime)) {
        KeatonForm_TickChargeLoop(play, player);
    }
    Forms_Pause(player);
    player->speedXZ = 0.0f;
    return 1;
}

static u8 KeatonForm_TickThrow(Player* player, PlayState* play) {
    if (PlayerAnimation_Update(play, &player->skelAnime)) {
        Forms_Release(player);
        KeatonForm_ClearState();
        return 0;
    }
    Forms_Pause(player);
    player->speedXZ = 0.0f;
    return 1;
}

// The first two airborne frames belong to vanilla (it is mid-swap to its own fall clip and would
// overwrite ours) — the same wait Roc's Feather uses.
static u8 KeatonForm_TickAir(Player* player, PlayState* play, const Input* input) {
    if (sState == KEATON_AIRKICK && KeatonForm_WasBlocked()) {
        KeatonForm_StartRecoil(play, player);
        return 1;
    }
    if (sJumpDelay > 0) {
        sJumpDelay--;
        if (sJumpDelay > 0) {
            return 0;
        }
        KeatonForm_Play(play, player, sLongJump, 0.0f, Animation_GetLastFrame(sLongJump));
    }
    if (sState == KEATON_LONGJUMP) {
        if (CHECK_BTN_ALL(input->press.button, BTN_B) && sAirKick != NULL) {
            KeatonForm_StartAirKick(play, player);
            return 1;
        }
        if (CHECK_BTN_ALL(input->cur.button, BTN_A) && (player->actor.bgCheckFlags & BGCHECKFLAG_WALL)) {
            Forms_Release(player);
            KeatonForm_ClearState();
            return 0;
        }
    }
    player->meleeWeaponState = (sState == KEATON_AIRKICK) ? PLAYER_MELEE_WEAPON_STATE_1 : PLAYER_MELEE_WEAPON_STATE_0;
    u8 landed = Forms_OnGround(player) && (player->actor.velocity.y <= 0.0f);
    if (landed || PlayerAnimation_Update(play, &player->skelAnime)) {
        Forms_Release(player);
        player->meleeWeaponState = PLAYER_MELEE_WEAPON_STATE_0;
        KeatonForm_ClearState();
        return 0;
    }
    Forms_Pause(player);
    return 1;
}

static u8 KeatonForm_TickRecoil(Player* player, PlayState* play) {
    if (PlayerAnimation_Update(play, &player->skelAnime)) {
        Forms_Release(player);
        KeatonForm_ClearState();
        return 0;
    }
    Forms_Pause(player);
    return 1;
}

u8 KeatonForm_Update(Player* player, PlayState* play) {
    KeatonForm_Load();
    KeatonForm_UpdateShot(play, player);
    KeatonTails_Update(player);
    const Input* input = Forms_RawInput();
    KeatonForm_TickClimb(player, play, CHECK_BTN_ALL(input->cur.button, BTN_A));

    // A controller holding the pause through a hit would leave DAMAGED set forever.
    if (player->stateFlags1 & FORMS_STATE1_DAMAGED) {
        if (sState != KEATON_IDLE || sReflector != REFLECTOR_OFF) {
            KeatonForm_ClearState();
            KeatonReflector_Reset();
            player->meleeWeaponState = PLAYER_MELEE_WEAPON_STATE_0;
            Forms_Release(player);
        }
        return 0;
    }
    if (KeatonReflector_Update(player, play)) {
        return 1;
    }
    switch (sState) {
        case KEATON_IDLE:
            return KeatonForm_TickIdle(player, play, input);
        case KEATON_COMBO:
            return KeatonForm_TickCombo(player, play, input);
        case KEATON_CHARGE:
            return KeatonForm_TickCharge(player, play, input);
        case KEATON_THROW:
            return KeatonForm_TickThrow(player, play);
        case KEATON_LONGJUMP:
        case KEATON_AIRKICK:
            return KeatonForm_TickAir(player, play, input);
        case KEATON_RECOIL:
            return KeatonForm_TickRecoil(player, play);
        default:
            KeatonForm_ClearState();
            return 0;
    }
}
