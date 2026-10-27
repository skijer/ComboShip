#include "z64.h"
extern "C" {
#include "functions.h"
#include "variables.h"
#include "macros.h"
}
#include "mods/transformation_masks/transformation_masks.h"
#include "mods/sound_translator/mm_audio_sfx.h"
#include "mods/sound_translator/mm_sfx_ids.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <initializer_list>

static struct {
    MmPlayerTransformation currentForm;
    bool zoraSwimEnabled, barrierActive, barrierColliderInit, fastSwimActive;
    s16 barrierIntensity, magicDrainTimer;
    LightNode* barrierLight;
    LightInfo barrierLightInfo;
    ColliderCylinder barrierCollider;
    int swimState, swimPhaseCounter, swimExitFlag, goronAction;
} gFormState;
SaveContext gSaveContext;
static PlayState play;
static Player player;
static int starts[1024], refreshes[1024];
static int underwaterStops;
static u8 sAudioBaseFilter, sAudioBaseFilter2;
static bool paused;
static u32 sMmAudioFrame;
static int renderCalls, activePlayback = 1;
struct MmAudioScopedLock {};
#define MMFORM_IS_ZORA_SWIM() (gFormState.currentForm == MM_PLAYER_FORM_ZORA || gFormState.zoraSwimEnabled)

extern "C" void Audio_SetVolScale(u8, u8, u8, u8) {}
extern "C" void MmSfxDispatch_PlayEntry(u8, MmSfxBankEntry* entry, u8) { ++starts[entry->sfxId & 0x3ff]; }
extern "C" void MmSfxDispatch_RefreshEntry(u8, MmSfxBankEntry* entry, u8) { ++refreshes[entry->sfxId & 0x3ff]; }
extern "C" void MmSfxDispatch_StopEntry(u8, MmSfxBankEntry*, u8) {}
extern "C" s32 MmSfxDispatch_IsEntryActive(u8, MmSfxBankEntry*) { return activePlayback; }
extern "C" int MmSfx_IsGamePaused(void) { return paused; }
extern "C" void MmSfxSynth_RenderInto(short*, unsigned int) { ++renderCalls; }
extern "C" s32 MmSfx_IsAvailable(void) { return 1; }
extern "C" s32 MmSfx_PlayAtPos(u16 id, Vec3f* pos) {
    AudioMmSfx_PlaySfx(id, pos, 4, &gMmSfxDefaultFreqAndVolScale,
                      &gMmSfxDefaultFreqAndVolScale, &gMmSfxDefaultReverb);
    return 1;
}
extern "C" void MmSfx_Stop(u16 id) { AudioMmSfx_StopById(id); }
void Player_PlaySfx(Actor*, u16) {}
void Audio_StopSfxById(u32 id) { if (id == NA_SE_PL_IN_BUBBLE) ++underwaterStops; }
void Audio_PlaySfxGeneral(u16, Vec3f*, u8, f32*, f32*, s8*) {}
void LightContext_RemoveLight(PlayState*, LightContext*, LightNode*) {}
void Lights_PointNoGlowSetInfo(LightInfo*, s16, s16, s16, u8, u8, u8, s16) {}
s32 CollisionCheck_SetAT(PlayState*, CollisionCheckContext*, Collider*) { return 0; }
s32 Math_StepToS(s16* value, s16 target, s16 step) {
    if (*value < target) *value = (*value + step < target) ? *value + step : target;
    else if (*value > target) *value = (*value - step > target) ? *value - step : target;
    return *value == target;
}
f32 Math_SinS(s16 angle) { return sinf(angle * (3.14159265358979323846f / 32768)); }
f32 Math_CosS(s16 angle) { return cosf(angle * (3.14159265358979323846f / 32768)); }
#include "zora_audio.inc"
#ifndef ZORA_AUDIO_CLEANUP
static void MmForm_UpdateZoraSwimSfx(Player*) {}
#endif

static void audioTick() {
#ifdef ZORA_AUDIO_GAME_CLOCK
    MmSfx_UpdateGameFrame();
#endif
    short samples[16] = {};
    for (int i=0; i<3; ++i) MmDirectAudio_MixInto(samples, 8);
}
static void reset() {
    AudioMmSfx_Reset(); AudioMmSfx_ProcessRequests();
    memset(&gFormState, 0, sizeof(gFormState)); memset(&player, 0, sizeof(player));
    memset(starts, 0, sizeof(starts)); memset(refreshes, 0, sizeof(refreshes));
    gFormState.currentForm = MM_PLAYER_FORM_ZORA;
    sAudioBaseFilter = sAudioBaseFilter2 = 0; underwaterStops = 0;
    paused = false; renderCalls = 0; activePlayback = 1;
}

