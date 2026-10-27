// Run the full production Wolf procs/update at the native audio boundary.
// Asset rendering and the audio device are fixture boundaries; routing, motion
// eligibility, cadence, volume and teardown are the production implementation.
#ifdef WOLF_AUDIO_MM
#include "2s2h/GameInteractor/GameInteractor.h"
#else
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"
#endif
#include WOLF_IMPLEMENTATION
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <atomic>
#include <limits>
#include <thread>

static std::vector<u16> playerEvents;
struct Request {
    u16 id;
    Vec3f* pos;
    f32* frequency;
    f32* volume;
};
static std::vector<Request> requests;
static std::vector<std::pair<Vec3f*, u16>> stops;
static std::map<std::string, f32> audioCvars;
static std::map<std::string, s32> audioIntegerCvars;
#ifndef WOLF_AUDIO_MM
static GameInteractor fixtureInteractor;
GameInteractor* GameInteractor::Instance = &fixtureInteractor;
#endif

extern "C" {
SaveContext gSaveContext{};
PlayState* gPlayState;
#ifdef WOLF_AUDIO_MM
static RegEditor registers{};
RegEditor* gRegEditor = &registers;
int GameInteractor_InvertControl(GIInvertType) { return 1; }
s16 Math_Atan2S_XY(f32 x, f32 y) { return std::atan2(y, x) * 32768 / 3.14159265358979323846; }
void Player_PlaySfx(Player*, u16 id) { playerEvents.push_back(id); }
s32 SSBBSkin_GetBoneWorldPos(const SSBBCharacterInstance*, s32, Vec3f*) { return 0; }
s32 SSBBSkin_ComputePose(SSBBCharacterInstance*) { return 0; }
void AudioSfx_PlaySfx(u16 id, Vec3f* pos, u8, f32* frequency, f32* volume, s8*) {
    requests.push_back({id, pos, frequency, volume});
}
void AudioSfx_StopByPosAndId(Vec3f* pos, u16 id) { stops.emplace_back(pos, id); }
#else
static GameInfo registers{};
GameInfo* gGameInfo = &registers;
s16 Math_Atan2S(f32 x, f32 y) { return std::atan2(y, x) * 32768 / 3.14159265358979323846; }
void Player_PlaySfx(Actor*, u16 id) { playerEvents.push_back(id); }
s32 SSBBSkin_GetBoneWorldPos(s32, Vec3f*) { return 0; }
void Audio_PlaySoundGeneral(u16 id, Vec3f* pos, u8, f32* frequency, f32* volume, s8*) {
    requests.push_back({id, pos, frequency, volume});
}
void Audio_StopSfxByPosAndId(Vec3f* pos, u16 id) { stops.emplace_back(pos, id); }
void ZeldaArena_FreeDebug(void* ptr, const char*, s32) { std::free(ptr); }
#endif
int32_t CVarGetInteger(const char* name, int32_t fallback) {
    const auto found = audioIntegerCvars.find(name);
    return found == audioIntegerCvars.end() ? fallback : found->second;
}
f32 CVarGetFloat(const char* name, f32 fallback) {
    const auto found = audioCvars.find(name);
    return found == audioCvars.end() ? fallback : found->second;
}
s16 sins(u16 angle) { return std::sin(angle * 3.14159265358979323846 / 32768) * 32767; }
s16 coss(u16 angle) { return std::cos(angle * 3.14159265358979323846 / 32768) * 32767; }
s16 Camera_GetInputDirYaw(Camera*) { return 0; }
s32 Collider_InitCylinder(PlayState*, ColliderCylinder* collider) {
    *collider = ColliderCylinder{};
    return 1;
}
s32 Collider_SetCylinder(PlayState*, ColliderCylinder* collider, Actor* actor, ColliderCylinderInit* init) {
    collider->base.actor = actor;
    collider->dim = init->dim;
    return 1;
}
s32 Collider_DestroyCylinder(PlayState*, ColliderCylinder*) { return 1; }
s32 CollisionCheck_SetAT(PlayState*, CollisionCheckContext*, Collider*) { return 1; }
void SSBBSkin_Destroy(SSBBSkinMesh*) {}
void ZeldaArena_Free(void* ptr) { std::free(ptr); }
void ActorShadow_DrawFeet(Actor*, Lights*, PlayState*) {}
}

