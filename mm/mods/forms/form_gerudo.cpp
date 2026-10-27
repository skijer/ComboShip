/*
 * form_gerudo.cpp - Gerudo moveset: Monster Hunter Rise dual blades (Skijer's NEI). Port of soh's
 * gerudo_mhr_combat.inc.c: the vanilla melee action runs the whole B chain off rewritten animation
 * tables; the controller owns only the moves vanilla has no action for (front slash, aerial slash,
 * Urbosa's Fury). Every table write has a restore on the way out of the form.
 */
#include "forms_internal.h"

#define MHRP(n) "__OTR__misc/link_animetion/gMonsterHunterRise_DualBlade_" n
#define MHRW(n) "__OTR__misc/link_animetion/gPlayerAnim_mhr_" n
#define GERUDO_SWORD_DL "__OTR__objects/forms/gerudo/object_link_boy/gLinkAdultLeftHandHoldingMasterSwordNearDL"

#define GMHR_FIGHTER_COLUMNS ((1 << PLAYER_ANIMTYPE_1) | (1 << PLAYER_ANIMTYPE_3))
#define GMHR_WALK_FRAMES 29
#define GMHR_RUN_FRAMES 20
#define GMHR_OOT_SWING_SPEED_COMP 1.5f
#define GMHR_A_TAP_FRAMES 8
#define GMHR_SPRINT_MUL 1.5f
#define GMHR_SPRINT_ANIM_RATE (1.0f / 1.5f)
#define GMHR_RAGE_MAX 400
#define GMHR_RAGE_HITS_TO_FILL 10
#define GMHR_RAGE_CHARGE_MUL 2
#define GMHR_FURY_STRIKE_FRAME 80
#define GMHR_FURY_IFRAMES 60
#define GMHR_FURY_BLAST_RADIUS 320
#define GMHR_FURY_BLAST_HEIGHT 400
#define GMHR_FURY_BLAST_FRAMES 90.0f
#define GMHR_FURY_DAMAGE_MIN 10
#define GMHR_FURY_DAMAGE_MAX 255
#define GMHR_LATK_DMGFLAGS (DMG_SWORD | DMG_GORON_POUND | DMG_FIRE_ARROW | DMG_LIGHT_ARROW)
#define GMHR_LATK_QUAKE_STRENGTH 6
#define GMHR_LATK_QUAKE_FRAMES 18
#define GMHR_LATK_DIM 140
#define GMHR_LATK_DIM_FADE 10
#define GMHR_FRONT_SLASH_DIST 100.0f
#define GMHR_FRONT_SLASH_FRAMES 30
#define GMHR_SPIN_CYL_RADIUS 45
#define GMHR_SPIN_CYL_HEIGHT 60
#define GMHR_WALL_MARGIN 14.0f
#define GMHR_COMBO_RESET_FRAMES 40
#define GMHR_JS_VY_MUL 2.1f
#define GMHR_CHARGE_WINDOW_PIN 3

typedef enum GerudoState { GERUDO_IDLE, GERUDO_FURY, GERUDO_FRONT_SLASH, GERUDO_AERIAL } GerudoState;

typedef struct GerudoWindow {
    s16 beg;
    s16 end;
} GerudoWindow;

typedef struct GerudoMeleeBinding {
    s32 mwa;
    const char* path;
    f32 speedMul;
    s16 swingEnd; // -1 = whole clip; where vanilla sees the swing end (next B chains, A cancels)
    GerudoWindow window;
} GerudoMeleeBinding;

// Source frames. Rows after swingEnd become the recovery so the settle-back never snaps.
static const GerudoMeleeBinding sMeleeBindings[] = {
    { PLAYER_MWA_FORWARD_SLASH_1H, MHRP("AlternatingCrossSlashLeftRight"), 1.7f, 25, { 13, 25 } },
    { PLAYER_MWA_FORWARD_COMBO_1H, MHRP("StationaryRisingSingleAerialSlash"), 1.7f, 28, { 1, 28 } },
    { PLAYER_MWA_RIGHT_SLASH_1H, MHRP("StationaryRightLeadDoubleTwinSlash"), 1.7f, 28, { 1, 28 } },
    { PLAYER_MWA_RIGHT_COMBO_1H, MHRP("RightRisingTripleAerialSlash"), 1.7f, 41, { 12, 41 } },
    { PLAYER_MWA_LEFT_SLASH_1H, MHRP("StationaryRisingSingleAerialSlash"), 1.7f, 28, { 1, 28 } },
    { PLAYER_MWA_LEFT_COMBO_1H, MHRP("StationaryRightLeadDoubleTwinSlash"), 1.7f, 28, { 1, 28 } },
    { PLAYER_MWA_STAB_1H, MHRP("ForwardRisingLeftLeadDoubleAerialSlash"), 1.5f, -1, { 4, 30 } },
    { PLAYER_MWA_STAB_COMBO_1H, MHRP("ForwardRisingLeftLeadDoubleAerialSlash"), 1.5f, -1, { 4, 30 } },
    { PLAYER_MWA_JUMPSLASH_START, MHRP("ForwardRisingDoubleAerialSlash_Variant18"), 1.0f, -1, { 2, 26 } },
    { PLAYER_MWA_JUMPSLASH_FINISH, MHRP("ForwardSingleTwinSlash"), 1.0f, -1, { 2, 40 } },
    { PLAYER_MWA_SPIN_ATTACK_1H, MHRP("ForwardTumbleDelayedCrossFinish"), 2.1f, -1, { 28, 40 } },
    { PLAYER_MWA_BIG_SPIN_1H, MHRP("ForwardRisingDoubleRushSlash_Variant21"), 2.1f, -1, { 6, 60 } },
};
#define GERUDO_MELEE_ROWS ((s32)(sizeof(sMeleeBindings) / sizeof(sMeleeBindings[0])))

typedef struct GerudoGroupBinding {
    s32 group;
    const char* path;
    s16 frames; // 0 = native length
    s16 srcFirst;
    s16 srcLast;
    f32 speed;
} GerudoGroupBinding;

