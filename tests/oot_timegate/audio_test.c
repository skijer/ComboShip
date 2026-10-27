// Complete production item and native sound bank. Resource/pose, input, camera,
// scene transition and audio-device output are the fixture boundaries.
#include "global.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#include "mods/items/logic/item_time_gate.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>

CustomItemState gCustomItemState;
AudioContext gAudioContext;
static GameInfo gameInfo;
GameInfo* gGameInfo = &gameInfo;
// Native layout 0 from code_800F9280.c.
u8 gChannelsPerBank[4][7] = { { 3, 2, 3, 3, 2, 1, 2 } };
u8 gUsedChannelsPerBank[4][7] = { { 3, 2, 3, 2, 2, 1, 1 } };
u8 gIsLargeSoundBank[7];
extern u8 sCurSfxPlayerChannelIdx;

static Player player;
static PlayState play;
static Camera camera;
static SequenceChannel channels[16];
static ItemInputState input;
static int ageSwitches, magicSpent, damaged, choiceReady, stopCommands;
static Vec3f otherSource = { 1.0f, 0.0f, 0.0f };

void Audio_QueueCmdS8(u32 op, s8 data) {
    if ((op >> 24) == 6 && (op & 0xFF) == 0 && data == 0) {
        ++stopCommands;
    }
}
u32 Audio_NextRandom(void) {
    return 0;
}
void Audio_SetSfxProperties(u8 bank, u8 entry, u8 channel) {
}
void Audio_SetVolScale(u8 playerIndex, u8 scale, u8 volume, u8 fade) {
}
u16 AudioEditor_GetReplacementSeq(u16 id) {
    return id;
}
void AudioDebug_ScrPrt(const s8* str, u16 num) {
}
void lusprintf(const char* file, int line, int level, const char* format, ...) {
}
void ItemInput_Update(ItemInputState* out, u8 item, Player* p, PlayState* ps) {
    *out = input;
}
u8 ItemInput_IsBlocked(Player* p, PlayState* ps) {
    return 0;
}
u8 ItemInput_CheckDamage(Player* p, s8* previous) {
    return damaged;
}
s32 ItemMagic_HasEnough(PlayState* ps, s16 cost) {
    return 1;
}
void ItemMagic_Consume(PlayState* ps, s16 cost) {
    magicSpent += cost;
}
void SwitchAge(void) {
    ++ageSwitches;
}
void Rumble_Request(f32 distance, u8 intensity, u8 timer, u8 step) {
}
void Message_StartTextbox(PlayState* ps, u16 textId, Actor* actor) {
}
void Message_CloseTextbox(PlayState* ps) {
}
u8 Message_GetState(MessageContext* ctx) {
    return choiceReady ? TEXT_STATE_CHOICE : TEXT_STATE_NONE;
}
u8 Message_ShouldAdvance(PlayState* ps) {
    return choiceReady;
}
Camera* Play_GetCamera(PlayState* ps, s16 id) {
    return &camera;
}
s32 Camera_RequestSetting(Camera* cam, s16 setting) {
    return 1;
}
void Camera_SetCameraData(Camera* cam, s16 flags, void* a, void* b, s16 c, s16 d, UNK_TYPE e) {
}
s16 func_8005B1A4(Camera* cam) {
    return 0;
}
s16 Animation_GetLastFrame(void* animation) {
    return 12;
}
void LinkAnimation_Change(PlayState* ps, SkelAnime* skel, LinkAnimationHeader* anim, f32 speed, f32 first, f32 last,
                          u8 mode, f32 morph) {
    skel->curFrame = first;
    skel->endFrame = last;
}
s32 LinkAnimation_Update(PlayState* ps, SkelAnime* skel) {
    skel->curFrame += 1.0f;
    return 0;
}
void ItemVoice_PlayId(Player* p, u16 sound) {
}
f32 Rand_ZeroFloat(f32 value) {
    return 0;
}
f32 Rand_CenteredFloat(f32 value) {
    return 0;
}
void EffectSsKiraKira_SpawnFocused(PlayState* ps, Vec3f* pos, Vec3f* velocity, Vec3f* accel, Color_RGBA8* prim,
                                   Color_RGBA8* env, s16 scale, s32 life) {
}

static void PumpAudio(void) {
    Audio_ProcessSoundRequests();
    sCurSfxPlayerChannelIdx = 0;
    for (u8 bank = 0; bank < 7; ++bank) {
        Audio_ChooseActiveSounds(bank);
        Audio_PlayActiveSounds(bank);
    }
}

static int HasSound(Vec3f* pos, u16 sound) {
    u8 bank = SFX_BANK_SHIFT(sound);
    for (u8 index = gSoundBanks[bank][0].next; index != 0xFF; index = gSoundBanks[bank][index].next) {
        SoundBankEntry* entry = &gSoundBanks[bank][index];
        if (entry->posX == &pos->x && entry->sfxId == sound) {
            return 1;
        }
    }
    return 0;
}

static void Tick(void) {
    Handle_TimeGate(&player, &play);
    PumpAudio();
    ++play.gameplayFrames;
}

static void ResetFixture(void) {
    memset(&gCustomItemState, 0, sizeof(gCustomItemState));
    memset(&player, 0, sizeof(player));
    memset(&play, 0, sizeof(play));
    memset(&input, 0, sizeof(input));
    damaged = choiceReady = ageSwitches = magicSpent = stopCommands = 0;
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    R_UPDATE_RATE = 2;
    Audio_ResetSounds();
    gAudioContext.seqPlayers[SEQ_PLAYER_SFX].enabled = 1;
    for (int i = 0; i < 16; ++i) {
        memset(&channels[i], 0, sizeof(channels[i]));
        gAudioContext.seqPlayers[SEQ_PLAYER_SFX].channels[i] = &channels[i];
    }
}

