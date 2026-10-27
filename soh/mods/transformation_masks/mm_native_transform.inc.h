// Included by mm_player_form.cpp after its form/skeleton helpers. Both the
// combined launcher and solo SoH use this controller; no MM DLL calls belong here.
// Donor: MM Player_Action_86/87, func_80855218 and Player_PostLimbDrawGameplay.

#include "mods/extended_player.h"
extern "C" {
void Player_DestroyHookshot(Player* player);
void Player_DetachHeldActor(PlayState* play, Player* player);
void Player_InitItemAction(PlayState* play, Player* player, s8 itemAction);
void Player_SetUpperActionFunc(Player* player, UpperActionFunc action);
f32 Camera_LERPCeilF(f32 target, f32 current, f32 stepScale, f32 minDiff);
s16 Camera_LERPCeilS(s16 target, s16 current, f32 stepScale, s16 minDiff);
void Camera_LERPCeilVec3f(Vec3f* target, Vec3f* current, f32 yStepScale, f32 xzStepScale, f32 minDiff);
}

struct MmNativeMaskCamera {
    VecSph previousOffset;
    f32 previousFov, swayDirection, fovStep;
    s16 state, timer, yaw;
};

struct MmNativeMaskCutscene {
    PlayState* owner;
    u8 active, removing, looping, recovering;
    u8 blurReady, blurCopied;
    u32 blurWidth, blurHeight, blurFrame;
    s16 faceAlpha;
    s16 recoveryTimer;
    u32 ownedFlags;
    LinkAnimationHeader *maskOn, *maskLoop, *maskOff, *formOff;
    s16 cameraId, previousCamera;
    MmNativeMaskCamera camera;
    f32 lighting, maskDepth, maskWidth;
    s16 ambient[3], light[3], fog[3], fogNear;
    LightInfo lightInfo;
    LightNode* lightNode;
    Gfx *maskDL, *stretchDL, *veilDL;
};
static MmNativeMaskCutscene sNativeMaskCs = {};
static u8 sNativeMaskInstantFallback = 0;

static u8 MmForm_IsNativeMaskForm(MmPlayerTransformation form) {
    return form == MM_PLAYER_FORM_DEKU || form == MM_PLAYER_FORM_GORON || form == MM_PLAYER_FORM_ZORA ||
           form == MM_PLAYER_FORM_FIERCE_DEITY;
}

static void MmForm_StopNativeMaskSfx(bool includeFlash = true) {
    // Even short cues retain the player's projected-position pointer in the
    // MM bank. Drain pending requests and active voices before scene teardown.
    static const u16 cues[] = {
        MM_NA_SE_PL_PUT_OUT_ITEM,          MM_NA_SE_IT_SET_TRANSFORM_MASK, MM_NA_SE_PL_FREEZE_S,
        MM_NA_SE_IT_TRANSFORM_MASK_BROKEN, MM_NA_SE_PL_TRANSFORM_VOICE,    MM_NA_SE_PL_FACE_CHANGE,
        MM_NA_SE_EV_LIGHTNING_HARD,
    };
    for (u16 cue : cues)
        MmSfx_Stop(cue);
    // Skip starts a new flash in this same update. Its stop guard must not
    // suppress that cue; final teardown still cancels it with the other sounds.
    if (includeFlash)
        MmSfx_Stop(MM_NA_SE_SY_TRANSFORM_MASK_FLASH);
}

static void MmForm_RestoreNativeMaskLighting(PlayState* play) {
    memcpy(play->envCtx.adjAmbientColor, sNativeMaskCs.ambient, sizeof(sNativeMaskCs.ambient));
    memcpy(play->envCtx.adjLight1Color, sNativeMaskCs.light, sizeof(sNativeMaskCs.light));
    memcpy(play->envCtx.adjFogColor, sNativeMaskCs.fog, sizeof(sNativeMaskCs.fog));
    play->envCtx.adjFogNear = sNativeMaskCs.fogNear;
}

static void MmForm_EndNativeMaskCutscene(Player* player, PlayState* play, bool stopSfx = true) {
    if (!sNativeMaskCs.active)
        return;
    // Successful recovery leaves the native one-shot tails playing. Interrupts
    // still drain the bank before a retained player-position pointer can expire.
    if (stopSfx)
        MmForm_StopNativeMaskSfx();
    // Player_Init receives the new PlayState after the old scene's camera and
    // light contexts were destroyed. Never release their handles through it.
    if (sNativeMaskCs.owner != play) {
        sNativeMaskCs = {};
        return;
    }
    MmForm_RestoreNativeMaskLighting(play);
    if (sNativeMaskCs.lightNode != NULL) {
        LightContext_RemoveLight(play, &play->lightCtx, sNativeMaskCs.lightNode);
    }
    if (sNativeMaskCs.cameraId != SUBCAM_NONE) {
        // Another cutscene may have taken over while this one was interrupted.
        const bool ownsCamera = play->activeCamera == sNativeMaskCs.cameraId;
        Play_ClearCamera(play, sNativeMaskCs.cameraId);
        if (ownsCamera)
            Play_ChangeCameraStatus(play, sNativeMaskCs.previousCamera, CAM_STAT_ACTIVE);
    }
    if (player != NULL) {
        player->stateFlags1 &= ~sNativeMaskCs.ownedFlags;
        player->stateFlags3 &= ~PLAYER_STATE3_PAUSE_ACTION_FUNC;
    }
    sNativeMaskCs = {};
}