// walk and run share one 0..29 phase counter, sampled at x1 and x(20/29): a 29-frame run row only
// ever shows its first 20 frames.
static const GerudoGroupBinding sGroupBindings[] = {
    { PLAYER_ANIMGROUP_wait, MHRP("StationaryReadyIdle_Variant03"), 0, -1, -1, 1.0f },
    { PLAYER_ANIMGROUP_walk, MHRP("ForwardCombatRun"), GMHR_WALK_FRAMES, -1, -1, 1.0f },
    { PLAYER_ANIMGROUP_run, MHRP("ForwardCombatRun"), GMHR_RUN_FRAMES, -1, -1, 1.0f },
    { PLAYER_ANIMGROUP_damage_run, MHRP("ForwardDoubleRushSlash_Variant15"), GMHR_RUN_FRAMES, -1, -1, 1.0f },
    { PLAYER_ANIMGROUP_defense, MHRP("DemonModeActivationFlourish"), 0, 1, 20, 2.0f },
    { PLAYER_ANIMGROUP_defense_wait, MHRP("DemonModeActivationFlourish"), 0, 30, 30, 2.0f },
    { PLAYER_ANIMGROUP_defense_end, MHRP("DemonModeActivationFlourish"), 0, 31, 45, 2.0f },
    { PLAYER_ANIMGROUP_landing, MHRP("ForwardSingleTwinSlash"), 0, -1, -1, 1.0f },
    { PLAYER_ANIMGROUP_short_landing, MHRP("ForwardSingleTwinSlash"), 0, -1, -1, 1.0f },
};
#define GERUDO_GROUP_ROWS ((s32)(sizeof(sGroupBindings) / sizeof(sGroupBindings[0])))

typedef struct GerudoClip {
    const char* path;
    f32 speed;
    GerudoWindow window;
} GerudoClip;

static const GerudoClip sFrontSlashClip = { MHRP("BackwardRisingDoubleAerialSlash"), 1.2f, { -1, -1 } };
static const GerudoClip sAerialClip = { MHRP("StationaryRisingTripleAerialSlash"), 1.2f, { 2, 40 } };
static const GerudoClip sFuryClip = { MHRW("gs_wirebug_attack04"), 2.4f, { 20, 28 } };
static const GerudoWindow sFurySecondWindow = { 80, 110 };
static const s32 sComboRows[4] = { PLAYER_MWA_FORWARD_SLASH_1H, PLAYER_MWA_FORWARD_COMBO_1H, PLAYER_MWA_RIGHT_SLASH_1H,
                                   PLAYER_MWA_RIGHT_COMBO_1H };

typedef struct GerudoSavedRow {
    PlayerAnimationHeader* swing;
    PlayerAnimationHeader* end;
    PlayerAnimationHeader* endLockOn;
    u8 hitStart;
    u8 hitEnd;
} GerudoSavedRow;

static struct {
    u8 installed;
    GerudoSavedRow melee[GERUDO_MELEE_ROWS];
    PlayerAnimationHeader* group[GERUDO_GROUP_ROWS][PLAYER_ANIMTYPE_MAX];
    PlayerAnimationHeader* runNormal;
    PlayerAnimationHeader* runSprint;
    PlayerAnimationHeader* frontSlash;
    PlayerAnimationHeader* aerial;
    PlayerAnimationHeader* fury;
} sTables;

static struct {
    GerudoState state;
    s32 timer;
    s16 lockedYaw;
    s32 airFrames;
    u8 aPending;
    s32 aFrames;
    u8 aSprint;
    u8 sprintRowIsSprint;
    s32 comboStep;
    s32 comboIdle;
    s32 rageMeter;
    u8 furyDamage;
    f32 furyPrevFrame;
    s32 screenDim;
    u8 cylOn;
    u32 cylDmgFlags;
    u8 cylDamage;
    s16 cylRadius;
    s16 cylHeight;
} sMhr;

static ColliderCylinder sCyl;
static u8 sCylInited;
static ColliderCylinderInit sCylInit = {
    { COL_MATERIAL_NONE, AT_ON | AT_TYPE_PLAYER, AC_NONE, OC1_NONE, OC2_TYPE_PLAYER, COLSHAPE_CYLINDER },
    { ELEM_MATERIAL_UNK2,
      { DMG_SWORD, 0x00, 0x02 },
      { 0x00000000, 0x00, 0x00 },
      ATELEM_ON | ATELEM_NEAREST | ATELEM_SFX_NORMAL,
      ACELEM_NONE,
      OCELEM_NONE },
    { GMHR_SPIN_CYL_RADIUS, GMHR_SPIN_CYL_HEIGHT, 0, { 0, 0, 0 } },
};

static u8 GerudoForm_IsActive(void) {
    return CustomForms_ActiveForm() == CUSTOM_FORM_GERUDO;
}

static u8 GerudoForm_Fighter(Player* player) {
    return Player_GetMeleeWeaponHeld(player) != PLAYER_MELEEWEAPON_NONE && player->heldItemAction != PLAYER_IA_HAMMER;
}

static s32 GerudoForm_Round(f32 v) {
    return (s32)(v + 0.5f);
}

