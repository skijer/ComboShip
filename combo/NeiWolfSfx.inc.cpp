// Wolf-only PCM player, included by each host's Wolf form. Like Stasis and
// Hourglass, it uses the native post-synth stereo mixer seam. Clip storage is
// immutable; the mutex protects cursors and publication across both threads.
#include <array>
#include <cstdint>
#include <mutex>

namespace {
struct WolfAudioClip {
    const char* source;
    const std::int16_t* samples;
    std::uint32_t count;
    float rate;
};
#include "assets/wolf_link_sfx_pcm.inc"

enum class WolfAudioCue { Enter, Exit, Bite, Jump, Spin, Exertion };
struct WolfAudioVoice {
    const WolfAudioClip* clip = nullptr;
    double position = 0;
    float gain = 1;
};
struct WolfAudioState {
    std::mutex mutex;
    std::array<WolfAudioVoice, 2> voices{}; // transform and attack can overlap
    std::array<std::uint32_t, 4> variants{};
    bool live = false;
    bool paused = false;
    float settingsGain = 0;
    std::uint32_t aliveSamples = 0;
};
WolfAudioState sWolfAudio;
constexpr std::uint32_t kWolfOutputRate = 32000;
constexpr std::uint32_t kWolfAliveSamples = kWolfOutputRate / 2;

static void WolfAudio_Stop() {
    std::lock_guard<std::mutex> lock(sWolfAudio.mutex);
    sWolfAudio.voices = {};
}

// Called on the game thread, including Human exit tails and paused frames.
static void WolfAudio_Publish(bool live, bool paused, float settingsGain) {
    std::lock_guard<std::mutex> lock(sWolfAudio.mutex);
    sWolfAudio.live = live;
    sWolfAudio.paused = paused;
    sWolfAudio.settingsGain = std::clamp(settingsGain, 0.0f, 1.0f);
    sWolfAudio.aliveSamples = live ? kWolfAliveSamples : 0;
    if (!live)
        sWolfAudio.voices = {};
}

static void WolfAudio_Play(WolfAudioCue cue, float gain) {
    std::lock_guard<std::mutex> lock(sWolfAudio.mutex);
    if (!sWolfAudio.live)
        return;
    std::uint32_t clip = 0;
    std::uint32_t voice = 0;
    switch (cue) {
        case WolfAudioCue::Enter:
            clip = 0;
            break;
        case WolfAudioCue::Exit:
            clip = 1;
            break;
        case WolfAudioCue::Bite:
            clip = 2 + (sWolfAudio.variants[0]++ % 5);
            voice = 1;
            break;
        case WolfAudioCue::Jump:
            clip = 7 + (sWolfAudio.variants[1]++ % 2);
            voice = 1;
            break;
        case WolfAudioCue::Spin:
            clip = 9 + (sWolfAudio.variants[2]++ % 3);
            voice = 1;
            break;
        case WolfAudioCue::Exertion:
            clip = 12 + (sWolfAudio.variants[3]++ % 3);
            voice = 1;
            break;
    }
    sWolfAudio.voices[voice] = { &kWolfClips[clip], 0, std::clamp(gain, 0.0f, 1.0f) };
}
} // namespace

// Each host supplies its own export name; no shared DLL audio state or owner.
extern "C" void WOLF_AUDIO_MIX_SYMBOL(std::int16_t* output, std::uint32_t frames) {
    if (!output || frames == 0)
        return;
    std::lock_guard<std::mutex> lock(sWolfAudio.mutex);
    if (!sWolfAudio.live || frames > sWolfAudio.aliveSamples) {
        sWolfAudio.voices = {};
        sWolfAudio.aliveSamples = 0;
        return;
    }
    sWolfAudio.aliveSamples -= frames;
    if (sWolfAudio.paused)
        return;
    for (std::uint32_t i = 0; i < frames; ++i) {
        double mixed = 0;
        for (auto& voice : sWolfAudio.voices) {
            if (!voice.clip)
                continue;
            const auto& clip = *voice.clip;
            if (voice.position >= clip.count) {
                voice = {};
                continue;
            }
            const auto index = static_cast<std::uint32_t>(voice.position);
            const auto next = std::min(index + 1, clip.count - 1);
            const double fraction = voice.position - index;
            const double sample = clip.samples[index] + (clip.samples[next] - clip.samples[index]) * fraction;
            mixed += sample * voice.gain * sWolfAudio.settingsGain;
            voice.position += clip.rate / kWolfOutputRate;
        }
        const auto sample = static_cast<std::int32_t>(mixed);
        for (std::uint32_t channel = 0; channel < 2; ++channel) {
            const auto outIndex = i * 2 + channel;
            output[outIndex] =
                static_cast<std::int16_t>(std::clamp<std::int32_t>(output[outIndex] + sample, -32768, 32767));
        }
    }
}
#undef WOLF_AUDIO_MIX_SYMBOL