static void MmForm_InterruptNativeMaskForScene(Player* player, PlayState* play, u8 abandon) {
    sNativeMaskInstantFallback = 0;
    if (!sNativeMaskCs.active) {
        // Recovery can release camera ownership before the last SFX finishes.
        MmForm_StopNativeMaskSfx();
        return;
    }
    // Preserve the body that actually reached the screen. Before the flash it
    // is still human; after it, a new scene can seamlessly reload the form.
    if (gFormState.skeletonLoaded && MmForm_IsNativeMaskForm(gFormState.currentForm)) {
        gFormState.state = MMFORM_STATE_ACTIVE;
        if (!sNativeMaskCs.removing)
            MmForm_SaveAndRestrictEquips(play);
    } else {
        gFormState.currentForm = MM_PLAYER_FORM_HUMAN;
        gFormState.state = MMFORM_STATE_INACTIVE;
        MmForm_RestoreEquips(play);
        ExtEquip_RestoreFromTransform();
    }
    gFormState.cutsceneTimer = gFormState.cutscenePhase = gFormState.flashAlpha = 0;
    if (abandon) {
        // Init may reuse precisely the old PlayState address. Pointer equality
        // cannot establish that its camera/light contexts still belong to us.
        MmForm_StopNativeMaskSfx();
        sNativeMaskCs = {};
    } else
        MmForm_EndNativeMaskCutscene(player, play);
}

extern "C" void MmForm_OnPlayDestroy(PlayState* play, Player* player) {
    MmForm_InterruptNativeMaskForScene(player, play, 0);
}

static void MmForm_CancelNativeMaskCutscene(u8 dying) {
    sNativeMaskInstantFallback = 0;
    if (!sNativeMaskCs.active) {
        // Recovery can release camera ownership before the last SFX finishes.
        MmForm_StopNativeMaskSfx();
        return;
    }
    // The existing death equipment/tunic rollback checks ACTIVE. A revealed
    // form has already applied those properties even during its recovery pose.
    if (dying && gFormState.skeletonLoaded && MmForm_IsNativeMaskForm(gFormState.currentForm))
        gFormState.state = MMFORM_STATE_ACTIVE;
    if (gPlayState != NULL)
        MmForm_EndNativeMaskCutscene(GET_PLAYER(gPlayState), gPlayState);
    else {
        MmForm_StopNativeMaskSfx();
        sNativeMaskCs = {};
    }
}