// ---- animation tables -----------------------------------------------------------------------------
// Vanilla plays a swing at 2/3 (Player_Anim_PlayOnceAdjusted) but the recovery at 1.0, so only the
// swing is compensated.
static void GerudoForm_InstallMeleeRow(const GerudoMeleeBinding* b, GerudoSavedRow* saved) {
    ExtPlayer_GetMeleeAnim(b->mwa, &saved->swing, &saved->end, &saved->endLockOn, &saved->hitStart, &saved->hitEnd);
    PlayerAnimationHeader* whole = Forms_LoadAnim(b->path);
    if (whole == NULL) {
        return;
    }
    s16 last = (s16)Animation_GetLastFrame(whole);
    f32 mul = b->speedMul;
    f32 swingMul = mul * GMHR_OOT_SWING_SPEED_COMP;
    s16 swingEnd = (b->swingEnd >= 0 && b->swingEnd < last) ? b->swingEnd : last;
    s16 swingFrames = (s16)GerudoForm_Round((swingEnd + 1) / swingMul);
    if (swingFrames < 2) {
        swingFrames = 2;
    }
    PlayerAnimationHeader* swing = Forms_LoadAnimRange(b->path, 0, swingEnd, swingFrames);
    PlayerAnimationHeader* recovery;
    if (swingEnd == last) {
        recovery = Forms_LoadAnimRange(b->path, last, last, 4);
    } else {
        s16 recFrames = (s16)GerudoForm_Round((last - swingEnd) / mul);
        if (recFrames < 2) {
            recFrames = 2;
        }
        recovery = Forms_LoadAnimRange(b->path, swingEnd + 1, last, recFrames);
    }
    if (swing == NULL || recovery == NULL) {
        return;
    }
    s32 hitStart = (s32)(b->window.beg / swingMul);
    s32 hitEnd = (s32)(b->window.end / swingMul + 0.999f);
    if (hitEnd > swingFrames - 1) {
        hitEnd = swingFrames - 1;
    }
    if (hitStart > hitEnd) {
        hitStart = hitEnd;
    }
    ExtPlayer_SetMeleeAnim(b->mwa, swing, recovery, recovery, (u8)hitStart, (u8)hitEnd);
}

static void GerudoForm_InstallGroupRow(const GerudoGroupBinding* b, PlayerAnimationHeader** saved) {
    PlayerAnimationHeader* anim;
    if (b->srcFirst >= 0) {
        s16 len = b->srcLast - b->srcFirst + 1;
        s16 frames = (s16)GerudoForm_Round(len / b->speed);
        anim = Forms_LoadAnimRange(b->path, b->srcFirst, b->srcLast, (frames < 2) ? 2 : frames);
    } else if (b->frames > 0) {
        anim = Forms_LoadAnimRange(b->path, -1, -1, b->frames);
    } else {
        anim = Forms_LoadAnim(b->path);
    }
    for (s32 col = 0; col < PLAYER_ANIMTYPE_MAX; col++) {
        saved[col] = ExtPlayer_GetAnimGroupAnim(b->group, col);
        if (anim != NULL && (GMHR_FIGHTER_COLUMNS & (1 << col))) {
            ExtPlayer_SetAnimGroupAnim(b->group, col, anim);
        }
    }
}

static void GerudoForm_InstallTables(void) {
    if (sTables.installed) {
        return;
    }
    for (s32 i = 0; i < GERUDO_MELEE_ROWS; i++) {
        GerudoForm_InstallMeleeRow(&sMeleeBindings[i], &sTables.melee[i]);
    }
    for (s32 i = 0; i < GERUDO_GROUP_ROWS; i++) {
        GerudoForm_InstallGroupRow(&sGroupBindings[i], sTables.group[i]);
    }
    sTables.runNormal = Forms_LoadAnimRange(MHRP("ForwardCombatRun"), -1, -1, GMHR_RUN_FRAMES);
    sTables.runSprint = Forms_LoadAnimRange(MHRP("ForwardCombatRun_Variant02"), -1, -1, GMHR_RUN_FRAMES);
    sTables.frontSlash = Forms_LoadAnim(sFrontSlashClip.path);
    sTables.aerial = Forms_LoadAnim(sAerialClip.path);
    sTables.fury = Forms_LoadAnim(sFuryClip.path);
    sTables.installed = 1;
    SPDLOG_INFO("[Gerudo] MHR dual-blade tables installed");
}

static void GerudoForm_RestoreTables(void) {
    if (!sTables.installed) {
        return;
    }
    for (s32 i = 0; i < GERUDO_MELEE_ROWS; i++) {
        const GerudoSavedRow* s = &sTables.melee[i];
        if (s->swing != NULL) {
            ExtPlayer_SetMeleeAnim(sMeleeBindings[i].mwa, s->swing, s->end, s->endLockOn, s->hitStart, s->hitEnd);
        }
    }
    for (s32 i = 0; i < GERUDO_GROUP_ROWS; i++) {
        for (s32 col = 0; col < PLAYER_ANIMTYPE_MAX; col++) {
            if (sTables.group[i][col] != NULL) {
                ExtPlayer_SetAnimGroupAnim(sGroupBindings[i].group, col, sTables.group[i][col]);
            }
        }
    }
    sTables.installed = 0;
    sMhr.sprintRowIsSprint = 0;
}

static void GerudoForm_TickSprintRow(void) {
    u8 want = sMhr.aSprint;
    if (want == sMhr.sprintRowIsSprint) {
        return;
    }
    PlayerAnimationHeader* row = want ? sTables.runSprint : sTables.runNormal;
    if (row == NULL) {
        return;
    }
    for (s32 col = 0; col < PLAYER_ANIMTYPE_MAX; col++) {
        if (GMHR_FIGHTER_COLUMNS & (1 << col)) {
            ExtPlayer_SetAnimGroupAnim(PLAYER_ANIMGROUP_run, col, row);
        }
    }
    sMhr.sprintRowIsSprint = want;
}

// ---- rage meter -----------------------------------------------------------------------------------
static s32 GerudoForm_RageCapacity(void) {
    s32 level = gSaveContext.save.saveInfo.playerData.magicLevel;
    if (level < 0) {
        level = 0;
    }
    if (level > 2) {
        level = 2;
    }
    return GMHR_RAGE_MAX << level;
}

static s32 GerudoForm_RageGain(void) {
    s32 gain = GerudoForm_RageCapacity() / GMHR_RAGE_HITS_TO_FILL;
    return (gain < 1) ? 1 : gain;
}

static void GerudoForm_AddRage(s32 amount) {
    sMhr.rageMeter += amount;
    if (sMhr.rageMeter > GerudoForm_RageCapacity()) {
        sMhr.rageMeter = GerudoForm_RageCapacity();
    }
}

