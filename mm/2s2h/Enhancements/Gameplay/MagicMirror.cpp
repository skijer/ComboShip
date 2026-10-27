#ifdef COMBO_BUILD
#include "global.h"
#include "ComboGameMode.h"
#include "ComboForestActors.h"
#include "entrances/SoloForest.h"
#include "entrances/SoloForestResources.h"
#include "entrances/NativeForest.h"
#include "entrances/NativeForestResourceData.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include "entrances/LostWoodsEntrance.h"
#include "gameplay/MagicMirrorAnchor.h"
#include "gameplay/MagicMirrorArrival.h"
#include "gameplay/MagicMirrorItem.h"
#include "gameplay/MagicMirrorPlayer.h"
#include "actors/ForestGiParticles.h"
#include "../../../../soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include "2s2h/Rando/NeiUsedMagicPresentation.h"
#include "mods/extended_inventory.h"
#include "mods/ext_buttons/ext_buttons.h"
#include "mods/items/helpers/equip_helper.h"
#include "2s2h/ShipInit.hpp"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/CustomItem/CustomItem.h"
#include "2s2h/CustomMessage/CustomMessage.h"
#include "2s2h/SaveManager/SaveManager.h"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

extern "C" {
u8 ResourceMgr_FileExists(const char* path);
int Combo_MM_CrossDungeonTransitionPending(void);
u8 Pacci_UltrahandModeActive(void);
u8 Sm64Mario_IsReady(void);
}
#define COMBO_MAGIC_MIRROR_MM
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
    Audio_PlaySfx(NA_SE_EV_TRE_BOX_FLASH);
}
void PlayDepartureCue() {
    Audio_PlaySfx(NA_SE_PL_MAGIC_WIND_WARP);
}
u32 Seed() {
    return gSaveContext.save.shipSaveInfo.rando.finalSeed;
}
bool PackAvailable() {
    return Combo_IsCombinedGame() ||
           NativeForest::SceneAvailable(NativeForest::kMmSoloScene, ResourceMgr_FileExists, ResourceMgr_FileExists,
                                        NativeForest::LoadDependencies) ||
           std::all_of(std::begin(kSoloForestResources), std::end(kSoloForestResources),
                       [](const char* path) { return ResourceMgr_FileExists(path); });
}
bool Busy(PlayState* play) {
    return ItemInput_IsBlocked(GET_PLAYER(play), play) || Combo_MM_CrossDungeonTransitionPending();
}
bool TransitionPending() {
    return Combo_MM_CrossDungeonTransitionPending();
}
bool AlternateModel() {
    return Sm64Mario_IsReady();
}
bool TimedActivity() {
    return std::any_of(std::begin(gSaveContext.timerStates), std::end(gSaveContext.timerStates),
                       [](u8 state) { return state != TIMER_STATE_OFF; });
}
void Save() {
    SaveManager_SaveCurrentForCombo();
}
void Error() {
    Audio_PlaySfx(NA_SE_SY_ERROR);
}
void Trace(const char* phase, PlayState* play) {
    const auto* player = GET_PLAYER(play);
    SPDLOG_INFO("Magic Mirror MM: phase={} scene={} text={:04x} mode={} length={} state1={:08x} ground={} cast={}",
                phase, play->sceneId, play->msgCtx.currentTextId, play->msgCtx.msgMode, play->msgCtx.msgLength,
                player ? player->stateFlags1 : 0, player && (player->actor.bgCheckFlags & BGCHECKFLAG_GROUND),
                MagicMirrorPlayer_State(player));
}
bool ButtonClaimed(u16) {
    return Pacci_UltrahandModeActive();
}
u16 ButtonItem(s32 index) {
    if (index >= 1 && index <= 3)
        return gSaveContext.buttonStatus[index] == BTN_DISABLED ? ITEM_NONE : ExtButton_GetItem(0, index);
    if (index < 4 || index > 7 || !CVarGetInteger("gEnhancements.Dpad.DpadEquips", 0))
        return ITEM_NONE;
    static constexpr int slots[] = { EQUIP_SLOT_D_UP, EQUIP_SLOT_D_DOWN, EQUIP_SLOT_D_LEFT, EQUIP_SLOT_D_RIGHT };
    const int slot = slots[index - 4];
    return gSaveContext.shipSaveContext.dpad.status[slot] == BTN_DISABLED ? ITEM_NONE : ExtButton_GetDpadItem(0, slot);
}
void OpenPrompt() {
    Message_StartTextbox(gPlayState, 0x3FEC, nullptr);
}
void GivePickup(Actor* actor, PlayState*) {
    MirrorRuntime::FinishPickup(CUSTOM_ITEM_PARAM);
    CustomMessage::Entry entry;
    entry.textboxType = TEXTBOX_TYPE_2;
    entry.msg = "You found the %gMagic Mirror%w!\nEquip it to a C Button to return\nto the Lost Woods from outdoors."
                "\x10Its reflection bridges worlds.\nUse the item selector to switch\nbetween Mirror and Time Gate.";
    if (CUSTOM_ITEM_FLAGS & CustomItem::GIVE_ITEM_CUTSCENE)
        CustomMessage::SetActiveCustomMessage(entry.msg, entry);
    else
        CustomMessage::StartTextbox(entry.msg, entry);
}
void DrawPickup(Actor*, PlayState* play) {
    Matrix_Scale(30, 30, 30, MTXMODE_APPLY);
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
    GameInteractor::Instance->events.emplace_back(GIEventGiveItem{
        .showGetItemCutscene = true,
        .param = static_cast<s16>(token),
        .giveItem = GivePickup,
        .drawItem = DrawPickup,
    });
    return true;
}
void ClearQueuedPickups() {
    auto& events = GameInteractor::Instance->events;
    std::erase_if(events, [](const GIEvent& event) {
        const auto* item = std::get_if<GIEventGiveItem>(&event);
        return item && item->giveItem == GivePickup;
    });
}
} // namespace MirrorHost

extern "C" void MagicMirror_DrawGI(PlayState* play) {
    if (!play)
        return;
    ComboMagicMirrorVisual::Draw(play);
    NeiGi_DrawMesh(play, ForestGiParticles::Mirror(play->gameplayFrames, NeiGi_CameraBasis(play)));
}

#include "gameplay/MagicMirrorRuntime.inc.h"

namespace {
void ResetMirror() {
    MirrorRuntime::Reset();
    MirrorHost::ClearQueuedPickups();
}
void OpenMirrorText(u16* text, bool* fromTable) {
    if (!text || !fromTable || *text != MirrorRuntime::kPromptText)
        return;
    CustomMessage::Entry entry;
    entry.msg = "Return to the Lost Woods?\002\021\302Yes\021No";
    CustomMessage::LoadCustomMessageIntoFont(entry);
    *fromTable = false;
}
void RegisterMagicMirror() {
    if (gComboGameMode != COMBO_GAME_BOTH && gComboGameMode != COMBO_GAME_MM)
        return;
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateMainStart>(MirrorRuntime::Update);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawWorldEnd>(MirrorRuntime::DrawWorld);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](s16, s8) { ResetMirror(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDestroy>([]() {
        MirrorRuntime::Destroy();
        MirrorHost::ClearQueuedPickups();
    });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>(OpenMirrorText);
}
RegisterShipInitFunc init(RegisterMagicMirror, {});
} // namespace
#endif
