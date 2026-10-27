/*
 * form_kafei.cpp - Kafei (SW97 moveset) on top of vanilla Link (Skijer's NEI). Port of soh's
 * kafei_form.cpp / kafei_landmine.cpp: passive shield while still, stamina sprint on A, no roll,
 * plain jump under Z, a sword slash that keeps the stride, and Bombchus that settle as landmines.
 */
#include "forms_internal.h"

extern "C" {
#include "overlays/actors/ovl_En_Bom_Chu/z_en_bom_chu.h"
#include "objects/gameplay_keep/gameplay_keep.h"
}

#define KAFEI_SHIELD_STILL_SPEED 0.1f
#define KAFEI_SHIELD_FORWARD 12.0f
#define KAFEI_SHIELD_HALF_WIDTH 20.0f
#define KAFEI_SPRINT_MUL 1.5f
#define KAFEI_SPRINT_ANIM_MUL 1.4f
#define KAFEI_WHEEL_FRAMES 220.0f
#define KAFEI_REFILL_PER_FRAME 2.2f
#define KAFEI_REFILL_DELAY 18
#define KAFEI_STICK_MOVING 10.0f
#define KAFEI_MOVING_SLASH_MIN_SPEED 2.0f
#define KAFEI_SLASH_NONE 0xFF
#define KAFEI_POSE_SLOTS 32

#define KAFEI_LANDMINE_TIMER 500
#define KAFEI_LANDMINE_REARM 200
#define KAFEI_LANDMINE_FALL 8.0f
#define LANDMINE_SPHERE "__OTR__objects/object_landmine/gLandMineSphereDL"
#define LANDMINE_PLANE "__OTR__objects/object_landmine/gLandMinePlaneDL"
#define LANDMINE_SPIKES "__OTR__objects/object_landmine/gLandMineSphere004DL"

typedef struct KafeiSlashClip {
    const char* path;
    u8 arcStart;
    u8 arcEnd;
} KafeiSlashClip;

// Arc = frames whose left-arm angular velocity exceeds 30% of the clip's peak (measured in soh).
static const KafeiSlashClip sSlashClips[2] = {
    { "__OTR__misc/link_animetion/gPlayerAnim_mhr_sw97_move_sword_slash", 1, 5 },
    { "__OTR__misc/link_animetion/gPlayerAnim_mhr_sw97_move_stick_slash", 2, 4 },
};
static PlayerAnimationHeader* sSlashAnims[2];
static Vec3s sSlashPose[KAFEI_POSE_SLOTS];
static u8 sSlashCopyMap[KAFEI_POSE_SLOTS];

static struct {
    f32 stamina;
    u8 sprinting;
    u8 winded;
    u8 windedAnimSet;
    u8 refillDelay;
    u8 slashClip;
    u8 slashFrame;
} sKafei;

static f32 KafeiForm_Capacity(void) {
    return gSaveContext.save.saveInfo.playerData.doubleDefense ? KAFEI_WHEEL_FRAMES * 2.0f : KAFEI_WHEEL_FRAMES;
}

static u8 KafeiForm_IsActive(void) {
    return CustomForms_ActiveForm() == CUSTOM_FORM_KAFEI;
}

void KafeiForm_Reset(void) {
    sKafei.stamina = KafeiForm_Capacity();
    sKafei.sprinting = 0;
    sKafei.winded = 0;
    sKafei.windedAnimSet = 0;
    sKafei.refillDelay = 0;
    sKafei.slashClip = KAFEI_SLASH_NONE;
    sKafei.slashFrame = 0;
}

f32 KafeiForm_RunSpeedMul(void) {
    return sKafei.sprinting ? KAFEI_SPRINT_MUL : 1.0f;
}

// Without the matching leg-cycle boost the legs skate under the faster body.
f32 KafeiForm_RunAnimRateMul(void) {
    return sKafei.sprinting ? KAFEI_SPRINT_ANIM_MUL : 1.0f;
}

// The roll fires on the PRESS, one frame before the sprint latch exists — so the raw held A is
// tested too, otherwise exactly one roll leaks on the first frame.
u8 KafeiForm_SuppressRoll(Player* player) {
    if (!KafeiForm_IsActive() || player->transformation != PLAYER_FORM_HUMAN) {
        return 0;
    }
    if (sKafei.sprinting) {
        return 1;
    }
    return CHECK_BTN_ALL(Forms_RawInput()->cur.button, BTN_A) ? 1 : 0;
}