static u8 GerudoForm_RageWorthy(Actor* hit) {
    return hit != NULL && (hit->category == ACTORCAT_ENEMY || hit->category == ACTORCAT_BOSS);
}

// Called before Player_UpdateCommon wipes the melee AT flags. Fury spent the meter; letting its own
// blast refill it would make the charge free against a crowd.
void GerudoForm_ScanBladeHits(Player* player) {
    if (sMhr.state == GERUDO_FURY) {
        return;
    }
    for (s32 i = 0; i < 2; i++) {
        ColliderQuad* quad = &player->meleeWeaponQuads[i];
        if ((quad->base.atFlags & AT_HIT) && GerudoForm_RageWorthy(quad->base.at)) {
            GerudoForm_AddRage(GerudoForm_RageGain());
        }
    }
    if (sCylInited && (sCyl.base.atFlags & AT_HIT) && GerudoForm_RageWorthy(sCyl.base.at)) {
        GerudoForm_AddRage(GerudoForm_RageGain());
        Player_RequestRumble(gPlayState, player, 120, 20, 100, SQ(0));
    }
    if (sCylInited) {
        Collider_ResetCylinderAT(gPlayState, &sCyl.base);
    }
}

extern "C" u8 GerudoForm_RageVisible(void) {
    return GerudoForm_IsActive() && sMhr.rageMeter > 0;
}

extern "C" f32 GerudoForm_RageFill(void) {
    return (f32)sMhr.rageMeter / (f32)GerudoForm_RageCapacity();
}

extern "C" u8 GerudoForm_RageReady(void) {
    return sMhr.rageMeter >= GerudoForm_RageCapacity() / 4;
}

// ---- combo chain ----------------------------------------------------------------------------------
// Vanilla picks the row by stick angle and reaches the _COMBO row only on the third hit; a fixed
// chain has to sequence the row itself and switch the +2 rule off (OwnsComboRow).
s32 GerudoForm_NextComboMwa(Player* player, s32 requested) {
    if (!GerudoForm_Fighter(player)) {
        return requested;
    }
    if (requested == PLAYER_MWA_SPIN_ATTACK_1H && !(player->stateFlags1 & PLAYER_STATE1_CHARGING_SPIN_ATTACK)) {
        return PLAYER_MWA_BIG_SPIN_1H;
    }
    if (requested == PLAYER_MWA_SPIN_ATTACK_1H || requested == PLAYER_MWA_BIG_SPIN_1H) {
        player->stateFlags2 |= PLAYER_STATE2_40000000;
        return PLAYER_MWA_SPIN_ATTACK_1H;
    }
    if (requested >= PLAYER_MWA_FLIPSLASH_START || (requested & 1)) {
        return requested;
    }
    s32 row = sComboRows[sMhr.comboStep];
    sMhr.comboStep = (sMhr.comboStep + 1) % 4;
    sMhr.comboIdle = 0;
    return row;
}

u8 GerudoForm_OwnsComboRow(void) {
    return gPlayState != NULL && GerudoForm_Fighter(GET_PLAYER(gPlayState));
}

// The reset timer measures the gap BETWEEN hits, never during one.
static void GerudoForm_TickCombo(Player* player) {
    if (sMhr.comboStep == 0) {
        return;
    }
    if (player->meleeWeaponState != PLAYER_MELEE_WEAPON_STATE_0) {
        sMhr.comboIdle = 0;
        return;
    }
    if (++sMhr.comboIdle >= GMHR_COMBO_RESET_FRAMES) {
        sMhr.comboStep = 0;
        sMhr.comboIdle = 0;
    }
}

// Vanilla opens the charge only if unk_ADC reads exactly 1 the frame after the swing; that is true
// for Link's 7-frame swings, never for 25-41 frame ones — so it is pinned while the swing runs.
u8 GerudoForm_HoldsChargeWindow(Player* player) {
    return (player->actionFunc == Player_Action_84) && (player->meleeWeaponAnimation < PLAYER_MWA_SPIN_ATTACK_1H) &&
           CHECK_BTN_ALL(Forms_RawInput()->cur.button, BTN_B);
}

void GerudoForm_AdjustJumpSlash(Player* player) {
    player->actor.velocity.y *= GMHR_JS_VY_MUL;
}

// ---- A tap / hold ---------------------------------------------------------------------------------
static void GerudoForm_TickA(Player* player, PlayState* play) {
    const Input* in = Forms_RawInput();
    u8 enabled = GerudoForm_Fighter(player) && sMhr.state == GERUDO_IDLE && Forms_OnGround(player) &&
                 !(player->stateFlags1 & PLAYER_STATE1_SHIELDING) && (player->actionFunc != Player_Action_26);
    if (!enabled) {
        sMhr.aPending = 0;
        sMhr.aFrames = 0;
        sMhr.aSprint = 0;
        return;
    }
    if (CHECK_BTN_ALL(in->press.button, BTN_A)) {
        sMhr.aPending = 1;
        sMhr.aFrames = 0;
    }
    if (CHECK_BTN_ALL(in->cur.button, BTN_A)) {
        if (sMhr.aPending && ++sMhr.aFrames >= GMHR_A_TAP_FRAMES) {
            sMhr.aPending = 0;
            sMhr.aSprint = 1;
        }
        return;
    }
    if (sMhr.aPending) {
        sMhr.aPending = 0;
        if (Forms_StickMagnitude(in) >= 10.0f) {
            func_80836B3C(play, player, 0.0f);
        }
    }
    sMhr.aSprint = 0;
}

// The controller runs after Player_UpdateCommon: on the press frame the latch is not set yet, so the
// raw press is refused too (the tap decides roll-vs-sprint on release).
u8 GerudoForm_SuppressRoll(Player* player) {
    if (!GerudoForm_IsActive() || !GerudoForm_Fighter(player)) {
        return 0;
    }
    if (sMhr.aPending || sMhr.aSprint) {
        return 1;
    }
    return CHECK_BTN_ALL(Forms_RawInput()->press.button, BTN_A) ? 1 : 0;
}