static void MmForm_UpdateNativeMaskCamera(Player* player, PlayState* play) {
    // MM Player_Action_86 faces the live cutscene camera every update. Keep
    // the captured front direction even if OoT's paused action changes yaw.
    player->actor.shape.rot.y = player->yaw = sNativeMaskCs.camera.yaw;
    if (sNativeMaskCs.cameraId == SUBCAM_NONE)
        return;
    Camera* camera = play->cameraPtrs[sNativeMaskCs.cameraId];
    auto& shot = sNativeMaskCs.camera;
    Vec3f at = camera->at;
    VecSph offset;
    OLib_Vec3fDiffToVecSphGeo(&offset, &at, &camera->eye);
    Vec3f focus = player->actor.focus.pos;
    const Vec3f pos = player->actor.world.pos;
    f32 fov = camera->fov;
    s16 roll = camera->roll;
    // The port switches skeletons under the white flash instead of restarting
    // Player_Init. Enter the donor's reveal camera before the flash fades.
    if (gFormState.cutscenePhase == 2)
        shot.state = sNativeMaskCs.removing ? 3 : 4;

    // Adapt MM Camera_Demo4/5 geometry to OoT's manual subcamera ABI. Preserve
    // their separate schedules, face focus, FOV, roll and original view offset.
    if (!sNativeMaskCs.removing) {
        switch (shot.state) {
            case 0:
                shot.state = 1;
                shot.timer = 0;
                offset.r = MIN(offset.r, 40.0f);
                fov = 80.0f;
                shot.swayDirection = Rand_ZeroOne() < 0.5f ? -1.0f : 1.0f;
                // fallthrough
            case 1: {
                const f32 sway = shot.timer >= 12
                                     ? shot.swayDirection * sinf(DEG_TO_RAD((shot.timer - 12) * (135.0f / 13.0f)))
                                     : 0.0f;
                const f32 lateral = shot.timer * (6.0f / 19.0f) * sway;
                focus.x = pos.x + Math_SinS(shot.yaw + 0x4000) * lateral;
                focus.z = pos.z + Math_CosS(shot.yaw + 0x4000) * lateral;
                focus.y -= (focus.y - pos.y) * 0.099999994f;
                Camera_LERPCeilVec3f(&focus, &at, 0.2f, 0.2f, 0.1f);
                roll = (s16)(shot.timer * (30.0f / 19.0f) * sway * (65535.0f / 360.0f) + 0.5f);
                offset.r = Camera_LERPCeilF(30.0f, offset.r, 1.0f / (38 - shot.timer), 0.1f);
                offset.pitch = 0;
                if (++shot.timer >= 38) {
                    shot.timer = 24;
                    shot.state = 2;
                    shot.fovStep = (32.0f - fov) / 24.0f;
                }
                break;
            }
            case 2:
                focus.x = pos.x - Math_SinS(shot.yaw) * 7.0f;
                focus.z = pos.z - Math_CosS(shot.yaw) * 7.0f;
                focus.y -= (focus.y - pos.y) * 0.1f;
                if (shot.timer == 24)
                    at = focus;
                else
                    Camera_LERPCeilVec3f(&focus, &at, 0.25f, 0.25f, 0.1f);
                if (shot.timer > 0)
                    fov += shot.fovStep;
                offset.r = 35.0f;
                offset.pitch = 0x2000;
                roll = Camera_LERPCeilS(0, roll, 0.1f, 5);
                if (--shot.timer <= 0) {
                    shot.timer = 0;
                    shot.state = 3;
                    shot.fovStep = (60.0f - fov) / 630.0f;
                }
                break;
            case 3:
                focus.x = pos.x;
                focus.z = pos.z;
                focus.y -= (focus.y - pos.y) * 0.1f;
                Camera_LERPCeilVec3f(&focus, &at, 0.25f, 0.25f, 0.1f);
                roll = Camera_LERPCeilS(0, roll, 0.1f, 5);
                fov += shot.fovStep * ++shot.timer;
                offset.r = 35.0f;
                offset.pitch = 0x2000;
                if (shot.timer >= 35)
                    shot.state = 4;
                break;
            case 999:
                shot.state = 4;
                break;
            case 4:
                focus.y -= (focus.y - pos.y) * 0.1f;
                Camera_LERPCeilVec3f(&focus, &at, 0.1f, 0.1f, 0.1f);
                offset = shot.previousOffset;
                fov = shot.previousFov;
                roll = 0;
                break;
        }
    } else {
        switch (shot.state) {
            case 0:
                shot.state = 1;
                shot.timer = 18;
                offset.r = 30.0f;
                offset.pitch = 0;
                fov = 80.0f;
                shot.swayDirection = Rand_ZeroOne() < 0.5f ? -1.0f : 1.0f;
                roll = 0;
                focus.x = pos.x;
                focus.z = pos.z;
                at = focus;
                // fallthrough
            case 1:
                if (--shot.timer <= 0) {
                    shot.timer = 46;
                    shot.state = 2;
                }
                break;
            case 2: {
                const f32 sway = shot.swayDirection * sinf(DEG_TO_RAD(shot.timer * (180.0f / 23.0f)));
                const f32 lateral = (46 - shot.timer) * (5.0f / 46.0f) * sway;
                focus.x = pos.x + Math_SinS(shot.yaw + 0x4000) * lateral;
                focus.z = pos.z + Math_CosS(shot.yaw + 0x4000) * lateral;
                focus.y -= (focus.y - pos.y) * 0.2f;
                Camera_LERPCeilVec3f(&focus, &at, 0.1f, 0.1f, 0.1f);
                roll = (s16)(sway * (shot.timer * (10.0f / 23.0f)) * (65535.0f / 360.0f) + 0.5f);
                offset.r = Camera_LERPCeilF(80.0f, offset.r, 1.0f / shot.timer, 0.1f);
                offset.pitch = 0;
                if (--shot.timer <= 0)
                    shot.state = 3;
                break;
            }
            case 999:
                shot.state = 3;
                break;
            case 3:
                focus.y -= (focus.y - pos.y) * 0.1f;
                Camera_LERPCeilVec3f(&focus, &at, 0.1f, 0.1f, 0.1f);
                offset = shot.previousOffset;
                fov = shot.previousFov;
                roll = 0;
                break;
        }
    }
    Vec3f eye;
    OLib_VecSphGeoToVec3f(&eye, &offset);
    eye.x += at.x;
    eye.y += at.y;
    eye.z += at.z;
    Play_CameraSetAtEye(play, sNativeMaskCs.cameraId, &at, &eye);
    Play_CameraSetFov(play, sNativeMaskCs.cameraId, fov);
    Play_SetCameraRoll(play, sNativeMaskCs.cameraId, roll);
}

