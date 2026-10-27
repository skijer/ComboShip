// Exercise the production adapter with real native Player/Input/PlayState types.
// Only asset transport and the process CVar store are supplied by this harness.
#include "../../combo/emotes/GameAdapter.inc"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <unordered_map>

static std::unordered_map<std::string, int32_t> integers;
static std::unordered_map<std::string, float> floats;
extern "C" {
SaveContext gSaveContext{};
PlayState* gPlayState = nullptr;
#if !COMBO_EMOTES_MM
static GameInfo info{};
GameInfo* gGameInfo = &info;
u8 TransformMasks_GetModelType() { return 0; }
u8 WolfLinkForm_IsSelected() { return 0; }
#endif
int32_t CVarGetInteger(const char* key, int32_t fallback) {
    const auto found = integers.find(key);
    return found == integers.end() ? fallback : found->second;
}
void CVarSetInteger(const char* key, int32_t value) { integers[key] = value; }
float CVarGetFloat(const char* key, float fallback) {
    const auto found = floats.find(key);
    return found == floats.end() ? fallback : found->second;
}
}
namespace Ship {
std::shared_ptr<ResourceManager> CrossRMRegistry::Get(const std::string&) { return {}; }
std::shared_ptr<ArchiveManager> ResourceManager::GetArchiveManager() { return {}; }
bool ArchiveManager::HasFile(const std::string&) { return false; }
std::shared_ptr<File> ArchiveManager::LoadFile(const std::string&) { return {}; }
}
static void Check(bool value, const char* why) {
    if (!value) throw std::runtime_error(why);
}
static void SetHealth(int health) {
#if COMBO_EMOTES_MM
    gSaveContext.save.saveInfo.playerData.health = health;
#else
    gSaveContext.health = health;
#endif
}
int main(int argc, char** argv) {
    Check(argc == 2, "provide the real bundled wave stream");
    std::ifstream file(argv[1], std::ios::binary);
    sClips[0] = { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
    Check(sClips[0].size() == 81 * 134, "real wave asset loaded");
    static PlayState play{};
    static ::Player player{};
    gPlayState = &play;
#if COMBO_EMOTES_MM
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    player.transformation = PLAYER_FORM_HUMAN;
    player.skelAnime.jointTable = reinterpret_cast<Vec3s*>(ALIGN16(reinterpret_cast<uintptr_t>(player.jointTableBuffer)));
    player.skelAnimeUpper.jointTable = reinterpret_cast<Vec3s*>(ALIGN16(reinterpret_cast<uintptr_t>(player.jointTableUpperBuffer)));
    play.state.framerateDivisor = 3;
#else
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
    player.skelAnime.jointTable = player.jointTable;
    R_UPDATE_RATE = 3;
#endif
    player.skelAnime.limbCount = 22;
    player.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    player.skelAnime.jointTable[0] = { 987, 654, 321 };
    player.headLimbRot = { 12, 34, 56 };
    player.upperLimbRot = { 78, 90, 12 };
    player.upperLimbYawSecondary = 123;
    player.skelAnime.movementFlags = 4;
    SetHealth(48);
    CVarSetInteger("gCombo.Emotes.Request", 1);
    ComboEmotes_BeginUpdate(&play);
    Check(ComboEmotes_IsActive(), "idle grounded human can start a built-in emote");
    Vec3s wire[24]{};
    Check(ComboEmotes_CopyPose(wire, sizeof(wire)), "pose available to the network before native animation flush");
    ComboEmotes_ApplyPose(&play);
    Check(!std::memcmp(wire, player.skelAnime.jointTable, 134), "render and network use the same frame");
    Check(player.headLimbRot.x == 0 && player.upperLimbRot.x == 0 && player.skelAnime.movementFlags == 0,
          "native draw offsets and root motion cannot distort an emote");
    Check(player.upperLimbYawSecondary == 0, "secondary torso yaw cannot distort an emote");
    ::Player peer = player;
    peer.headLimbRot = { 9, 8, 7 };
    peer.upperLimbRot = { 6, 5, 4 };
    peer.upperLimbYawSecondary = 234;
    ComboEmotes::ClearDrawOffsets(peer);
    Check(peer.headLimbRot.x == 0 && peer.upperLimbRot.x == 0 && peer.upperLimbYawSecondary == 0,
          "viewer-owned look offsets are removed from a remote native template");
    CVarSetInteger("gCombo.Emotes.Request", -1);
    ComboEmotes_BeginUpdate(&play);
    Check(!ComboEmotes_IsActive() && player.skelAnime.jointTable[0].x == 987 &&
          player.headLimbRot.x == 12 && player.upperLimbYawSecondary == 123 && player.skelAnime.movementFlags == 4,
          "cancellation restores the native pose and flags");
    CVarSetInteger("gCombo.Emotes.Request", 1);
    ComboEmotes_BeginUpdate(&play);
    SetHealth(40);
    Check(!ComboEmotes_CopyPose(wire, sizeof(wire)), "damage after input processing cancels before transmission");
    SetHealth(48);
    play.state.input[0].cur.button = ComboEmotes::kDefaultChords[0];
    play.state.input[0].press.button = 0x800;
    ComboEmotes_BeginUpdate(&play);
    Check(ComboEmotes_IsActive() && play.state.input[0].cur.button == 0 && play.state.input[0].press.button == 0,
          "emote shortcut is consumed before native D-pad handling");
    play.state.input[0].cur.button = ComboEmotes::kDefaultChords[0];
    play.state.input[0].press.button = 0;
    ComboEmotes_BeginUpdate(&play);
    Check(sEmote.Frame() == 1, "holding a shortcut advances instead of restarting");
    play.state.input[0].cur.button = 0;
    play.state.input[0].cur.stick_x = 30;
    ComboEmotes_BeginUpdate(&play);
    Check(!ComboEmotes_IsActive(), "movement cancels without consuming the movement input");
    play.state.input[0].cur.stick_x = 0;
    play.pauseCtx.state = 1;
    CVarSetInteger("gCombo.Emotes.Request", 1);
    ComboEmotes_BeginUpdate(&play);
    Check(!ComboEmotes_IsActive() && CVarGetInteger("gCombo.Emotes.Request", 0) == 1,
          "menu preview waits until gameplay resumes");
    play.pauseCtx.state = 0;
    ComboEmotes_BeginUpdate(&play);
    Check(ComboEmotes_IsActive(), "queued preview starts after pause");
    const int16_t sound[]{ 1000, 2000 };
    { std::lock_guard lock(sAudioMutex); sVoice.Start(sound); }
    ComboEmotes_Reset();
    Check(!ComboEmotes_IsActive(), "scene/game reset removes the active emote");
    int16_t mixed[]{ 10, 20 };
    ComboEmotes_MixAudio(mixed, 1);
    Check(mixed[0] == 10 && mixed[1] == 20, "scene/game reset stops an active sound voice");
    // Distinct host settings catch accidental reuse of the other game's keys.
    integers["gSettings.Volume.Master"] = 50;
    integers["gSettings.Volume.SFX"] = 25;
    floats["gSettings.Audio.MasterVolume"] = 0.2f;
    floats["gSettings.Audio.SoundEffectsVolume"] = 0.5f;
    floats["gCombo.Emotes.Volume"] = 0.5f;
    const int16_t volumeProbe[]{ 16000 };
    auto probe = [&]() {
        { std::lock_guard lock(sAudioMutex); sVoice.Start(volumeProbe); }
        std::array<int16_t, 2> output{ 10, 20 };
        ComboEmotes_MixAudio(output.data(), 1);
        return output;
    };
    const int expected = COMBO_EMOTES_MM ? 800 : 1000;
    auto scaled = probe();
    Check(scaled[0] == 10 + expected && scaled[1] == 20 + expected,
          "emote PCM gain follows this game's native master and SFX settings");
#if COMBO_EMOTES_MM
    floats["gSettings.Audio.MasterVolume"] = 0.0f;
#else
    integers["gSettings.Volume.Master"] = 0;
#endif
    Check(probe() == std::array<int16_t, 2>{ 10, 20 },
          "native master zero mutes emotes without changing the synth mix");
#if COMBO_EMOTES_MM
    floats["gSettings.Audio.MasterVolume"] = 0.2f;
    floats["gSettings.Audio.SoundEffectsVolume"] = 0.0f;
#else
    integers["gSettings.Volume.Master"] = 50;
    integers["gSettings.Volume.SFX"] = 0;
#endif
    Check(probe() == std::array<int16_t, 2>{ 10, 20 },
          "native SFX zero mutes emotes without changing the synth mix");
    sClips[0][80 * 134 + 132] = 3;
    CVarSetInteger("gCombo.Emotes.Request", 1);
    ComboEmotes_BeginUpdate(&play);
    Check(!ComboEmotes_IsActive(), "malformed late appearance rejects a clip before its first frame");
    std::cout << "PASS native " << (COMBO_EMOTES_MM ? "MM" : "OoT") << " emote lifecycle, pose parity and inputs\n";
}
