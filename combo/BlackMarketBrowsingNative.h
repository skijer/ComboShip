// Included inside each native controller's private namespace after Purchase().
// The existing offer/receipt policy is an adapter to native host camera flows.
BlackMarketBrowsing::State sBrowser;
s16 sBrowseCamera = -1, sBrowseParent = -1;
s16 sBrowseCameraUid = -1;
Camera* sBrowseCameraPtr = nullptr;
bool sBrowsePlayerWasHidden = false;
bool sBrowseTextboxSaved = false;
std::array<s16, 6> sBrowseTextboxLayout{};
#include "BlackMarketShopCameraNative.h"
void PlayBrowseCursorSound() {
#ifdef BLACK_MARKET_BROWSING_OOT
    Sfx_PlaySfxCentered(NA_SE_SY_CURSOR);
#else
    Audio_PlaySfx(NA_SE_SY_CURSOR);
#endif
}
void UpdateBrowsingCamera(PlayState* play);
bool BrowsingMessage(PlayState* play) {
#ifdef BLACK_MARKET_BROWSING_OOT
    const auto text = play->msgCtx.textId;
#else
    const auto text = play->msgCtx.currentTextId;
#endif
    return text == kWelcomeText || text == kBrowseText || text == kConfirmText || text == kResultText ||
           text == kKeeperTalkText || text == kKeeperHelpChoiceText || text == kKeeperHelpText;
}

void ContinueBrowsingText(PlayState* play, u16 text) {
    // Native ContinueTextbox skips the START positioning step. Keep the
    // welcome box's accepted placement, and move merchandise below the row
    // without reopening the box or changing native choice/message behavior.
    auto& message = play->msgCtx;
#ifdef BLACK_MARKET_BROWSING_OOT
    const std::array<s16, 6> current{ R_TEXTBOX_Y,           R_TEXTBOX_Y_TARGET,    R_TEXTBOX_END_YPOS,
                                      R_TEXT_CHOICE_YPOS(0), R_TEXT_CHOICE_YPOS(1), R_TEXT_CHOICE_YPOS(2) };
#else
    const std::array<s16, 6> current{ message.textboxY,    message.textboxYTarget, message.unk11FFE[0],
                                      message.unk11FFE[1], message.unk11FFE[2],    0 };
#endif
    const bool keeperText =
        text == kWelcomeText || text == kKeeperTalkText || text == kKeeperHelpChoiceText || text == kKeeperHelpText;
    if (!keeperText && !sBrowseTextboxSaved) {
        sBrowseTextboxLayout = current;
        sBrowseTextboxSaved = true;
    }
    Message_ContinueTextbox(play, text);
    if (keeperText && !sBrowseTextboxSaved)
        return;
#ifdef BLACK_MARKET_BROWSING_OOT
    const auto layout = keeperText ? sBrowseTextboxLayout : std::array<s16, 6>{ 142, 142, 201, 162, 174, 186 };
    R_TEXTBOX_Y = layout[0];
    R_TEXTBOX_Y_TARGET = layout[1];
    R_TEXTBOX_END_YPOS = layout[2];
    for (int row = 0; row < 3; ++row)
        R_TEXT_CHOICE_YPOS(row) = layout[3 + row];
#else
    const auto layout = keeperText ? sBrowseTextboxLayout : std::array<s16, 6>{ 142, 142, 162, 174, 186, 0 };
    message.textboxY = layout[0];
    message.textboxYTarget = layout[1];
    for (int row = 0; row < 3; ++row)
        message.unk11FFE[row] = layout[2 + row];
#endif
    if (keeperText)
        sBrowseTextboxSaved = false;
}

