// Compile the complete production updater, real ShipInit registration and the
// real sequence-command queue. Only game/resources and the audio thread are
// boundary fixtures; a command receipt is not proof of audible playback.
#include "test_require.h"
#include <cstring>
#include <vector>
#include "soh/Enhancements/audio/AudioCollection.h"
#include "soh/Enhancements/audio/HyruleFieldNightMusicInternal.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/ShipInit.hpp"
extern "C" {
#include "functions.h"
#include "variables.h"
AudioContext gAudioContext = {};
NightTestSaveContext gSaveContext = {};
PlayState* gPlayState = nullptr;
u8 gSfxChannelLayout = 0;
s8 D_80133390[] = "SEQ H", D_80133398[] = "    L";
}

static bool enabled = true;
static uint16_t nightSelection = 0x15D;
static int previewSequence = 0;
static std::vector<u32> commands;
static GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;
AudioCollection::AudioCollection() = default;
static AudioCollection collection;
AudioCollection* AudioCollection::Instance = &collection;
bool AudioCollection::HasSequenceNum(uint16_t id) {
    return id == 0xFF02 || id == 0x15D || id == 0x1C6 || id == NA_BGM_FIELD_LOGIC || id == NA_BGM_KAKARIKO_ADULT;
}
uint16_t AudioCollection::GetReplacementSequence(uint16_t id) {
    return id == 0xFF02 ? nightSelection : id;
}

extern "C" {
int CVarGetInteger(const char* name, int fallback) {
    if (std::strcmp(name, "gAudioEditor.HyruleFieldNightMusic") == 0)
        return enabled;
    if (std::strcmp(name, "gAudioEditor.HyruleFieldNightSequence") == 0)
        return nightSelection;
    if (std::strcmp(name, "gAudioEditor.Playing") == 0)
        return previewSequence;
    return fallback;
}
void CVarSetInteger(const char* name, int value) {
    if (std::strcmp(name, "gAudioEditor.Playing") == 0)
        previewSequence = value;
}
#ifndef NIGHT_COMBAT_RUNTIME_TEST
void Audio_CancelPreviewRestore(void) {
}
#endif
int Player_InCsMode(PlayState* play) {
    return play->csCtx.state != CS_STATE_IDLE;
}
u16 AudioEditor_GetReplacementSeq(u16 id) {
    return id;
}
void Audio_QueueCmdS32(u32 command, s32) {
    commands.push_back(command);
}
void Audio_QueueCmdF32(u32, f32) {
}
void Audio_QueueCmdS8(u32, s8) {
}
void Audio_QueueCmdU16(u32, u16) {
}
void AudioDebug_ScrPrt(const s8*, u16) {
}
void func_800E5F88(u8) {
}
void func_800F71BC(u8) {
}
s32 func_800E5E20(u32*) {
    return 0;
}
s32 func_800E5EDC(void) {
    return 1;
}
void func_800F7170(void) {
}
}

#ifdef NIGHT_EDITOR_REFRESH_TEST
#include "night_editor_replay.inc"
#endif

static void Frame() {
    for (auto& hook : GameInteractor::Hooks<GameInteractor::OnGameFrameUpdate>())
        hook();
    Audio_ProcessSeqCmds();
}
static void SetPlayer(int player, u16 id) {
    gAudioContext.seqPlayers[player].enabled = id != 0xFFFF;
    gActiveSeqs[player].seqId = id;
}
static void Reset(PlayState& play) {
    Audio_ProcessSeqCmds();
    HyruleFieldNightMusic_Reset();
    commands.clear();
    enabled = true;
    nightSelection = 0x15D;
    previewSequence = 0;
    gSaveContext.nightFlag = 1;
    play = { SCENE_HYRULE_FIELD, { CS_STATE_IDLE } };
    gPlayState = &play;
    for (int i = 0; i < 4; ++i)
        SetPlayer(i, 0xFFFF);
    SetPlayer(0, NA_BGM_FIELD_LOGIC);
    gAudioContext.audioBufferParameters.updatesPerFrame = 4;
}
static void StartNight(PlayState& play) {
    Reset(play);
    Frame();
    REQUIRE((commands == std::vector<u32>{ 0x8200015D }));
    SetPlayer(0, 0x5D); // Audio thread has accepted the full-width sequence.
    commands.clear();
    Frame();
    REQUIRE(commands.empty());
}

#ifdef NIGHT_COMBAT_RUNTIME_TEST
#include "night_combat_fixture.h"
#endif

