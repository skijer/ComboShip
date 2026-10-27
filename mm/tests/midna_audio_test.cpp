#include "2s2h/Enhancements/Companion/MidnaAudio.h"
#include "2s2h/Enhancements/Companion/MidnaAudioResources.h"
#include "test_require.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <map>
#include <string>
#include <thread>
#include <vector>

static bool model = true;
static bool enabled = true;
static float gain = 1.0f;
static std::map<std::string, std::vector<uint8_t>> files;
static int reads;
static std::map<std::string, std::string> assignments;

namespace MMMidnaAudioResources {
std::vector<std::string> ListClips() {
    std::vector<std::string> result;
    for (const auto& [path, bytes] : files) {
        result.push_back(path);
    }
    return result;
}
std::string ReadAssignment(const char* event, const char* fallback) {
    const auto it = assignments.find(event);
    return it == assignments.end() ? fallback : it->second;
}
void WriteAssignment(const char* event, const std::string& path) {
    assignments[event] = path;
}
bool Enabled() {
    return enabled;
}
bool HasModel() {
    return model;
}
bool ReadClip(const char* path, std::vector<uint8_t>& bytes) {
    ++reads;
    const auto it = files.find(path);
    if (it == files.end())
        return false;
    bytes = it->second;
    return true;
}
float Gain() {
    return gain;
}
} // namespace MMMidnaAudioResources

static void le(std::vector<uint8_t>& bytes, size_t offset, uint32_t value, size_t width) {
    for (size_t i = 0; i < width; ++i)
        bytes.at(offset + i) = value >> (i * 8);
}

static std::vector<uint8_t> wav(std::initializer_list<int16_t> samples) {
    std::vector<uint8_t> result(44 + samples.size() * 2);
    std::memcpy(result.data(), "RIFF", 4);
    le(result, 4, result.size() - 8, 4);
    std::memcpy(result.data() + 8, "WAVEfmt ", 8);
    le(result, 16, 16, 4);
    le(result, 20, 1, 2); // PCM
    le(result, 22, 1, 2); // mono
    le(result, 24, 32000, 4);
    le(result, 28, 64000, 4);
    le(result, 32, 2, 2);
    le(result, 34, 16, 2);
    std::memcpy(result.data() + 36, "data", 4);
    le(result, 40, samples.size() * 2, 4);
    size_t offset = 44;
    for (int16_t sample : samples) {
        le(result, offset, static_cast<uint16_t>(sample), 2);
        offset += 2;
    }
    return result;
}

static void movementSpacing() {
    files["objects/midna_navi/audio/appear.wav"] = wav({ 1000, -2000, 3000, 4000 });
    MMMidnaAudio_Init();
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_DASH));
    std::array<int16_t, 8> output = {};
    MMMidnaAudio_Mix(output.data(), 4);
    REQUIRE(output[0] == 1000);
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_APPEAR));
    output = {};
    MMMidnaAudio_Mix(output.data(), 4);
    REQUIRE(output[0] == 0);                            // alternating movement events share one cooldown
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_DASH)); // handled, no native fallback
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_VANISH));
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_CALL));
    output = {};
    MMMidnaAudio_Mix(output.data(), 4);
    REQUIRE(output[0] == 700); // recall and attention calls are not throttled
    MMMidnaAudio_Reset();      // changing scenes must not bypass spacing
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_DASH));
    output = {};
    MMMidnaAudio_Mix(output.data(), 4);
    REQUIRE(output[0] == 0);
    std::vector<int16_t> silence((256000 - 17) * 2);
    MMMidnaAudio_Mix(silence.data(), 256000 - 17);
    REQUIRE(std::all_of(silence.begin(), silence.end(), [](int16_t x) { return x == 0; }));
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_APPEAR));
    output = {};
    MMMidnaAudio_Mix(output.data(), 1);
    REQUIRE(output[0] == 0); // one sample before expiry; skipped cues do not queue
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_APPEAR));
    output = {};
    MMMidnaAudio_Mix(output.data(), 4);
    REQUIRE(output[0] == 1000);
    files.erase("objects/midna_navi/audio/appear.wav");
    MMMidnaAudio_Init();
    REQUIRE(!MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_APPEAR));
}

