// Production cap state machine + native SFX request/bank implementation.
// libsm64, audio-device output and projectile cleanup are boundary fixtures.
#include "global.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef TEST_MM
#define TEST_GAME "MM"
#define BANKS gSfxBanks
#define BANK(sfx) SFX_BANK(sfx)
#define RESET_AUDIO AudioSfx_Reset
#define PROCESS_REQUESTS AudioSfx_ProcessRequests
#define PLAY AudioSfx_PlaySfx
#define CONTEXT gAudioCtx
AudioContext gAudioCtx;
u8 D_801D6608[7];
#else
#define TEST_GAME "OoT"
#define BANKS gSoundBanks
#define BANK(sfx) SFX_BANK_SHIFT(sfx)
#define RESET_AUDIO Audio_ResetSounds
#define PROCESS_REQUESTS Audio_ProcessSoundRequests
#define PLAY Audio_PlaySoundGeneral
#define CONTEXT gAudioContext
AudioContext gAudioContext;
extern u8 sCurSfxPlayerChannelIdx;
#endif

u8 gChannelsPerBank[4][7] = { { 3, 2, 3, 3, 2, 1, 2 } };
u8 gUsedChannelsPerBank[4][7] = { { 3, 2, 3, 2, 2, 1, 1 } };
u8 gIsLargeSfxBank[7];
u8 gIsLargeSoundBank[7];
static SequenceChannel channels[16];
static Vec3f otherSource = { 1, 0, 0 };
static int musicStops;
static u16 music;
static s32 sSm64MarioId = 1;
static struct { u32 flags; u32 action; } sSm64OutState;
static void SetState(s32 id, u32 flags) { sSm64OutState.flags = flags; }
static void SetAction(s32 id, u32 action) { sSm64OutState.action = action; }
static void Interact(s32 id, u32 flag, u16 duration, u8 playMusic) { music = 0x040E; }
static void PlayMusic(u8 player, u16 seq, u16 fade) { music = seq; }
static void StopMusic(u16 seq) { assert(seq == music); music = 0; ++musicStops; }
static u16 GetMusic(void) { return music; }
static void (*p_sm64_set_mario_state)(s32, u32) = SetState;
static void (*p_sm64_set_mario_action)(s32, u32) = SetAction;
static void (*p_sm64_mario_interact_cap)(s32, u32, u16, u8) = Interact;
static void (*p_sm64_play_music)(u8, u16, u16) = PlayMusic;
static void (*p_sm64_stop_background_music)(u16) = StopMusic;
static u16 (*p_sm64_get_current_background_music)(void) = GetMusic;
static void Sm64Mario_KillAllFireballs(void) {}
static void Sm64Cappy_Kill(void) {}
void lusprintf(const char* file, int line, int level, const char* fmt, ...) {}