// ---- passive shield -------------------------------------------------------------------------------
// Standing still with a shield equipped guards the front without raising it (SW97: speedXZ <= 0.1,
// so backing up still guards).
static void KafeiForm_SubmitPassiveShield(Player* player, PlayState* play) {
    if (player->currentShield == PLAYER_SHIELD_NONE || player->speedXZ > KAFEI_SHIELD_STILL_SPEED ||
        (player->stateFlags1 & PLAYER_STATE1_SHIELDING)) {
        return;
    }
    f32 sinYaw = Math_SinS(player->actor.shape.rot.y);
    f32 cosYaw = Math_CosS(player->actor.shape.rot.y);
    Vec3f center = player->actor.world.pos;
    center.x += sinYaw * KAFEI_SHIELD_FORWARD;
    center.z += cosYaw * KAFEI_SHIELD_FORWARD;
    Vec3f right = { cosYaw * KAFEI_SHIELD_HALF_WIDTH, 0.0f, -sinYaw * KAFEI_SHIELD_HALF_WIDTH };
    Vec3f a = { center.x - right.x, center.y + 10.0f, center.z - right.z };
    Vec3f b = { center.x + right.x, center.y + 10.0f, center.z + right.z };
    Vec3f c = { a.x, center.y + 60.0f, a.z };
    Vec3f d = { b.x, center.y + 60.0f, b.z };

    player->shieldQuad.base.colMaterial = COL_MATERIAL_METAL;
    Collider_SetQuadVertices(&player->shieldQuad, &a, &b, &c, &d);
    CollisionCheck_SetAC(play, &play->colChkCtx, &player->shieldQuad.base);
    CollisionCheck_SetAT(play, &play->colChkCtx, &player->shieldQuad.base);
}

// ---- moving slash ---------------------------------------------------------------------------------
static PlayerAnimationHeader* KafeiForm_SlashAnim(u8 clip) {
    if (sSlashAnims[clip] == NULL) {
        sSlashAnims[clip] = Forms_LoadAnim(sSlashClips[clip].path);
    }
    return sSlashAnims[clip];
}

static void KafeiForm_EndMovingSlash(PlayState* play, Player* player) {
    sKafei.slashClip = KAFEI_SLASH_NONE;
    if (player->meleeWeaponState != PLAYER_MELEE_WEAPON_STATE_0) {
        func_8082FA5C(play, player, PLAYER_MELEE_WEAPON_STATE_0);
    }
}

// Called from Player_ActionHandler_7 on the confirmed B press; 1 = the vanilla swing (which brakes
// to a stop) must not start.
u8 KafeiForm_StartMovingSlash(Player* player) {
    if (!KafeiForm_IsActive()) {
        return 0;
    }
    if (sKafei.slashClip != KAFEI_SLASH_NONE) {
        return 1;
    }
    if (!Forms_OnGround(player)) {
        return 0;
    }
    if (Forms_StickMagnitude(Forms_RawInput()) < KAFEI_STICK_MOVING || player->speedXZ < KAFEI_MOVING_SLASH_MIN_SPEED) {
        return 0;
    }
    PlayerMeleeWeapon weapon = Player_MeleeWeaponFromIA((PlayerItemAction)player->heldItemAction);
    if (weapon == PLAYER_MELEEWEAPON_NONE || weapon > PLAYER_MELEEWEAPON_DEKU_STICK) {
        return 0;
    }
    u8 clip = (weapon == PLAYER_MELEEWEAPON_DEKU_STICK) ? 1 : 0;
    if (KafeiForm_SlashAnim(clip) == NULL) {
        return 0;
    }
    sKafei.slashClip = clip;
    sKafei.slashFrame = 0;
    // The swing sfx and grunt are picked off meleeWeaponAnimation by the state setter.
    player->meleeWeaponAnimation = PLAYER_MWA_FORWARD_SLASH_1H;
    player->unk_ADD = 0;
    return 1;
}