void EndBrowsing(PlayState* play, bool destroying = false) {
    const bool wasTalking = sTalking;
    EndNativeBrowsingCamera(play, destroying);
    if (wasTalking && !destroying && play)
        if (auto* player = GET_PLAYER(play); player && !sBrowsePlayerWasHidden)
            player->stateFlags2 &= ~(1u << 29);
    sBrowseCamera = sBrowseParent = sBrowseCameraUid = -1;
    sBrowseCameraPtr = nullptr;
    sBrowseTextboxSaved = false;
    sBrowser = {};
    sTalking = false;
}
bool BeginBrowsing(PlayState* play) {
    if (sTalking)
        return true;
    sBrowseParent = Play_GetActiveCamId(play);
    if (sBrowseParent != CAM_ID_MAIN || !GET_PLAYER(play))
        return false;
    sBrowser = {};
    sSelected = -1;
    if (!BeginNativeBrowsingCamera(play))
        return false;
    auto* player = GET_PLAYER(play);
    sBrowsePlayerWasHidden = (player->stateFlags2 & (1u << 29)) != 0;
    player->stateFlags2 |= 1u << 29;
    sTalking = true;
    UpdateBrowsingCamera(play);
    return true;
}
void UpdateBrowsingCamera(PlayState* play) {
    if (!sTalking)
        return;
    sBrowser.TickCamera();
    TickNativeBrowsingCamera(play);
}
void MonitorBrowsing() {
    if (!sTalking)
        return;
    auto* play = gPlayState;
    if (!play) {
        EndBrowsing(nullptr, true);
        return;
    }
    if (!Active() || !OwnNativeBrowsingCamera(play) || !BrowsingMessage(play) ||
        play->transitionTrigger != TRANS_TRIGGER_OFF || play->transitionMode != TRANS_MODE_OFF ||
        play->gameOverCtx.state != GAMEOVER_INACTIVE) {
        if (BrowsingMessage(play))
            Message_CloseTextbox(play);
        EndBrowsing(play);
    }
}
void BrowseInput(PlayState* play) {
    using BlackMarketBrowsing::Phase;
    MonitorBrowsing();
    if (!sTalking)
        return;
    const auto state = Message_GetState(&play->msgCtx);
    if (state == TEXT_STATE_NONE || state == TEXT_STATE_CLOSING) {
        EndBrowsing(play);
        return;
    }
    if (play->pauseCtx.state != 0)
        return;
    UpdateBrowsingCamera(play);
    auto& input = play->state.input[0];
    if (input.press.button & BTN_B) {
        if (sBrowser.phase == Phase::KeeperTalk || sBrowser.phase == Phase::KeeperHelpChoice ||
            sBrowser.phase == Phase::KeeperHelp) {
            sBrowser.phase = Phase::Browse;
            ContinueBrowsingText(play, kWelcomeText);
        } else if (sBrowser.phase != Phase::Browse) {
            sBrowser.phase = Phase::Browse;
            ContinueBrowsingText(play, kBrowseText);
        } else if (sSelected >= 0) {
            sBrowser.row = sSelected & 1;
            sBrowser.slot = sSelected = -1;
            PlayBrowseCursorSound();
            ContinueBrowsingText(play, kWelcomeText);
        } else {
            Message_CloseTextbox(play);
            EndBrowsing(play);
        }
        sBrowser.ResetInput();
        return;
    }
    // Native EnOssan has separate look-to-shelf/look-to-keeper states. Do not
    // consume navigation or A/Buy while the camera is still turning; B remains
    // available above so an interrupted browse can always be cancelled.
    if (!NativeBrowsingCameraSettled(play)) {
        sBrowser.ResetInput();
        return;
    }
    if (sBrowser.phase == Phase::KeeperTalk || sBrowser.phase == Phase::KeeperHelpChoice ||
        sBrowser.phase == Phase::KeeperHelp) {
        if (!(input.press.button & BTN_A) || !Message_ShouldAdvance(play))
            return;
        if (sBrowser.phase == Phase::KeeperHelpChoice && state == TEXT_STATE_CHOICE) {
            if (play->msgCtx.choiceIndex == 0) {
                sBrowser.phase = Phase::KeeperHelp;
                ContinueBrowsingText(play, kKeeperHelpText);
            } else {
                sBrowser.phase = Phase::Browse;
                ContinueBrowsingText(play, kWelcomeText);
            }
        } else if (state == TEXT_STATE_EVENT || state == TEXT_STATE_DONE) {
            const bool intro = sBrowser.phase == Phase::KeeperTalk;
            sBrowser.phase = intro ? Phase::KeeperHelpChoice : Phase::Browse;
            ContinueBrowsingText(play, intro ? kKeeperHelpChoiceText : kWelcomeText);
        }
        sBrowser.ResetInput();
        return;
    }
    if (sBrowser.phase == Phase::Confirm) {
        if (state == TEXT_STATE_CHOICE && (input.press.button & BTN_A) && Message_ShouldAdvance(play)) {
            if (play->msgCtx.choiceIndex == 0) {
                if (Purchase()) {
                    // Release the native shop view and shop-hide bit before
                    // the engine starts its own get-item camera and receipt.
                    Message_CloseTextbox(play);
                    EndBrowsing(play);
                    return;
                }
                sBrowser.phase = Phase::Result;
                ContinueBrowsingText(play, kResultText);
            } else {
                sBrowser.phase = Phase::Browse;
                ContinueBrowsingText(play, kBrowseText);
            }
            sBrowser.ResetInput();
        }
        return;
    }
    if (sBrowser.phase == Phase::Result) {
        if ((state == TEXT_STATE_EVENT || state == TEXT_STATE_DONE) && (input.press.button & BTN_A) &&
            Message_ShouldAdvance(play)) {
            sBrowser.phase = Phase::Browse;
            sBrowser.ResetInput();
            ContinueBrowsingText(play, kBrowseText);
        }
        return;
    }
    if (state != TEXT_STATE_EVENT && state != TEXT_STATE_DONE)
        return;
    int x = sBrowser.horizontal.Step(input.rel.stick_x), y = sBrowser.vertical.Step(input.rel.stick_y);
    if (input.press.button & BTN_DLEFT)
        x = -1;
    else if (input.press.button & BTN_DRIGHT)
        x = 1;
    if (input.press.button & BTN_DUP)
        y = 1;
    else if (input.press.button & BTN_DDOWN)
        y = -1;
#ifdef BLACK_MARKET_BROWSING_OOT
    if (CVarGetInteger(CVAR_ENHANCEMENT("MirroredWorld"), 0))
        x = -x;
#endif
    if (sBrowser.Move(x, y)) {
        sSelected = sBrowser.slot;
        PlayBrowseCursorSound();
        ContinueBrowsingText(play, sSelected < 0 ? kWelcomeText : kBrowseText);
    } else if ((input.press.button & BTN_A) && Message_ShouldAdvance(play)) {
        if (sSelected < 0) {
            sBrowser.phase = Phase::KeeperTalk;
            ContinueBrowsingText(play, kKeeperTalkText);
        } else {
            sBrowser.phase = Phase::Confirm;
            ContinueBrowsingText(play, kConfirmText);
        }
        sBrowser.ResetInput();
    }
}
void DrawBrowseCursor() {
    auto* play = gPlayState;
    if (!sTalking || !Active() || sSelected < 0 || Play_GetActiveCamId(play) != sBrowseCamera ||
        sBrowser.phase != BlackMarketBrowsing::Phase::Browse || !NativeBrowsingCameraSettled(play))
        return;
    Actor display{};
    // Both native Actor_GetScreenPos implementations project focus.pos, as
    // EnOssan does for EnGirlA. A display-only actor has no focus update.
    display.focus.pos = kShelves[sSelected];
    s16 x, y;
    Actor_GetScreenPos(play, &display, &x, &y);
#ifdef BLACK_MARKET_BROWSING_OOT
    if (CVarGetInteger(CVAR_ENHANCEMENT("MirroredWorld"), 0))
#else
    if (CVarGetInteger("gModes.MirroredWorld.State", 0))
#endif
        x = SCREEN_WIDTH - x;
    if (x < 0 || x > SCREEN_WIDTH || y < 0 || y > SCREEN_HEIGHT ||
        !BlackMarketGraphicsArena::HasRoom(play->state.gfxCtx, 0, 8, 8, 32))
        return;
    alignas(2) static const char texture[] = "__OTR__objects/gameplay_keep/gSelectionCursorTex";
    OPEN_DISPS(play->state.gfxCtx);
#ifdef BLACK_MARKET_BROWSING_OOT
    Gfx_SetupDL_39Overlay(play->state.gfxCtx);
#else
    Gfx_SetupDL39_Overlay(play->state.gfxCtx);
#endif
    gDPSetPrimColor(OVERLAY_DISP++, 0, 0, 0, 255, 80, 220);
    gDPLoadTextureBlock_4b(OVERLAY_DISP++, texture, G_IM_FMT_IA, 16, 16, 0, G_TX_MIRROR | G_TX_WRAP,
                           G_TX_MIRROR | G_TX_WRAP, 4, 4, G_TX_NOLOD, G_TX_NOLOD);
    gSPTextureRectangle(OVERLAY_DISP++, (x - 20) * 4, (y - 20) * 4, (x + 20) * 4, (y + 20) * 4, G_TX_RENDERTILE, 0, 0,
                        819, 819);
    CLOSE_DISPS(play->state.gfxCtx);
}
