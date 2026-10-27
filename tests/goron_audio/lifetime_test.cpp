#include "z64.h"
#include "functions.h"
#include "variables.h"
#include "macros.h"
#include "din_fire_sword.h"
#include "mods/transformation_masks/transformation_masks.h"
#include "mods/sound_translator/mm_audio_sfx.h"
#include "mods/sound_translator/mm_sfx_ids.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <sanitizer/asan_interface.h>

// Only sound synthesis/output and the hook registry are service boundaries.
// The bank, dispatcher, stop bodies and lifetime entrypoints are real.
struct MmAudioScopedLock {};
#include "goron_audio_types.inc"
static MmPlayingSound sPlayingSounds[MM_DIRECT_MAX_SOUNDS];

class GameInteractor {
  public:
    struct ShouldActorDestroy {};
    static GameInteractor* Instance;
    std::map<int, std::function<void(void*, bool*)>> destroyHooks;
    template <typename Hook> void RegisterGameHookForID(int id, std::function<void(void*, bool*)> callback) {
        destroyHooks[id] = callback;
    }
};
static GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;

SaveContext gSaveContext;
PlayState* gPlayState;
u16 gEquipMasks[4];
u8 gEquipShifts[4];
static PlayState play;
static Player player;
static int finishForgetCalls;
// The elemental finish observer is a boundary; its implementation has its own
// native tests. Keep the actual Actor_Destroy body and verify its handoff here.
extern "C" void DinFireSword_FinishForget(PlayState* state, Actor* actor) {
    assert(state == &play && actor == &player.actor);
    ++finishForgetCalls;
}
static struct {
    int state, currentForm, targetForm, initialized, skeletonLoaded, goronAction;
    int cutsceneTimer, cutscenePhase, flashAlpha, zoraSwimEnabled, groundPoundCrackTimer;
    int zoraMeleeActive, zoraJumpTrailActive, hazardVoidType;
    void* savedAgeProperties;
    u8 savedTunic, savedTunicEquip;
} gFormState;
static int sFleetPendingForm = -1, sPendingSoftReload, sPendingReactivate, sForceInstantTransform;
static MmPlayerTransformation sPendingReactivateForm = MM_PLAYER_FORM_HUMAN;
static bool sEquipsSaved;
static decltype(gSaveContext.equips) sPreTransformEquips;
static bool available = true;
static bool actionExits;
static u16 dispatchedSfx;
static bool synthReady;
static signed char channelIO[16][8];
static u16 channelSfx[16];
// Native mask pose/render ownership is exercised by native_transformations;
// this fixture keeps the real bank/dispatcher and Goron lifetime entrypoints.
static void MmForm_InterruptNativeMaskForScene(Player*, PlayState*, u8) {}
static void MmForm_CancelNativeMaskCutscene(u8) {}

extern "C" void Audio_SetVolScale(u8, u8, u8, u8) {
}
extern "C" s32 MmSfx_IsAvailable(void) {
    return available;
}
extern "C" void lusprintf(const char*, int32_t, int32_t, const char*, ...) {
}
extern "C" s32 MmSfxBridge_Play(u16 id, f32, Vec3f*) {
    for (auto& sound : sPlayingSounds) {
        if (!sound.active) {
            sound.mmSfxId = id;
            sound.active = 1;
            sound.envPhase = 2;
            sound.envVolume = 1.0f;
            return 1;
        }
    }
    assert(false && "test audio sink full");
    return 0;
}
extern "C" void MmSfxBridge_RefreshProperties(u16, Vec3f*, f32) {
}
extern "C" void MmSfxBridge_StopByMmSfxId(u16) {
}
extern "C" s32 MmSfxBridge_IsActive(u16 id) {
    for (const auto& sound : sPlayingSounds)
        if (sound.active && sound.mmSfxId == id)
            return 1;
    return 0;
}
extern "C" int MmSfxSynth_IsReady() {
    return synthReady;
}
extern "C" void MmSfxSynth_SetChannelState(int, float, float, signed char, signed char) {
}
extern "C" int MmSfxSynth_ReadChannelIO(int channel, int port) {
    return channelIO[channel][port];
}
extern "C" void MmSfxSynth_WriteChannelIO(int channel, int port, signed char value) {
    channelIO[channel][port] = value;
    if (port == 4) {
        for (int bank = 0; bank < 7; ++bank) {
            for (u8 index = gMmSfxBanks[bank][0].next; index != 0xFF; index = gMmSfxBanks[bank][index].next) {
                const auto& entry = gMmSfxBanks[bank][index];
                if (entry.channelIndex == channel && (entry.sfxId & 0xFF) == static_cast<u8>(value)) {
                    channelSfx[channel] = entry.sfxId;
                }
            }
        }
    }
}

