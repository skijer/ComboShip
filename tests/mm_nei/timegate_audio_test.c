// Run complete production item and native SFX engine; pose/resource, input, and
// audio-device output are boundaries. Missing or incorrectly scoped stops fail.
#include "global.h"
#include "soh/_nei_compat_core.h"
#include "objects/gameplay_keep/gameplay_keep.h"
#define linearVelocity speedXZ
#include "mods/items/logic/item_time_gate.c"
#include <assert.h>
#include <stdio.h>
#include <string.h>

CustomItemState gCustomItemState;
AudioContext gAudioCtx;
RegEditor regEditor;
RegEditor* gRegEditor = &regEditor;
// Native channel layouts from code_8019AF00.c; only layout 0 is used.
u8 gChannelsPerBank[4][7] = { { 3, 2, 3, 3, 2, 1, 2 } };
u8 gUsedChannelsPerBank[4][7] = { { 3, 2, 3, 2, 2, 1, 1 } };
u8 gIsLargeSfxBank[7];
u8 D_801D6608[7];

static Player player;
static PlayState play;
static Camera camera;
static SequenceChannel channels[16];
static ItemInputState input;
static int toggles, magicSpent, damaged, choiceReady, stopCommands;
static Vec3f otherSource;

void AudioThread_QueueCmdS8(u32 opArgs, s8 data) {
    if ((opArgs >> 24) == 6 && (opArgs & 0xFF) == 0 && data == 0) {
        ++stopCommands;
    }
}
u32 AudioThread_NextRandom(void) {
    return 0;
}
void AudioSfx_SetProperties(u8 bank, u8 entry, u8 channel) {
}
void AudioSeq_SetVolumeScale(u8 playerIndex, u8 scale, u8 volume, u8 fade) {
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
void AdultLink_Toggle(void) {
    ++toggles;
}
void func_800AA000(f32 distance, u8 intensity, u8 timer, u8 step) {
}
void TimeGate_OpenPromptTextbox(void) {
}
void Message_CloseTextbox(PlayState* ps) {
}
u8 Message_GetState(MessageContext* ctx) {
    return choiceReady ? TEXT_STATE_CHOICE : TEXT_STATE_NONE;
}
bool Message_ShouldAdvance(PlayState* ps) {
    return choiceReady;
}
Camera* Play_GetCamera(PlayState* ps, s16 id) {
    return &camera;
}
s32 Camera_ChangeSetting(Camera* cam, s16 setting) {
    return 1;
}
void Camera_SetCameraData(Camera* cam, s16 flags, void* a, void* b, s16 c, s16 d) {
}
s16 Animation_GetLastFrame(void* animation) {
    return 12;
}
void PlayerAnimation_Change(PlayState* ps, SkelAnime* skel, PlayerAnimationHeader* anim, f32 speed, f32 first, f32 last,
                            u8 mode, f32 morph) {
    skel->curFrame = first;
    skel->endFrame = last;
}
s32 PlayerAnimation_Update(PlayState* ps, SkelAnime* skel) {
    skel->curFrame += 1;
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
void EffectSsGSpk_SpawnAccel(PlayState* ps, Actor* actor, Vec3f* pos, Vec3f* vel, Vec3f* accel, Color_RGBA8* prim,
                             Color_RGBA8* env, s16 scale, s16 step) {
}

static void PumpAudio(void) {
    AudioSfx_ProcessRequests();
    AudioSfx_ProcessActiveSfx();
}

static int HasSound(Vec3f* pos, u16 sound) {
    for (u8 index = gSfxBanks[SFX_BANK(sound)][0].next; index != 0xFF; index = gSfxBanks[SFX_BANK(sound)][index].next) {
        SfxBankEntry* entry = &gSfxBanks[SFX_BANK(sound)][index];
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

static void BeginCast(void) {
    input.wasEquipped = input.isPressed = 1;
    Tick();
    input.isPressed = 0;
    for (int i = 0; i < 200 && !tgPromptShown; ++i) {
        Tick();
    }
    assert(tgActive && tgPromptShown);
    assert(HasSound(&player.actor.world.pos, NA_SE_EV_WARP_HOLE));
    assert(HasSound(&player.actor.world.pos, NA_SE_PL_MAGIC_WIND_WARP));
}

static void AssertStopped(const char* exitName) {
    assert(!tgActive && !tgPortalActive);
    if (HasSound(&player.actor.world.pos, NA_SE_EV_WARP_HOLE) ||
        HasSound(&player.actor.world.pos, NA_SE_PL_MAGIC_WIND_WARP)) {
        fprintf(stderr, "FAIL: Time Gate sound survives %s\n", exitName);
        assert(0);
    }
    assert(HasSound(&otherSource, NA_SE_EV_WARP_HOLE));
    assert(HasSound(&otherSource, NA_SE_PL_MAGIC_WIND_WARP));
    assert(HasSound(&player.actor.world.pos, NA_SE_IT_SWORD_SWING));
    assert(stopCommands >= 2);
    printf("PASS: %s stops Time Gate sounds and preserves other sounds\n", exitName);
}

static void ResetFixture(void) {
    memset(&gCustomItemState, 0, sizeof(gCustomItemState));
    memset(&player, 0, sizeof(player));
    memset(&play, 0, sizeof(play));
    memset(&input, 0, sizeof(input));
    damaged = choiceReady = toggles = magicSpent = stopCommands = 0;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    R_UPDATE_RATE = 2;
    AudioSfx_Reset();
    gAudioCtx.seqPlayers[SEQ_PLAYER_SFX].enabled = 1;
    for (int i = 0; i < 16; ++i) {
        memset(&channels[i], 0, sizeof(channels[i]));
        gAudioCtx.seqPlayers[SEQ_PLAYER_SFX].channels[i] = &channels[i];
    }
}

static void StartOtherSounds(void) {
    AudioSfx_PlaySfx(NA_SE_EV_WARP_HOLE, &otherSource, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale,
                     &gSfxDefaultReverb);
    AudioSfx_PlaySfx(NA_SE_PL_MAGIC_WIND_WARP, &otherSource, 4, &gSfxDefaultFreqAndVolScale,
                     &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    AudioSfx_PlaySfx(NA_SE_IT_SWORD_SWING, &player.actor.world.pos, 4, &gSfxDefaultFreqAndVolScale,
                     &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    PumpAudio();
}

int main(void) {
    const char* exits[] = { "Yes", "No", "B", "timeout", "damage", "unequip" };
    for (int route = 0; route < 6; ++route) {
        ResetFixture();
        BeginCast();
        StartOtherSounds();
        if (route < 2) {
            choiceReady = 1;
            play.msgCtx.choiceIndex = route;
        } else if (route == 2) {
            play.state.input[0].press.button = BTN_B;
        } else if (route == 3) {
            tgTimer = 1200;
        } else if (route == 4) {
            damaged = 1;
        } else {
            input.wasEquipped = 0;
        }
        for (int i = 0; i < 40 && tgActive; ++i) {
            Tick();
        }
        AssertStopped(exits[route]);
        assert(toggles == (route == 0));
        assert(magicSpent == (route == 0 ? 48 : 0));

        // A later activation must still play, and must be independently stoppable.
        choiceReady = damaged = 0;
        play.state.input[0].press.button = 0;
        BeginCast();
        input.wasEquipped = 0;
        Tick();
        AssertStopped("repeat use");
    }

    // Cancellation can arrive before the audio thread processes the queued request.
    ResetFixture();
    input.wasEquipped = input.isPressed = 1;
    Tick();
    input.isPressed = 0;
    Handle_TimeGate(&player, &play); // deferred setup
    Handle_TimeGate(&player, &play); // first animation
    player.skelAnime.curFrame = TGATE_CAST_ITEM_FRAME;
    Handle_TimeGate(&player, &play);
    assert(tgPortalActive);
    input.wasEquipped = 0;
    Handle_TimeGate(&player, &play);
    PumpAudio();
    assert(!HasSound(&player.actor.world.pos, NA_SE_EV_WARP_HOLE));
    puts("PASS: cancellation purges pending Time Gate sound requests");
    return 0;
}
