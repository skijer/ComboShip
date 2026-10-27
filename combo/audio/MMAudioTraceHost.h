#pragma once
// Included in MM's host TU. All PCM/statistics access belongs to its audio worker;
// resource registration alone can run on importer threads.
#include "MMAudioTrace.h"
#include <mutex>
#include <unordered_map>
#include <fstream>
#include <filesystem>
#include <atomic>
#include <chrono>

namespace MMAudioTrace {
struct Resources {
    std::mutex mutex;
    std::unordered_map<uintptr_t, std::string> samples, sequences;
};
inline Resources& Registry() {
    static Resources r;
    return r;
}
inline std::string Lookup(bool sequence, uintptr_t identity) {
    auto& r = Registry();
    std::lock_guard<std::mutex> lock(r.mutex);
    const auto& map = sequence ? r.sequences : r.samples;
    const auto found = map.find(identity);
    return found == map.end() ? "unregistered" : found->second;
}
inline Trace& WorkerTrace() {
    static Trace trace([](const std::string& line) { SPDLOG_INFO("[MMAudioPCM] {}", line); }, Lookup);
    return trace;
}
inline std::atomic<int> scene{ -1 }, room{ -1 }, mode{ -1 };
inline void Poll() try {
    auto& trace = WorkerTrace();
    const bool wasEnabled = trace.enabled;
    trace.SetEnabled(CVarGetInteger("gDeveloperTools.MMAudioPCMTrace", 0) != 0);
    if (!trace.enabled)
        return;
    static int previousScene = -2, previousRoom = -2, previousMode = -2, previousClean = -1, previousAlt = -1;
    const int s = scene.load(), r = room.load(), m = mode.load();
    const int clean = CVarGetInteger("gEnhancements.Fixes.MMStreamedPositionalAudio", 0);
    const int alt = CVarGetInteger("gEnhancements.Mods.AlternateAssets", 0);
    if (!wasEnabled || s != previousScene || r != previousRoom || m != previousMode || clean != previousClean ||
        alt != previousAlt) {
        trace.Flush();
        SPDLOG_INFO("[MMAudioPCM] context scene={} room={} mode={} alt={} gainBufferFix=1 cleanStreamedPositional={}",
                    s, r, m, alt, clean);
        previousScene = s;
        previousRoom = r;
        previousMode = m;
        previousClean = clean;
        previousAlt = alt;
    }
} catch (...) {}
inline void Native(const int16_t* pcm, size_t frames) {
    try {
        WorkerTrace().Native(pcm, frames);
    } catch (...) {}
}
inline void Final(const int16_t* pcm, size_t frames, bool muted) {
    try {
        WorkerTrace().Final(pcm, frames, muted);
    } catch (...) {}
}
inline void Queue(int frames, int gap) {
    try {
        WorkerTrace().Queue(frames, gap);
    } catch (...) {}
}
inline void Stopped() {
    try {
        if (WorkerTrace().enabled)
            WorkerTrace().Flush();
        WorkerTrace().enabled = false; // Resume starts a fresh summary window; the joined host saves capture.
    } catch (...) {}
}
// Save after joining the worker. No audio-thread disk writes or capture threads.
inline void SaveCapture(const std::string& directory) {
    auto& trace = WorkerTrace();
    if (trace.capture.empty())
        return;
    std::filesystem::create_directories(directory);
    const auto id = std::chrono::system_clock::now().time_since_epoch().count();
    const auto path = std::filesystem::path(directory) / ("mm-audio-pcm-" + std::to_string(id) + ".wav");
    std::ofstream file(path, std::ios::binary);
    const uint32_t bytes = static_cast<uint32_t>(trace.capture.size() * sizeof(int16_t));
    const auto word = [&](uint32_t value, unsigned width) {
        for (unsigned i = 0; i < width; ++i)
            file.put(static_cast<char>(value >> (i * 8)));
    };
    file.write("RIFF", 4);
    word(36 + bytes, 4);
    file.write("WAVEfmt ", 8);
    word(16, 4);
    word(1, 2);
    word(2, 2);
    word(32000, 4);
    word(32000 * 4, 4);
    word(4, 2);
    word(16, 2);
    file.write("data", 4);
    word(bytes, 4);
    for (const auto sample : trace.capture)
        word(static_cast<uint16_t>(sample), 2);
    file.close();
    if (file.fail()) {
        SPDLOG_ERROR("[MMAudioPCM] capture write failed: {}", path.string());
        return; // Keep the evidence available for a later successful shutdown save.
    }
    SPDLOG_INFO("[MMAudioPCM] capture={} frames={} rate=32000 channels=2", path.string(), trace.capture.size() / 2);
    trace.capture.clear();
}
} // namespace MMAudioTrace

extern "C" int MM_AudioTraceEnabled(void) try { return MMAudioTrace::WorkerTrace().enabled; } catch (...) {
    return 0;
}
extern "C" unsigned MM_AudioTraceNoteSamples(unsigned n) {
    return MMAudioTrace::WorkerTrace().NoteSamples(n);
}
extern "C" void MM_AudioTraceSetNote(const MMAudioTraceNote* note) {
    try {
        MMAudioTrace::WorkerTrace().SetNote(*note);
    } catch (...) {}
}
extern "C" void MM_AudioTracePCM(int stage, const int16_t* pcm, size_t count, unsigned clips) {
    try {
        MMAudioTrace::WorkerTrace().PCM(stage, pcm, count, clips);
    } catch (...) {}
}
extern "C" void MM_AudioTraceEvent(const char* event, int requested, int actual) {
    try {
        MMAudioTrace::WorkerTrace().Event(event, requested, actual);
    } catch (...) {}
}
extern "C" void MM_AudioTraceResource(int sequence, uintptr_t identity, const char* resource, const char* source,
                                      int streamed) {
    try {
        auto& r = MMAudioTrace::Registry();
        std::lock_guard<std::mutex> lock(r.mutex);
        auto& map = sequence ? r.sequences : r.samples;
        if (map.size() >= 4096 && !map.count(identity))
            return;
        map[identity] =
            std::string(resource ? resource : "").substr(0, 512) + " source=" +
            std::string(source ? source : (sequence && streamed ? "generated" : "embedded")).substr(0, 512) +
            " streamed=" + std::to_string(streamed);
    } catch (...) {}
}
extern "C" void MM_AudioTraceAlias(int sequence, uintptr_t destination, uintptr_t source) {
    try {
        auto& r = MMAudioTrace::Registry();
        std::lock_guard<std::mutex> lock(r.mutex);
        auto& map = sequence ? r.sequences : r.samples;
        const auto found = map.find(source);
        if (found != map.end() && (map.size() < 4096 || map.count(destination))) {
            const auto description = found->second;
            map[destination] = description;
        }
    } catch (...) {}
}
extern "C" void MM_AudioTraceFormat(uintptr_t sample, unsigned pcmRate, unsigned channels, uint64_t frames) {
    try {
        auto& r = MMAudioTrace::Registry();
        std::lock_guard<std::mutex> lock(r.mutex);
        const auto found = r.samples.find(sample);
        if (found != r.samples.end()) {
            found->second += " pcmRate=" + std::to_string(pcmRate) + " sourceChannels=" + std::to_string(channels) +
                             " sourceFrames=" + std::to_string(frames);
        }
    } catch (...) {}
}
