#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>

namespace ComboEmotes {
// Synchronization belongs to the game adapter. No allocation in the audio callback.
class AudioVoice {
  public:
    void Start(std::span<const int16_t> samples) {
        mSamples = samples;
        mPosition = 0;
    }
    void Stop() {
        mSamples = {};
        mPosition = 0;
    }
    void Mix(int16_t* stereo, size_t frames, float gain) {
        if (!stereo || !std::isfinite(gain))
            return;
        gain = std::clamp(gain, 0.0f, 1.0f);
        const size_t count = std::min(frames, mSamples.size() - mPosition);
        for (size_t i = 0; i < count; ++i) {
            const int sample = static_cast<int>(mSamples[mPosition + i] * gain);
            for (size_t ch = 0; ch < 2; ++ch) {
                stereo[i * 2 + ch] = static_cast<int16_t>(std::clamp<int>(stereo[i * 2 + ch] + sample, -32768, 32767));
            }
        }
        mPosition += count;
    }

  private:
    std::span<const int16_t> mSamples;
    size_t mPosition = 0;
};
} // namespace ComboEmotes