f32 GerudoForm_RunSpeedMul(void) {
    return sMhr.aSprint ? GMHR_SPRINT_MUL : 1.0f;
}

f32 GerudoForm_RunAnimRateMul(void) {
    return sMhr.aSprint ? GMHR_SPRINT_ANIM_RATE : 1.0f;
}

static u8 GerudoForm_WantsFrontSlash(Player* player) {
    if (sMhr.aSprint) {
        return 1;
    }
    return player->focusActor == NULL && Forms_StickMagnitude(Forms_RawInput()) >= 10.0f &&
           player->controlStickDirections[player->controlStickDataIndex] == PLAYER_STICK_DIR_FORWARD;
}

// R+B and the forward slash are ours: vanilla must not see that B or it starts an ordinary swing
// and the two fight over the animation.
void GerudoForm_FilterInput(Player* player, Input* input) {
    if (!GerudoForm_Fighter(player)) {
        return;
    }
    if (CHECK_BTN_ALL(input->cur.button, BTN_R) || GerudoForm_WantsFrontSlash(player)) {
        input->cur.button &= ~BTN_B;
        input->press.button &= ~BTN_B;
    }
}

// ---- body cylinder --------------------------------------------------------------------------------
static void GerudoForm_CylOn(u32 dmgFlags, u8 damage) {
    sMhr.cylOn = 1;
    sMhr.cylDmgFlags = dmgFlags;
    sMhr.cylDamage = damage;
}

static void GerudoForm_CylOff(void) {
    sMhr.cylOn = 0;
    sMhr.cylRadius = 0;
    sMhr.cylHeight = 0;
    if (sCylInited) {
        sCyl.base.atFlags &= ~(AT_ON | AT_HIT);
    }
}

// Centred on her so a tall blast reaches above and below.
static void GerudoForm_SubmitCyl(PlayState* play, Player* player) {
    if (!sMhr.cylOn) {
        return;
    }
    if (!sCylInited) {
        Collider_InitCylinder(play, &sCyl);
        Collider_SetCylinder(play, &sCyl, &player->actor, &sCylInit);
        sCylInited = 1;
    }
    sCyl.elem.atDmgInfo.dmgFlags = sMhr.cylDmgFlags;
    sCyl.elem.atDmgInfo.damage = sMhr.cylDamage;
    sCyl.dim.radius = sMhr.cylRadius ? sMhr.cylRadius : GMHR_SPIN_CYL_RADIUS;
    sCyl.dim.height = sMhr.cylHeight ? sMhr.cylHeight : GMHR_SPIN_CYL_HEIGHT;
    sCyl.dim.yShift = -sCyl.dim.height / 2;
    sCyl.dim.pos.x = (s16)player->actor.world.pos.x;
    sCyl.dim.pos.y = (s16)player->actor.world.pos.y;
    sCyl.dim.pos.z = (s16)player->actor.world.pos.z;
    sCyl.base.atFlags |= AT_ON;
    CollisionCheck_SetAT(play, &play->colChkCtx, &sCyl.base);
}

// ---- controller clips -----------------------------------------------------------------------------
static void GerudoForm_StartClip(PlayState* play, Player* player, const GerudoClip* clip, PlayerAnimationHeader* anim,
                                 u32 bladeDmgFlags, u8 bladeDamage) {
    Forms_PlayClip(play, player, anim, clip->speed, 0.0f, Animation_GetLastFrame(anim), 0, -2.0f);
    sMhr.timer = 0;
    sMhr.furyPrevFrame = -1.0f;
    sMhr.lockedYaw = player->actor.shape.rot.y;
    func_80833728(player, 0, bladeDmgFlags, bladeDamage);
    func_80833728(player, 1, bladeDmgFlags, bladeDamage);
    player->meleeWeaponState = PLAYER_MELEE_WEAPON_STATE_0;
    GerudoForm_CylOff();
}

static void GerudoForm_Plant(Player* player) {
    Forms_Pause(player);
    player->actor.shape.rot.y = sMhr.lockedYaw;
    player->actor.world.rot.y = sMhr.lockedYaw;
    player->yaw = sMhr.lockedYaw;
    player->speedXZ = 0.0f;
    player->actor.speed = 0.0f;
}

static void GerudoForm_Hold(Player* player) {
    Forms_Pause(player);
    player->actor.world.rot.y = player->yaw;
}

// Damage windows are checked by CROSSING between frames, not by point.
static void GerudoForm_TickClipBlades(Player* player, const GerudoWindow* a, const GerudoWindow* b) {
    f32 cur = player->skelAnime.curFrame;
    f32 prev = (sMhr.furyPrevFrame < 0.0f) ? cur - 1.0f : sMhr.furyPrevFrame;
    u8 hit = 0;
    if (a->beg >= 0 && cur >= a->beg && prev <= a->end) {
        hit = 1;
    }
    if (b != NULL && cur >= b->beg && prev <= b->end) {
        hit = 1;
    }
    sMhr.furyPrevFrame = cur;
    player->meleeWeaponState = hit ? PLAYER_MELEE_WEAPON_STATE_1 : PLAYER_MELEE_WEAPON_STATE_0;
}

static void GerudoForm_StepForward(PlayState* play, Player* player, f32 dist) {
    Vec3f from = player->actor.world.pos;
    Vec3f to = from;
    Vec3f hit;
    CollisionPoly* poly;
    s32 bgId;
    f32 sinYaw = Math_SinS(player->actor.shape.rot.y);
    f32 cosYaw = Math_CosS(player->actor.shape.rot.y);
    from.y += 20.0f;
    to.x += sinYaw * (dist + GMHR_WALL_MARGIN);
    to.z += cosYaw * (dist + GMHR_WALL_MARGIN);
    to.y += 20.0f;
    if (BgCheck_EntityLineTest1(&play->colCtx, &from, &to, &hit, &poly, true, false, false, true, &bgId)) {
        return;
    }
    player->actor.world.pos.x += sinYaw * dist;
    player->actor.world.pos.z += cosYaw * dist;
    if (Forms_OnGround(player)) {
        player->actor.world.pos.y = player->actor.floorHeight;
    }
}