// Catch timer consumption while hidden/moving and idle audio stealing a cue.
static void idleYawn() {
    files["objects/midna_navi/audio/yawn.wav"] = wav({ 1700, 1800, 1900, 2000 });
    MMMidnaAudio_Init();
    auto tick = [](bool eligible) {
        MMMidnaAudio_UpdateIdle(eligible);
        std::array<int16_t, 2> out = {};
        MMMidnaAudio_Mix(out.data(), 1);
        return out[0];
    };
    for (int i = 0; i < 2000; ++i)
        REQUIRE(tick(false) == 0);
    int elapsed = 0;
    while (++elapsed <= 900) {
        int16_t sample = tick(true);
        if (sample != 0) {
            REQUIRE(sample == 1700);
            break;
        }
        if (elapsed % 100 == 0)
            REQUIRE(tick(false) == 0); // resetting instead of pausing would never reach 20 seconds
    }
    REQUIRE(elapsed >= 400 && elapsed <= 900); // 20–45 seconds of eligible 20 Hz updates
    REQUIRE(tick(true) == 1800);               // continues, never restarts every update
    REQUIRE(tick(false) == 0);                 // movement/recall interrupts the idle cue

    // After a pause the unspent interval resumes, without a catch-up burst.
    for (int i = 0; i < 100; ++i)
        REQUIRE(tick(true) == 0);
    for (int i = 0; i < 2000; ++i)
        REQUIRE(tick(false) == 0);
    elapsed = 0;
    while (++elapsed <= 800 && tick(true) == 0) {}
    REQUIRE(elapsed >= 299 && elapsed <= 799);
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_CALL));
    REQUIRE(tick(true) == 200); // Hey replaces the yawn, without an added voice
    REQUIRE(tick(true) == 300);

    // Even a missing custom cue must cancel the yawn before native fallback.
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_YAWN));
    REQUIRE(!MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_TALK));
    REQUIRE(tick(true) == 0);
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_YAWN));
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_VANISH));
    REQUIRE(tick(true) == 500);                          // recall stops the yawn rather than layering it
    REQUIRE(!MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_YAWN)); // busy voices beat idle audio
    REQUIRE(tick(true) == 600);
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_YAWN));
    MMMidnaAudio_Reset();
    REQUIRE(tick(true) == 0);
    files.erase("objects/midna_navi/audio/yawn.wav");
    MMMidnaAudio_Init();
    for (int i = 0; i < 2000; ++i)
        REQUIRE(tick(true) == 0); // older packs remain silent, with no native substitute
}

static void optOut() {
    files["objects/midna_navi/audio/call.wav"] = wav({ 700, 800, 900, 1000 });
    enabled = false;
    MMMidnaAudio_Init();
    REQUIRE(!MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_CALL));
    std::array<int16_t, 4> out{};
    MMMidnaAudio_Mix(out.data(), 2);
    REQUIRE(out[0] == 0);
    enabled = true;
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_CALL));
    enabled = false;
    MMMidnaAudio_Mix(out.data(), 1);
    REQUIRE(out[0] == 0);
    enabled = true;
    MMMidnaAudio_Mix(out.data(), 1);
    REQUIRE(out[0] == 0); // disabling erased the pending voice
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_CALL));
    MMMidnaAudio_Reset(); // UI reset must erase voices even when toggled rapidly while paused
    MMMidnaAudio_Mix(out.data(), 1);
    REQUIRE(out[0] == 0);
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_CALL));
    model = false;
    MMMidnaAudio_Mix(out.data(), 1);
    REQUIRE(out[0] == 0); // model removal cannot leave a private voice behind
    model = true;
    MMMidnaAudio_Mix(out.data(), 1);
    REQUIRE(out[0] == 0);
    model = false;
    REQUIRE(!MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_CALL));
    model = true;
    files["objects/midna_navi/audio/call.wav"] = wav({ 200, 300 });
}