static void MmForm_UpdateNativeMaskLighting(Player* player, PlayState* play) {
    // Exact fog/ambient keys from MM D_8085D848. Restore the pre-existing OoT
    // adjustment on exit, including weather effects and indoor lighting.
    static const struct {
        s16 fogNear;
        u8 fog[3], ambient[3];
    } keys[] = {
        { 650, { 0, 0, 0 }, { 10, 0, 30 } },
        { 300, { 200, 200, 255 }, { 0, 0, 0 } },
        { 600, { 0, 0, 0 }, { 0, 0, 200 } },
    };
    const s16 timer = gFormState.cutsceneTimer;
    if (gFormState.cutscenePhase == 2) {
        MmForm_RestoreNativeMaskLighting(play);
        return;
    }
    if (timer >= 63)
        Math_StepToF(&sNativeMaskCs.lighting, 3.0f, 0.2f);
    else if (timer >= 59)
        Math_StepToF(&sNativeMaskCs.lighting, 2.0f, 0.5f);
    else if (timer >= 16)
        Math_StepToF(&sNativeMaskCs.lighting, 1.0f, 0.1f);
    const f32 value = sNativeMaskCs.lighting;
    const int key = value > 2.0f ? 2 : value > 1.0f ? 1 : 0;
    const f32 weight = value - key;
    const auto& base = play->envCtx.lightSettings;
    for (s32 i = 0; i < 3; ++i) {
        const f32 ambient = key ? keys[key - 1].ambient[i] : base.ambientColor[i];
        const f32 fog = key ? keys[key - 1].fog[i] : base.fogColor[i];
        const f32 light = key ? 0 : base.light1Color[i];
        play->envCtx.adjAmbientColor[i] =
            (s16)(ambient + (keys[key].ambient[i] - ambient) * weight) - base.ambientColor[i];
        play->envCtx.adjFogColor[i] = (s16)(fog + (keys[key].fog[i] - fog) * weight) - base.fogColor[i];
        play->envCtx.adjLight1Color[i] = (s16)(light * (1.0f - weight)) - base.light1Color[i];
    }
    const f32 fogNear = key ? keys[key - 1].fogNear : base.fogNear;
    play->envCtx.adjFogNear = (s16)(fogNear + (keys[key].fogNear - fogNear) * weight) - base.fogNear;
    const u8 flash = value > 1.0f && value < 2.0f;
    Lights_PointNoGlowSetInfo(&sNativeMaskCs.lightInfo, player->actor.world.pos.x, player->actor.world.pos.y + 20.0f,
                              player->actor.world.pos.z, flash ? 255 : 120, flash ? 255 : 200, 255,
                              (s16)(sNativeMaskCs.removing ? 100 : value * 1000.0f));
}