static void queue(u16 id) {
    AudioMmSfx_PlaySfx(id, &player.actor.projectedPos, 4, &gMmSfxDefaultFreqAndVolScale, &gMmSfxDefaultFreqAndVolScale,
                       &gMmSfxDefaultReverb);
}
static void tick() {
    AudioMmSfx_ProcessRequests();
    AudioMmSfx_ProcessActiveSfx();
}
static void FormJingle_Update() {
}
static int CVarGetInteger(const char*, int fallback) {
    return fallback;
}
static u8 MmForm_IsEnabled() {
    return 1;
}
static u8 MmForm_SoftReload(PlayState*, Player*, MmPlayerTransformation) {
    return 1;
}
static void MmForm_RestoreMeleeColliders(Player*) {
}
static void MmForm_UpdateBlink() {
}
static void MmForm_CheckBarrierInput(Player*, PlayState*) {
}
static void MmForm_UpdateBarrier(Player*, PlayState*) {
}
static void MmForm_UpdateSkinOcarinaVoice(Player*, PlayState*) {
}
static void MmForm_UpdateTransforming(Player*, PlayState*) {
}
static void MmForm_FDSkinSpeedBoost(Player*, PlayState*) {
}
static void KafeiForm_Tick(Player*, PlayState*) {
}
static void MmForm_UpdateActive(Player*, PlayState*) {
    if (dispatchedSfx)
        queue(dispatchedSfx);
    if (actionExits)
        gFormState.goronAction = 0; // e.g. textbox or native action takes control
}
static void MmForm_UpdateDetransforming(Player*, PlayState*) {
}
static void MmForm_UpdateZoraMelee(Player*, PlayState*) {
}
static void MmForm_UpdateZoraSwimTrails(PlayState*) {
}
static void MmForm_UpdateZoraSwimSfx(Player*) {
}
static void MmForm_ClearZoraSwimTrails(PlayState*) {
}
static void MmForm_RitoWindClear() {
}
static void ExtEquip_RestoreFromTransform() {
}
static void NameTag_RemoveAllForActor(Actor*) {
}
bool GameInteractor_ShouldActorDestroy(void* ref) {
    auto* actor = static_cast<Actor*>(ref);
    bool result = true;
    auto hook = interactor.destroyHooks.find(actor->id);
    if (hook != interactor.destroyHooks.end())
        hook->second(ref, &result);
    return result;
}
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
#define MMFORM_LOG(...) ((void)0)
#include "goron_audio.inc"

static bool audible(u16 id) {
    if (synthReady) {
        for (int channel = 0; channel < 16; ++channel)
            if (channelSfx[channel] == id && channelIO[channel][0] != 0)
                return true;
        return false;
    }
    for (const auto& sound : sPlayingSounds)
        if (sound.active && sound.mmSfxId == id)
            return true;
    return false;
}

static void reset() {
    memset(&play, 0, sizeof(play));
    memset(&player, 0, sizeof(player));
    memset(&gFormState, 0, sizeof(gFormState));
    memset(sPlayingSounds, 0, sizeof(sPlayingSounds));
    memset(channelIO, 0, sizeof(channelIO));
    memset(channelSfx, 0, sizeof(channelSfx));
    AudioMmSfx_Reset();
    AudioMmSfx_ProcessRequests();
    gFormState.state = MMFORM_STATE_ACTIVE;
    gFormState.currentForm = MM_PLAYER_FORM_GORON;
    gFormState.initialized = 1;
    gFormState.goronAction = GORON_ACT_GORON_ROLL;
    play.roomCtx.curRoom.num = 0;
    gPlayState = nullptr;
    player.actor.id = ACTOR_PLAYER;
    actionExits = false;
    finishForgetCalls = 0;
    dispatchedSfx = 0;
    available = true;
    sPendingSoftReload = sPendingReactivate = sForceInstantTransform = 0;
    MmForm_Update(&play, &player); // establish this room before playback
    tick();
}

static void destroy(Actor*, PlayState*) {
}

