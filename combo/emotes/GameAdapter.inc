// Instantiated privately in each game DLL. Native types never cross a DLL seam.
#include "EmoteBridge.h"
#include "EmotePlayer.h"
#include "EmoteAudio.h"
#include <array>
#include <mutex>
#include <string>
#include <vector>
#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/File.h>
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
#include "z64.h"
#include "variables.h"
#include "macros.h"
#include "regs.h"
extern PlayState* gPlayState;
}
#if COMBO_EMOTES_MM
#include "2s2h/BenPort.h"
#else
#include "soh/OTRGlobals.h"
extern "C" u8 TransformMasks_GetModelType(void);
extern "C" u8 WolfLinkForm_IsSelected(void);
#endif

namespace {
using namespace ComboEmotes;
static_assert(BTN_CUSTOM_MODIFIER1 == kModifier1);
static_assert(sizeof(Vec3s) == 6 && PLAYER_LIMB_MAX == 22);
ComboEmotes::Playback sEmote;
PlayState* sPlay = nullptr;
::Player* sPlayer = nullptr;
std::array<std::vector<uint8_t>, 8> sClips;
std::array<std::vector<int16_t>, 8> sSounds;
std::mutex sAudioMutex;
AudioVoice sVoice;
bool sSoundStarted = false;
int sHealth = 0;
uint32_t sConsumed = 0;
struct NativePose {
    bool saved = false;
    Vec3s* joints = nullptr;
    std::array<uint8_t, kFrameBytes> main{}, upper{};
    Vec3s head{}, torso{}, prev{};
    s16 secondaryYaw = 0;
    u8 movement = 0;
} sNative;

std::vector<uint8_t> LoadBytes(const char* name) {
    const auto manager = Ship::CrossRMRegistry::Get(COMBO_EMOTES_MM ? "mm" : "oot");
    if (!manager) return {};
    const auto archives = manager->GetArchiveManager();
    const std::string path = std::string("emotes/") + name;
    if (!archives || !archives->HasFile(path)) return {};
    auto file = archives->LoadFile(path);
    if (!file || !file->Buffer || file->Buffer->size() > 32000 * 2 * 120) return {};
    return { file->Buffer->begin(), file->Buffer->end() };
}

void RestoreNative() {
    if (!sNative.saved) return;
    if (sPlay == gPlayState && sPlayer == GET_PLAYER(sPlay) && sPlayer->skelAnime.jointTable == sNative.joints) {
        std::memcpy(sPlayer->skelAnime.jointTable, sNative.main.data(), kFrameBytes);
#if COMBO_EMOTES_MM
        std::memcpy(sPlayer->skelAnimeUpper.jointTable, sNative.upper.data(), kFrameBytes);
#else
        std::memcpy(sPlayer->upperJointTable, sNative.upper.data(), kFrameBytes);
#endif
        sPlayer->headLimbRot = sNative.head;
        sPlayer->upperLimbRot = sNative.torso;
        sPlayer->upperLimbYawSecondary = sNative.secondaryYaw;
        sPlayer->skelAnime.prevTransl = sNative.prev;
        sPlayer->skelAnime.movementFlags = sNative.movement;
    }
    sNative.saved = false;
}

void Stop() {
    sEmote.Stop();
    sSoundStarted = false;
    std::lock_guard lock(sAudioMutex);
    sVoice.Stop();
}

int Health(PlayState* play) {
    (void)play;
#if COMBO_EMOTES_MM
    return gSaveContext.save.saveInfo.playerData.health;
#else
    return gSaveContext.health;
#endif
}

bool Eligible(PlayState* play, ::Player* player) {
    if (!play || !player || play->pauseCtx.state || play->csCtx.state ||
        play->transitionTrigger || play->transitionMode || play->msgCtx.msgMode ||
        player->actor.freezeTimer || player->csAction || player->meleeWeaponState ||
        !(player->actor.bgCheckFlags & BGCHECKFLAG_GROUND) || player->heldActor ||
        !player->skelAnime.jointTable || player->skelAnime.limbCount != 22) return false;
    // Same action-state bits in the two native player implementations. Reject
    // item use/changes, targeting, damage, water, climbing and cutscene states.
    constexpr uint32_t blocked = (1u << 1) | (1u << 2) | (1u << 5) | (1u << 6) | (1u << 7) |
        (1u << 8) | (1u << 9) | (1u << 10) | (1u << 11) | (1u << 12) | (1u << 13) |
        (1u << 14) | (1u << 15) | (1u << 18) | (1u << 19) | (1u << 20) | (1u << 21) |
        (1u << 22) | (1u << 23) | (1u << 26) | (1u << 27) | (1u << 28) | (1u << 29);
    if (player->stateFlags1 & blocked) return false;
#if COMBO_EMOTES_MM
    if (player->transformation != PLAYER_FORM_HUMAN || std::abs(player->speedXZ) > 0.1f ||
        !player->skelAnimeUpper.jointTable) return false;
#else
    if (TransformMasks_GetModelType() || WolfLinkForm_IsSelected() || std::abs(player->linearVelocity) > 0.1f)
        return false;
#endif
    return Health(play) > 0;
}

bool Start(int id) {
    const auto* entry = Find(id);
    if (!entry) return false;
    auto& clip = sClips[id - 1];
    if (clip.empty()) clip = LoadBytes(entry->animation);
    if (!ValidateClip(clip, entry->frames)) { Stop(); return false; }
    Stop();
    sHealth = Health(sPlay);
    // Load and decode on the game thread, before the audio callback sees a voice.
    auto& sound = sSounds[id - 1];
    if (entry->audio[0] && sound.empty()) {
        const auto bytes = LoadBytes(entry->audio);
        if (bytes.size() % 2 == 0) {
            sound.resize(bytes.size() / 2);
            for (size_t i = 0; i < sound.size(); ++i)
                sound[i] = std::bit_cast<int16_t>(static_cast<uint16_t>(bytes[i * 2] | (bytes[i * 2 + 1] << 8)));
        }
    }
    return sEmote.Start(id, entry->frames);
}

bool CurrentPose(Pose& pose) {
    if (!sEmote.Active()) return false;
    if (!Eligible(sPlay, sPlayer) || Health(sPlay) != sHealth) { Stop(); return false; }
    if (!DecodeFrame(sClips[sEmote.Id() - 1], sEmote.Frame(), pose)) { Stop(); return false; }
    return true;
}
} // namespace

