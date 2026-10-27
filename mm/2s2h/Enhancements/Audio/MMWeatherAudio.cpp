#include "MMWeatherAudio.h"
#include "MMWeather.h"
#include "MMWeatherMixer.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <exception>
#include <mutex>
#include "audio/soundfont.h"
#include <libultraship/bridge/consolevariablebridge.h>

extern "C" {
extern char** gFontMap;
extern size_t gFontMapSize;
extern float gPitchFrequencies[];
// BenPort resolves this through CrossRMRegistry::GetOrActive("mm"), including
// nested sample loads. It does not depend on which game's resource manager is active.
SoundFont* ResourceMgr_LoadAudioSoundFontByName(const char* path);
}

namespace {
std::mutex sMutex;
MMWeather::Mixer sMixer;
bool sLoadAttempted = false;

MMWeather::DecodedSample LoadInstrument(const SoundFont* font, unsigned int index, unsigned int note) {
    if (font == nullptr || font->instruments == nullptr || index >= font->numInstruments ||
        font->instruments[index] == nullptr) {
        return {};
    }
    const Instrument* instrument = font->instruments[index];
    const TunedSample& tuned = note < instrument->normalRangeLo   ? instrument->lowPitchTunedSample
                               : note > instrument->normalRangeHi ? instrument->highPitchTunedSample
                                                                  : instrument->normalPitchTunedSample;
    const Sample* sample = tuned.sample;
    if (sample == nullptr || sample->book == nullptr || sample->book->order != 2 || sample->book->numPredictors < 1 ||
        sample->book->numPredictors > 16) {
        return {};
    }
    // The MM resource's AdpcmLoop.end is the native ABI's loopEnd at offset 4.
    // Do not read sampleEnd: the resource does not populate that extension.
    const MMWeather::EncodedSample input = {
        sample->sampleAddr,
        sample->size,
        static_cast<int>(sample->codec),
        sample->book->codeBook,
        static_cast<size_t>(sample->book->order * sample->book->numPredictors * 8),
        sample->book->order,
        sample->book->numPredictors,
        sample->loop != nullptr ? sample->loop->start : 0,
        sample->loop != nullptr ? sample->loop->loopEnd : 0,
        tuned.tuning * gPitchFrequencies[note],
    };
    return MMWeather::Decode(input);
}

void EnsureSamples() {
    // Called only by the game thread. AudioLoad_Init creates the font map after
    // OTRAudio_Init, so defer until an added shower actually needs audio.
    if (sLoadAttempted || gFontMap == nullptr || gFontMapSize == 0) {
        return;
    }
    sLoadAttempted = true;
    try {
        const SoundFont* font =
            gFontMapSize > 2 && gFontMap[2] != nullptr ? ResourceMgr_LoadAudioSoundFontByName(gFontMap[2]) : nullptr;
        // Native ambience sequence 1: font 2, rain instrument 18 (C4),
        // low thunder 19 (A2), lightning 20 (C4). No guessed sample filenames.
        auto rain = LoadInstrument(font, 18, 0x27);
        auto rumble = LoadInstrument(font, 19, 0x18);
        auto lightning = LoadInstrument(font, 20, 0x27);
        if (rain.pcm.empty() || rumble.pcm.empty() || lightning.pcm.empty()) {
            std::fprintf(stderr,
                         "[MM weather] unavailable/unsupported nature samples: rain=%d rumble=%d lightning=%d; "
                         "native audio remains active\n",
                         !rain.pcm.empty(), !rumble.pcm.empty(), !lightning.pcm.empty());
        }
        std::lock_guard<std::mutex> lock(sMutex);
        sMixer.SetSamples(std::move(rain), std::move(rumble), std::move(lightning));
    } catch (const std::exception& exception) {
        std::fprintf(stderr, "[MM weather] unable to load nature samples: %s\n", exception.what());
    }
}

float Volume(const char* key, float fallback) {
    const float value = CVarGetFloat(key, fallback);
    return std::isfinite(value) ? std::clamp(value, 0.0f, 1.0f) : 0.0f;
}
} // namespace

extern "C" void MMWeatherAudio_SetRain(float gain) {
    if (gain > 0.0f) {
        EnsureSamples();
    }
    std::lock_guard<std::mutex> lock(sMutex);
    sMixer.SetRain(gain);
}

extern "C" void MMWeatherAudio_Thunder(float gain) {
    if (gain > 0.0f) {
        EnsureSamples();
    }
    std::lock_guard<std::mutex> lock(sMutex);
    sMixer.Thunder(gain);
}

extern "C" void MMWeatherAudio_Mix(int16_t* interleavedStereo, size_t frameCount) {
    const float volume =
        Volume("gSettings.Audio.MasterVolume", 0.4f) * Volume("gSettings.Audio.SoundEffectsVolume", 1.0f);
    std::lock_guard<std::mutex> lock(sMutex);
    const float thunderVolume = std::clamp(CVarGetInteger(MM_WEATHER_CVAR("ThunderVolume"), 100), 0, 100) / 100.0f;
    sMixer.Mix(interleavedStereo, frameCount, volume, thunderVolume);
}

extern "C" void MMWeatherAudio_Reset() {
    std::lock_guard<std::mutex> lock(sMutex);
    sMixer.Reset();
}

extern "C" void MMWeatherAudio_Shutdown() {
    // The audio worker is joined by the caller; next MM entry reloads its own samples.
    std::lock_guard<std::mutex> lock(sMutex);
    sMixer.SetSamples({}, {}, {});
    sLoadAttempted = false;
}
