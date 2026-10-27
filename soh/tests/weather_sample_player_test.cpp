#include "test_require.h"
#include <cstring>
#include <cstdint>

#include "../soh/Enhancements/audio/WeatherSamplePlayer.h"
#include "weather_audio_fixture.h"

static int32_t sSfxVolume = 100;
extern "C" SoundFontSample* ResourceMgr_LoadAudioSample(const char* path) {
    return WeatherAudioFixture(path);
}
extern "C" int32_t CVarGetInteger(const char* name, int32_t fallback) {
    if (std::strcmp(name, "gAudioEditor.WeatherAudioDiagnostics") == 0)
        return 0;
    (void)fallback;
    return sSfxVolume;
}

static void TestLoopAndOneShotsPreserveEngineMix() {
    WeatherSamplePlayer_Init();
    WeatherSamplePlayer_SetLoop("audio/samples/Rainfall_META", 1.0f);
    const bool thunderStarted = WeatherSamplePlayer_Play("audio/samples/Low Thunder_META", 1.0f);
    const bool lightningStarted = WeatherSamplePlayer_Play("audio/samples/Lightning_META", 1.0f);
    const bool excessStarted = WeatherSamplePlayer_Play("audio/samples/Low Thunder_META", 1.0f);
    REQUIRE(thunderStarted);
    REQUIRE(lightningStarted);
    REQUIRE(!excessStarted);
    // Arbitrary incoming stereo PCM checks additive mixing only. Real engine
    // SFX eligibility is covered by global_outdoor_rain_audio_test.
    int16_t mixed[] = { 10, -10, 20, -20, 30, -30, 40, -40, 50, -50, 60, -60 };
    WeatherSamplePlayer_Mix(mixed, 6);
    const int16_t expected[] = { 16, -4, 27, -13, 38, -22, 49, -31, 60, -40, 71, -49 };
    for (size_t i = 0; i < 12; ++i)
        REQUIRE(mixed[i] == expected[i]);

    WeatherSamplePlayer_SetLoop("audio/samples/Rainfall_META", 0.5f);
    int16_t next[] = { 100, -100 };
    WeatherSamplePlayer_Mix(next, 1);
    REQUIRE(next[0] == 108 && next[1] == -92); // rain position 6: 7 * .5 -> 3; shots: 2 + 3

    WeatherSamplePlayer_SetLoop(nullptr, 0.0f);
    int16_t afterStop[] = { 100, -100 };
    WeatherSamplePlayer_Mix(afterStop, 1);
    REQUIRE(afterStop[0] == 105 && afterStop[1] == -95);
    WeatherSamplePlayer_SetLoop("missing", 1.0f);
    sSfxVolume = 0;
    WeatherSamplePlayer_Mix(afterStop, 1);
    REQUIRE(afterStop[0] == 105 && afterStop[1] == -95);
    sSfxVolume = 100;
    int16_t tail[14] = {};
    WeatherSamplePlayer_Mix(tail, 7);
    for (int16_t sample : tail)
        REQUIRE(sample == 5);
    int16_t silent[2] = {};
    WeatherSamplePlayer_Mix(silent, 1);
    REQUIRE(silent[0] == 0 && silent[1] == 0);

    WeatherSamplePlayer_SetLoop("audio/samples/Rainfall_META", 1.0f);
    int16_t wrapped[36] = {};
    WeatherSamplePlayer_Mix(wrapped, 18);
    // Play the intro once through loopEnd, then return to the authored
    // loopStart instead of restarting the sample at zero.
    REQUIRE(wrapped[22] == 5 && wrapped[24] == 5 && wrapped[26] == 6);
    REQUIRE(wrapped[30] == 1 && wrapped[32] == 2 && wrapped[34] == 3);
    WeatherSamplePlayer_Reset();
}

static void TestResourceLoopEndBoundsThunderWithoutReadingPadding() {
    uint8_t smallAdpcm[5] = { 0, 0x55, 0x55, 0x55, 0x55 };
    for (const char* path : { "audio/samples/Low Thunder_META", "audio/samples/Lightning_META" }) {
        SoundFontSample* sample = WeatherAudioFixture(path);
        const SoundFontSample original = *sample;
        const int16_t nativeValue = std::strstr(path, "Low Thunder") != nullptr ? 2 : 3;
        for (uint32_t codec : { 0u, 3u }) {
            *sample = original;
            if (codec == 3) {
                sample->codec = codec;
                sample->sampleAddr = smallAdpcm;
                sample->size = sample->fileSize = sizeof(smallAdpcm);
            }
            for (uint32_t end : { 16u, 12u }) {
                for (uint32_t padding : { 1u, 0u, UINT32_MAX }) {
                    // AudioSampleFactory fills start/end/count/state. Its loop
                    // offset 0x0C is unused padding, not a sample-length field.
                    AdpcmLoop loop = { 4, end, 0, padding, {} };
                    sample->loop = &loop;
                    WeatherSamplePlayer_Init();
                    REQUIRE(WeatherSamplePlayer_Play(path, 1.0f));
                    int16_t mixed[32] = {};
                    WeatherSamplePlayer_Mix(mixed, 16);
                    const int16_t value = codec == 3 ? 1 : nativeValue;
                    for (size_t frame = 0; frame < 16; ++frame) {
                        const int16_t expected = frame < end ? value : 0;
                        REQUIRE(mixed[frame * 2] == expected);
                        REQUIRE(mixed[frame * 2 + 1] == expected);
                    }
                }
            }
        }
        *sample = original;
    }
    WeatherSamplePlayer_Reset();
}

int main() {
    TestLoopAndOneShotsPreserveEngineMix();
    TestResourceLoopEndBoundsThunderWithoutReadingPadding();
    REQUIRE(WeatherSamplePlayer_ClampGain(-1.0f) == 0.0f);
    REQUIRE(WeatherSamplePlayer_ClampGain(0.35f) == 0.35f);
    REQUIRE(WeatherSamplePlayer_ClampGain(2.0f) == 1.0f);

    int16_t destination[] = { 32000, -32000, 100, -100 };
    const int16_t source[] = { 2000, -2000 };
    const size_t clampCount = WeatherSamplePlayer_TestMixMonoCountClamps(destination, source, 2, 1.0f);
    REQUIRE(clampCount == 1);
    REQUIRE(destination[0] == 32767);
    REQUIRE(destination[1] == -30000);
    REQUIRE(destination[2] == -1900);
    REQUIRE(destination[3] == -2100);

    size_t position = 3;
    REQUIRE(WeatherSamplePlayer_AdvanceLoopPosition(11, 4, 12, 1) == 4);
    REQUIRE(WeatherSamplePlayer_AdvanceLoopPosition(11, 4, 12, 7) == 10);
    REQUIRE(WeatherSamplePlayer_AdvanceLoopPosition(position, 0, 0, 6) == 0);
    return 0;
}