// A clip started out of a run must not resume that action from mid-animation.
static void GerudoForm_EndClip(PlayState* play, Player* player) {
    sMhr.state = GERUDO_IDLE;
    sMhr.timer = 0;
    GerudoForm_CylOff();
    func_8082DC38(player);
    Forms_Release(player);
    if (Forms_OnGround(player)) {
        func_80839E74(player, play);
    }
}

// ---- Urbosa's Fury presentation (stock lightning / sparkles / shockwave only) ----------------------
static const Color_RGBA8 sFuryGold = { 255, 210, 45, 255 };
static const Color_RGBA8 sFuryOrange = { 255, 90, 0, 180 };
static const Color_RGBA8 sFuryBlue = { 85, 150, 255, 255 };
static const Color_RGBA8 sFuryDeepBlue = { 25, 45, 190, 190 };
static const Color_RGBA8 sFuryWhite = { 255, 255, 220, 255 };

static u8 GerudoForm_Crossed(f32 prev, f32 cur, f32 mark) {
    return prev < mark && cur >= mark;
}

static void GerudoForm_FurySpawnBurst(PlayState* play, Player* player, s32 burst) {
    f32 progress = burst / 5.0f;
    f32 radius = 28.0f + progress * 48.0f;
    for (s32 i = 0; i < 3; i++) {
        s16 yaw = (s16)(burst * 0x1D00 + i * 0x5555 + play->gameplayFrames * 0x300);
        Vec3f pos = player->actor.world.pos;
        pos.x += Math_SinS(yaw) * radius;
        pos.y += 12.0f + i * 22.0f;
        pos.z += Math_CosS(yaw) * radius;
        u8 warm = ((burst + i) % 3) == 0;
        Color_RGBA8 prim = warm ? sFuryGold : sFuryBlue;
        Color_RGBA8 env = warm ? sFuryOrange : sFuryDeepBlue;
        EffectSsLightning_Spawn(play, &pos, &prim, &env, 75 + burst * 10, yaw, 10, 2);
        Vec3f vel = { -Math_SinS(yaw) * 1.4f, 1.2f + progress, -Math_CosS(yaw) * 1.4f };
        Vec3f accel = { 0.0f, -0.08f, 0.0f };
        Color_RGBA8 white = sFuryWhite;
        EffectSsKirakira_SpawnFocused(play, &pos, &vel, &accel, &prim, &white, 260 + burst * 24, 14);
    }
}

static void GerudoForm_FurySpawnRing(PlayState* play, Player* player, f32 radius, s32 count) {
    for (s32 i = 0; i < count; i++) {
        s16 yaw = (s16)(i * (0x10000 / count) + play->gameplayFrames * 0x200);
        Vec3f pos = player->actor.world.pos;
        pos.x += Math_SinS(yaw) * radius;
        pos.y += 10.0f;
        pos.z += Math_CosS(yaw) * radius;
        Color_RGBA8 prim = sFuryGold;
        Color_RGBA8 env = sFuryOrange;
        EffectSsLightning_Spawn(play, &pos, &prim, &env, 90, yaw, 8, 2);
    }
}

static void GerudoForm_FuryImpact(PlayState* play, Player* player) {
    Actor_RequestQuake(play, GMHR_LATK_QUAKE_STRENGTH, GMHR_LATK_QUAKE_FRAMES);
    Player_RequestRumble(play, player, 255, 20, 150, SQ(0));
    sMhr.screenDim = GMHR_LATK_DIM;
    play->envCtx.fillScreen = true;
    play->envCtx.screenFillColor[0] = 0;
    play->envCtx.screenFillColor[1] = 0;
    play->envCtx.screenFillColor[2] = 40;
    play->envCtx.screenFillColor[3] = (u8)sMhr.screenDim;
    Vec3f ground = player->actor.world.pos;
    ground.y = player->actor.floorHeight;
    Vec3f zero = { 0.0f, 0.0f, 0.0f };
    EffectSsBlast_SpawnWhiteShockwave(play, &ground, &zero, &zero);
    GerudoForm_FurySpawnRing(play, player, 110.0f, 8);
}

static void GerudoForm_TickScreenDim(PlayState* play) {
    if (sMhr.screenDim <= 0) {
        return;
    }
    sMhr.screenDim -= GMHR_LATK_DIM_FADE;
    if (sMhr.screenDim <= 0) {
        sMhr.screenDim = 0;
        play->envCtx.fillScreen = false;
        return;
    }
    play->envCtx.screenFillColor[3] = (u8)sMhr.screenDim;
}

static void GerudoForm_FuryVfx(PlayState* play, Player* player, f32 prev, f32 cur) {
    for (s32 burst = 0; burst < 6; burst++) {
        if (GerudoForm_Crossed(prev, cur, 36.0f + burst * 7.0f)) {
            GerudoForm_FurySpawnBurst(play, player, burst);
        }
    }
    if (GerudoForm_Crossed(prev, cur, 57.0f) || GerudoForm_Crossed(prev, cur, 67.0f) ||
        GerudoForm_Crossed(prev, cur, 75.0f)) {
        GerudoForm_FurySpawnRing(play, player, 70.0f, 6);
    }
    if (cur >= 34.0f && cur < GMHR_FURY_STRIKE_FRAME) {
        Actor_PlaySfx_Flagged(&player->actor, NA_SE_EN_BIRI_SPARK);
    }
    if (GerudoForm_Crossed(prev, cur, 88.0f) || GerudoForm_Crossed(prev, cur, 98.0f) ||
        GerudoForm_Crossed(prev, cur, 108.0f)) {
        GerudoForm_FurySpawnRing(play, player, 160.0f, 10);
    }
}