static u8 MmForm_BeginNativeMaskCutscene(Player* player, PlayState* play, u8 removing) {
    const MmPlayerTransformation form = removing ? gFormState.currentForm : gFormState.targetForm;
    if (!MmForm_IsNativeMaskForm(form))
        return 0;
    if (gFormState.cutsceneTimer != 0)
        return 0;
    // Capture the preference once. Changing it during the motion must not hand
    // a half-owned pose to the instant controller or reload the form twice.
    if (CVarGetInteger("gMods.TransformMasks.InstantTransform", 0) || (!removing && sForceInstantTransform))
        return 0;
    sNativeMaskCs = {};
    sNativeMaskCs.maskOn = MmAnim_Load(MM_ANIM_CL_SETMASK);
    sNativeMaskCs.maskLoop = MmAnim_Load(MM_ANIM_CL_SETMASKEND);
    sNativeMaskCs.maskOff = MmAnim_Load(MM_ANIM_CL_MASKOFF);
    const MmAnimId off = form == MM_PLAYER_FORM_GORON  ? MM_ANIM_PG_MASKOFFSTART
                         : form == MM_PLAYER_FORM_DEKU ? MM_ANIM_PN_MASKOFFSTART
                                                       : MM_ANIM_PZ_MASKOFFSTART;
    sNativeMaskCs.formOff = MmAnim_Load(off);
    if (!sNativeMaskCs.maskOn || !sNativeMaskCs.maskLoop || !sNativeMaskCs.maskOff || !sNativeMaskCs.formOff) {
        MMFORM_LOG("[MmForm] Native mask cutscene unavailable for form %d: using instant transition", form);
        sNativeMaskInstantFallback = 1;
        return 0;
    }
    static const char* masks[4] = {
        "objects/gameplay_keep/gFierceDeityMaskDL",
        "objects/gameplay_keep/gGoronMaskDL",
        "objects/gameplay_keep/gZoraMaskDL",
        "objects/gameplay_keep/gDekuMaskDL",
    };
    static const char* stretched[4] = {
        "objects/object_mask_boy/object_mask_boy_DL_000900",
        "objects/object_mask_goron/object_mask_goron_DL_0014A0",
        "objects/object_mask_zora/object_mask_zora_DL_000DB0",
        "objects/object_mask_nuts/object_mask_nuts_DL_001D90",
    };
    // Strict archive ownership prevents OoT's same-named gameplay_keep from
    // supplying another game's geometry or textures. Graphs retain their resources.
    sNativeMaskCs.maskDL = MmAssets_LoadDisplayListGraphStrict(masks[form]);
    sNativeMaskCs.stretchDL = MmAssets_LoadDisplayListGraphStrict(stretched[form]);
    sNativeMaskCs.veilDL = MmAssets_LoadDisplayListGraphStrict("objects/gameplay_keep/gameplay_keep_DL_054C90");
    if (!sNativeMaskCs.maskDL || !sNativeMaskCs.stretchDL || !sNativeMaskCs.veilDL) {
        MMFORM_LOG("[MmForm] Native mask meshes unavailable for form %d: using instant transition", form);
        sNativeMaskInstantFallback = 1;
        return 0;
    }
    sNativeMaskCs.active = 1;
    sNativeMaskInstantFallback = 0;
    sNativeMaskCs.owner = play;
    sNativeMaskCs.removing = removing;
    sNativeMaskCs.ownedFlags = (PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_IN_CUTSCENE) & ~player->stateFlags1;
    player->stateFlags1 |= PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_IN_CUTSCENE;
    // Mask C-input can leave bubble aim before its action gets to cancel it.
    // Exit both the custom camera and OoT slingshot pipeline before taking over.
    if (removing && form == MM_PLAYER_FORM_DEKU) {
        ItemCamera_Exit(&gFormState.bubbleCameraState, player, play);
        Player_DekuBubbleCleanup(player);
        gFormState.bubbleCharging = 0;
        gFormState.bubbleCharge = 0.0f;
        gFormState.bubbleChargeTimer = 0;
        gFormState.dekuCheekScale = 1.0f;
        MmSfx_Stop(MM_NA_SE_PL_DEKUNUTS_BUBLE_BREATH);
    }
    FirstPerson_Exit(player, play);
    // Player_UseItem only queues a sword-to-empty change, whose upper action
    // would be paused by this cutscene. Use OoT's synchronous init route so
    // custom held props, hookshots, hand models and actions all finish stowing.
    Player_DestroyHookshot(player);
    Player_DetachHeldActor(play, player);
    player->stateFlags1 &= ~(PLAYER_STATE1_START_CHANGING_HELD_ITEM | PLAYER_STATE1_SHIELDING);
    player->heldItemId = ITEM_NONE;
    player->nextModelGroup = Player_ActionToModelGroup(player, PLAYER_IA_NONE);
    Player_InitItemAction(play, player, PLAYER_IA_NONE);
    Player_SetUpperActionFunc(player, ExtPlayer_GetItemActionUpdateFunc(PLAYER_IA_NONE));
    player->idleType = PLAYER_IDLE_DEFAULT;
    player->meleeWeaponState = 0;
    player->meleeWeaponAnimation = -1;
    // A transformation replaces the currently worn mask, including its saved
    // scene memory; otherwise OoT's face pass would draw two masks together.
    player->currentMask = gSaveContext.ship.maskMemory = PLAYER_MASK_NONE;
    memcpy(sNativeMaskCs.ambient, play->envCtx.adjAmbientColor, sizeof(sNativeMaskCs.ambient));
    memcpy(sNativeMaskCs.light, play->envCtx.adjLight1Color, sizeof(sNativeMaskCs.light));
    memcpy(sNativeMaskCs.fog, play->envCtx.adjFogColor, sizeof(sNativeMaskCs.fog));
    sNativeMaskCs.fogNear = play->envCtx.adjFogNear;
    sNativeMaskCs.previousCamera = play->activeCamera;
    Camera* previous = GET_ACTIVE_CAM(play);
    OLib_Vec3fDiffToVecSphGeo(&sNativeMaskCs.camera.previousOffset, &previous->at, &previous->eye);
    sNativeMaskCs.camera.previousFov = previous->fov;
    sNativeMaskCs.camera.yaw = sNativeMaskCs.camera.previousOffset.r > 0.0f ? sNativeMaskCs.camera.previousOffset.yaw
                                                                            : Camera_GetCamDirYaw(previous) + 0x8000;
    player->actor.shape.rot.y = player->yaw = sNativeMaskCs.camera.yaw;
    sNativeMaskCs.cameraId = Play_CreateSubCamera(play);
    if (sNativeMaskCs.cameraId != SUBCAM_NONE) {
        Play_CameraSetAtEye(play, sNativeMaskCs.cameraId, &previous->at, &previous->eye);
        Play_CameraSetFov(play, sNativeMaskCs.cameraId, previous->fov);
        Play_SetCameraRoll(play, sNativeMaskCs.cameraId, 0);
        Play_ChangeCameraStatus(play, sNativeMaskCs.previousCamera, CAM_STAT_WAIT);
        Play_ChangeCameraStatus(play, sNativeMaskCs.cameraId, CAM_STAT_ACTIVE);
    }
    sNativeMaskCs.lightNode = LightContext_InsertLight(play, &play->lightCtx, &sNativeMaskCs.lightInfo);
    SkelAnime* skel = removing ? &gFormState.formSkelAnime : &player->skelAnime;
    LinkAnimationHeader* anim = removing ? sNativeMaskCs.formOff : sNativeMaskCs.maskOn;
    // MM's adjusted rate is 2/3; OoT advances Link animations by 1.5 per tick.
    // This produces one source frame per update, keeping animation/SFX in sync.
    LinkAnimation_Change(play, skel, anim, 2.0f / 3.0f, 0.0f, Animation_GetLastFrame(anim), ANIMMODE_ONCE, -6.0f);
    return 1;
}

