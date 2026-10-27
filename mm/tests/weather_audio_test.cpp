#include "2s2h/Enhancements/Audio/MMWeatherAudio.h"
#include "audio/soundfont.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstring>

extern "C" {
char* fontPaths[3] = { nullptr, nullptr, const_cast<char*>("mm-nature") };
char** gFontMap = fontPaths;
size_t gFontMapSize = 3;
float gPitchFrequencies[128];
}
static SoundFont font{};
static float master = 1.0f;
static float sfx = 1.0f;
static int loads;
static int thunderVolume = 100;
extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    return std::strcmp(key, "gAudioEditor.MMWeather.ThunderVolume") == 0 ? thunderVolume : fallback;
}
extern "C" float CVarGetFloat(const char* key, float fallback) {
    if (std::strcmp(key, "gSettings.Audio.MasterVolume") == 0) {
        return master;
    }
    if (std::strcmp(key, "gSettings.Audio.SoundEffectsVolume") == 0) {
        return sfx;
    }
    return fallback;
}
extern "C" SoundFont* ResourceMgr_LoadAudioSoundFontByName(const char* path) {
    assert(std::strcmp(path, "mm-nature") == 0);
    ++loads;
    return &font;
}

#ifdef COMBO_BUILD
// The extracted production submission block now observes final PCM. Retain
// that real observer while replacing only its log sink in this adapter test.
template <class... T> static void TestTraceLog(const T&...) {
}
#define SPDLOG_INFO(...) TestTraceLog(__VA_ARGS__)
#define SPDLOG_ERROR(...) TestTraceLog(__VA_ARGS__)
#include "../../combo/audio/MMAudioTraceHost.h"
#endif

static std::atomic<bool> gFscAudioMuted{ false };
static bool midnaVoicePending = true;
static int midnaMixCalls;
static int midnaResetCalls;
static int emoteMixCalls;
static void MMMidnaAudio_Mix(int16_t* buffer, size_t frames) {
    ++midnaMixCalls;
    if (midnaVoicePending) {
        for (size_t i = 0; i < frames * 2; ++i) {
            buffer[i] += 100;
        }
    }
}
static void MMMidnaAudio_Reset() {
    ++midnaResetCalls;
    midnaVoicePending = false;
}
static void ComboEmotes_MixAudio(int16_t* buffer, size_t frames) {
    ++emoteMixCalls;
    for (size_t i = 0; i < frames * 2; ++i) {
        buffer[i] += 200;
    }
}
static int16_t played[2];
static void AudioPlayer_Play(uint8_t* buffer, size_t size) {
    assert(size == sizeof(played));
    std::memcpy(played, buffer, size);
}
#define AUDIO_FRAMES_PER_UPDATE 1
#define NUM_AUDIO_CHANNELS 2
#include "weather_output.inc"

int main() {
    int16_t coefficients[16] = {};
    AdpcmBook book{ 2, 1, coefficients };
    AdpcmLoop loop{};
    loop.loopEnd = 16;
    uint8_t data[9] = { 0xA0, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 };
    Sample sample{};
    sample.sampleAddr = data;
    sample.size = sizeof(data);
    sample.book = &book;
    sample.loop = &loop;
    Instrument instrument{};
    instrument.normalRangeHi = 127;
    instrument.normalPitchTunedSample = { &sample, 1.0f };
    Instrument* instruments[21] = {};
    instruments[18] = instruments[19] = instruments[20] = &instrument;
    font.numInstruments = 21;
    font.instruments = instruments;
    gPitchFrequencies[0x27] = 1.0f;
    gPitchFrequencies[0x18] = 0.420448f;
    MMWeatherAudio_SetRain(0);
    assert(loads == 0);
    MMWeatherAudio_SetRain(1);
    assert(loads == 1);
    int16_t output[4] = {};
    MMWeatherAudio_Mix(output, 2);
    assert(output[0] == 1024 && output[1] == 1024 && loads == 1);
    gFscAudioMuted = true;
#ifdef COMBO_BUILD
    MMAudioTrace::WorkerTrace().SetEnabled(true);
#endif
    int16_t inactive[] = { 123, -123 };
    SubmitWeatherAudio(inactive, 1);
    assert(played[0] == 0 && played[1] == 0);
#ifdef COMBO_BUILD
    const auto& captured = MMAudioTrace::WorkerTrace().capture;
    assert(captured.size() == 2 && captured[0] == 0 && captured[1] == 0);
#endif
    assert(midnaMixCalls == 1 && midnaResetCalls == 1 && !midnaVoicePending);
    assert(emoteMixCalls == 1);
    gFscAudioMuted = false;
    for (int channel = 0; channel < 2; ++channel) {
        master = channel == 0 ? 0.0f : 1.0f;
        sfx = channel == 1 ? 0.0f : 1.0f;
        MMWeatherAudio_Thunder(1);
        int16_t muted[] = { 11, -11 };
        MMWeatherAudio_Mix(muted, 1);
        assert(muted[0] == 11 && muted[1] == -11);
    }
    master = sfx = 1;
    MMWeatherAudio_Reset();
    int16_t active[] = { 21, -21 };
    SubmitWeatherAudio(active, 1);
    assert(played[0] == 221 && played[1] == 179 && emoteMixCalls == 2);
    MMWeatherAudio_Thunder(1);
    thunderVolume = 0; // A live slider change must also mute an already playing shot.
    int16_t quietThunder[] = { 19, -19 };
    MMWeatherAudio_Mix(quietThunder, 1);
    assert(quietThunder[0] == 19 && quietThunder[1] == -19);
    thunderVolume = 100;
    MMWeatherAudio_Reset();
    int16_t reset[] = { 13, -13 };
    MMWeatherAudio_Mix(reset, 1);
    assert(reset[0] == 13 && reset[1] == -13);
    MMWeatherAudio_Shutdown();
    MMWeatherAudio_SetRain(1);
    assert(loads == 2);
    MMWeatherAudio_Shutdown();
    font.instruments = nullptr;
    MMWeatherAudio_SetRain(1);
    int16_t missing[] = { 17, -17 };
    MMWeatherAudio_Mix(missing, 1);
    assert(missing[0] == 17 && missing[1] == -17);
    for (int i = 0; i < 100; ++i) {
        MMWeatherAudio_SetRain(1);
    }
    assert(loads == 3);
    std::puts(
        "PASS MM weather audio adapter: native instruments, lazy load, master/SFX mute, reset, reload, missing sample");
}