int main(int argc, char** argv) {
    assert(argc == 3);
    const char* scenario = argv[1];
    synthReady = !strcmp(argv[2], "sequence");
    MmForm_StopGoronRollSfx(); // cold stop must return before the bank is initialized
    if (!strcmp(scenario, "transformation-cues")) {
        const u16 cues[] = { MM_NA_SE_PL_PUT_OUT_ITEM, MM_NA_SE_IT_SET_TRANSFORM_MASK,
            MM_NA_SE_PL_FREEZE_S, MM_NA_SE_IT_TRANSFORM_MASK_BROKEN, MM_NA_SE_PL_TRANSFORM_VOICE,
            MM_NA_SE_PL_FACE_CHANGE, MM_NA_SE_EV_LIGHTNING_HARD, MM_NA_SE_SY_TRANSFORM_MASK_FLASH };
        for (u16 cue : cues) {
            for (bool active : {false, true}) {
                reset(); queue(cue);
                if (active) {
                    tick();
                    assert(AudioMmSfx_IsPlaying(cue) && audible(cue) && "native cue must reach the real MM dispatcher");
                }
                MmForm_StopNativeMaskSfx();
                // Play_Destroy frees the player arena after native cleanup.
                // Neither pending requests nor live voices may read it again.
                __asan_poison_memory_region(&player.actor.projectedPos, sizeof(player.actor.projectedPos));
                tick();
                __asan_unpoison_memory_region(&player.actor.projectedPos, sizeof(player.actor.projectedPos));
                assert(!AudioMmSfx_IsPlaying(cue) && !audible(cue) && "teardown must stop queued and active native cues");
            }
        }
        reset();
        queue(MM_NA_SE_PL_TRANSFORM_VOICE);
        MmForm_StopNativeMaskSfx(false);
        queue(MM_NA_SE_SY_TRANSFORM_MASK_FLASH);
        tick();
        assert(!AudioMmSfx_IsPlaying(MM_NA_SE_PL_TRANSFORM_VOICE));
        assert(AudioMmSfx_IsPlaying(MM_NA_SE_SY_TRANSFORM_MASK_FLASH) && audible(MM_NA_SE_SY_TRANSFORM_MASK_FLASH) &&
               "skip cancellation must allow the new same-frame flash cue");
        printf("PASS transformation SFX bank/%s: 8 cues, queued/active cancellation and freed-position safety\n", argv[2]);
        return 0;
    }
    const u16 ids[] = { 0x0980, 0x098F, 0x08EB, 0x09AD, 0x0990, 0x099F };
    constexpr u16 unrelated = MM_NA_SE_IT_DEKUNUTS_FLOWER_CLOSE;
    for (u16 id : ids) {
        reset();
        // Init installs the destruction hook; only its pre-asset cleanup is needed.
        if (!strcmp(scenario, "destroy")) {
            MmForm_Init(&play, &player);
            tick(); // a later roll starts after Init's stop-guard drain
        }
        queue(id);
        queue(unrelated);
        if (strcmp(scenario, "queued")) {
            tick();
            assert(AudioMmSfx_IsPlaying(id) && audible(id));
        }
        if (!strcmp(scenario, "scene-init"))
            MmForm_Init(&play, &player);
        else if (!strcmp(scenario, "reset"))
            MmForm_Reset();
        else if (!strcmp(scenario, "death"))
            MmForm_OnDeath();
        else if (!strcmp(scenario, "destroy")) {
            player.actor.destroy = destroy;
            Actor_Destroy(&player.actor, &play);
            assert(finishForgetCalls == 1 && "actor destruction must notify the elemental finish observer");
        } else if (!strcmp(scenario, "unavailable")) {
            available = false;
            MmForm_StopGoronRollSfx();
        } else {
            if (!strcmp(scenario, "void"))
                gFormState.goronAction = MMFORM_ACT_WATER_VOID;
            else if (!strcmp(scenario, "transition-trigger") || !strcmp(scenario, "queued")) {
                play.transitionTrigger = TRANS_TRIGGER_START;
                dispatchedSfx = id; // a last same-frame roll request must be purged too
            } else if (!strcmp(scenario, "transition-mode"))
                play.transitionMode = TRANS_MODE_SETUP;
            else if (!strcmp(scenario, "room"))
                play.roomCtx.curRoom.num = 1;
            else if (!strcmp(scenario, "action-exit"))
                actionExits = true;
            else if (!strcmp(scenario, "form-exit"))
                gFormState.state = MMFORM_STATE_DETRANSFORMING;
            else
                assert(!strcmp(scenario, "rolling"));
            MmForm_Update(&play, &player);
        }
        tick();
        if (!strcmp(scenario, "rolling")) {
            assert(AudioMmSfx_IsPlaying(id) && audible(id) && "active rolling must keep its sound");
            for (int action : { GORON_ACT_GORON_ROLL_JUMP, GORON_ACT_GORON_ROLL_POUND }) {
                gFormState.goronAction = action;
                MmForm_Update(&play, &player);
                tick();
                assert(AudioMmSfx_IsPlaying(id) && audible(id) && "airborne ball states keep their sound");
            }
        } else {
            if (AudioMmSfx_IsPlaying(id) || audible(id)) {
                fprintf(stderr, "FAIL %s: stale Goron SFX 0x%04X survived ownership exit\n", scenario, id);
                return 1;
            }
            // The next room/life may start another roll after its stop-guard drain.
            queue(id);
            tick();
            assert(AudioMmSfx_IsPlaying(id) && audible(id) && "later roll must be able to replay");
        }
        assert(AudioMmSfx_IsPlaying(unrelated) && audible(unrelated) && "preserve unrelated MM sound");
    }
    printf("PASS Goron audio %s/%s (6 normal/ice/charge/slip IDs)\n", argv[2], scenario);
}