static u8 MmForm_UpdateNativeMaskCutscene(Player* player, PlayState* play, u8 removing) {
    if (sNativeMaskInstantFallback)
        return 0;
    if (!sNativeMaskCs.active && !MmForm_BeginNativeMaskCutscene(player, play, removing))
        return 0;
    player->stateFlags1 |= PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_IN_CUTSCENE;
    player->stateFlags3 |= PLAYER_STATE3_PAUSE_ACTION_FUNC;
    player->linearVelocity = 0.0f;
    player->actor.velocity.y = 0.0f;
    player->headLimbRot = {};
    player->upperLimbRot = {};
    MmForm_UpdateNativeMaskCamera(player, play);
    if (gFormState.cutscenePhase == 0) {
        ++gFormState.cutsceneTimer;
        SkelAnime* skel = removing ? &gFormState.formSkelAnime : &player->skelAnime;
        const bool done = LinkAnimation_Update(play, skel);
        if (!removing && !sNativeMaskCs.looping) {
            static const struct {
                s16 frame;
                u16 sfx;
            } cues[] = {
                { 2, MM_NA_SE_PL_PUT_OUT_ITEM },     { 4, MM_NA_SE_IT_SET_TRANSFORM_MASK },
                { 11, MM_NA_SE_PL_FREEZE_S },        { 20, MM_NA_SE_IT_TRANSFORM_MASK_BROKEN },
                { 30, MM_NA_SE_PL_TRANSFORM_VOICE },
            };
            for (const auto& cue : cues) {
                if (LinkAnimation_OnFrame(skel, cue.frame))
                    MmSfx_PlayAtPos(cue.sfx, &player->actor.projectedPos);
            }
            if (done) {
                LinkAnimation_Change(play, skel, sNativeMaskCs.maskLoop, 2.0f / 3.0f, 0,
                                     Animation_GetLastFrame(sNativeMaskCs.maskLoop), ANIMMODE_LOOP, 0);
                sNativeMaskCs.looping = 1;
            }
        } else if (removing) {
            if (gFormState.cutsceneTimer == 8)
                MmSfx_PlayAtPos(MM_NA_SE_IT_SET_TRANSFORM_MASK, &player->actor.projectedPos);
            if (gFormState.cutsceneTimer == 15)
                MmSfx_PlayAtPos(MM_NA_SE_PL_FACE_CHANGE, &player->actor.projectedPos);
        }
        const s16 timer = gFormState.cutsceneTimer;
        if (timer >= (removing ? 20 : 58)) {
            sNativeMaskCs.faceAlpha = MIN(255, sNativeMaskCs.faceAlpha + (removing ? 20 : 50));
        }
        if (!removing && timer == 59)
            MmSfx_PlayAtPos(MM_NA_SE_EV_LIGHTNING_HARD, NULL);
        if (!removing) {
            Math_StepToF(&sNativeMaskCs.maskDepth,
                         timer >= 64   ? 0.0f
                         : timer >= 14 ? 0.3f
                                       : 0.0f,
                         timer >= 64 ? 0.015f : 0.3f);
            Math_StepToF(&sNativeMaskCs.maskWidth,
                         timer > 65    ? 0.0f
                         : timer >= 16 ? -0.1f
                                       : 0.0f,
                         timer > 65 ? 0.02f : 0.1f);
        }
        const bool skip = timer >= 5 && CHECK_BTN_ANY(play->state.input[0].press.button,
                                                      BTN_A | BTN_B | BTN_CUP | BTN_CDOWN | BTN_CLEFT | BTN_CRIGHT);
        if (timer >= (removing ? 57 : 85) || skip) {
            if (skip) {
                sNativeMaskCs.camera.state = 999;
                MmForm_StopNativeMaskSfx(false);
            }
            gFormState.cutscenePhase = 1;
            MmSfx_PlayTransformFlash();
        }
    } else if (gFormState.cutscenePhase == 1) {
        gFormState.flashAlpha += 45;
        if (gFormState.flashAlpha >= 255) {
            gFormState.flashAlpha = 255;
            if (removing) {
                gFormState.currentForm = MM_PLAYER_FORM_HUMAN;
                gFormState.skeletonLoaded = 0;
                MmForm_RestoreOotState(player);
                LinkAnimation_Change(play, &player->skelAnime, sNativeMaskCs.maskOn, -2.0f / 3.0f, 9.0f, 0.0f,
                                     ANIMMODE_ONCE, 0.0f);
            } else {
                if (!MmForm_LoadFormSkeleton(play, gFormState.targetForm)) {
                    MmForm_EndNativeMaskCutscene(player, play);
                    gFormState.flashAlpha = 0;
                    gFormState.state = MMFORM_STATE_INACTIVE;
                    MmForm_RestoreEquips(play);
                    return 1;
                }
                ExtEquip_UnequipForTransform();
                gFormState.currentForm = gFormState.targetForm;
                MmForm_ApplyFormProperties(player, gFormState.currentForm);
                Player_SetBootData(play, player);
                LinkAnimation_Change(play, &gFormState.formSkelAnime, sNativeMaskCs.maskOff, 2.0f / 3.0f, 0.0f,
                                     Animation_GetLastFrame(sNativeMaskCs.maskOff), ANIMMODE_ONCE, 0.0f);
            }
            gFormState.cutscenePhase = 2;
            sNativeMaskCs.faceAlpha = 0;
        }
    } else {
        ++sNativeMaskCs.recoveryTimer;
        gFormState.flashAlpha = MAX(0, gFormState.flashAlpha - 45);
        SkelAnime* skel = removing ? &player->skelAnime : &gFormState.formSkelAnime;
        const bool done = LinkAnimation_Update(play, skel);
        if (!removing && sNativeMaskCs.recoveryTimer == 5) {
            LinkAnimation_Change(play, skel, gFormState.idleAnim, 1.0f, 0.0f, 20.0f, ANIMMODE_ONCE, 20.0f);
            sNativeMaskCs.recovering = 1;
        }
        const bool moving = sNativeMaskCs.recoveryTimer > 10 &&
                            (ABS(play->state.input[0].cur.stick_x) > 20 || ABS(play->state.input[0].cur.stick_y) > 20);
        if (gFormState.flashAlpha == 0 && (done || moving) && (removing || sNativeMaskCs.recovering)) {
            MmForm_EndNativeMaskCutscene(player, play, false);
            sForceInstantTransform = 0;
            if (removing) {
                gFormState.state = MMFORM_STATE_INACTIVE;
                MmForm_RestoreEquips(play);
                ExtEquip_RestoreFromTransform();
            } else {
                LinkAnimation_PlayLoop(play, &gFormState.formSkelAnime, gFormState.idleAnim);
                LinkAnimation_PlayLoop(play, &player->skelAnime, gFormState.idleAnim);
                gFormState.state = MMFORM_STATE_ACTIVE;
                MmForm_SaveAndRestrictEquips(play);
                if (MMFORM_IS_ZORA_SWIM() && player->actor.yDistToWater > ZORA_SWIM_THRESHOLD) {
                    MmForm_EnterSwimIdle(player, play);
                    player->stateFlags1 |= PLAYER_STATE1_IN_WATER;
                }
            }
            return 1;
        }
    }
    MmForm_UpdateNativeMaskLighting(player, play);
    return 1;
}