extern "C" void ComboEmotes_Reset() {
    RestoreNative();
    Stop();
    sPlay = nullptr;
    sPlayer = nullptr;
    sConsumed = 0;
}

extern "C" void ComboEmotes_BeginUpdate(void* state) {
    auto* play = static_cast<PlayState*>(state);
    auto* player = GET_PLAYER(play);
    RestoreNative();
    if (sPlay != play || sPlayer != player) { ComboEmotes_Reset(); sPlay = play; sPlayer = player; }
    Input& input = play->state.input[0];
    const uint32_t held = input.cur.button;
    const uint32_t pressed = input.press.button;
    sConsumed &= held;
    int request = CVarGetInteger("gCombo.Emotes.Request", 0);
    if (request && (!play->pauseCtx.state || request < 0)) CVarSetInteger("gCombo.Emotes.Request", 0);
    std::array<uint32_t, kSlots> chords;
    for (size_t i = 0; i < kSlots; ++i) chords[i] = CVarGetInteger(kChordKeys[i], kDefaultChords[i]);
    const int slot = SelectBinding(chords, held, pressed);
    bool started = false;
    if (!CVarGetInteger("gCombo.Emotes.Enabled", 1) || !Eligible(play, player) ||
        std::abs(input.cur.stick_x) > 8 || std::abs(input.cur.stick_y) > 8 || request < 0) {
        Stop();
    } else {
        if (slot >= 0) request = CVarGetInteger(kSlotKeys[slot], kDefaultEmotes[slot]);
        if (request > 0 && Start(request)) {
            started = true;
            if (slot >= 0) sConsumed |= chords[slot];
        }
        if (sEmote.Active() && ((pressed & ~sConsumed & ~uint32_t(0xc0)) || Health(play) != sHealth)) Stop();
        if (sEmote.Active() && !started) {
#if COMBO_EMOTES_MM
            sEmote.Tick(play->state.framerateDivisor / 60.0);
#else
            sEmote.Tick(R_UPDATE_RATE / 60.0);
#endif
            if (!sEmote.Active()) Stop();
        }
    }
    // Consume a triggered chord until released. Normal D-pad controls remain
    // available when no valid emote can start, and holding never restarts one.
    input.cur.button &= ~sConsumed;
    input.press.button &= ~sConsumed;
    input.rel.button &= ~sConsumed;
    if (sEmote.Active() && !sSoundStarted && sEmote.Frame() >= 5) {
        std::lock_guard lock(sAudioMutex);
        sVoice.Start(sSounds[sEmote.Id() - 1]);
        sSoundStarted = true;
    }
}

