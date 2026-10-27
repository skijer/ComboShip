#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace MMWeather {
struct EncodedSample {
    const uint8_t* data;
    size_t size;
    int codec;
    const int16_t* book;
    size_t bookSize;
    int order;
    int predictors;
    size_t loopStart;
    size_t loopEnd;
    double rate;
};

struct DecodedSample {
    std::vector<int16_t> pcm;
    size_t loopStart = 0;
    size_t loopEnd = 0;
    double rate = 1.0;
};

DecodedSample Decode(const EncodedSample& input);

// The owner serializes access. Voices index owned PCM, never game resources.
class Mixer {
  public:
    void SetSamples(DecodedSample rain, DecodedSample rumble, DecodedSample lightning);
    void SetRain(float gain);
    void Thunder(float gain);
    void Mix(int16_t* interleavedStereo, size_t frames, float gain, float thunderGain = 1.0f);
    void Reset();

  private:
    struct Voice {
        bool active = false;
        double position = 0.0;
        float gain = 0.0f;
    };
    double Next(Voice& voice, const DecodedSample& sample, bool loop);
    std::array<DecodedSample, 3> samples;
    std::array<Voice, 3> voices;
};
} // namespace MMWeather