#ifdef TEST_MM
void AudioThread_QueueCmdS8(u32 op, s8 data) {}
u32 AudioThread_NextRandom(void) { return 0; }
void AudioSfx_SetProperties(u8 bank, u8 entry, u8 channel) {}
void AudioSeq_SetVolumeScale(u8 player, u8 scale, u8 volume, u8 fade) {}
void Audio_PlaySfx(u16 id) {
    PLAY(id, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
}
#else
void Audio_QueueCmdS8(u32 op, s8 data) {}
u32 Audio_NextRandom(void) { return 0; }
void Audio_SetSfxProperties(u8 bank, u8 entry, u8 channel) {}
void Audio_SetVolScale(u8 player, u8 scale, u8 volume, u8 fade) {}
u16 AudioEditor_GetReplacementSeq(u16 id) { return id; }
void AudioDebug_ScrPrt(const s8* str, u16 num) {}
void Sfx_PlaySfxCentered(u16 id) {
    PLAY(id, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
}
#endif

#include "cap_state.inc"

static void Pump(void) {
    PROCESS_REQUESTS();
#ifdef TEST_MM
    AudioSfx_ProcessActiveSfx();
#else
    sCurSfxPlayerChannelIdx = 0;
    for (u8 bank = 0; bank < 7; ++bank) {
        Audio_ChooseActiveSounds(bank);
        Audio_PlayActiveSounds(bank);
    }
#endif
}

static int HasSound(Vec3f* pos, u16 sfx) {
    for (u8 i = BANKS[BANK(sfx)][0].next; i != 0xFF; i = BANKS[BANK(sfx)][i].next) {
        if (BANKS[BANK(sfx)][i].posX == &pos->x && BANKS[BANK(sfx)][i].sfxId == sfx) return 1;
    }
    return 0;
}

static Vec3f* CapSource(u16 sfx) {
    for (u8 i = BANKS[BANK(sfx)][0].next; i != 0xFF; i = BANKS[BANK(sfx)][i].next) {
        if (BANKS[BANK(sfx)][i].sfxId == sfx && BANKS[BANK(sfx)][i].posX != &otherSource.x)
            return (Vec3f*)BANKS[BANK(sfx)][i].posX;
    }
    return NULL;
}

static void Reset(void) {
    RESET_AUDIO();
    CONTEXT.seqPlayers[SEQ_PLAYER_SFX].enabled = 1;
    for (int i = 0; i < 16; ++i) {
        memset(&channels[i], 0, sizeof(channels[i]));
        CONTEXT.seqPlayers[SEQ_PLAYER_SFX].channels[i] = &channels[i];
    }
    sCapStatesInited = 0;
    Sm64Caps_EnsureInit();
    musicStops = 0;
    music = 0;
    sSm64MarioId = 1;
}

static void StartOtherSounds(u16 sameSound) {
    PLAY(sameSound, &otherSource, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    PLAY(sameSound, &gSfxDefaultPos, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    PLAY(NA_SE_EV_WARP_HOLE, &otherSource, 4, &gSfxDefaultFreqAndVolScale, &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
    Pump();
}

int main(void) {
    const char* exits[] = { "toggle off", "switch cap", "expire", "suspend" };
    for (int n = 0; n < SM64_CAP_SLOT_COUNT; ++n) {
        int cap = (n + 1) % SM64_CAP_SLOT_COUNT; // Lead with the reported Metal loop.
        for (int route = 0; route < 4; ++route) {
            Reset();
            Sm64Caps_Press(cap);
            Pump();
            u16 sfx = kCapDefs[cap].sfx;
            Vec3f* source = CapSource(sfx);
            assert(source && HasSound(source, sfx));
            assert(source != &gSfxDefaultPos);
            StartOtherSounds(sfx);
            for (int tick = 0; tick < 4; ++tick) Sm64MarioCaps_Tick();
            if (route == 0) Sm64Caps_Press(cap);
            if (route == 1) Sm64Caps_Press(cap == SM64_CAP_SLOT_WING ? SM64_CAP_SLOT_METAL : SM64_CAP_SLOT_WING);
            if (route == 2) {
                sCapStates[cap].elapsed = kCapDefs[cap].activeDur - 1;
                Sm64MarioCaps_Tick();
            }
            if (route == 3) { sSm64MarioId = -1; Sm64MarioCaps_OnSuspend(); }
            Pump();
            if (HasSound(source, sfx)) {
                fprintf(stderr, "FAIL %s: %s sound survives %s\n", TEST_GAME, kCapDefs[cap].name, exits[route]);
                return 1;
            }
            assert(HasSound(&otherSource, sfx));
            assert(HasSound(&gSfxDefaultPos, sfx));
            assert(HasSound(&otherSource, NA_SE_EV_WARP_HOLE));
            assert(musicStops == 1);
            assert(sCapStates[cap].phase == SM64_CAP_PHASE_COOLDOWN);
            assert(sCapStates[cap].cooldownDur == (route == 2 ? kCapDefs[cap].maxCooldown :
                (s32)(((f32)4 / kCapDefs[cap].activeDur) * kCapDefs[cap].maxCooldown)));
            printf("PASS %s: %s %s; unrelated SFX and cooldown preserved\n", TEST_GAME, kCapDefs[cap].name, exits[route]);
        }
        // A toggle before the sound thread processes the request must cancel it too.
        Reset();
        Sm64Caps_Press(cap);
        Sm64Caps_Press(cap);
        Pump();
        assert(CapSource(kCapDefs[cap].sfx) == NULL);
        // Zero-tick toggle returns to READY, and a later activation remains audible.
        assert(sCapStates[cap].phase == SM64_CAP_PHASE_READY);
        Sm64Caps_Press(cap);
        Pump();
        assert(CapSource(kCapDefs[cap].sfx) != NULL);
        Sm64MarioCaps_OnSuspend();
        Pump();
        assert(CapSource(kCapDefs[cap].sfx) == NULL);
    }
    printf("PASS %s: queued cancellation, repeat use and suspend cleanup\n", TEST_GAME);
    return 0;
}