static void reassignArchiveClips() {
    const std::string extra = "objects/midna_navi/audio/extra_laugh.wav";
    files[extra] = wav({ 1234, -2345, 3456 });
    files["objects/midna_navi/audio/broken.wav"] = { 0, 1, 2 };
    MMMidnaAudio_Init();
    const auto choices = MMMidnaAudio_GetAvailableClips();
    REQUIRE(std::find(choices.begin(), choices.end(), extra) != choices.end());
    REQUIRE(std::find(choices.begin(), choices.end(), "objects/midna_navi/audio/broken.wav") == choices.end());
    REQUIRE(std::strcmp(MMMidnaAudio_GetEventLabel(MM_MIDNA_AUDIO_APPEAR), "Midna Emerge") == 0);
    const int archiveReads = reads;
    REQUIRE(MMMidnaAudio_Assign(MM_MIDNA_AUDIO_APPEAR, extra));
    REQUIRE(MMMidnaAudio_GetAssignment(MM_MIDNA_AUDIO_APPEAR) == extra);
    REQUIRE(assignments.at("Emerge") == extra);
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_APPEAR));
    std::array<int16_t, 2> out{};
    MMMidnaAudio_Mix(out.data(), 1);
    REQUIRE(out[0] == 1234 && out[1] == 1234);
    REQUIRE(reads == archiveReads); // choosing/playing never opens the archive

    // A remap stops the previous event's tail immediately and updates idle routing too.
    REQUIRE(MMMidnaAudio_Assign(MM_MIDNA_AUDIO_APPEAR, ""));
    out = {};
    MMMidnaAudio_Mix(out.data(), 1);
    REQUIRE(out[0] == 0);
    REQUIRE(!MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_APPEAR));
    REQUIRE(MMMidnaAudio_Assign(MM_MIDNA_AUDIO_YAWN, extra));
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_YAWN));
    out = {};
    MMMidnaAudio_Mix(out.data(), 1);
    REQUIRE(out[0] == 1234);
    REQUIRE(!MMMidnaAudio_Assign(MM_MIDNA_AUDIO_EVENT_COUNT, extra));
    REQUIRE(!MMMidnaAudio_Assign(MM_MIDNA_AUDIO_CALL, "absent.wav"));

    MMMidnaAudio_Init(); // restart uses saved assignments, including explicit native fallback
    REQUIRE(!MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_APPEAR));
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_YAWN));
    out = {};
    MMMidnaAudio_Mix(out.data(), 1);
    REQUIRE(out[0] == 1234);
    files.erase(extra);
    MMMidnaAudio_Init();
    REQUIRE(MMMidnaAudio_GetAssignment(MM_MIDNA_AUDIO_YAWN) == extra);
    REQUIRE(!MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_YAWN)); // removed pack keeps selection, falls back safely
    MMMidnaAudio_ResetAssignments();
    REQUIRE(MMMidnaAudio_GetAssignment(MM_MIDNA_AUDIO_CALL) == "objects/midna_navi/audio/call.wav");
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_CALL));
    files.erase("objects/midna_navi/audio/broken.wav");
    assignments.clear();
}