struct MmNativeMaskAttachment {
    Gfx* dl;
    u8 hand;
};

static MmNativeMaskAttachment MmForm_NativeMaskAttachment(Player* player, s32 limbIndex) {
    if (!sNativeMaskCs.active)
        return {};
    if (gFormState.cutscenePhase == 2) {
        const SkelAnime* skel = sNativeMaskCs.removing ? &player->skelAnime : &gFormState.formSkelAnime;
        if (limbIndex == PLAYER_LIMB_L_HAND &&
            (sNativeMaskCs.removing ? skel->curFrame >= 8.0f : sNativeMaskCs.recoveryTimer < 5))
            return { sNativeMaskCs.maskDL, 1 };
        return {};
    }
    if (sNativeMaskCs.removing)
        return {};
    const f32 frame = player->skelAnime.curFrame;
    const bool head = sNativeMaskCs.looping || frame >= 12.0f;
    if (head ? limbIndex != PLAYER_LIMB_HEAD : limbIndex != PLAYER_LIMB_L_HAND || frame < 8.0f)
        return {};
    Gfx* dl = head && (sNativeMaskCs.looping || frame >= 51.0f) && sNativeMaskCs.stretchDL ? sNativeMaskCs.stretchDL
                                                                                           : sNativeMaskCs.maskDL;
    return { dl, (u8)!head };
}

extern "C" u8 MmForm_IsNativeTransformationPlaying(PlayState* play, Player* player) {
    return sNativeMaskCs.active && sNativeMaskCs.owner == play && player == GET_PLAYER(play);
}