// Overlays the slash onto the locomotion pose: only the left shoulder/forearm/hand are replaced, both
// as queued anim tasks so they land after Player_UpdateCommon's own frame load.
static void KafeiForm_UpdateMovingSlash(Player* player, PlayState* play) {
    if (sKafei.slashClip == KAFEI_SLASH_NONE) {
        return;
    }
    PlayerAnimationHeader* anim = KafeiForm_SlashAnim(sKafei.slashClip);
    if (anim == NULL || !Forms_OnGround(player) ||
        Player_MeleeWeaponFromIA((PlayerItemAction)player->heldItemAction) == PLAYER_MELEEWEAPON_NONE) {
        KafeiForm_EndMovingSlash(play, player);
        return;
    }
    if (sSlashCopyMap[PLAYER_LIMB_LEFT_HAND] == 0) {
        sSlashCopyMap[PLAYER_LIMB_LEFT_SHOULDER] = 1;
        sSlashCopyMap[PLAYER_LIMB_LEFT_FOREARM] = 1;
        sSlashCopyMap[PLAYER_LIMB_LEFT_HAND] = 1;
    }
    AnimTaskQueue_AddLoadPlayerFrame(play, anim, sKafei.slashFrame, player->skelAnime.limbCount, sSlashPose);
    AnimTaskQueue_AddCopyUsingMap(play, player->skelAnime.limbCount, player->skelAnime.jointTable, sSlashPose,
                                  sSlashCopyMap);

    const KafeiSlashClip* clip = &sSlashClips[sKafei.slashClip];
    u8 blade = (sKafei.slashFrame >= clip->arcStart) && (sKafei.slashFrame <= clip->arcEnd);
    u8 armed = player->meleeWeaponState != PLAYER_MELEE_WEAPON_STATE_0;
    if (blade != armed) {
        func_8082FA5C(play, player, blade ? PLAYER_MELEE_WEAPON_STATE_1 : PLAYER_MELEE_WEAPON_STATE_0);
    }
    sKafei.slashFrame++;
    if (sKafei.slashFrame > (u8)Animation_GetLastFrame(anim)) {
        KafeiForm_EndMovingSlash(play, player);
    }
}

// ---- stamina --------------------------------------------------------------------------------------
static void KafeiForm_PlayWindedAnim(Player* player, PlayState* play) {
    if (sKafei.windedAnimSet) {
        return;
    }
    sKafei.windedAnimSet = 1;
    PlayerAnimationHeader* windedAnim = Forms_VanillaAnim(gPlayerAnim_link_normal_landing);
    PlayerAnimation_Change(play, &player->skelAnime, windedAnim, 1.0f, 0.0f, Animation_GetLastFrame(windedAnim),
                           ANIMMODE_ONCE, -6.0f);
}

void KafeiForm_Update(Player* player, PlayState* play) {
    KafeiForm_UpdateMovingSlash(player, play);
    KafeiForm_SubmitPassiveShield(player, play);

    if (sKafei.winded) {
        sKafei.sprinting = 0;
        player->speedXZ = 0.0f;
        player->actor.speed = 0.0f;
        KafeiForm_PlayWindedAnim(player, play);
        if (sKafei.stamina >= KafeiForm_Capacity()) {
            sKafei.winded = 0;
        }
    }

    const Input* in = Forms_RawInput();
    u8 wantsSprint = CHECK_BTN_ALL(in->cur.button, BTN_A) && Forms_OnGround(player) &&
                     (Forms_StickMagnitude(in) >= KAFEI_STICK_MOVING) &&
                     !(player->stateFlags1 & PLAYER_STATE1_SHIELDING);
    sKafei.sprinting = wantsSprint && (sKafei.stamina > 0.0f);
    if (sKafei.sprinting) {
        sKafei.stamina -= 1.0f;
        sKafei.refillDelay = KAFEI_REFILL_DELAY;
        if (sKafei.stamina <= 0.0f) {
            sKafei.stamina = 0.0f;
            sKafei.sprinting = 0;
            sKafei.winded = 1;
            sKafei.windedAnimSet = 0;
        }
        return;
    }
    if (sKafei.refillDelay > 0) {
        sKafei.refillDelay--;
        return;
    }
    sKafei.stamina += KAFEI_REFILL_PER_FRAME;
    if (sKafei.stamina > KafeiForm_Capacity()) {
        sKafei.stamina = KafeiForm_Capacity();
    }
}

// ---- HUD readouts ---------------------------------------------------------------------------------
extern "C" u8 KafeiForm_MeterVisible(void) {
    if (!KafeiForm_IsActive()) {
        return 0;
    }
    return sKafei.sprinting || sKafei.winded || (sKafei.stamina < KafeiForm_Capacity());
}

extern "C" s32 KafeiForm_WheelCount(void) {
    return gSaveContext.save.saveInfo.playerData.doubleDefense ? 2 : 1;
}

extern "C" f32 KafeiForm_WheelFill(s32 wheel) {
    f32 remaining = sKafei.stamina - wheel * KAFEI_WHEEL_FRAMES;
    if (remaining <= 0.0f) {
        return 0.0f;
    }
    if (remaining >= KAFEI_WHEEL_FRAMES) {
        return 1.0f;
    }
    return remaining / KAFEI_WHEEL_FRAMES;
}

