#ifdef COMBO_BUILD
#include "global.h"
#include "ComboGameMode.h"
#include "entrances/SoloForest.h"
#include "entrances/SoloForestResources.h"
#include "entrances/NativeForest.h"
#include "entrances/NativeForestResourceData.h"
#include "mods/transformation_masks/assets/mm_asset_loader.h"
#include "entrances/LostWoodsEntrance.h"
#include "gameplay/MagicMirrorAnchor.h"
#include "gameplay/MagicMirrorArrival.h"
#include "gameplay/MagicMirrorItem.h"
#include "gameplay/MagicMirrorPlayer.h"
#include "actors/ForestGiParticles.h"
#include "soh/Enhancements/randomizer/NeiGiRender.h"
#include "soh/Enhancements/randomizer/NeiUsedMagicPresentation.h"
#include "mods/extended_inventory.h"
#include "mods/ext_buttons/ext_buttons.h"
#include "mods/items/helpers/equip_helper.h"
#include "soh/SaveManager.h"
#include "soh/ShipInit.hpp"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/randomizer/SeedContext.h"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

extern "C" {
u8 ResourceMgr_FileExists(const char* path);
int SOH_QueueMagicMirrorHandoff(void);
int Combo_SOH_CrossDungeonTransitionPending(void);
u8 Sm64Mario_IsReady(void);
}
#include "gameplay/MagicMirrorDraw.inc.h"
#include "gameplay/MagicMirrorCamera.inc.h"

namespace MirrorRuntime {
bool FinishPickup(int token);
}
#include "MagicMirrorRando.inc.h"
namespace MirrorHost {
bool Shuffled() {
    return MirrorShuffle::Enabled();
}
bool PickupAvailable() {
    return Shuffled() ? MirrorShuffle::StumpAvailable() : !MagicMirror_IsOwned();
}
void CancelPickup() {
    MirrorShuffle::CancelStumpReward();
}
void UpdateCastCamera(PlayState* play, Player* player) {
    ComboMagicMirrorCamera::Begin(play, player);
}
void ReleaseCastCamera(PlayState* play) {
    ComboMagicMirrorCamera::Release(play);
}
void ForgetCastCamera() {
    ComboMagicMirrorCamera::Forget();
}
void PlayActivationChime() {
    Sfx_PlaySfxCentered(NA_SE_EV_TRE_BOX_FLASH);
}
void PlayDepartureCue() {
    Sfx_PlaySfxCentered(NA_SE_PL_MAGIC_WIND_WARP);
}
u32 Seed() {
    return IS_RANDO ? Rando::Context::GetInstance()->GetSeed() : 0;
}
bool PackAvailable() {
    if (Combo_IsCombinedGame())
        return true;
    if (NativeForest::CoreAvailable(NativeForest::kOotSoloScene,
                                    [](const char* path) { return ResourceMgr_FileExists(path); })) {
        MmAssets_Init();
        if (NativeForest::SceneAvailable(NativeForest::kOotSoloScene, ResourceMgr_FileExists,
                                         MmAssets_BaseResourceExists, NativeForest::LoadDependencies))
            return true;
    }
    return std::all_of(std::begin(kSoloForestResources), std::end(kSoloForestResources),
                       [](const char* path) { return ResourceMgr_FileExists(path); });
}
bool Busy(PlayState* play) {
    return ItemInput_IsBlocked(GET_PLAYER(play), play) || Combo_SOH_CrossDungeonTransitionPending();
}
bool TransitionPending() {
    return Combo_SOH_CrossDungeonTransitionPending();
}
bool AlternateModel() {
    return Sm64Mario_IsReady();
}
bool TimedActivity() {
    return gSaveContext.minigameState != 0 || gSaveContext.timerState != 0 || gSaveContext.subTimerState != 0;
}
void Save() {
    Play_PerformSave(gPlayState);
    SaveManager::Instance->ThreadPoolWait();
}
void Error() {
    Sfx_PlaySfxCentered(NA_SE_SY_ERROR);
}
void Trace(const char* phase, PlayState* play) {
    const auto* player = GET_PLAYER(play);
    SPDLOG_INFO("Magic Mirror OoT: phase={} scene={} text={:04x} mode={} length={} state1={:08x} ground={} cast={}",
                phase, play->sceneNum, play->msgCtx.textId, play->msgCtx.msgMode, play->msgCtx.msgLength,
                player ? player->stateFlags1 : 0, player && (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND),
                MagicMirrorPlayer_State(player));
}
bool ButtonClaimed(u16 button) {
    return ItemInput_ButtonIsClaimed(button);
}
u16 ButtonItem(s32 index) {
    if (index < 1 || index > 7 || (index > 3 && !CVarGetInteger(CVAR_ENHANCEMENT("DpadEquips"), 0)) ||
        gSaveContext.buttonStatus[index] == BTN_DISABLED)
        return ITEM_NONE;
    return ExtButton_GetItem(index);
}
bool RequestHandoff() {
    return SOH_QueueMagicMirrorHandoff() != 0;
}
void OpenPrompt() {
    Message_StartTextbox(gPlayState, 0x3FEC, nullptr);
}
void DrawPickup(PlayState* play, GetItemEntry*) {
    // Native OoT get-item draws at .2; MM's item actor uses .015 * 30 = .45.
    Matrix_Scale(2.25f, 2.25f, 2.25f, MTXMODE_APPLY);
    ComboMagicMirrorVisual::Draw(play);
    NeiGi_DrawMesh(play, ForestGiParticles::Mirror(play->gameplayFrames, NeiGi_CameraBasis(play)));
}
void DrawWorldShimmer(PlayState* play) {
    NeiGi_DrawWorldParticles(play, ForestGiParticles::Mirror(play->gameplayFrames, NeiGi_CameraBasis(play)));
}
void DrawWorldPickup(PlayState* play) {
    if (Shuffled()) {
        MirrorShuffle::DrawStumpReward(play);
    } else {
        ComboMagicMirrorVisual::Draw(play);
        DrawWorldShimmer(play);
    }
}
void DrawCastAura(PlayState* play, Player* player, float scale, float alpha) {
    NeiUsedMagic_DrawMirrorCast(play, player, scale, alpha);
}
bool QueuePickup(int token) {
    if (Shuffled())
        return MirrorShuffle::QueueStumpReward(token);
    GetItemEntry entry = GET_ITEM_NONE;
    entry.gi = 1;
    entry.getItemId = GI_MAX;
    entry.objectId = OBJECT_GAMEPLAY_KEEP;
    entry.field = 0x80;
    entry.collectable = true;
    entry.textId = 0x3FEB;
    entry.getItemCategory = ITEM_CATEGORY_MAJOR;
    entry.drawFunc = DrawPickup;
    // The native item-give accepts this first. Its placeholder itemId is NONE,
    // so only the independent mirror grant below changes permanent inventory.
    if (!GiveItemEntryWithoutActor(gPlayState, entry))
        return false;
    MirrorRuntime::FinishPickup(token);
    return true;
}
} // namespace MirrorHost