int main() {
    ShipInit::InitAll();
    PlayState play;
    // Rebinding the CVar must not multiply frame callbacks/start commands.
    ShipInit::Init("gAudioEditor.HyruleFieldNightMusic");
    ShipInit::Init("gAudioEditor.HyruleFieldNightSequence");
    StartNight(play);

    // Dawn and disable replace MAIN once; no stop of the new sequence.
    gSaveContext.nightFlag = 0;
    Frame();
    REQUIRE((commands == std::vector<u32>{ 0x82000002 }));
    StartNight(play);
    enabled = false;
    Frame();
    REQUIRE((commands == std::vector<u32>{ 0x82000002 }));

    // Disabling during a fanfare defers restoration, not loses ownership.
    StartNight(play);
    enabled = false;
    SetPlayer(1, 0x20);
    Frame();
    REQUIRE(commands.empty());
    SetPlayer(1, 0xFFFF);
    Frame();
    REQUIRE((commands == std::vector<u32>{ 0x82000002 }));

    // Explicit overrides also retain ownership until the override releases.
    for (int overrideKind = 0; overrideKind < 3; ++overrideKind) {
        StartNight(play);
        enabled = false;
        if (overrideKind == 0)
            play.csCtx.state = CS_STATE_IDLE + 1;
        if (overrideKind == 1)
            SetPlayer(SEQ_PLAYER_BGM_SUB, 0x20);
        if (overrideKind == 2)
            SetPlayer(SEQ_PLAYER_BGM_MAIN, 0x20);
        Frame();
        REQUIRE(commands.empty());
        play.csCtx.state = CS_STATE_IDLE;
        SetPlayer(SEQ_PLAYER_BGM_SUB, 0xFFFF);
        SetPlayer(SEQ_PLAYER_BGM_MAIN, NA_BGM_FIELD_LOGIC);
        Frame();
        REQUIRE((commands == std::vector<u32>{ 0x82000002 }));
    }

    // Destination scene owns MAIN, even if its ID aliases the night ID.
    StartNight(play);
    play.sceneNum = 0x20;
    SetPlayer(0, 0x5D);
    Frame();
    REQUIRE(commands.empty());
    SetPlayer(0, 0x1D); // child Market
    Frame();
    REQUIRE(commands.empty());
    REQUIRE(func_800FA0B4(0) == 0x1D);

    // A selected 16-bit ID ending in FF must not match disabled MAIN.
    REQUIRE(!HyruleFieldNightMusic_IsNightSequencePlaying(true, 0xFFFF, 0x1FF));
#ifdef NIGHT_EDITOR_REFRESH_TEST
    // An editor/automatic reroll must refresh the owned exact bank instead of
    // replaying MAIN's truncated native bookkeeping ID or its stale cache.
    StartNight(play);
    nightSelection = 0x1C6;
    ReplayCurrentBGM();
    Frame();
    REQUIRE((commands == std::vector<u32>{ 0x820001C6 }));
    SetPlayer(0, 0xC6);
    commands.clear();
    Frame();
    REQUIRE(commands.empty());
    // Hold a reroll until each independent native override releases. Preview
    // shares the explicit-override gate with cutscenes; fanfare uses its own.
    for (int block : { 0, 1, 2 }) {
        StartNight(play);
        nightSelection = 0x1C6;
        if (block == 0)
            SetPlayer(SEQ_PLAYER_FANFARE, 0x20);
        if (block == 1)
            play.csCtx.state = CS_STATE_IDLE + 1;
        if (block == 2)
            previewSequence = 0x1234;
        ReplayCurrentBGM();
        Frame();
        REQUIRE(commands.empty());
        SetPlayer(SEQ_PLAYER_FANFARE, 0xFFFF);
        play.csCtx.state = CS_STATE_IDLE;
        previewSequence = 0;
        Frame();
        REQUIRE((commands == std::vector<u32>{ 0x820001C6 }));
    }
    StartNight(play);
    SetPlayer(SEQ_PLAYER_BGM_MAIN, 0x20); // A MAIN cutscene/event override.
    nightSelection = 0x1C6;
    UpdateCurrentBGM(0xFF02, SEQ_BGM_WORLD);
    Frame();
    REQUIRE(commands.empty());
    SetPlayer(SEQ_PLAYER_BGM_MAIN, NA_BGM_FIELD_LOGIC);
    Frame();
    REQUIRE((commands == std::vector<u32>{ 0x820001C6 }));
    // Standalone SoH also clears transient audition state on actual Play
    // destruction; a stale Playing CVar cannot suppress the next night start.
    StartNight(play);
    previewSequence = 0x35;
    for (auto& callback : GameInteractor::Hooks<GameInteractor::OnPlayDestroy>())
        callback();
    SetPlayer(SEQ_PLAYER_BGM_MAIN, NA_BGM_FIELD_LOGIC);
    Frame();
    REQUIRE((commands == std::vector<u32>{ 0x8200015D }));
    // Full-width explicit ownership wins even when its bookkeeping low byte
    // aliases the cached night bank. A pending edit waits for its release.
    StartNight(play);
    Audio_QueueResolvedSeqCmd(SEQ_PLAYER_BGM_MAIN, 0x25D, 0);
    Audio_ProcessSeqCmds();
    commands.clear();
    nightSelection = 0x1C6;
    UpdateCurrentBGM(0xFF02, SEQ_BGM_WORLD);
    Frame();
    REQUIRE(commands.empty());
    Audio_QueueResolvedSeqCmd(SEQ_PLAYER_BGM_MAIN, NA_BGM_FIELD_LOGIC, 0);
    Audio_ProcessSeqCmds();
    commands.clear();
    Frame();
    REQUIRE((commands == std::vector<u32>{ 0x820001C6 }));
#endif
#ifdef NIGHT_COMBAT_RUNTIME_TEST
    TestCombat(play);
#endif
}
