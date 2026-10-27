#include "MMWeatherMixer.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace MMWeather {
namespace {
int16_t ClampSample(int64_t value) {
    return static_cast<int16_t>(std::clamp<int64_t>(value, -32768, 32767));
}

float Gain(float value) {
    return std::isfinite(value) ? std::clamp(value, 0.0f, 1.0f) : 0.0f;
}

bool Valid(const DecodedSample& sample) {
    return !sample.pcm.empty() && sample.loopStart < sample.loopEnd && sample.loopEnd <= sample.pcm.size() &&
           std::isfinite(sample.rate) && sample.rate > 0.0 && sample.rate <= 16.0;
}
} // namespace

DecodedSample Decode(const EncodedSample& input) {
    const size_t frameSize = input.codec == 0 ? 9 : input.codec == 3 ? 5 : 0;
    if (frameSize == 0 || input.data == nullptr || input.book == nullptr || input.order != 2 || input.predictors < 1 ||
        input.predictors > 16 || input.bookSize < static_cast<size_t>(input.predictors * 16) ||
        input.size < frameSize || !std::isfinite(input.rate) || input.rate <= 0.0 || input.rate > 16.0) {
        return {};
    }
    // ROM sample sizes can include alignment bytes after the last complete frame.
    const size_t count = input.size / frameSize * 16;
    const size_t end = input.loopEnd == 0 ? count : input.loopEnd;
    if (end > count || input.loopStart >= end) {
        return {};
    }
    DecodedSample decoded{ {}, input.loopStart, end, input.rate };
    decoded.pcm.reserve(end);
    int16_t history[2] = {};
    for (size_t offset = 0; offset + frameSize <= input.size && decoded.pcm.size() < end; offset += frameSize) {
        const uint8_t* frame = input.data + offset;
        const int predictor = frame[0] & 15;
        const int shift = frame[0] >> 4;
        if (predictor >= input.predictors) {
            return {};
        }
        const int16_t* first = input.book + predictor * 16;
        const int16_t* second = first + 8;
        for (size_t half = 0; half < 2 && decoded.pcm.size() < end; ++half) {
            int16_t residual[8] = {};
            for (size_t i = 0; i < 8; ++i) {
                const size_t index = half * 8 + i;
                const int bits = input.codec == 0 ? 4 : 2;
                const int perByte = 8 / bits;
                const int mask = (1 << bits) - 1;
                int value = (frame[1 + index / perByte] >> (8 - bits * (index % perByte + 1))) & mask;
                if (value >= (1 << (bits - 1))) {
                    value -= 1 << bits;
                }
                // ADPCM residuals are signed 16-bit; multiplying avoids undefined
                // negative left shifts in the donor decoder.
                residual[i] = static_cast<int16_t>(value * (1 << std::min(shift, 16 - bits)));
            }
            int16_t output[8] = {};
            for (size_t i = 0; i < 8; ++i) {
                int64_t value = static_cast<int64_t>(first[i]) * history[0] +
                                static_cast<int64_t>(second[i]) * history[1] + residual[i] * 2048;
                for (size_t previous = 0; previous < i; ++previous) {
                    value += static_cast<int64_t>(second[i - previous - 1]) * residual[previous];
                }
                output[i] = ClampSample(value >> 11);
                if (decoded.pcm.size() < end) {
                    decoded.pcm.push_back(output[i]);
                }
            }
            history[0] = output[6];
            history[1] = output[7];
        }
    }
    return decoded;
}

void Mixer::SetSamples(DecodedSample rain, DecodedSample rumble, DecodedSample lightning) {
    Reset();
    samples = { std::move(rain), std::move(rumble), std::move(lightning) };
    for (auto& sample : samples) {
        if (!Valid(sample)) {
            sample = {};
        }
    }
}

void Mixer::SetRain(float gain) {
    gain = Gain(gain);
    if (gain == 0.0f || samples[0].pcm.empty()) {
        voices[0] = {};
    } else {
        voices[0].active = true;
        voices[0].gain = gain;
    }
}

void Mixer::Thunder(float gain) {
    for (size_t i = 1; i < voices.size(); ++i) {
        if (!voices[i].active && !samples[i].pcm.empty() && Gain(gain) > 0.0f) {
            voices[i] = { true, 0.0, Gain(gain) };
        }
    }
}

double Mixer::Next(Voice& voice, const DecodedSample& sample, bool loop) {
    if (!voice.active) {
        return 0.0;
    }
    const size_t index = static_cast<size_t>(voice.position);
    const size_t next = index + 1 < sample.loopEnd ? index + 1 : loop ? sample.loopStart : index;
    const double fraction = voice.position - index;
    const double value = (sample.pcm[index] + (sample.pcm[next] - sample.pcm[index]) * fraction) * voice.gain;
    voice.position += sample.rate;
    if (voice.position >= sample.loopEnd) {
        if (loop) {
            voice.position =
                sample.loopStart + std::fmod(voice.position - sample.loopEnd, sample.loopEnd - sample.loopStart);
        } else {
            voice = {};
        }
    }
    return value;
}

void Mixer::Mix(int16_t* interleavedStereo, size_t frames, float gain, float thunderGain) {
    if (interleavedStereo == nullptr) {
        return;
    }
    gain = Gain(gain);
    thunderGain = Gain(thunderGain);
    for (size_t frame = 0; frame < frames; ++frame) {
        double weather = 0.0;
        for (size_t i = 0; i < voices.size(); ++i) {
            weather += Next(voices[i], samples[i], i == 0) * (i == 0 ? 1.0f : thunderGain);
        }
        const int64_t added = static_cast<int64_t>(std::lround(weather * gain));
        interleavedStereo[frame * 2] = ClampSample(interleavedStereo[frame * 2] + added);
        interleavedStereo[frame * 2 + 1] = ClampSample(interleavedStereo[frame * 2 + 1] + added);
    }
}

void Mixer::Reset() {
    voices = {};
}
} // namespace MMWeather