// Both skeleton renderers call this in the actual limb's matrix. Masks move
// from the left hand (frames 8..11) to the face, then use MM's stretched mesh.
extern "C" void MmForm_DrawNativeTransformationLimb(PlayState* play, Player* player, s32 limbIndex) {
    if (!MmForm_IsNativeTransformationPlaying(play, player))
        return;
    const auto mask = MmForm_NativeMaskAttachment(player, limbIndex);
    const bool veil = limbIndex == PLAYER_LIMB_HEAD && sNativeMaskCs.faceAlpha && sNativeMaskCs.veilDL &&
                      gFormState.currentForm >= MM_PLAYER_FORM_FIERCE_DEITY &&
                      gFormState.currentForm <= MM_PLAYER_FORM_HUMAN;
    if (!mask.dl && !veil)
        return;
    OPEN_DISPS(play->state.gfxCtx);
    MmAssets_EnsureStrictTextureBindings();
    if (mask.dl) {
        Matrix_Push();
        if (!mask.hand) {
            Matrix_Scale(1.0f, 1.0f - sNativeMaskCs.maskWidth, 1.0f - sNativeMaskCs.maskDepth, MTXMODE_APPLY);
        } else {
            Matrix_Translate(-323.67f, 412.15f, -969.96f, MTXMODE_APPLY);
            Matrix_RotateZYX(-0x32BE, -0x50DE, -0x7717, MTXMODE_APPLY);
        }
        gSPMatrix(POLY_OPA_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gSPDisplayList(POLY_OPA_DISP++, mask.dl);
        Matrix_Pop();
    }
    if (veil) {
        // MM gameplay_keep_Matanimheader_054F18: segment -4 => 0x0B,
        // dual 16x16 scrolls (-1,0) and (1,-2); Y is negated by AnimatedMat.
        static const Vec3f offsets[5] = {
            { 0, 0, 0 }, { -578.3f, -1100.9f, 0 }, { -189.5f, -594.87f, 0 }, { -570, -812, 0 }, { -230, -520, 0 }
        };
        const s32 form = (s32)gFormState.currentForm;
        const Vec3f& offset = offsets[form];
        Matrix_Push();
        Matrix_Translate(offset.x, offset.y, 0.0f, MTXMODE_APPLY);
        if (form == MM_PLAYER_FORM_ZORA)
            Matrix_Scale(0.7f, 0.7f, 0.7f, MTXMODE_APPLY);
        gSPSegment(POLY_XLU_DISP++, 0x0B,
                   (uintptr_t)Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, -(s32)play->gameplayFrames, 0, 16, 16, 1,
                                                 play->gameplayFrames, play->gameplayFrames * 2, 16, 16, -1, 0, 1, 2));
        gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
                  G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
        gDPSetEnvColor(POLY_XLU_DISP++, 0, 0, 255, (u8)sNativeMaskCs.faceAlpha);
        gSPDisplayList(POLY_XLU_DISP++, sNativeMaskCs.veilDL);
        Matrix_Pop();
    }
    CLOSE_DISPS(play->state.gfxCtx);
}

extern "C" void MmForm_DrawNativeTransformationPostProcess(PlayState* play) {
    if (!sNativeMaskCs.active || sNativeMaskCs.owner != play || gFormState.cutsceneTimer < 20 ||
        gFormState.cutscenePhase >= 2)
        return;
    // Same post-world ordering, alpha and framebuffer helpers as MM's native
    // Play_DrawMotionBlur. Capture before blending and restart after resize.
    const u32 width = OTRGetGameRenderWidth(), height = OTRGetGameRenderHeight();
    const bool ready =
        sNativeMaskCs.blurReady && sNativeMaskCs.blurWidth == width && sNativeMaskCs.blurHeight == height;
    FB_CreateFramebuffers();
    OPEN_DISPS(play->state.gfxCtx);
    Gfx* head = POLY_OPA_DISP;
    Gfx* gfx = Graph_GfxPlusOne(head);
    gSPDisplayList(OVERLAY_DISP++, gfx);
    if (ready)
        FB_DrawFromFramebuffer(&gfx, gBlurFrameBuffer, 100);
    if (!sNativeMaskCs.blurReady || sNativeMaskCs.blurFrame != play->gameplayFrames)
        sNativeMaskCs.blurCopied = 0;
    FB_CopyToFramebuffer(&gfx, 0, gBlurFrameBuffer, true, &sNativeMaskCs.blurCopied);
    gSPEndDisplayList(gfx++);
    Graph_BranchDlist(head, gfx);
    POLY_OPA_DISP = gfx;
    CLOSE_DISPS(play->state.gfxCtx);
    sNativeMaskCs.blurReady = 1;
    sNativeMaskCs.blurFrame = play->gameplayFrames;
    sNativeMaskCs.blurWidth = width;
    sNativeMaskCs.blurHeight = height;
}