static void check(bool value, const char* message) {
    if (!value) {
        std::fprintf(stderr, "FAIL Wolf audio: %s\n", message);
        std::exit(1);
    }
}

int main(int argc, char** argv) {
    assert(argc == 2);
    const bool chainOnly = std::strcmp(argv[1], "chain") == 0;
    const bool transformOnly = std::strcmp(argv[1], "transform") == 0;
    const bool mixerOnly = std::strcmp(argv[1], "mixer") == 0;
    const bool defaultsOnly = std::strcmp(argv[1], "defaults") == 0;
    const bool dashOnly = std::strcmp(argv[1], "dash") == 0;
    const bool exertionOnly = std::strcmp(argv[1], "exertion") == 0;
    PlayState play{};
    Player player{};
    Camera camera{};
    gPlayState = &play;
    play.cameraPtrs[0] = &camera;
#ifdef WOLF_AUDIO_MM
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
#else
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
#endif
    R_UPDATE_RATE = 3;
    sWolf = WolfRuntime{};
    sAnimations.resize(WANM_COUNT);
    for (s32 i = 0; i < WANM_COUNT; ++i) {
        sAnimations[i].numFrames = 60;
        sWolf.animIndex[i] = i;
    }
    sWolf.initialized = 1;
    sWolf.character.ssbbAnim = &sAnimations[WANM_WAIT];
    WolfLinkForm_Select(1);
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    auto setSpeed = [&](f32 speed) {
#ifdef WOLF_AUDIO_MM
        player.speedXZ = speed;
#else
        player.linearVelocity = speed;
#endif
    };
    auto tick = [&] {
        player.actor.projectedPos = player.actor.world.pos;
#ifdef WOLF_AUDIO_MM
        WolfLinkForm_Update(&player, &play, &play.state.input[0], 0);
#else
        WolfLinkForm_Update(&player, &play);
#endif
        ++play.gameplayFrames;
    };
    auto mix = [&](s16* output, u32 frames) {
#ifdef WOLF_AUDIO_MM
        MM_WolfLinkSfx_MixInto(output, frames);
#else
        OOT_WolfLinkSfx_MixInto(output, frames);
#endif
    };
    auto resetMove = [&] {
        ProcMoveInit(&player);
        ResetCombo();
        player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
        player.actor.velocity.y = 0;
        player.stateFlags1 = player.stateFlags3 = 0;
        sWolf.wasOnGround = 1;
        setSpeed(6);
    };
    auto publish = [&] {
#ifdef WOLF_AUDIO_MM
        WolfLinkForm_UpdateSfx(gPlayState);
#else
        GameInteractor::Instance->ExecuteHooks<GameInteractor::OnGameFrameUpdate>();
#endif
    };
#ifndef WOLF_AUDIO_MM
    ShipInit::InitAll();
#endif
    publish();
    if (exertionOnly) {
#ifdef WOLF_AUDIO_MM
        WolfLinkForm_PlayTransformSfx(1);
        const auto* transform = sWolfAudio.voices[0].clip;
        for (s32 variant = 0; variant < 3; ++variant) {
            WolfLinkForm_PlayExertionSfx();
            const auto* huff = &kWolfClips[12 + variant];
            check(sWolfAudio.voices[1].clip == huff && sWolfAudio.voices[0].clip == transform,
                  "climb exertion must use authentic Huff_A variants without "
                  "replacing transform audio");
            check(huff->rate == 16000 && huff->count >= 5299 && huff->count <= 5780,
                  "climb exertion must preserve each supplied huff's original rate "
                  "and duration");
            std::vector<s16> output(2 * std::ceil(huff->count * kWolfOutputRate / huff->rate), 0);
            mix(output.data(), output.size() / 2);
            check(std::any_of(output.begin(), output.end(), [](s16 value) { return value != 0; }),
                  "authentic exertion PCM must reach the native stereo mixer");
            s16 tail[4]{};
            mix(tail, 2);
            check(!sWolfAudio.voices[1].clip, "each exertion huff must finish without native-loop renewal");
            publish();
        }
        check(playerEvents.empty() && !WolfLinkForm_OwnsPlayerAction(),
              "exertion audio must not emit a human voice or take over native "
              "climbing controls");
        WolfLinkForm_PlayExertionSfx();
        play.transitionTrigger = TRANS_TRIGGER_START;
        publish();
        check(!sWolfAudio.voices[1].clip, "scene fades must stop outstanding exertion playback");
        std::puts("PASS supplied MM Wolf climb exertion: exact Huff_A variants, original "
                  "duration, native mixer, independent transform and fade cleanup");
#else
        check(false, "exertion dispatch applies to the MM native climb hook");
#endif
        return 0;
    }
    if (dashOnly) {
#ifdef WOLF_AUDIO_MM
        resetMove();
        player.cylinder.dim.radius = 12;
        ProcDashInit(&player, &play);
        check(playerEvents.size() == 1 && playerEvents.back() == NA_SE_PL_ROLL_DUST - SFX_FLAG,
              "MM dash dust must use the native one-frame cue, which expires without refresh");
        playerEvents.clear();
        for (s32 i = 0; i < 3; ++i)
            tick();
        check(sWolf.proc == PROC_WOLF_DASH && playerEvents.empty(),
              "ongoing dash startup must not renew the one-frame dust cue");
        sWolf.character.curFrame = kADashAnm.cancel + 1;
        tick();
        check(sWolf.proc == PROC_WOLF_MOVE && sWolf.dashModeTimer > 0,
              "dash startup ends before the retained movement speed boost");
        for (s32 i = 0; i < 80; ++i)
            tick();
        check(std::none_of(playerEvents.begin(), playerEvents.end(),
                           [](u16 id) { return id == NA_SE_PL_ROLL_DUST || id == NA_SE_PL_ROLL_DUST - SFX_FLAG; }),
              "Wolf locomotion must not renew MM roll-dust audio after dash startup");

        resetMove();
        for (s32 i = 0; i < 6; ++i) {
            player.actor.world.pos.x += 6;
            tick();
        }
        check(!requests.empty() && sWolf.chainPlaying, "real movement must start the owned chain before fade interruption");
        const Request movingChain = requests.back();
        WolfLinkForm_PlayTransformSfx(1);
        ProcWaitAttackInit(&player, &play, 0);
        check(sWolfAudio.voices[0].clip && sWolfAudio.voices[1].clip && WolfLinkForm_OwnsPlayerAction(),
              "fade interruption must start with live transform and attack voices and action ownership");
        const size_t beforeFadeStops = stops.size();
        play.transitionTrigger = TRANS_TRIGGER_START;
        publish();
        WolfLinkForm_SuspendAction(&player);
        check(stops.size() == beforeFadeStops + 1 && stops.back() == std::make_pair(movingChain.pos, movingChain.id),
              "fade interruption must stop precisely the owned movement sound");
        check(!sWolfAudio.voices[0].clip && !sWolfAudio.voices[1].clip && !WolfLinkForm_OwnsPlayerAction() &&
                  WolfLinkForm_IsReady() && WolfLinkForm_IsSelected() && WolfLinkForm_SpeedMultiplier() == 1 &&
                  player.cylinder.dim.radius == 12,
              "fade interruption must clear Wolf audio and actions while retaining presentation and native collision");
        player.cylinder.dim.radius = 17;
        WolfLinkForm_SuspendAction(&player);
        WolfLinkForm_ApplyCollisionShape(&player);
        check(stops.size() == beforeFadeStops + 1 && player.cylinder.dim.radius == 17,
              "repeated fade suspension must preserve native collision without retriggering audio cleanup");
        std::puts("PASS production MM dash audio: expiring startup cue, no renewal through DASH/MOVE, fade audio/action release with retained model");
#else
        check(false, "dash case applies to the MM native audio bank");
#endif
        return 0;
    }
    if (defaultsOnly) {
        check(audioCvars.empty() && audioIntegerCvars.empty(), "the default-volume check must have no stored CVars");
        check(std::fabs(sWolfAudio.settingsGain - 0.4f) < 0.0001f,
              "absent volume CVars must use ComboShip's actual 40% Master and 100% SFX defaults");
        s16 defaultPcm[1024]{}, fullPcm[1024]{};
        WolfLinkForm_PlayTransformSfx(1);
        mix(defaultPcm, 512);
#ifdef WOLF_AUDIO_MM
        audioCvars["gSettings.Audio.MasterVolume"] = 1;
        audioCvars["gSettings.Audio.SoundEffectsVolume"] = 1;
#else
        audioIntegerCvars["gSettings.Volume.Master"] = 100;
        audioIntegerCvars["gSettings.Volume.SFX"] = 100;
#endif
        WolfLinkForm_PlayTransformSfx(1);
        mix(fullPcm, 512);
        check(std::any_of(std::begin(fullPcm), std::end(fullPcm), [](s16 value) { return value != 0; }),
              "native full-volume transform output must contain the supplied recording");
        for (size_t i = 0; i < std::size(defaultPcm); ++i)
            check(std::fabs(defaultPcm[i] - fullPcm[i] * 0.4f) <= 1.5f,
                  "actual default PCM must be 40% of the host's full-volume PCM");
#ifdef WOLF_AUDIO_MM
        audioCvars["gSettings.Audio.MasterVolume"] = 0.25f;
        audioCvars["gSettings.Audio.SoundEffectsVolume"] = 0.5f;
        publish();
        check(std::fabs(sWolfAudio.settingsGain - 0.125f) < 0.0001f,
              "MM native float sliders must directly control the new mixer");
        audioCvars["gSettings.Audio.MasterVolume"] = 0;
        WolfLinkForm_PlayTransformSfx(1);
        std::fill(std::begin(fullPcm), std::end(fullPcm), 0);
        mix(fullPcm, 512);
        check(std::all_of(std::begin(fullPcm), std::end(fullPcm), [](s16 value) { return value == 0; }),
              "MM native float Master mute must silence actual supplied PCM");
        audioCvars.clear();
        audioIntegerCvars["gSettings.Volume.Master"] = 20;
        audioIntegerCvars["gSettings.Volume.SFX"] = 80;
        publish();
        check(std::fabs(sWolfAudio.settingsGain - 0.16f) < 0.0001f,
              "MM must honor canonical Combo volume before float mirroring occurs");
        audioCvars["gSettings.Audio.MasterVolume"] = std::numeric_limits<f32>::quiet_NaN();
        audioCvars["gSettings.Audio.SoundEffectsVolume"] = std::numeric_limits<f32>::infinity();
        publish();
        check(std::fabs(sWolfAudio.settingsGain - 0.16f) < 0.0001f,
              "non-finite MM native volume values must fall back to finite canonical gain");
        WolfLinkForm_PlayTransformSfx(1);
        std::fill(std::begin(fullPcm), std::end(fullPcm), 0);
        mix(fullPcm, 512);
        check(std::any_of(std::begin(fullPcm), std::end(fullPcm), [](s16 value) { return value != 0; }),
              "invalid MM volume CVars must produce bounded real PCM at the mixer boundary");
#endif
        std::puts("PASS actual Wolf audio defaults: absent-CVar 40% PCM, native host volume keys and finite Combo fallback");
        return 0;
    }
    if (mixerOnly) {
        audioIntegerCvars["gSettings.Volume.Master"] = 100;
        publish();
        audioCvars["gMods.WolfLink.TransformVolume"] = std::numeric_limits<f32>::infinity();
        audioCvars["gMods.WolfLink.VoiceVolume"] = std::numeric_limits<f32>::quiet_NaN();
        WolfLinkForm_PlayTransformSfx(1);
        ProcWaitAttackInit(&player, &play, 0);
        check(sWolfAudio.voices[0].gain == 0.65f && sWolfAudio.voices[1].gain == 0.8f,
              "non-finite cue gains must use bounded defaults");
        audioCvars["gMods.WolfLink.TransformVolume"] = 1000;
        WolfLinkForm_PlayTransformSfx(1);
        check(sWolfAudio.voices[0].gain == 1, "oversized cue gain must be bounded");
        audioCvars.clear();
        std::atomic<bool> firstMix{false};
        bool heardRealPcm = false;
        std::thread audioThread([&] {
            s16 output[1024]{};
            mix(output, 512);
            heardRealPcm = std::any_of(std::begin(output), std::end(output), [](s16 value) { return value != 0; });
            firstMix.store(true);
            for (int i = 0; i < 1000; ++i) {
                std::fill(std::begin(output), std::end(output), 0);
                mix(output, 512);
            }
        });
        while (!firstMix.load())
            std::this_thread::yield();
        for (int i = 0; i < 1000; ++i) {
            publish();
            WolfAudio_Play(WolfAudioCue::Enter, 0.65f);
            WolfAudio_Play(WolfAudioCue::Bite, 0.8f);
            if (i % 7 == 0)
                WolfAudio_Stop();
        }
        audioThread.join();
        check(heardRealPcm, "the concurrent native mixer must receive real transform/attack PCM");
        // Constant extreme inputs prove the additive seam saturates stereo output.
        const s16 positive[] = {32767, 32767};
        const s16 negative[] = {-32768, -32768};
        const WolfAudioClip positiveClip{"fixture saturation", positive, 2, 32000};
        const WolfAudioClip negativeClip{"fixture saturation", negative, 2, 32000};
        for (const auto* clip : {&positiveClip, &negativeClip}) {
            publish();
            {
                std::lock_guard<std::mutex> lock(sWolfAudio.mutex);
                sWolfAudio.voices = {WolfAudioVoice{clip, 0, 1}, WolfAudioVoice{clip, 0, 1}};
            }
            s16 output[2] = {123, -123};
            mix(output, 1);
            const s16 expected = clip == &positiveClip ? 32767 : -32768;
            check(output[0] == expected && output[1] == expected,
                  "simultaneous cues and native PCM must saturate rather than wrap");
        }
        WolfLinkForm_PlayTransformSfx(1);
        ProcWaitAttackInit(&player, &play, 0);
        std::vector<s16> staleOutput(2 * (kWolfAliveSamples + 1), 123);
        mix(staleOutput.data(), kWolfAliveSamples + 1);
        check(!sWolfAudio.voices[0].clip && !sWolfAudio.voices[1].clip &&
              std::all_of(staleOutput.begin(), staleOutput.end(), [](s16 value) { return value == 123; }),
              "missing host heartbeat must clear outstanding cues without changing native PCM");
        WolfLinkForm_PlayTransformSfx(1);
        ProcWaitAttackInit(&player, &play, 0);
#ifdef WOLF_AUDIO_MM
        WolfLinkForm_Cleanup(&player, &play);
#else
        WolfLinkForm_Cleanup();
#endif
        check(!sWolfAudio.voices[0].clip && !sWolfAudio.voices[1].clip,
              "production cleanup must stop concurrent transform/attack voices");
        std::puts("PASS native Wolf mixer: simultaneous thread publication, bounded gains, stereo saturation, heartbeat and cleanup");
        return 0;
    }
    if (transformOnly) {
        WolfLinkForm_PlayTransformSfx(1);
        check(sWolfAudio.voices[0].clip && std::strcmp(sWolfAudio.voices[0].clip->source, "TP_Transform_Wolf.wav") == 0,
              "deliberate Wolf entry must select the supplied transform recording");
        const auto* transform = sWolfAudio.voices[0].clip;
        ProcWaitAttackInit(&player, &play, 0);
        check(sWolfAudio.voices[0].clip == transform && sWolfAudio.voices[1].clip,
              "transform and bite must have independent voices");
        s16 output[1024]{};
        mix(output, 512);
        check(std::any_of(std::begin(output), std::end(output), [](s16 value) { return value != 0; }),
              "real transform and bite PCM must reach the stereo output");
        const double position = sWolfAudio.voices[0].position;
        play.pauseCtx.state = 6;
        publish();
        std::fill(std::begin(output), std::end(output), 0);
        mix(output, 512);
        check(sWolfAudio.voices[0].position == position &&
              std::all_of(std::begin(output), std::end(output), [](s16 value) { return value == 0; }),
              "pause must silence the cue without advancing its cursor");
        play.pauseCtx.state = 0;
        publish();
        WolfLinkForm_Select(1);
        check(sWolfAudio.voices[0].position == position, "scene selection restoration must not restart the stinger");
        WolfLinkForm_PlayTransformSfx(0);
        check(sWolfAudio.voices[0].clip && std::strcmp(sWolfAudio.voices[0].clip->source, "TP_Transform_Human.wav") == 0,
              "deliberate Human exit must select the supplied reverse transform");
        const auto* human = sWolfAudio.voices[0].clip;
        u32 frames = 0;
        while (sWolfAudio.voices[0].clip) {
            publish();
            std::fill(std::begin(output), std::end(output), 0);
            mix(output, 512);
            frames += 512;
        }
        const u32 expectedFrames = std::ceil(human->count * 32000.0 / human->rate);
        check(frames >= expectedFrames && frames < expectedFrames + 513,
              "transform playback must preserve the recording's full original duration");
        WolfLinkForm_PlayTransformSfx(1);
        audioIntegerCvars["gSettings.Volume.Master"] = 0;
        publish();
        std::fill(std::begin(output), std::end(output), 0);
        mix(output, 512);
        check(std::all_of(std::begin(output), std::end(output), [](s16 value) { return value == 0; }),
              "the native Master setting must mute actual PCM");
        audioIntegerCvars["gSettings.Volume.Master"] = 100;
        audioIntegerCvars["gSettings.Volume.SFX"] = 0;
        publish();
        std::fill(std::begin(output), std::end(output), 0);
        mix(output, 512);
        check(std::all_of(std::begin(output), std::end(output), [](s16 value) { return value == 0; }),
              "the native SFX setting must mute actual PCM");
        audioIntegerCvars["gSettings.Volume.SFX"] = 100;
#ifndef WOLF_AUDIO_MM
        for (int sessionBoundary = 0; sessionBoundary < 3; ++sessionBoundary) {
            WolfLinkForm_PlayTransformSfx(0);
            ProcWaitAttackInit(&player, &play, 0);
            if (sessionBoundary == 0)
                GameInteractor::Instance->ExecuteHooks<GameInteractor::OnLoadGame>(0);
            else if (sessionBoundary == 1)
                GameInteractor::Instance->ExecuteHooks<GameInteractor::OnExitGame>(0);
            else
                GameInteractor::Instance->ExecuteHooks<GameInteractor::OnZTitleInit>(&play.state);
            check(!sWolfAudio.voices[0].clip && !sWolfAudio.voices[1].clip,
                  "the registered native load/exit/title hook must clear both voices");
        }
#endif
        WolfLinkForm_PlayTransformSfx(1);
        WolfLinkForm_UpdateSfx(nullptr);
        check(!sWolfAudio.voices[0].clip && !sWolfAudio.voices[1].clip,
              "title/host/session departure must discard all voices");
        std::puts("PASS supplied Wolf transform PCM: entry/exit parity, simultaneous bite, pause, silent restore, full duration and native volume");
        return 0;
    }
    if (!chainOnly) {
        auto noLinkVoice = [&] {
            check(!playerEvents.empty(), "physical attack sounds must remain");
            for (u16 id : playerEvents)
                check(id != NA_SE_VO_LI_SWORD_N && id != NA_SE_VO_LI_SWORD_L,
                      "bite/lunge/spin must not emit Link's sword attack voice");
            playerEvents.clear();
        };
        for (s32 tier : { 1, 4 }) {
            for (s32 attack = 0; attack < 4; ++attack) {
                sWolf.comboCount = tier;
                ProcWaitAttackInit(&player, &play, attack);
                check(sWolfAudio.voices[1].clip >= kWolfClips + 2 && sWolfAudio.voices[1].clip < kWolfClips + 7,
                      "bite must select one of the five actual supplied Lunge recordings");
                noLinkVoice();
            }
            ProcJumpAttackInit(&player, &play, 0);
            check(sWolfAudio.voices[1].clip >= kWolfClips + 7 && sWolfAudio.voices[1].clip < kWolfClips + 9,
                  "jump attack must select an actual supplied JumpAttack recording");
            noLinkVoice();
            ProcRollAttackInit(&player, &play, 1);
            check(sWolfAudio.voices[1].clip >= kWolfClips + 9 && sWolfAudio.voices[1].clip < kWolfClips + 12,
                  "spin must select an actual supplied Charge_Attack_A recording");
            noLinkVoice();
        }
        s16 output[1024]{};
        mix(output, 512);
        check(std::any_of(std::begin(output), std::end(output), [](s16 value) { return value != 0; }),
              "supplied attack PCM must be audible at the actual mixer boundary");
        std::puts("PASS production Wolf attack routing: actual Lunge/JumpAttack/Charge_Attack_A PCM and physical SFX without Link voices");
        return 0;
    }
    resetMove();
    tick();
    for (s32 i = 0; i < 30; ++i)
        tick();
    check(requests.empty(), "idle with residual speed must not rattle the chain");
    auto move = [&](s32 frames = 30) {
        for (s32 i = 0; i < frames; ++i) {
            player.actor.world.pos.x += 6;
            tick();
        }
    };
    move();
    check(!requests.empty(), "grounded movement must emit a soft chain tick");
    check(requests.size() <= 5, "chain ticks must have bounded cadence instead of retriggering every frame");
    for (const auto& request : requests) {
        check(request.id == NA_SE_IT_HOOKSHOT_REFLECT, "chain must use the known one-shot metal clink");
        check(*request.volume > 0 && *request.volume <= 0.15f, "chain default volume must stay subtle");
        check(request.pos != &player.actor.projectedPos && request.pos != &player.actor.world.pos,
              "audio must retain its own position beyond player allocation lifetime");
        check(*request.frequency > 0 && *request.frequency < 2, "chain frequency must be valid");
    }
    const Request initial = requests.back();
    size_t before = requests.size();
    tick();
    check(requests.size() == before && !stops.empty() && stops.back() == std::make_pair(initial.pos, initial.id),
          "stationary frame stops the owned chain sound");
    resetMove();
    move();
    before = requests.size();
    player.actor.bgCheckFlags = 0;
    move();
    check(requests.size() == before, "airborne movement must stay quiet");
    resetMove();
    move();
    before = requests.size();
    player.stateFlags1 |= PLAYER_STATE1_IN_WATER;
    move();
    check(requests.size() == before, "water must stay quiet even with a stale ground flag");
    resetMove();
    move();
    before = requests.size();
    player.stateFlags3 |= PLAYER_STATE3_FLYING_WITH_HOOKSHOT;
    move();
    check(requests.size() == before, "hookshot flight must stay quiet even with a stale ground flag");
    resetMove();
    audioCvars["gMods.WolfLink.ChainVolume"] = 0;
    move();
    check(requests.size() == before, "zero chain volume must mute requests");
    audioCvars["gMods.WolfLink.ChainVolume"] = 0.05f;
    move();
    check(requests.size() > before && std::fabs(*requests.back().volume - 0.05f) < 0.0001f,
          "chain volume must be adjustable through the existing CVar pattern");
    const Request final = requests.back();
    const size_t stopsBefore = stops.size();
#ifdef WOLF_AUDIO_MM
    WolfLinkForm_Cleanup(&player, &play);
#else
    WolfLinkForm_Cleanup();
#endif
    check(stops.size() == stopsBefore + 1 && stops.back() == std::make_pair(final.pos, final.id),
          "cleanup stops precisely the Wolf-owned chain position and ID");
    std::puts("PASS production Wolf chain: actual grounded motion, low adjustable volume, idle/water/flight silence and cleanup");
}
