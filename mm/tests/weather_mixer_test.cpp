#include "2s2h/Enhancements/Audio/MMWeatherMixer.h"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <limits>

using namespace MMWeather;

int main() {
    int16_t book[16] = {};
    uint8_t bytes[9] = { 0, 0x18, 0x27, 0x36, 0x45, 0xF1, 0x18, 0x27, 0x36 };
    EncodedSample input{ bytes, sizeof(bytes), 0, book, 16, 2, 1, 0, 16, 1.0 };
    auto decoded = Decode(input);
    assert(decoded.pcm.size() == 16 && decoded.pcm[0] == 1 && decoded.pcm[1] == -8);
    bytes[0] = 1;
    assert(Decode(input).pcm.empty()); // predictor outside codebook
    bytes[0] = 0;
    input.order = 1;
    assert(Decode(input).pcm.empty());
    input.order = 2;
    input.loopEnd = 17;
    assert(Decode(input).pcm.empty());
    input.loopEnd = 16;
    input.rate = std::numeric_limits<double>::quiet_NaN();
    assert(Decode(input).pcm.empty());
    input.rate = 1.0;
    input.codec = 3;
    input.size = 5;
    bytes[1] = 0x1B;
    decoded = Decode(input);
    assert(decoded.pcm.size() == 16 && decoded.pcm[0] == 0 && decoded.pcm[1] == 1 && decoded.pcm[2] == -2 &&
           decoded.pcm[3] == -1);

    Mixer mixer;
    mixer.SetSamples({ { 100, 200, 300, 400 }, 1, 3, 1.0 }, { { 1000, 1000 }, 0, 2, 1.0 },
                     { { 2000, 2000 }, 0, 2, 1.0 });
    mixer.SetRain(1.0f);
    int16_t stereo[12] = {};
    mixer.Mix(stereo, 6, 1.0f);
    const int16_t expected[] = { 100, 200, 300, 200, 300, 200 };
    for (int i = 0; i < 6; ++i) {
        assert(stereo[i * 2] == expected[i] && stereo[i * 2 + 1] == expected[i]);
    }
    mixer.Thunder(1.0f);
    int16_t concurrent[6] = { 10, -10, 10, -10, 10, -10 };
    mixer.Mix(concurrent, 3, 1.0f);
    assert(concurrent[0] == 3310 && concurrent[1] == 3290);
    assert(concurrent[4] == 310 && concurrent[5] == 290); // transients finished, rain survives
    int16_t silent[] = { 1000, -1000, 1000, -1000 };
    mixer.Mix(silent, 2, 0.0f);
    assert(silent[0] == 1000 && silent[1] == -1000);
    mixer.Reset();
    mixer.Mix(silent, 2, 1.0f);
    assert(silent[2] == 1000 && silent[3] == -1000);

    mixer.SetSamples({ { 0, 1000, 2000 }, 0, 3, 0.5 }, {}, {});
    mixer.SetRain(1.0f);
    int16_t resampled[10] = {};
    mixer.Mix(resampled, 5, 1.0f);
    assert(resampled[0] == 0 && resampled[2] == 500 && resampled[4] == 1000 && resampled[8] == 2000);
    mixer.SetSamples({ { 32767 }, 0, 1, 1.0 }, { { 32767 }, 0, 1, 1.0 }, { { 32767 }, 0, 1, 1.0 });
    mixer.SetRain(1.0f);
    mixer.Thunder(1.0f);
    int16_t loud[] = { 32000, -32000 };
    mixer.Mix(loud, 1, 1.0f);
    assert(loud[0] == 32767 && loud[1] == 32767);
    mixer.SetSamples({ { 100 }, 1, 1, 1.0 }, {}, {});
    mixer.SetRain(1.0f);
    int16_t invalid[] = { 7, -7 };
    mixer.Mix(invalid, 1, 1.0f);
    assert(invalid[0] == 7 && invalid[1] == -7);
    std::puts("PASS MM weather mixer: ADPCM bounds, signed residuals, looping, resampling, concurrent voices, mute, "
              "clipping, reset");
}