int main(int argc, char** argv) {
    assert(argc == 2); reset();
    if (!strcmp(argv[1], "barrier")) {
        gSaveContext.magic = 48; gSaveContext.magicState = MAGIC_STATE_IDLE;
        gFormState.barrierActive = true; gFormState.magicDrainTimer = 10;
        for (int i=0; i<20; ++i) { MmForm_UpdateBarrier(&player, &play); audioTick(); }
        assert(starts[MM_NA_SE_PL_ZORA_SPARK_BARRIER & 0x3ff] == 1 && "held shield must refresh one loop, not re-trigger it");
        assert(refreshes[MM_NA_SE_PL_ZORA_SPARK_BARRIER & 0x3ff] > 0);
        gFormState.barrierActive = false;
        for (int i=0; i<6; ++i) { MmForm_UpdateBarrier(&player, &play); audioTick(); }
        assert(!AudioMmSfx_IsPlaying(MM_NA_SE_PL_ZORA_SPARK_BARRIER - SFX_FLAG));
    } else if (!strcmp(argv[1], "swim")) {
        gFormState.swimState = 1; gFormState.fastSwimActive = true;
        gFormState.goronAction = MMFORM_ACT_SWIM_FAST; player.actor.yDistToWater = 150;
        for (int i=0; i<20; ++i) { MmForm_UpdateZoraSwimSfx(&player); audioTick(); }
        assert(starts[MM_NA_SE_PL_ZORA_SWIM_LV & 0x3ff] == 1 && "fast swim must refresh one loop");
        assert(refreshes[MM_NA_SE_PL_ZORA_SWIM_LV & 0x3ff] > 0);
        for (int action : {MMFORM_ACT_DOLPHIN_JUMP, MMFORM_ACT_SWIM_IDLE, MMFORM_ACT_OOT_ACTION}) {
            gFormState.goronAction = action; MmForm_UpdateZoraSwimSfx(&player); audioTick();
            assert(!AudioMmSfx_IsPlaying(MM_NA_SE_PL_ZORA_SWIM_LV - SFX_FLAG));
            gFormState.goronAction = MMFORM_ACT_SWIM_FAST; MmForm_UpdateZoraSwimSfx(&player); audioTick();
            assert(AudioMmSfx_IsPlaying(MM_NA_SE_PL_ZORA_SWIM_LV - SFX_FLAG));
        }
        gFormState.swimExitFlag = 1; MmForm_UpdateZoraSwimSfx(&player); audioTick();
        assert(!AudioMmSfx_IsPlaying(MM_NA_SE_PL_ZORA_SWIM_LV - SFX_FLAG));
    } else if (!strcmp(argv[1], "cadence")) {
        const u16 dash = MM_NA_SE_PL_ZORA_SWIM_DASH;
        MmSfx_PlayAtPos(dash, &player.actor.projectedPos); audioTick();
        assert(starts[dash & 0x3ff] == 1 && renderCalls == 3);
        activePlayback = 0; audioTick();
        assert(!AudioMmSfx_IsPlaying(dash) && renderCalls == 6);
        paused = true; audioTick(); assert(renderCalls == 6);
        paused = false; audioTick(); assert(renderCalls == 9);
    } else if (!strcmp(argv[1], "land")) {
        Audio_SetBaseFilter(0x20);
        player.stateFlags2 = PLAYER_STATE2_UNDERWATER | PLAYER_STATE2_DIVING;
        player.underwaterTimer = 77; player.actor.yDistToWater = -10;
        MmForm_UpdateZoraSwimSfx(&player);
        assert(underwaterStops == 1 && sAudioBaseFilter == 0 && player.underwaterTimer == 0);
        assert(!(player.stateFlags2 & (PLAYER_STATE2_UNDERWATER | PLAYER_STATE2_DIVING)));
        Audio_SetBaseFilter(0x20); player.actor.yDistToWater = 150;
        MmForm_UpdateZoraSwimSfx(&player);
        assert(sAudioBaseFilter == 0x20 && "still submerged: leave native ambience alone");
        gFormState.currentForm = MM_PLAYER_FORM_HUMAN; player.actor.yDistToWater = -10;
        MmForm_UpdateZoraSwimSfx(&player);
        assert(sAudioBaseFilter == 0x20 && "other forms do not own Zora water cleanup");
        gFormState.zoraSwimEnabled = true; MmForm_UpdateZoraSwimSfx(&player);
        assert(sAudioBaseFilter == 0 && underwaterStops == 2);
    }
    printf("PASS OoT Zora audio %s\n", argv[1]);
}