// ---- states ---------------------------------------------------------------------------------------
static void GerudoForm_TickFury(PlayState* play, Player* player) {
    f32 prev = sMhr.furyPrevFrame;
    f32 cur = player->skelAnime.curFrame;
    GerudoForm_TickClipBlades(player, &sFuryClip.window, &sFurySecondWindow);
    GerudoForm_Plant(player);
    GerudoForm_FuryVfx(play, player, (prev < 0.0f) ? cur - 1.0f : prev, cur);
    if (player->invincibilityTimer > -GMHR_FURY_IFRAMES) {
        player->invincibilityTimer = -GMHR_FURY_IFRAMES;
    }
    GerudoForm_CylOn(GMHR_LATK_DMGFLAGS, sMhr.furyDamage);
    f32 grow = cur / GMHR_FURY_BLAST_FRAMES;
    if (grow > 1.0f) {
        grow = 1.0f;
    }
    sMhr.cylRadius = (s16)(GMHR_SPIN_CYL_RADIUS + (GMHR_FURY_BLAST_RADIUS - GMHR_SPIN_CYL_RADIUS) * grow);
    sMhr.cylHeight = (s16)(GMHR_SPIN_CYL_HEIGHT + (GMHR_FURY_BLAST_HEIGHT - GMHR_SPIN_CYL_HEIGHT) * grow);
    if (GerudoForm_Crossed((prev < 0.0f) ? cur - 1.0f : prev, cur, GMHR_FURY_STRIKE_FRAME)) {
        GerudoForm_FuryImpact(play, player);
    }
    if (PlayerAnimation_Update(play, &player->skelAnime)) {
        GerudoForm_EndClip(play, player);
    }
}

// Cancellable from frame 0: vanilla already ate this frame's B, so a cancel that only ended the clip
// would throw the press away.
static void GerudoForm_TickFrontSlash(PlayState* play, Player* player) {
    const Input* in = Forms_RawInput();
    u8 aPress = CHECK_BTN_ALL(in->press.button, BTN_A);
    u8 bPress = CHECK_BTN_ALL(in->press.button, BTN_B);
    GerudoForm_Hold(player);
    player->speedXZ = 0.0f;
    if (sMhr.timer < GMHR_FRONT_SLASH_FRAMES) {
        GerudoForm_StepForward(play, player, GMHR_FRONT_SLASH_DIST / GMHR_FRONT_SLASH_FRAMES);
    }
    if (sMhr.timer >= 4 && sMhr.timer <= 60) {
        GerudoForm_CylOn(DMG_SWORD, 2);
    } else {
        GerudoForm_CylOff();
    }
    sMhr.timer++;
    if (aPress || bPress) {
        GerudoForm_EndClip(play, player);
        if (bPress) {
            func_80833864(play, player, PLAYER_MWA_FORWARD_SLASH_1H);
        }
        return;
    }
    if (PlayerAnimation_Update(play, &player->skelAnime)) {
        GerudoForm_EndClip(play, player);
    }
}

static void GerudoForm_TickAerial(PlayState* play, Player* player) {
    GerudoForm_Hold(player);
    GerudoForm_TickClipBlades(player, &sAerialClip.window, NULL);
    if (PlayerAnimation_Update(play, &player->skelAnime) || Forms_OnGround(player)) {
        GerudoForm_EndClip(play, player);
    }
}

static u8 GerudoForm_TryTriggers(PlayState* play, Player* player) {
    const Input* in = Forms_RawInput();
    u8 bPress = CHECK_BTN_ALL(in->press.button, BTN_B);
    u8 aPress = CHECK_BTN_ALL(in->press.button, BTN_A);
    u8 rHeld = CHECK_BTN_ALL(in->cur.button, BTN_R);
    u8 onGround = Forms_OnGround(player);
    u8 rolling = player->actionFunc == Player_Action_26;
    if (!GerudoForm_Fighter(player) || !Forms_CanAct(player)) {
        return 0;
    }
    if (onGround && rHeld && bPress && !rolling && sMhr.rageMeter > 0 && sTables.fury != NULL) {
        f32 charge = (f32)sMhr.rageMeter / (f32)GerudoForm_RageCapacity();
        sMhr.furyDamage = (u8)(GMHR_FURY_DAMAGE_MIN + (GMHR_FURY_DAMAGE_MAX - GMHR_FURY_DAMAGE_MIN) * charge);
        sMhr.rageMeter = 0;
        sMhr.comboStep = 0;
        player->stateFlags1 &= ~PLAYER_STATE1_SHIELDING;
        GerudoForm_StartClip(play, player, &sFuryClip, sTables.fury, GMHR_LATK_DMGFLAGS, sMhr.furyDamage);
        sMhr.state = GERUDO_FURY;
        Player_AnimSfx_PlayVoice(player, NA_SE_VO_LI_AUTO_JUMP);
        return 1;
    }
    if (onGround && bPress && !rolling && GerudoForm_WantsFrontSlash(player) && sTables.frontSlash != NULL) {
        GerudoForm_StartClip(play, player, &sFrontSlashClip, sTables.frontSlash, DMG_SWORD, 2);
        sMhr.state = GERUDO_FRONT_SLASH;
        Player_PlaySfx(player, NA_SE_IT_SWORD_SWING);
        Player_AnimSfx_PlayVoice(player, NA_SE_VO_LI_SWORD_N);
        return 1;
    }
    // The press that started a hop or jump slash also arrives on its first airborne frame.
    if (!onGround && aPress && sMhr.airFrames >= 2 && player->meleeWeaponState == PLAYER_MELEE_WEAPON_STATE_0 &&
        !(player->stateFlags1 & (FORMS_STATE1_HANGING | FORMS_STATE1_CLIMBING_LEDGE)) && sTables.aerial != NULL) {
        GerudoForm_StartClip(play, player, &sAerialClip, sTables.aerial, DMG_SWORD, 2);
        sMhr.state = GERUDO_AERIAL;
        Player_PlaySfx(player, NA_SE_IT_SWORD_SWING);
        return 1;
    }
    return 0;
}