extern "C" u8 KafeiForm_IsWinded(void) {
    return sKafei.winded;
}

// ---- SW97 landmine on the Bombchu actor ----------------------------------------------------------
extern "C" u8 KafeiLandmine_Active(void) {
    return KafeiForm_IsActive();
}

extern "C" void KafeiLandmine_OnSpawn(Actor* chu) {
    if (KafeiForm_IsActive()) {
        ((EnBomChu*)chu)->timer = KAFEI_LANDMINE_TIMER;
    }
}

// Replaces EnBomChu_Move: the chu stops where it was dropped, sinks to the floor and recycles its fuse
// so it never reaches the red "about to blow" blink.
extern "C" u8 KafeiLandmine_Settle(Actor* actor, PlayState* play) {
    if (!KafeiForm_IsActive()) {
        return 0;
    }
    EnBomChu* chu = (EnBomChu*)actor;
    chu->movingSpeed = 0.0f;
    chu->actor.speed = 0.0f;
    chu->visualJitter = 0.0f;
    if (chu->timer < KAFEI_LANDMINE_REARM) {
        chu->timer = KAFEI_LANDMINE_TIMER;
    }
    Actor_UpdateBgCheckInfo(play, &chu->actor, 5.0f, 5.0f, 0.0f,
                            UPDBGCHECKINFO_FLAG_1 | UPDBGCHECKINFO_FLAG_2 | UPDBGCHECKINFO_FLAG_4 |
                                UPDBGCHECKINFO_FLAG_8 | UPDBGCHECKINFO_FLAG_10);
    if (chu->actor.world.pos.y > chu->actor.floorHeight) {
        chu->actor.world.pos.y -= KAFEI_LANDMINE_FALL;
        if (chu->actor.world.pos.y < chu->actor.floorHeight) {
            chu->actor.world.pos.y = chu->actor.floorHeight;
        }
    }
    return 1;
}

// No fuse timeout: it goes off when attacked or when an ENEMY steps on it.
extern "C" u8 KafeiLandmine_ShouldDetonate(Actor* actor) {
    EnBomChu* chu = (EnBomChu*)actor;
    u8 struck = (chu->collider.base.acFlags & AC_HIT) != 0;
    u8 touched = (chu->collider.base.ocFlags1 & OC1_HIT) != 0;
    return struck || (touched && chu->collider.base.oc != NULL && chu->collider.base.oc->category == ACTORCAT_ENEMY);
}

static void KafeiLandmine_DrawPiece(PlayState* play, Actor* actor, const char* path, f32 pieceScale, u8 faceCamera,
                                    u8 fade) {
    const char* p = path + 7;
    if (!ResourceMgr_FileExists(p)) {
        return;
    }
    Gfx* dl = ResourceMgr_LoadGfxByName(path);
    if (dl == NULL) {
        return;
    }
    OPEN_DISPS(play->state.gfxCtx);
    Matrix_Translate(actor->world.pos.x, actor->world.pos.y, actor->world.pos.z, MTXMODE_NEW);
    if (faceCamera) {
        Matrix_RotateYS(Camera_GetCamDirYaw(GET_ACTIVE_CAM(play)), MTXMODE_APPLY);
        Matrix_RotateXS(-Camera_GetCamDirPitch(GET_ACTIVE_CAM(play)), MTXMODE_APPLY);
    } else {
        Matrix_RotateYS(actor->shape.rot.y, MTXMODE_APPLY);
    }
    f32 s = actor->scale.x * pieceScale;
    Matrix_Scale(s, s, s, MTXMODE_APPLY);
    // The plane's own DL forces a black PRIM, so every piece re-emits its tint.
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, fade, fade, 255);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, dl);
    CLOSE_DISPS(play->state.gfxCtx);
}

extern "C" void KafeiLandmine_Draw(PlayState* play, Actor* chu, f32 colorIntensity) {
    u8 fade = (u8)(255.0f - colorIntensity * 170.0f);
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
    KafeiLandmine_DrawPiece(play, chu, LANDMINE_SPHERE, 95.0f, 0, fade);
    KafeiLandmine_DrawPiece(play, chu, LANDMINE_PLANE, 105.0f, 1, fade);
    KafeiLandmine_DrawPiece(play, chu, LANDMINE_SPIKES, 1.05f, 0, fade);
}