static void BeginCast(void) {
    input.wasEquipped = input.isPressed = 1;
    Tick();
    input.isPressed = 0;
    for (int i = 0; i < 200 && !tgPromptShown; ++i) {
        Tick();
    }
    assert(tgActive && tgState == TGATE_STATE_HOVERING && tgPromptShown);
    assert(tgItemVisible && tgPortalActive);
    assert(HasSound(&player.actor.world.pos, NA_SE_EV_WARP_HOLE));
    assert(HasSound(&player.actor.world.pos, NA_SE_PL_MAGIC_WIND_WARP));
    assert(!magicSpent && !ageSwitches);
}

static void QueueOtherSounds(void) {
    Audio_PlaySoundGeneral(NA_SE_EV_WARP_HOLE, &otherSource, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    Audio_PlaySoundGeneral(NA_SE_PL_MAGIC_WIND_WARP, &otherSource, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    Audio_PlaySoundGeneral(NA_SE_IT_SWORD_SWING, &player.actor.world.pos, 4, &gSfxDefaultFreqAndVolScale,
                           &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
}

static void AssertStopped(const char* exitName) {
    assert(!tgActive && tgState == TGATE_STATE_IDLE && !tgPromptShown);
    assert(!tgItemVisible && !tgPortalActive && tgPortalAlpha == 0.0f && tgPortalScale == 0.0f);
    assert(!(player.stateFlags1 & (PLAYER_STATE1_IN_ITEM_CS | PLAYER_STATE1_INPUT_DISABLED)));
    if (HasSound(&player.actor.world.pos, NA_SE_EV_WARP_HOLE) ||
        HasSound(&player.actor.world.pos, NA_SE_PL_MAGIC_WIND_WARP)) {
        fprintf(stderr, "FAIL: OoT Time Gate sound survives %s\n", exitName);
        assert(0);
    }
    assert(HasSound(&otherSource, NA_SE_EV_WARP_HOLE));
    assert(HasSound(&otherSource, NA_SE_PL_MAGIC_WIND_WARP));
    assert(HasSound(&player.actor.world.pos, NA_SE_IT_SWORD_SWING));
    printf("PASS: OoT %s stops Time Gate sounds and preserves unrelated sounds\n", exitName);
}

int main(void) {
    const char* exits[] = { "Yes", "No", "B then No", "long hover then No", "damage", "unequip", "invalid state" };
    for (int route = 0; route < 7; ++route) {
        ResetFixture();
        BeginCast();
        QueueOtherSounds();
        PumpAudio();
        if (route == 2 || route == 3) {
            // OoT delegates A/B to the textbox and has no MM hover timeout.
            // These controls must leave the prompt and spell audio intact.
            if (route == 2) {
                play.state.input[0].press.button = BTN_B;
                input.otherButtonPressed = 1;
            } else {
                tgTimer = 1200;
            }
            Tick();
            assert(tgActive && tgState == TGATE_STATE_HOVERING && tgPromptShown && tgItemVisible);
            assert(HasSound(&player.actor.world.pos, NA_SE_EV_WARP_HOLE));
            assert(HasSound(&player.actor.world.pos, NA_SE_PL_MAGIC_WIND_WARP));
            assert(!magicSpent && !ageSwitches);
        }
        if (route < 4) {
            choiceReady = 1;
            play.msgCtx.choiceIndex = (route == 0) ? 0 : 1;
            Tick();
            assert(tgActive && !tgItemVisible);
            assert(tgState == ((route == 0) ? TGATE_STATE_SWITCHING : TGATE_STATE_CANCEL));
            assert(!magicSpent && !ageSwitches);
        } else if (route == 4) {
            damaged = 1;
        } else if (route == 5) {
            input.wasEquipped = 0;
        } else {
            tgState = 255;
        }
        for (int i = 0; i < 40 && tgActive; ++i) {
            Tick();
        }
        AssertStopped(exits[route]);
        assert(stopCommands == 2);
        assert(ageSwitches == (route == 0));
        assert(magicSpent == ((route == 0) ? 48 : 0));
        assert(HasSound(&player.actor.world.pos, NA_SE_SY_WHITE_OUT_T) == (route == 0));

        // A later activation remains audible and independently stoppable.
        choiceReady = damaged = 0;
        input.otherButtonPressed = 0;
        play.state.input[0].press.button = 0;
        magicSpent = ageSwitches = 0;
        BeginCast();
        input.wasEquipped = 0;
        Tick();
        AssertStopped("repeat use");
        assert(!magicSpent && !ageSwitches);
    }

    // Terminate while both requests are still waiting for the audio thread.
    ResetFixture();
    input.wasEquipped = input.isPressed = 1;
    Handle_TimeGate(&player, &play);
    input.isPressed = 0;
    for (int i = 0; i < 200 && !tgPromptShown; ++i) {
        Handle_TimeGate(&player, &play);
    }
    assert(tgPromptShown && tgPortalActive);
    QueueOtherSounds();
    input.wasEquipped = 0;
    Handle_TimeGate(&player, &play);
    PumpAudio();
    AssertStopped("pending-request cancellation");
    assert(stopCommands == 0 && !magicSpent && !ageSwitches);
    return 0;
}