extern "C" void MagicMirror_DrawGI(PlayState* play) {
    if (play)
        MirrorHost::DrawPickup(play, nullptr);
}

#include "gameplay/MagicMirrorRuntime.inc.h"

namespace {
void OpenMirrorText(u16* text, bool* fromTable) {
    if (!text || !fromTable || (*text != MirrorRuntime::kReceiptText && *text != MirrorRuntime::kPromptText))
        return;
    const char* body =
        *text == MirrorRuntime::kReceiptText
            // OoT's native newline is 0x01; C++ '\n' is its persistent-text
            // control (0x0A), which cannot close the native get-item receipt.
            ? "You found the Magic Mirror!&Equip it to a C Button to return&to the Lost Woods from outdoors.\x04"
              "Its reflection bridges worlds.&Use the item selector to switch&between Mirror and Time Gate."
            : "Return to the Lost Woods?\x01\x1B\x05\x42"
              "Yes\x01No\x05\x40";
    CustomMessage message(body);
    message.AutoFormat();
    message.LoadIntoFont();
    *fromTable = false;
}
void RegisterMagicMirror() {
    if (gComboGameMode != COMBO_GAME_BOTH && gComboGameMode != COMBO_GAME_OOT)
        return;
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateMainStart>(MirrorRuntime::Update);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawEnd>(MirrorRuntime::DrawWorld);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s16) { MirrorRuntime::Reset(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>(MirrorRuntime::Destroy);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>(OpenMirrorText);
}
RegisterShipInitFunc init(RegisterMagicMirror);
} // namespace
#endif
