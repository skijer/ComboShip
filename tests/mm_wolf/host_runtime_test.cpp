#define main wolf_core_fixture_main
#include "core_runtime_test.cpp"
#undef main
#include "mods/ext_buttons/ext_buttons.h"
#include "mods/extended_inventory.h"
#include "mods/nei_save.h"
#include "mods/forms/custom_forms.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

static GameInteractor fixtureInteractor;
GameInteractor* GameInteractor::Instance = &fixtureInteractor;

static s32 otherForm = CUSTOM_FORM_NONE;
static u8 pendant, beetle, kite, trident, mario, customItemBlock, pakModel, pakBodyModel, o2rModel;
static unsigned nativeActions, nativeDamageTicks, iceBursts, thawCompletions;
static bool nativeAnimationDone;
static u16 hookSuppressedButtons;
static unsigned drawContinuations;
static u8 ironBoots;
struct FloorWalkRequest {
    Vec3f* pos;
    u16 id;
    f32 speed;
};
static std::vector<FloorWalkRequest> floorWalkRequests;
static unsigned climbContacts;
static std::vector<u16> giantVoices;
extern "C" {
void Play_Init(GameState*) {}
void TitleSetup_Init(GameState*) {}
void WolfFixture_DrawNativePrefix(PlayState*, Player*);
void Player_AnimSfx_PlayFloorWalk(Player*, f32);
void Player_AnimSfx_PlayFloorJump(Player*);
void Player_AnimSfx_PlayFloorLand(Player*);
void Player_AnimSfx_PlayVoice(Player*, u16);
void Player_Action_49(Player*, PlayState*);
void Audio_PlaySfx_GiantsMask(Vec3f*, u16 id) {
    giantVoices.push_back(id);
}
s32 PlayerAnimation_OnFrame(SkelAnime* animation, f32 frame) {
    return animation->curFrame == frame;
}
void Player_AnimReplace_SetupLedgeClimb(Player*, s32) {
}
void func_80839E74(Player*, PlayState*) {
}
void func_8084C124(PlayState*, Player*) {
    ++climbContacts;
}
u8 VanillaTB_IsIronBoots(void) {
    return ironBoots;
}
void Audio_PlaySfx_AtPosForMetalEffectsWithSyncedFreqAndVolume(Vec3f* pos, u16 id, f32 speed) {
    floorWalkRequests.push_back({ pos, id, speed });
}
void Matrix_Push(void) {}
void Matrix_Pop(void) {}
Mtx* Matrix_Finalize(GraphicsContext* gfx) {
    return (Mtx*)Graph_Alloc(gfx, sizeof(Mtx));
}
void Player_DrawGetItem(PlayState*, Player*) { ++drawContinuations; }
void func_80122D44(PlayState* play, struct_80122D44_arg1*) {
    ++drawContinuations;
    OPEN_DISPS(play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}
s32 CustomItems_OverrideDraw(Player*, PlayState*) { ++drawContinuations; return 0; }
void ExtEquip_DrawBehavior(void*, void*) { ++drawContinuations; }
#include "wolf-native-scroll-gbi.inc"
Input* sPlayerControlInput;
f32 sControlStickMagnitude;
s16 sControlStickAngle;
NeiSaveData* Nei_Save(void) {
    return &gSaveContext.save.shipSaveInfo.nei;
}
s32 CustomForms_ActiveForm(void) {
    return otherForm;
}
u8 ExtEquip_PendantActive(void) {
    return pendant;
}
u8 Beetle_IsFlying(void) {
    return beetle;
}
u8 KiteSurf_IsActive(void) {
    return kite;
}
u8 Trident_OwnsPlayerAction(void) {
    return trident;
}
u8 Sm64Mario_IsActive(void) {
    return mario;
}
s32 CustomItems_BlocksMovement(Player*) {
    return customItemBlock;
}
u8 PakLoader_HasActiveModel(void) {
    return pakModel;
}
u8 PakLoader_HasActiveBodyModel(void) {
    return pakBodyModel;
}
u8 O2rLoader_HasActiveModel(void) {
    return o2rModel;
}
void Player_Action_20(Player*, PlayState*) {
    ++nativeActions;
}
void Player_Action_21(Player*, PlayState*) {
    ++nativeActions;
}
void Player_Action_83(Player*, PlayState*) {
    ++nativeActions;
}
void NativeIdle(Player*, PlayState*) {
    ++nativeActions;
}
s32 Player_InflictDamage(PlayState*, s32 damage) {
    ++nativeDamageTicks;
    gSaveContext.save.saveInfo.playerData.health += damage;
    return 1;
}
void func_80836988(Player* player, PlayState*) {
    ++thawCompletions;
    player->actionFunc = NativeIdle;
}
void func_808339B4(Player* player, s32 timer) {
    player->invincibilityTimer = timer;
}
void func_80834104(PlayState*, Player*) {
    assert(false);
}
s32 PlayerAnimation_Update(PlayState*, SkelAnime*) {
    return nativeAnimationDone;
}
void EffectSsIcePiece_SpawnBurst(PlayState*, Vec3f*, f32) {
    ++iceBursts;
}
void Player_Action_82(Player*, PlayState*);
void WolfFixture_SelectNativeInput(PlayState*, Player*, Input*);
void GameInteractor_ExecuteOnPassPlayerInputs(Input* input) {
    input->cur.button &= ~hookSuppressedButtons;
    input->press.button &= ~hookSuppressedButtons;
}
}
#ifdef MM_WOLF_HOST
#include "mods/forms/wolf_link_host.cpp"
#else
static void WolfLinkHost_PreUpdate(PlayState*, Player*) {
}
static void WolfLinkHost_FilterInput(Player*, Input*) {
}
static void WolfLinkHost_BeforeAction(PlayState*, Player*, Input*) {
}
static u8 WolfLinkHost_Draw(PlayState*, Player*) {
    return 0;
}
static void WolfLinkHost_Destroy(PlayState*, Player*) {
}
#endif

int main(int argc, char** argv) {
    assert(argc == 2);
    assetDirectory = argv[1];
    captureWolfLogs();
    std::memset(&gSaveContext, 0, sizeof(gSaveContext));
    std::memset(gSaveContext.save.saveInfo.equips.buttonItems, ITEM_NONE,
                sizeof(gSaveContext.save.saveInfo.equips.buttonItems));
    gSaveContext.save.saveInfo.playerData.health = 48;
    integerCvars["gMods.TransformMasks.Enabled"] = 0; // Native MM Wolf does not depend on OoT's master gate.
    ShipInit::InitAll();
    Nei_Save()->ownedItems[SLOT_SHADOW_CRYSTAL - 24] = EXT_ITEM_SHADOW_CRYSTAL;
    PlayState play{};
    Player player{};
    PlayerAgeProperties nativeAge{};
    nativeAge.voiceSfxIdOffset = SFX_VOICE_BANK_SIZE;
    player.ageProperties = &nativeAge;
    Camera camera{};
    GraphicsContext gfx{};
    Gfx commands[128]{};
    play.state.gfxCtx = &gfx;
    play.state.running = true;
    play.state.init = Play_Init;
    play.cameraPtrs[0] = &camera;
    gPlayState = &play;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    player.transformation = PLAYER_FORM_HUMAN;
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    player.cylinder.dim.radius = 12;
    player.cylinder.dim.height = 50;
    player.actor.shape.shadowDraw = ActorShadow_DrawFeet;
    R_UPDATE_RATE = 3;
    ExtButton_SetItem(0, EQUIP_SLOT_C_LEFT, EXT_ITEM_SHADOW_CRYSTAL);
    assert(load(makeAsset()));
    sAssetsLoaded = 0;
    bool installFreezeBeforeAction = false;
    auto clearFootsteps = [&] {
        fixturePlayerSfx.clear();
        floorWalkRequests.clear();
    };
    auto nativeFootstep = [&](u16 floor, f32 speed = 6) {
        clearFootsteps();
        player.floorSfxOffset = floor;
        Player_AnimSfx_PlayFloorWalk(&player, speed);
    };
    auto frame = [&](u16 buttons = 0, u16 held = 0, s8 stick = 0) {
        play.state.input[0] = {};
        play.state.input[0].press.button = buttons;
        play.state.input[0].cur.button = held;
        play.state.input[0].rel.stick_y = stick;
        play.state.input[0].cur.stick_y = stick;
        WolfLinkHost_PreUpdate(&play, &player);
        Input filtered{};
        WolfFixture_SelectNativeInput(&play, &player, &filtered);
        WolfLinkHost_FilterInput(&player, &filtered);
        if (installFreezeBeforeAction) {
            player.actionFunc = Player_Action_82;
            player.av1.actionVar1 = 6;
            player.av2.actionVar2 = 0;
            installFreezeBeforeAction = false;
        }
        sPlayerControlInput = &filtered;
        WolfLinkHost_BeforeAction(&play, &player, &filtered);
        if (!(player.stateFlags3 & PLAYER_STATE3_4) && player.actionFunc)
            player.actionFunc(&player, &play);
        ++play.gameplayFrames;
        return filtered;
    };
    nativeFootstep(NA_SE_PL_WALK_CONCRETE - SFX_FLAG, 2);
    assert(fixturePlayerSfx.empty() && floorWalkRequests.size() == 1 &&
           floorWalkRequests.back().id == NA_SE_PL_WALK_CONCRETE && floorWalkRequests.back().speed == 2 &&
           floorWalkRequests.back().pos == &player.actor.projectedPos);
    Player_AnimSfx_PlayVoice(&player, NA_SE_VO_LI_CLIMB_END);
    assert(fixturePlayerSfx.back() == NA_SE_VO_LI_CLIMB_END + nativeAge.voiceSfxIdOffset);
    pakModel = 1; // Equipment-only PAK activity must not own Link's body.
    frame(BTN_CLEFT);
    pakModel = 0;
    if (!WolfLinkForm_IsSelected() || !WolfLinkForm_IsReady()) {
        std::fputs("FAIL full-width C-button Shadow Crystal did not activate the production MM Wolf runtime\n", stderr);
        return 1;
    }
    assert(logged("result=activated"));
    clearFootsteps();
    player.skelAnime.endFrame = 60;
    player.skelAnime.curFrame = 26;
    Player_Action_49(&player, &play);
    assert(sWolfAudio.voices[1].clip &&
           std::strcmp(sWolfAudio.voices[1].clip->source, "TP_WolfLink_Huff_A1.wav") == 0 &&
           "native ledge-climb voice contact must emit supplied Wolf exertion "
           "rather than young Link");
    assert(fixturePlayerSfx.size() == 1 && fixturePlayerSfx.back() == NA_SE_PL_CLIMB_CLIFF && climbContacts == 1 &&
           !WolfLinkForm_OwnsPlayerAction() && sWolfAudio.voices[1].gain == 0.8f);
    for (u16 voice : { NA_SE_VO_LI_AUTO_JUMP, NA_SE_VO_LI_HANG, NA_SE_VO_LI_FALL_S, NA_SE_VO_LI_LAND_DAMAGE_S }) {
        fixturePlayerSfx.clear();
        Player_AnimSfx_PlayVoice(&player, voice);
        assert(fixturePlayerSfx.size() == 1 && fixturePlayerSfx.back() == voice + nativeAge.voiceSfxIdOffset);
    }
    Player otherActor = player;
    otherActor.actor.id = ACTOR_EN_WF;
    fixturePlayerSfx.clear();
    Player_AnimSfx_PlayVoice(&otherActor, NA_SE_VO_LI_CLIMB_END);
    assert(fixturePlayerSfx.empty() && "non-player actors must not acquire Wolf's exertion voice");
    floorWalkRequests.clear();
    otherActor.floorSfxOffset = NA_SE_PL_WALK_CONCRETE - SFX_FLAG;
    Player_AnimSfx_PlayFloorWalk(&otherActor, 2);
    assert(floorWalkRequests.size() == 1 && floorWalkRequests.back().id == NA_SE_PL_WALK_CONCRETE);
    player.transformation = PLAYER_FORM_ZORA;
    fixturePlayerSfx.clear();
    Player_AnimSfx_PlayVoice(&player, NA_SE_VO_LI_CLIMB_END);
    assert(fixturePlayerSfx.size() == 1 &&
           fixturePlayerSfx.back() == NA_SE_VO_LI_CLIMB_END + nativeAge.voiceSfxIdOffset);
    player.transformation = PLAYER_FORM_HUMAN;
    for (u16 surface : { NA_SE_PL_WALK_GROUND, NA_SE_PL_WALK_SAND, NA_SE_PL_WALK_CONCRETE, NA_SE_PL_WALK_DIRT,
                         NA_SE_PL_WALK_GRASS, NA_SE_PL_WALK_SNOW, NA_SE_PL_WALK_ICE }) {
        nativeFootstep(surface - SFX_FLAG);
        assert(floorWalkRequests.empty() && fixturePlayerSfx.size() == 1 &&
               fixturePlayerSfx.back() == NA_SE_EN_WOLFOS_WALK &&
               "native dry walk contact must emit the complete Wolfos paw sample "
               "without human equipment layers");
    }
    ironBoots = 1;
    nativeFootstep(NA_SE_PL_WALK_GROUND - SFX_FLAG);
    assert(floorWalkRequests.empty() && fixturePlayerSfx.size() == 1 &&
           fixturePlayerSfx.back() == NA_SE_EN_WOLFOS_WALK);
    ironBoots = 0;
    for (u16 surface : { NA_SE_PL_WALK_WATER0, NA_SE_PL_WALK_WATER1, NA_SE_PL_WALK_WATER2, NA_SE_PL_WALK_MAGMA,
                         NA_SE_PL_WALK_IRON, NA_SE_PL_WALK_GLASS, NA_SE_PL_WALK_METAL1 }) {
        nativeFootstep(surface - SFX_FLAG, 4);
        assert(fixturePlayerSfx.empty() && floorWalkRequests.size() == 1 && floorWalkRequests.back().id == surface &&
               floorWalkRequests.back().speed == 4);
    }
    player.actor.bgCheckFlags = 0;
    nativeFootstep(NA_SE_PL_WALK_GROUND - SFX_FLAG);
    assert(fixturePlayerSfx.empty() && floorWalkRequests.empty() && "airborne Wolf cannot emit a dry paw contact");
    for (u16 surface : { NA_SE_PL_WALK_WATER0, NA_SE_PL_WALK_METAL1 }) {
        nativeFootstep(surface - SFX_FLAG, 3);
        assert(fixturePlayerSfx.empty() && floorWalkRequests.size() == 1 && floorWalkRequests.back().id == surface &&
               floorWalkRequests.back().speed == 3 &&
               "Wolf routing must preserve native non-dry contact dispatch even "
               "without a ground flag");
    }
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    clearFootsteps();
    player.floorSfxOffset = NA_SE_PL_WALK_WATER0 - SFX_FLAG;
    Player_AnimSfx_PlayFloorJump(&player);
    Player_AnimSfx_PlayFloorLand(&player);
    assert(fixturePlayerSfx.size() == 2 && fixturePlayerSfx[0] == NA_SE_PL_JUMP_GROUND + player.floorSfxOffset &&
           fixturePlayerSfx[1] == NA_SE_PL_LAND_GROUND + player.floorSfxOffset);
    player.currentMask = PLAYER_MASK_GIANT;
    nativeFootstep(0);
    assert(fixturePlayerSfx.empty() && floorWalkRequests.size() == 1 &&
           floorWalkRequests.back().id == NA_SE_PL_GIANT_WALK);
    Player_AnimSfx_PlayVoice(&player, NA_SE_VO_LI_CLIMB_END);
    assert(giantVoices.size() == 1 && giantVoices.back() == NA_SE_VO_LI_CLIMB_END);
    player.currentMask = PLAYER_MASK_SCENTS;
    fixturePlayerSfx.clear();
    Player_AnimSfx_PlayVoice(&player, NA_SE_VO_LI_CLIMB_END);
    assert(fixturePlayerSfx.size() == 1 && fixturePlayerSfx.back() == NA_SE_VO_LI_CLIMB_END + SFX_VOICE_BANK_SIZE * 7);
    player.currentMask = PLAYER_MASK_NONE;
    integerCvars["gMods.WolfLink.Enabled"] = 0;
    nativeFootstep(0);
    assert(fixturePlayerSfx.empty() && floorWalkRequests.size() == 1 &&
           floorWalkRequests.back().id == NA_SE_PL_WALK_GROUND);
    integerCvars["gMods.WolfLink.Enabled"] = 1;
    player.stateFlags1 = PLAYER_STATE1_20;
    auto blocked = frame(BTN_B, BTN_R);
    if (sWolf.procOwnsPlayer || (player.stateFlags3 & PLAYER_STATE3_4)) {
        std::fputs("FAIL Wolf ignored native disabled-input/remote-tool ownership\n", stderr);
        return 1;
    }
    assert((play.state.input[0].press.button & BTN_B) && (play.state.input[0].cur.button & BTN_R));
    WolfLinkHost_Destroy(&play, &player);
    frame(BTN_CLEFT);
    assert(!WolfLinkForm_IsReady());
    assert(logged("result=native-input-state"));
    player.stateFlags1 = 0;
    frame(BTN_CLEFT);
    assert(WolfLinkForm_IsReady());
    play.actorCtx.isOverrideInputOn = true;
    play.actorCtx.overrideInput = {};
    frame(BTN_B);
    assert(!sWolf.procOwnsPlayer && !(player.stateFlags3 & PLAYER_STATE3_4));
    frame(BTN_CLEFT);
    assert(WolfLinkForm_IsReady() && WolfLinkForm_IsSelected() &&
           "native override input suspends actions without removing the selected presentation");
    nativeFootstep(0);
    assert(floorWalkRequests.empty() && fixturePlayerSfx.size() == 1 &&
           fixturePlayerSfx.back() == NA_SE_EN_WOLFOS_WALK && !WolfLinkForm_OwnsPlayerAction() &&
           "native override walking must match the retained Wolf presentation");
    Player_AnimSfx_PlayVoice(&player, NA_SE_VO_LI_CLIMB_END);
    assert(sWolfAudio.voices[1].clip &&
           std::strcmp(sWolfAudio.voices[1].clip->source, "TP_WolfLink_Huff_A2.wav") == 0 &&
           !WolfLinkForm_OwnsPlayerAction() && "native-led climb voice must match suspended Wolf presentation");
    play.actorCtx.isOverrideInputOn = false;
    frame();
    assert(WolfLinkForm_IsReady());
    player.textboxBtnCooldownTimer = 3;
    frame(BTN_B | BTN_A);
    assert(!sWolf.procOwnsPlayer && !(player.stateFlags3 & PLAYER_STATE3_4));
    player.textboxBtnCooldownTimer = 0;
    hookSuppressedButtons = BTN_B | BTN_A;
    frame(BTN_B | BTN_A);
    assert(!sWolf.procOwnsPlayer && !(player.stateFlags3 & PLAYER_STATE3_4));
    hookSuppressedButtons = BTN_CLEFT;
    frame(BTN_CLEFT);
    assert(WolfLinkForm_IsReady());
    assert(logged("result=input-suppressed"));
    hookSuppressedButtons = 0;
    auto filtered = frame(BTN_B, BTN_R);
    assert(!(filtered.press.button & BTN_B) && !(filtered.cur.button & BTN_R));
    assert(sWolf.procOwnsPlayer && sWolf.proc == PROC_WOLF_WAIT_ATTACK);
    player.stateFlags1 = PLAYER_STATE1_20000000;
    player.actionFunc = NativeIdle;
    const auto blockedNativeActions = nativeActions;
    frame(BTN_B);
    assert(!sWolf.procOwnsPlayer && !(player.stateFlags3 & PLAYER_STATE3_4) &&
           nativeActions == blockedNativeActions + 1);
    WolfLinkHost_Destroy(&play, &player);
    frame(BTN_CLEFT);
    assert(!WolfLinkForm_IsReady());
    player.stateFlags1 = 0;
    player.actionFunc = nullptr;
    frame(BTN_CLEFT);
    frame(BTN_B, BTN_R);
    assert(sWolf.procOwnsPlayer && sWolf.proc == PROC_WOLF_WAIT_ATTACK);
    // Native collision-shape fitting cannot overwrite Wolf's low body.
#ifdef MM_WOLF_HOST
    player.cylinder.dim.height = 80;
    WolfLinkHost_ApplyCollisionShape(&player);
    assert(player.cylinder.dim.height == 30);
#endif
    // Knockback/electricity pointers remain native; fixture boundary bodies count dispatches.
    for (auto action : { Player_Action_20, Player_Action_21, Player_Action_83 }) {
        player.actionFunc = action;
        player.stateFlags1 = 0;
        player.invincibilityTimer = 0;
        const auto before = nativeActions;
        const auto health = gSaveContext.save.saveInfo.playerData.health;
        frame(BTN_A | BTN_B, 0, 60);
        assert(nativeActions == before + 1 && player.actionFunc == action);
        assert(!sWolf.procOwnsPlayer && !sWolf.atActive && !(player.stateFlags3 & PLAYER_STATE3_4));
        assert(gSaveContext.save.saveInfo.playerData.health == health);
    }
    // A hit may install freeze after filtering this frame's pad. Only permitted mash
    // buttons are recovered from Wolf's reservation; an input hook can still suppress them.
    player.actionFunc = nullptr;
    installFreezeBeforeAction = true;
    frame(BTN_A | BTN_B);
    assert(player.av2.actionVar2 == 6 && !sWolf.procOwnsPlayer && !(player.stateFlags3 & PLAYER_STATE3_4));
    player.actionFunc = nullptr;
    hookSuppressedButtons = BTN_A | BTN_B;
    installFreezeBeforeAction = true;
    frame(BTN_A | BTN_B);
    assert(player.av2.actionVar2 == 1 && !sWolf.procOwnsPlayer && !(player.stateFlags3 & PLAYER_STATE3_4));
    hookSuppressedButtons = 0;
    // Actual native freeze action + actual mash helper are extracted unchanged from z_player.c.
    // They must tick periodic native damage once, thaw on A/B mashing, and finish their animation.
    player.actionFunc = Player_Action_82;
    player.av1.actionVar1 = 6;
    player.av2.actionVar2 = 0;
    const auto damageBefore = nativeDamageTicks;
    const auto healthBefore = gSaveContext.save.saveInfo.playerData.health;
    play.gameplayFrames = 40;
    for (int i = 0; i < 4; ++i)
        frame();
    assert(nativeDamageTicks == damageBefore + 1 && gSaveContext.save.saveInfo.playerData.health == healthBefore - 1);
    unsigned freezeFrames = 0;
    while (player.av1.actionVar1 >= 0 && freezeFrames++ < 20) {
        filtered = frame(BTN_A | BTN_B);
        assert((filtered.press.button & (BTN_A | BTN_B)) == (BTN_A | BTN_B));
        assert(!sWolf.procOwnsPlayer && !sWolf.atActive && !(player.stateFlags3 & PLAYER_STATE3_4));
    }
    assert(player.av1.actionVar1 == -1 && iceBursts == 1);
    const auto thawHealth = gSaveContext.save.saveInfo.playerData.health;
    for (int i = 0; i < 3; ++i) {
        frame(BTN_B);
        assert(player.actionFunc == Player_Action_82 && !sWolf.procOwnsPlayer);
    }
    assert(gSaveContext.save.saveInfo.playerData.health == thawHealth && thawCompletions == 0);
    nativeAnimationDone = true;
    frame(BTN_B);
    assert(player.actionFunc == NativeIdle && thawCompletions == 1 && player.invincibilityTimer == 20 &&
           !sWolf.procOwnsPlayer);
    nativeAnimationDone = false;
    player.actionFunc = nullptr;
    Actor npc{};
    player.talkActor = &npc;
    player.stateFlags2 |= PLAYER_STATE2_CAN_ACCEPT_TALK_OFFER;
    filtered = frame(BTN_A, 0, 60);
    assert(filtered.press.button & BTN_A);
    assert(!sWolf.procOwnsPlayer);
    player.talkActor = nullptr;
    player.stateFlags2 = 0;
    Gfx xluCommands[128]{}, overlayCommands[128]{};
    gfx.polyOpa.p = commands; gfx.polyOpa.d = commands;
    gfx.polyXlu.p = xluCommands; gfx.polyXlu.d = xluCommands;
    gfx.overlay.p = overlayCommands; gfx.overlay.d = overlayCommands;
    const auto oldBuffer = sSkin.bufIndex;
    assert(WolfLinkHost_Draw(&play, &player)==2 && "selected Wolf with exhausted arenas must skip the complete player draw");
    WolfFixture_DrawNativePrefix(&play, &player);
    assert(drawContinuations == 0 && gfx.overlay.p == gfx.overlay.d);
    assert(sSkin.bufIndex==oldBuffer && gfx.polyOpa.p==gfx.polyOpa.d && gfx.polyXlu.p==gfx.polyXlu.d);
    assert(WolfLinkForm_IsReady());
    gfx.polyOpa.d=commands+128;
    gfx.polyXlu.d=xluCommands+128;
    gfx.overlay.d=overlayCommands+128;
    player.stateFlags2 |= PLAYER_STATE2_4000;
    gfx.polyOpa.d=commands+20;
    assert(WolfLinkHost_Draw(&play, &player)==2 && "freeze scroll, matrix and native continuation must be reserved before skin submission");
    WolfFixture_DrawNativePrefix(&play, &player);
    assert(drawContinuations == 0 && gfx.polyOpa.p == commands);
    assert(sSkin.bufIndex==oldBuffer);
    gfx.polyOpa.d=commands+128;
    WolfFixture_DrawNativePrefix(&play, &player);
    assert(drawContinuations == 3 && gfx.polyOpa.p <= gfx.polyOpa.d && gfx.polyXlu.p <= gfx.polyXlu.d);
    player.stateFlags2 &= ~PLAYER_STATE2_4000;
    frame(BTN_CLEFT);
    assert(!WolfLinkForm_IsReady() && !(player.stateFlags3 & PLAYER_STATE3_4));
    // Full-width D-pad store is independent from the three shared C slots.
    ExtButton_ClearItem(0, EQUIP_SLOT_C_LEFT);
    ExtButton_SetDpadItem(0, EQUIP_SLOT_D_UP, EXT_ITEM_SHADOW_CRYSTAL);
    frame(BTN_DUP);
    assert(!WolfLinkForm_IsReady());
    integerCvars["gEnhancements.Dpad.DpadEquips"] = 1;
    hookSuppressedButtons = BTN_DUP;
    frame(BTN_DUP);
    assert(!WolfLinkForm_IsReady());
    hookSuppressedButtons = 0;
    frame(BTN_DUP);
    assert(WolfLinkForm_IsReady());
    frame(BTN_DUP);
    assert(!WolfLinkForm_IsReady());
    // Native mask press reaches the existing item path and releases Wolf before that path runs.
    ExtButton_SetItem(0, EQUIP_SLOT_C_LEFT, EXT_ITEM_SHADOW_CRYSTAL);
    frame(BTN_CLEFT);
    assert(WolfLinkForm_IsReady());
    BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_C_RIGHT) = ITEM_MASK_DEKU;
    filtered = frame(BTN_CRIGHT);
    assert(!WolfLinkForm_IsReady() && (filtered.press.button & BTN_CRIGHT));
    // An enabled equipment moveset must not reject activation or eject Wolf.
    pendant = 1;
    frame(BTN_CLEFT);
    assert(WolfLinkForm_IsReady());
    frame();
    assert(WolfLinkForm_IsReady());
    frame(BTN_CLEFT);
    assert(!WolfLinkForm_IsReady());
    pendant = 0;
    // Active tool/other-form arbitration must never silently steal their input/action.
    for (u8* blocker : { &beetle, &kite, &trident, &mario, &customItemBlock, &pakBodyModel, &o2rModel }) {
        *blocker = 1;
        frame(BTN_CLEFT);
        assert(!WolfLinkForm_IsReady());
        *blocker = 0;
    }
    assert(logged("result=pak-body-model") && logged("result=o2r-body-model"));
    player.itemAction = PLAYER_IA_SWORD_KOKIRI;
    frame(BTN_CLEFT);
    assert(!WolfLinkForm_IsReady() && logged("result=held-item"));
    const auto quietLogs = wolfLogs.str();
    frame();
    assert(wolfLogs.str() == quietLogs && "idle rejected states must not log per frame");
    player.itemAction = PLAYER_IA_NONE;
    otherForm = CUSTOM_FORM_GARO;
    frame(BTN_CLEFT);
    assert(!WolfLinkForm_IsReady());
    otherForm = CUSTOM_FORM_NONE;
    player.stateFlags3 = PLAYER_STATE3_FLYING_WITH_HOOKSHOT;
    frame(BTN_CLEFT);
    assert(!WolfLinkForm_IsReady());
    player.stateFlags3 = 0;
    frame(BTN_CLEFT);
    assert(WolfLinkForm_IsReady());
    player.transformation = PLAYER_FORM_GORON;
    frame();
    assert(!WolfLinkForm_IsReady());
    player.transformation = PLAYER_FORM_HUMAN;
    frame(BTN_CLEFT);
    assert(WolfLinkForm_IsReady());
    ProcDashInit(&player, &play);
    player.actionFunc = NativeIdle;
    const auto exitActions = nativeActions;
    auto drawTransitionWolf = [&] {
        gfx.polyOpa.p = commands; gfx.polyOpa.d = commands + 128;
        gfx.polyXlu.p = xluCommands; gfx.polyXlu.d = xluCommands + 128;
        gfx.overlay.p = overlayCommands; gfx.overlay.d = overlayCommands + 128;
        return WolfLinkHost_Draw(&play, &player);
    };
    play.transitionTrigger = TRANS_TRIGGER_START;
    frame();
    assert(WolfLinkForm_IsReady() && WolfLinkForm_IsSelected() &&
           "scene exit must retain the selected Wolf presentation until actual teardown");
    assert(!(player.stateFlags3 & PLAYER_STATE3_4) && player.cylinder.dim.radius == 12);
    assert(!sWolf.atActive && !WolfLinkForm_OwnsPlayerAction() && player.actionFunc == NativeIdle &&
           nativeActions == exitActions + 1 && WolfLinkForm_SpeedMultiplier() == 1.f);
    assert(drawTransitionWolf() == 1 &&
           "scene exit draw must not expose the native human model");
    player.cylinder.dim.radius = 17;
    WolfLinkHost_ApplyCollisionShape(&player);
    assert(player.cylinder.dim.radius == 17 && "presentation must preserve native transition collision");
    play.actorCtx.isOverrideInputOn = true;
    play.actorCtx.overrideInput.press.button = BTN_A | BTN_B;
    player.csAction = PLAYER_CSACTION_1;
    auto exitInput = frame();
    assert((exitInput.press.button & (BTN_A | BTN_B)) == (BTN_A | BTN_B) &&
           player.actionFunc == NativeIdle && !WolfLinkForm_OwnsPlayerAction());
    assert(WolfLinkForm_IsReady() && drawTransitionWolf() == 1);
    play.state.running = false;
    WolfLinkHost_Destroy(&play, &player);
    assert(!WolfLinkForm_IsReady() && WolfLinkForm_IsSelected() && !sWolf.atCylInit);
    ++play.sceneId;
    play.gameplayFrames = 0;
    play.state.running = true;
    play.transitionTrigger = TRANS_TRIGGER_END;
    frame();
    assert(WolfLinkForm_IsReady() && WolfLinkForm_IsSelected() &&
           drawTransitionWolf() == 1 &&
           "arrival must render Wolf before native entrance controls return");
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    frame();
    assert(WolfLinkForm_IsReady() && WolfLinkForm_IsSelected() &&
           drawTransitionWolf() == 1 && !WolfLinkForm_OwnsPlayerAction());
    play.actorCtx.isOverrideInputOn = false;
    play.actorCtx.overrideInput = {};
    player.csAction = PLAYER_CSACTION_NONE;
    frame();
    assert(WolfLinkForm_IsReady() && WolfLinkForm_IsSelected() && sWolf.owner == &player &&
           "arrival restores Wolf without a second Shadow Crystal press");
    frame(BTN_CLEFT);
    assert(!WolfLinkForm_IsReady() && !WolfLinkForm_IsSelected());
    ++play.sceneId;
    frame();
    assert(!WolfLinkForm_IsReady() && !WolfLinkForm_IsSelected() &&
           "deliberate detransformation must survive the next scene");
    frame(BTN_CLEFT);
    assert(WolfLinkForm_IsReady());
    player.stateFlags1 = PLAYER_STATE1_DEAD;
    frame();
    assert(!WolfLinkForm_IsReady());
    player.stateFlags1 = 0;
    frame(BTN_CLEFT);
    assert(WolfLinkForm_IsReady());
    ++play.sceneId;
    frame();
    assert(WolfLinkForm_IsReady() && WolfLinkForm_IsSelected() && sWolf.owner == &player &&
           "scene-id fallback must rebuild the transient runtime and retain Wolf");
    Player replacement{};
    replacement.transformation = PLAYER_FORM_HUMAN;
    replacement.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    replacement.cylinder.dim.radius = 12;
    replacement.cylinder.dim.height = 50;
    replacement.actor.shape.shadowDraw = ActorShadow_DrawFeet;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &replacement.actor;
    play.state.input[0] = {};
    WolfLinkHost_PreUpdate(&play, &replacement);
    Input replacementInput{};
    WolfLinkHost_FilterInput(&replacement, &replacementInput);
    WolfLinkHost_BeforeAction(&play, &replacement, &replacementInput);
    assert(WolfLinkForm_IsReady() && WolfLinkForm_IsSelected() && sWolf.owner == &replacement &&
           "actor replacement must not dereference the departed owner or erase the selection");
    play.state.running = false;
    WolfLinkHost_Destroy(&play, &replacement);
    assert(!WolfLinkForm_IsReady() && WolfLinkForm_IsSelected());
    play.state.running = true;
    // The departed actor is gone in-game; reconstruct the next native player here.
    player = Player{};
    player.transformation = PLAYER_FORM_HUMAN;
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    player.cylinder.dim.radius = 12;
    player.cylinder.dim.height = 50;
    player.actor.shape.shadowDraw = ActorShadow_DrawFeet;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    frame();
    assert(WolfLinkForm_IsReady());
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnSaveLoad>((s16)gSaveContext.fileNum);
    assert(!WolfLinkForm_IsReady() && !WolfLinkForm_IsSelected() &&
           "same-slot save reload must clear the transient Wolf selection");
    frame(BTN_CLEFT);
    assert(WolfLinkForm_IsReady());
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnSaveInit>((s16)gSaveContext.fileNum);
    assert(!WolfLinkForm_IsReady() && !WolfLinkForm_IsSelected());
    frame(BTN_CLEFT);
    ++gSaveContext.fileNum;
    frame();
    assert(!WolfLinkForm_IsReady() && !WolfLinkForm_IsSelected() && "file switching resets Wolf");
    frame(BTN_CLEFT);
    integerCvars["gMods.WolfLink.Enabled"] = 0;
    frame();
    assert(!WolfLinkForm_IsReady() && !WolfLinkForm_IsSelected());
    integerCvars["gMods.WolfLink.Enabled"] = 1;
    frame();
    assert(!WolfLinkForm_IsReady());
    frame(BTN_CLEFT);
    play.transitionTrigger = TRANS_TRIGGER_START;
    frame();
    assert(WolfLinkForm_IsSelected() && WolfLinkForm_IsReady());
    integerCvars["gMods.WolfLink.Enabled"] = 0;
    frame();
    assert(!WolfLinkForm_IsSelected() && !WolfLinkForm_IsReady() &&
           "disable also cancels a pending scene restoration");
    integerCvars["gMods.WolfLink.Enabled"] = 1;
    play.transitionTrigger = TRANS_TRIGGER_OFF;
    frame(BTN_CLEFT);
    play.state.running = false;
    play.state.init = TitleSetup_Init;
    WolfLinkHost_Destroy(&play, &player);
    assert(!WolfLinkForm_IsReady() && !WolfLinkForm_IsSelected() && "title handoff resets Wolf");
    play.state.running = true;
    play.state.init = Play_Init;
    frame(BTN_CLEFT);
    gSaveContext.gameMode = GAMEMODE_OWL_SAVE;
    play.state.running = false;
    WolfLinkHost_Destroy(&play, &player);
    assert(!WolfLinkForm_IsReady() && !WolfLinkForm_IsSelected() && !sWolf.atCylInit &&
           !(player.stateFlags3 & PLAYER_STATE3_4) && "owl-save quit resets Wolf");
    gSaveContext.gameMode = GAMEMODE_NORMAL;
    play.state.running = true;
    frame(BTN_CLEFT);
    assert(sWolfAudio.voices[0].clip == kWolfClips && "a deliberate MM entry starts the supplied Wolf cue");
    frame(BTN_CLEFT);
    assert(!WolfLinkForm_IsReady() && !WolfLinkForm_IsSelected() &&
           sWolfAudio.voices[0].clip == kWolfClips + 1 &&
           "the deliberate MM exit starts Human audio after runtime cleanup");
    s16 tailPcm[1024]{};
    play.pauseCtx.state = PAUSE_STATE_MAIN;
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnGameStateUpdate>();
    MM_WolfLinkSfx_MixInto(tailPcm, 512);
    assert(sWolfAudio.voices[0].position == 0 &&
           std::all_of(std::begin(tailPcm), std::end(tailPcm), [](s16 value) { return value == 0; }) &&
           "the registered MM game-state hook pauses Human tails without an active Wolf player");
    play.pauseCtx.state = PAUSE_STATE_OFF;
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnGameStateUpdate>();
    MM_WolfLinkSfx_MixInto(tailPcm, 512);
    assert(sWolfAudio.voices[0].position > 0 &&
           std::any_of(std::begin(tailPcm), std::end(tailPcm), [](s16 value) { return value != 0; }));
    GameInteractor::Instance->ExecuteHooks<GameInteractor::OnSaveLoad>((s16)gSaveContext.fileNum);
    assert(!sWolfAudio.voices[0].clip && !sWolfAudio.voices[1].clip);
    // A malformed replacement on a fresh load retains the normal player's draw, input and shape.
    sAssetsLoaded = 0;
    auto bad = makeAsset();
    write32(bad, field(bad, 6), 0x7fc00001);
    assert(!load(bad));
    filtered = frame(BTN_CLEFT | BTN_B);
    assert(!WolfLinkForm_IsReady() && !WolfLinkHost_Draw(&play, &player) && (filtered.press.button & BTN_B));
    assert(player.cylinder.dim.radius == 12 && player.cylinder.dim.height == 50 &&
           player.actor.shape.shadowDraw == ActorShadow_DrawFeet);
    std::remove((assetDirectory + "/wolf_link.bin").c_str());
    sAssetsLoaded = 0;
    filtered = frame(BTN_CLEFT | BTN_B);
    assert(!WolfLinkForm_IsReady() && (filtered.press.button & BTN_B));
    assert(logged("reason=open-failed") && logged("result=asset-load-failed"));
    assert(logged((assetDirectory + "/wolf_link.bin").c_str()));
    std::puts("PASS production MM Wolf host: full-width C/D-pad toggles, native damage/freeze/thaw action dispatch, "
              "input/tool arbitration, rendering, scene/actor persistence, save/file/title resets and teardown");
}