extern "C" int ComboEmotes_CopyPose(void* destination, size_t size) {
    Pose pose;
    return destination && CurrentPose(pose) && WritePose({ static_cast<uint8_t*>(destination), size }, 0, pose);
}
extern "C" int ComboEmotes_CopyRawBuffer(void* destination, size_t size, size_t offset) {
    Pose pose;
    return destination && CurrentPose(pose) && WritePose({ static_cast<uint8_t*>(destination), size }, offset, pose);
}
extern "C" int ComboEmotes_IsActive() {
    Pose pose;
    return CurrentPose(pose);
}
extern "C" void ComboEmotes_ApplyPose(void* state) {
    if (state != sPlay) return;
    Pose pose;
    if (!CurrentPose(pose)) return;
    sNative.saved = true;
    sNative.joints = sPlayer->skelAnime.jointTable;
    std::memcpy(sNative.main.data(), sNative.joints, kFrameBytes);
#if COMBO_EMOTES_MM
    auto* upper = sPlayer->skelAnimeUpper.jointTable;
#else
    auto* upper = sPlayer->upperJointTable;
#endif
    std::memcpy(sNative.upper.data(), upper, kFrameBytes);
    sNative.head = sPlayer->headLimbRot;
    sNative.torso = sPlayer->upperLimbRot;
    sNative.secondaryYaw = sPlayer->upperLimbYawSecondary;
    sNative.prev = sPlayer->skelAnime.prevTransl;
    sNative.movement = sPlayer->skelAnime.movementFlags;
    WritePose({ reinterpret_cast<uint8_t*>(sNative.joints), kFrameBytes }, 0, pose);
    WritePose({ reinterpret_cast<uint8_t*>(upper), kFrameBytes }, 0, pose);
    ClearDrawOffsets(*sPlayer);
    sPlayer->skelAnime.prevTransl = { pose.joints[0], pose.joints[1], pose.joints[2] };
    sPlayer->skelAnime.movementFlags = 0;
}
extern "C" void ComboEmotes_MixAudio(int16_t* stereo, size_t frames) {
#if COMBO_EMOTES_MM
    const float master = CVarGetFloat("gSettings.Audio.MasterVolume", 0.4f);
    const float sfx = CVarGetFloat("gSettings.Audio.SoundEffectsVolume", 1.0f);
#else
    const float master = CVarGetInteger("gSettings.Volume.Master", 40) / 100.0f;
    const float sfx = CVarGetInteger("gSettings.Volume.SFX", 100) / 100.0f;
#endif
    const float gain = std::clamp(CVarGetFloat("gCombo.Emotes.Volume", 0.8f), 0.0f, 1.0f) *
        std::clamp(master, 0.0f, 1.0f) * std::clamp(sfx, 0.0f, 1.0f);
    std::lock_guard lock(sAudioMutex);
    sVoice.Mix(stereo, frames, gain);
}