int main() {
    files["objects/midna_navi/audio/dash.wav"] = wav({ 1000, -2000, 3000, 4000 });
    files["objects/midna_navi/audio/vanish.wav"] = wav({ 500, 600 });
    files["objects/midna_navi/audio/call.wav"] = wav({ 200, 300 });
    reassignArchiveClips();
    reads = 0;
    model = false;
    MMMidnaAudio_Init();
    REQUIRE(!MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_DASH));
    REQUIRE(reads == 0); // audio pack alone must not change native Navi
    model = true;
    MMMidnaAudio_Init();
    REQUIRE(!MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_HINT));
    REQUIRE(!MMMidnaAudio_TryPlay(static_cast<MMMidnaAudioEvent>(-1)));
    REQUIRE(!MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_EVENT_COUNT));
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_DASH));
    const int startupReads = reads;
    std::array<int16_t, 4> output = { 10, 20, 30, 40 };
    MMMidnaAudio_Mix(output.data(), 2);
    REQUIRE((output == std::array<int16_t, 4>{ 1010, 1020, -1970, -1960 }));
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_DASH)); // don't restart a still-playing identical event
    output = {};
    MMMidnaAudio_Mix(output.data(), 2);
    REQUIRE((output == std::array<int16_t, 4>{ 3000, 3000, 4000, 4000 }));
    MMMidnaAudio_Mix(output.data(), 2);
    REQUIRE(output[0] == 3000);     // exhausted clips add nothing
    REQUIRE(reads == startupReads); // no archive I/O on playback or the audio thread

    MMMidnaAudio_Init();
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_DASH));
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_VANISH)); // latest movement replaces earlier movement
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_CALL));   // speech has an independent voice
    output = {};
    MMMidnaAudio_Mix(output.data(), 2);
    REQUIRE((output == std::array<int16_t, 4>{ 700, 700, 900, 900 }));
    gain = 0.5f;
    MMMidnaAudio_Init();
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_DASH));
    output = {};
    MMMidnaAudio_Mix(output.data(), 2);
    REQUIRE(output[0] == 500 && output[2] == -1000);
    gain = 0;
    output = {};
    MMMidnaAudio_Mix(output.data(), 2);
    REQUIRE(output[0] == 0 && output[2] == 0);
    gain = 1;
    MMMidnaAudio_Mix(output.data(), 2);
    REQUIRE(output[0] == 0); // muted audio still advances
    MMMidnaAudio_Init();
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_DASH));
    output = { 32760, -32760, -32760, 32760 };
    MMMidnaAudio_Mix(output.data(), 2);
    REQUIRE(output[0] == 32767 && output[2] == -32768);
    MMMidnaAudio_Reset();
    output = {};
    MMMidnaAudio_Mix(output.data(), 2);
    REQUIRE(output[0] == 0);

    // Unsupported and damaged files must retain native fallback independently.
    auto valid = files["objects/midna_navi/audio/dash.wav"];
    for (auto offset : { 0, 8, 20, 22, 24, 28, 32, 34, 40 }) {
        auto bad = valid;
        bad[offset] ^= 0x7F;
        files["objects/midna_navi/audio/dash.wav"] = bad;
        MMMidnaAudio_Init();
        REQUIRE(!MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_DASH));
        REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_CALL));
    }
    for (size_t length = 0; length < valid.size(); ++length) {
        files["objects/midna_navi/audio/dash.wav"] = { valid.begin(), valid.begin() + length };
        MMMidnaAudio_Init();
        REQUIRE(!MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_DASH));
    }
    // RIFF may contain an odd-sized unknown chunk; skip its alignment byte.
    auto padded = valid;
    padded.insert(padded.begin() + 12, { 'J', 'U', 'N', 'K', 1, 0, 0, 0, 42, 0 });
    le(padded, 4, padded.size() - 8, 4);
    files["objects/midna_navi/audio/dash.wav"] = padded;
    MMMidnaAudio_Init();
    REQUIRE(MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_DASH));
    gain = std::numeric_limits<float>::quiet_NaN();
    output = {};
    MMMidnaAudio_Mix(output.data(), 2);
    REQUIRE(output[0] == 0);
    gain = 1;
    // Exercise the game/audio thread boundary and teardown with real synchronization.
    std::thread mixer([] {
        std::array<int16_t, 8> buffer;
        for (int i = 0; i < 1000; ++i) {
            buffer = {};
            MMMidnaAudio_Mix(buffer.data(), 4);
        }
    });
    for (int i = 0; i < 1000; ++i) {
        REQUIRE(MMMidnaAudio_Assign(MM_MIDNA_AUDIO_CALL, i % 2 ? "objects/midna_navi/audio/vanish.wav"
                                                               : "objects/midna_navi/audio/call.wav"));
        MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_CALL);
        MMMidnaAudio_TryPlay(MM_MIDNA_AUDIO_DASH);
        MMMidnaAudio_Reset();
    }
    mixer.join();
    MMMidnaAudio_ResetAssignments();
    MMMidnaAudio_Reset();
    optOut();
    movementSpacing();
    idleYawn();
    puts(
        "PASS: MM Midna archive choices, saved/live assignments, native fallback, PCM validation, mixing and teardown");
}