u8 GerudoForm_Update(Player* player, PlayState* play) {
    GerudoForm_InstallTables();
    if (Forms_OnGround(player)) {
        sMhr.airFrames = 0;
    } else if (sMhr.airFrames < 1000) {
        sMhr.airFrames++;
    }
    GerudoForm_TickScreenDim(play);
    GerudoForm_TickA(player, play);
    GerudoForm_TickSprintRow();
    GerudoForm_TickCombo(player);
    if (GerudoForm_HoldsChargeWindow(player)) {
        player->unk_ADC = GMHR_CHARGE_WINDOW_PIN;
    }

    if (sMhr.state != GERUDO_IDLE && (player->stateFlags1 & FORMS_STATE1_DAMAGED)) {
        GerudoForm_EndClip(play, player);
    }
    switch (sMhr.state) {
        case GERUDO_IDLE:
            GerudoForm_TryTriggers(play, player);
            break;
        case GERUDO_FURY:
            GerudoForm_TickFury(play, player);
            break;
        case GERUDO_FRONT_SLASH:
            GerudoForm_TickFrontSlash(play, player);
            break;
        case GERUDO_AERIAL:
            GerudoForm_TickAerial(play, player);
            break;
        default:
            GerudoForm_EndClip(play, player);
            break;
    }
    GerudoForm_SubmitCyl(play, player);
    return sMhr.state != GERUDO_IDLE;
}

u8 GerudoForm_OwnsAction(void) {
    return sMhr.state != GERUDO_IDLE;
}

// ---- visuals --------------------------------------------------------------------------------------
u8 GerudoForm_SwordsOut(Player* player) {
    return Player_GetMeleeWeaponHeld(player) != PLAYER_MELEEWEAPON_NONE;
}

// One display list serves both hands and both ages: the right-hand bone matrix is mirrored, and the
// child skeleton's smaller bone scale shrinks it.
Gfx* GerudoForm_HandDL(Player* player, s32 limbIndex) {
    if (limbIndex != PLAYER_LIMB_LEFT_HAND && limbIndex != PLAYER_LIMB_RIGHT_HAND) {
        return NULL;
    }
    if (!GerudoForm_SwordsOut(player) || !ResourceMgr_FileExists(GERUDO_SWORD_DL + 7)) {
        return NULL;
    }
    return ResourceMgr_LoadGfxByName(GERUDO_SWORD_DL);
}

// The vanilla trail and quads hang off the LEFT hand only; the right blade gets its own trail and
// takes over melee quad 1 (the left blade keeps quad 0). Same tip/base rows as z_player_lib's sword.
static EffectBlureInit2 sRightTrailInit = {
    0,
    EFFECT_BLURE_ELEMENT_FLAG_8,
    0,
    { 255, 255, 255, 255 },
    { 255, 255, 255, 64 },
    { 255, 255, 255, 0 },
    { 255, 255, 255, 0 },
    4,
    0,
    EFF_BLURE_DRAW_MODE_SMOOTH,
    0,
    { 0, 0, 0, 0 },
    { 0, 0, 0, 0 },
};
static s32 sRightTrailIndex = -1;
static PlayState* sRightTrailPlay;

void GerudoForm_PostLimb(PlayState* play, Player* player, s32 limbIndex) {
    if (limbIndex != PLAYER_LIMB_RIGHT_HAND || !GerudoForm_SwordsOut(player)) {
        return;
    }
    if (sRightTrailPlay != play) {
        Effect_Add(play, &sRightTrailIndex, EFFECT_BLURE2, 0, 0, &sRightTrailInit);
        sRightTrailPlay = play;
    }
    Vec3f tipLocal = { 5000.0f, 400.0f, 0.0f };
    Vec3f baseLocal = { 0.0f, 400.0f, 0.0f };
    Vec3f tip;
    Vec3f base;
    Matrix_MultVec3f(&tipLocal, &tip);
    Matrix_MultVec3f(&baseLocal, &base);
    if (player->meleeWeaponState == PLAYER_MELEE_WEAPON_STATE_0) {
        return;
    }
    EffectBlure* trail = (EffectBlure*)Effect_GetByIndex(sRightTrailIndex);
    if (trail != NULL && !(player->stateFlags1 & PLAYER_STATE1_SHIELDING)) {
        EffectBlure_AddVertex(trail, &tip, &base);
    }
    Vec3f quadTipLocal = { 5000.0f, 1400.0f, -1000.0f };
    Vec3f quadBaseLocal = { 0.0f, 1400.0f, -1000.0f };
    Vec3f quadTip;
    Vec3f quadBase;
    Matrix_MultVec3f(&quadTipLocal, &quadTip);
    Matrix_MultVec3f(&quadBaseLocal, &quadBase);
    func_80126440(play, &player->meleeWeaponQuads[1], &player->meleeWeaponInfo[2], &quadTip, &quadBase);
}

// ---- lifecycle ------------------------------------------------------------------------------------
void GerudoForm_Enter(void) {
    memset(&sMhr, 0, sizeof(sMhr));
    sMhr.state = GERUDO_IDLE;
    sMhr.furyPrevFrame = -1.0f;
    GerudoForm_InstallTables();
}

// Rage cannot survive leaving the form: the tables are restored on the way out.
void GerudoForm_Exit(void) {
    GerudoForm_RestoreTables();
    if (gPlayState != NULL && sMhr.screenDim > 0) {
        gPlayState->envCtx.fillScreen = false;
    }
    memset(&sMhr, 0, sizeof(sMhr));
    sMhr.state = GERUDO_IDLE;
    sMhr.furyPrevFrame = -1.0f;
    if (sCylInited) {
        sCyl.base.atFlags &= ~(AT_ON | AT_HIT);
    }
}
