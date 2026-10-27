#pragma once

// OoT host only. Travel is deliberately outside this owned dialogue/camera lifetime.
#include "global.h"
#include "ComboGameMode.h"
#include "entrances/TempleSagesPolicy.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include <string>

namespace TempleSages::ConfirmationNative {
enum class Result { Pending, Yes, No, Abort };
inline constexpr u16 kTextId = 0x8F70; // Private dynamic OnOpenText namespace, not a vanilla table replacement.
inline constexpr const char* kDestinations[] = { "Sacred Forest Meadow", "Death Mountain Crater", "Lake Hylia",
                                                 "Desert Colossus", "Graveyard" };
inline std::string Text(int route, bool returning) {
    if (route < 0 || route >= int(kPadCount))
        return {};
    // Native dungeon palette: green, red, blue, yellow, purple. The renderer
    // has no arbitrary RGB text control; Light Blue is the native light hue.
    constexpr char colors[] = { 0x42, 0x41, 0x43, 0x46, 0x45 };
    std::string text = returning ? "Teleport back to the " : "Teleport to ";
    text += '\x05';
    text += returning ? '\x44' : colors[route];
    text += returning ? "Chamber of Sages" : kDestinations[route];
    text += "\x05\x40?\x01\x1BYes\x01No\x02";
    return text;
}
struct State {
    PlayState* play = nullptr;
    Visit visit{};
    int file = -1, mode = -1, game = -1, route = -1;
    bool returning = false, active = false, seenText = false, handoff = false;
    unsigned ticks = 0;
    s16 camera = -1, cameraUid = -1, parentUid = -1;
    Camera* cameraPtr = nullptr;
    Camera* parentPtr = nullptr;
    Player* player = nullptr;
    bool controlHeld = false;
};
inline State state;
inline Visit Identity(PlayState* play) {
    if (!play)
        return {};
    return { play,
             play->sceneSegment,
             play->roomCtx.curRoom.segment,
             play->sceneNum,
             play->roomCtx.curRoom.num,
             gSaveContext.sceneLayer,
             play->roomCtx.status == 0 && play->roomCtx.curRoom.segment != nullptr };
}
inline bool Active() {
    return state.active;
}
inline bool Holding() {
    return state.active && state.handoff;
}
inline bool SameVisit(PlayState* play) {
    const auto visit = Identity(play);
    return play == state.play && LoadedVisit(visit) && visit.scene == state.visit.scene &&
           visit.room == state.visit.room && visit.setup == state.visit.setup &&
           visit.sceneData == state.visit.sceneData && visit.roomData == state.visit.roomData;
}
inline bool OwnCamera(PlayState* play) {
    return state.camera >= CAM_ID_SUB_FIRST && state.camera < NUM_CAMS &&
           play->cameraPtrs[state.camera] == state.cameraPtr && state.cameraPtr->uid == state.cameraUid;
}
inline bool OwnText(PlayState* play) {
    return play->msgCtx.textId == kTextId;
}
inline void ReleaseControl(PlayState* play, bool destroying = false) {
    // A room transition can retain the live Player; a replacement scene or
    // Player must never inherit cleanup writes based on recycled addresses.
    if (state.controlHeld && play && play == state.play && GET_PLAYER(play) == state.player &&
        (destroying ||
         (play == gPlayState && play->sceneSegment == state.visit.sceneData && play->sceneNum == state.visit.scene &&
          gSaveContext.sceneLayer == state.visit.setup && gSaveContext.fileNum == state.file))) {
        state.player->stateFlags1 &= ~PLAYER_STATE1_INPUT_DISABLED;
    }
    state.controlHeld = false;
}
inline void Reset(PlayState* play, bool destroying = false) {
    // A destruction hook runs before Play_Destroy frees cameras/message memory.
    // A changed PlayState must never be dereferenced through the old pointer.
    if (state.active && play && play == state.play && (destroying || play == gPlayState)) {
        ReleaseControl(play, destroying);
        if (OwnText(play))
            Message_CloseTextbox(play);
        if (OwnCamera(play)) {
            // Restore only our parent, and only if no other camera has taken over.
            const bool parentOwned =
                play->cameraPtrs[CAM_ID_MAIN] == state.parentPtr && state.parentPtr->uid == state.parentUid;
            if (Play_GetActiveCamId(play) == state.camera && parentOwned)
                Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_ACTIVE);
            for (auto* camera : play->cameraPtrs) {
                if (!camera || camera == state.cameraPtr)
                    continue;
                if (camera->parentCamIdx == state.camera)
                    camera->parentCamIdx = CAM_ID_MAIN;
                if (camera->childCamIdx == state.camera)
                    camera->childCamIdx = SUBCAM_FREE;
            }
            Play_ClearCamera(play, state.camera);
        }
    }
    state = {};
}
inline void OpenText(u16* textId, bool* fromTable) {
    if (!state.active || gPlayState != state.play || *textId != kTextId || !SameVisit(gPlayState))
        return;
    CustomMessage entry(Text(state.route, state.returning));
    entry.AutoFormat();
    entry.LoadIntoFont();
    *fromTable = false;
}
inline void Register() {
    static bool registered = false;
    if (!registered) {
        GameInteractor::Instance->RegisterGameHook<GameInteractor::OnOpenText>(OpenText);
        registered = true;
    }
}
inline bool Start(PlayState* play, int route, bool returning, Pad portalPose) {
    if (state.active || !play || play != gPlayState || !LoadedVisit(Identity(play)) || route < 0 ||
        route >= int(kPadCount) || !Finite(portalPose.position) || !GET_PLAYER(play) ||
        (GET_PLAYER(play)->stateFlags1 & PLAYER_STATE1_INPUT_DISABLED) ||
        Message_GetState(&play->msgCtx) != TEXT_STATE_NONE || play->msgCtx.msgMode != 0 ||
        !GameInteractor::IsPlayerInControl())
        return false;
    Register();
    state.play = play;
    state.visit = Identity(play);
    state.file = gSaveContext.fileNum;
    state.mode = CVarGetInteger("gEnhancements.TempleSagesTravel", 1);
    state.game = gComboGameMode;
    state.route = route;
    state.returning = returning;
    state.active = true;
    state.player = GET_PLAYER(play);
    state.controlHeld = true;
    // Player_Update neutralizes this actor's copy of pad input for this flag,
    // including transformed forms. The native textbox still reads the raw pad.
    state.player->stateFlags1 |= PLAYER_STATE1_INPUT_DISABLED;
    state.player->actor.speedXZ = state.player->linearVelocity = 0.0f;
    // Never interrupt an actor/door/cutscene camera, even when a spare slot exists.
    auto* parent = play->cameraPtrs[CAM_ID_MAIN];
    if (Play_GetActiveCamId(play) == CAM_ID_MAIN && parent && parent->childCamIdx < CAM_ID_SUB_FIRST) {
        const s16 camera = Play_CreateSubCamera(play);
        if (camera >= CAM_ID_SUB_FIRST && camera < NUM_CAMS && play->cameraPtrs[camera]) {
            state.camera = camera;
            state.cameraPtr = play->cameraPtrs[camera];
            state.cameraUid = state.cameraPtr->uid;
            state.parentPtr = parent;
            state.parentUid = parent->uid;
            state.cameraPtr->parentCamIdx = CAM_ID_MAIN;
            state.cameraPtr->childCamIdx = SUBCAM_FREE;
            Camera_InitPlayerSettings(state.cameraPtr, GET_PLAYER(play));
            Camera_RequestSetting(state.cameraPtr, CAM_SET_FREE0);
            const auto& link = GET_PLAYER(play)->actor.world.pos;
            Vec3f at{ (portalPose.position.x + link.x) * 0.5f, (portalPose.position.y + link.y) * 0.5f + 60.0f,
                      (portalPose.position.z + link.z) * 0.5f };
            const float yaw = portalPose.yaw * (6.28318530718f / 65536.0f);
            Vec3f eye{ at.x + std::sin(yaw) * 180.0f + std::cos(yaw) * 100.0f, at.y + 85.0f,
                       at.z + std::cos(yaw) * 180.0f - std::sin(yaw) * 100.0f };
            Play_CameraSetAtEye(play, camera, &at, &eye);
            Play_CameraSetFov(play, camera, 55.0f);
            Play_ChangeCameraStatus(play, CAM_ID_MAIN, CAM_STAT_WAIT);
            Play_ChangeCameraStatus(play, camera, CAM_STAT_ACTIVE);
        }
    }
    // Camera exhaustion degrades to an ordinary usable Yes/No textbox.
    Message_StartTextbox(play, kTextId, nullptr);
    return true;
}
inline Result Update(PlayState* play) {
    if (!state.active)
        return Result::Abort;
    auto finish = [play](Result result) {
        Reset(play);
        return result;
    };
    // Yes is emitted once. Preserve the framing while the native adapter fades
    // and transfers scenes; its destruction/reset hook releases the camera.
    // Failure to start that transfer must explicitly Reset in the adapter.
    if (state.handoff) {
        if (!play || play != gPlayState || !SameVisit(play) || state.file != gSaveContext.fileNum ||
            state.game != gComboGameMode || state.mode != CVarGetInteger("gEnhancements.TempleSagesTravel", 1) ||
            ++state.ticks > 1800 ||
            (state.camera >= CAM_ID_SUB_FIRST && (!OwnCamera(play) || Play_GetActiveCamId(play) != state.camera)))
            return finish(Result::Abort);
        return Result::Pending;
    }
    if (!play || play != gPlayState || !SameVisit(play) || state.file != gSaveContext.fileNum ||
        state.game != gComboGameMode || state.mode != CVarGetInteger("gEnhancements.TempleSagesTravel", 1) ||
        !play->state.running || gGameState != &play->state || play->pauseCtx.state != 0 ||
        play->csCtx.state != CS_STATE_IDLE || play->gameOverCtx.state != GAMEOVER_INACTIVE ||
        play->transitionMode != TRANS_MODE_OFF || play->transitionTrigger != TRANS_TRIGGER_OFF ||
        gSaveContext.respawnFlag != 0 || GET_PLAYER(play) != state.player || !state.controlHeld ||
        !(GET_PLAYER(play)->stateFlags1 & PLAYER_STATE1_INPUT_DISABLED) || GET_PLAYER(play)->csAction != 0 ||
        gSaveContext.cutsceneTrigger || gSaveContext.gameMode != GAMEMODE_NORMAL ||
        (GET_PLAYER(play)->stateFlags1 &
         (PLAYER_STATE1_DEAD | PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_IN_CUTSCENE)) ||
        GET_PLAYER(play)->unk_6AD == 4 || ++state.ticks > 1800)
        return finish(Result::Abort);
    if (state.camera >= CAM_ID_SUB_FIRST && (!OwnCamera(play) || Play_GetActiveCamId(play) != state.camera))
        return finish(Result::Abort);
    const auto message = Message_GetState(&play->msgCtx);
    if (!OwnText(play))
        return finish(Result::Abort);
    if (play->state.input[0].press.button & BTN_B)
        return finish(Result::No);
    if (message == TEXT_STATE_CLOSING)
        return finish(Result::No);
    if (message == TEXT_STATE_NONE) {
        if (state.seenText || state.ticks > 15)
            return finish(Result::No);
        return Result::Pending;
    }
    state.seenText = true;
    // Advancement must be a new explicit A press at the native choice state.
    if (state.ticks > 1 && message == TEXT_STATE_CHOICE && (play->state.input[0].press.button & BTN_A) &&
        Message_ShouldAdvance(play)) {
        if (play->msgCtx.choiceIndex != 0)
            return finish(Result::No);
        Message_CloseTextbox(play);
        ReleaseControl(play);
        state.handoff = true;
        return Result::Yes;
    }
    return Result::Pending;
}
} // namespace TempleSages::ConfirmationNative
