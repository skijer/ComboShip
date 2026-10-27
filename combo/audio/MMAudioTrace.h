#pragma once
#include "MMAudioTraceBridge.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <sstream>
#include <string>
#include <vector>

namespace MMAudioTrace {
struct Stats {
    uint64_t count = 0, squares = 0, rails = 0, zeros = 0, wouldClip = 0;
    uint64_t fingerprint = 14695981039346656037ULL;
    int peak = 0, maxStep = 0, previous = 0;
    bool hasPrevious = false;
    void Add(const int16_t* pcm, size_t n, size_t stride = 1, unsigned clips = 0) {
        wouldClip += clips;
        for (size_t i = 0; i < n; ++i) {
            const int v = pcm[i * stride];
            peak = std::max(peak, std::abs(v));
            if (hasPrevious)
                maxStep = std::max(maxStep, std::abs(v - previous));
            previous = v;
            hasPrevious = true;
            squares += static_cast<int64_t>(v) * v;
            rails += v == -32768 || v == 32767;
            zeros += v == 0;
            const auto bits = static_cast<uint16_t>(v);
            fingerprint = (fingerprint ^ (bits & 255u)) * 1099511628211ULL;
            fingerprint = (fingerprint ^ (bits >> 8)) * 1099511628211ULL;
        }
        count += n;
    }
    std::string Describe() const {
        std::ostringstream s;
        s << "samples=" << count << " peak=" << peak
          << " rms=" << (count ? std::sqrt(static_cast<double>(squares) / count) : 0) << " railSamples=" << rails
          << " zeros=" << zeros << " maxStep=" << maxStep << " wouldClip=" << wouldClip << " pcmHash=" << std::hex
          << fingerprint;
        return s.str();
    }
};
class Trace {
    struct Slot {
        bool active = false;
        MMAudioTraceNote note{};
        std::array<Stats, MM_TRACE_STAGE_COUNT> stages{};
    };
    std::array<Slot, 128> notes{};
    std::array<Stats, 4> output{}; // native L/R, final L/R; never compare interleaved opposite channels.
    std::function<void(const std::string&)> sink;
    std::function<std::string(bool, uintptr_t)> resource;
    int current = -1, minQueue = INT32_MAX, maxWaitMs = 0;
    uint64_t frames = 0, windowFrames = 0, emptyQueue = 0, submitted = 0, suppressed = 0;
    unsigned lines = 0;
    void Emit(const std::string& s) {
        if (lines++ < 128)
            sink(s);
        else
            ++suppressed;
    }
    void FlushSlot(Slot& slot) {
        if (!slot.active)
            return;
        static const char* stages[] = { "decoded", "resampled", "gain", "filter", "processed" };
        const auto& n = slot.note;
        for (size_t i = 0; i < slot.stages.size(); ++i) {
            auto& stats = slot.stages[i];
            if (!stats.count)
                continue;
            std::ostringstream s;
            s << "note=" << n.note << " player=" << n.player << " seq=" << n.sequence << " font=" << n.font
              << " codec=" << n.codec << " gain=" << n.gain << " pitch=" << n.pitch << " filter=" << n.filter
              << " comb=" << n.comb << " tuning=" << n.tuning << " bytes=" << n.bytes << " stage=" << stages[i] << ' '
              << stats.Describe() << " sequenceResource=" << resource(true, n.sequenceData)
              << " sampleResource=" << resource(false, n.sample);
            Emit(s.str());
            // Keep boundary continuity within the same note/context, including across summaries.
            const int previous = stats.previous;
            stats = {};
            stats.previous = previous;
            stats.hasPrevious = true;
        }
    }

  public:
    bool enabled = false;
    std::vector<int16_t> capture;
    explicit Trace(std::function<void(const std::string&)> log, std::function<std::string(bool, uintptr_t)> lookup)
        : sink(std::move(log)), resource(std::move(lookup)) {
    }
    void SetEnabled(bool value) {
        if (value == enabled)
            return;
        if (!value)
            Flush();
        enabled = value;
        if (value) {
            notes = {};
            output = {};
            current = -1;
            frames = windowFrames = emptyQueue = submitted = suppressed = lines = 0;
            minQueue = INT32_MAX;
            maxWaitMs = 0;
            // Keep previously captured PCM across checkbox comparisons until the
            // joined-worker save. Re-enabling must not discard the user's evidence.
            capture.reserve(32000 * 2 * 20);
        }
        sink(value ? "enabled rate=32000 captureLimitSeconds=20 player0=mainBGM player1=fanfare player2=SFX "
                     "player3=subBGM player4=ambience"
                   : "disabled; capture is saved when MM audio stops (quit or game switch)");
    }
    void SetNote(const MMAudioTraceNote& n) {
        if (!enabled)
            return;
        current = n.note >= 0 && n.note < static_cast<int>(notes.size()) ? n.note : -1;
        if (current < 0) {
            ++suppressed;
            return;
        }
        auto& slot = notes[current];
        const auto& old = slot.note;
        const bool changed =
            slot.active && (old.player != n.player || old.sequence != n.sequence || old.font != n.font ||
                            old.sequenceData != n.sequenceData || old.sample != n.sample || old.codec != n.codec ||
                            old.gain != n.gain || old.pitch != n.pitch || old.tuning != n.tuning ||
                            old.filter != n.filter || old.comb != n.comb);
        if (changed) {
            FlushSlot(slot);
            slot = {};
        }
        slot.note = n;
        slot.active = true;
    }
    void PCM(int stage, const int16_t* pcm, size_t count, unsigned clips) {
        if (enabled && current >= 0 && stage >= 0 && stage < MM_TRACE_STAGE_COUNT)
            notes[current].stages[stage].Add(pcm, count, 1, clips);
    }
    unsigned NoteSamples(unsigned requested) const {
        return current >= 0 ? std::min(requested, static_cast<unsigned>(std::max(0, notes[current].note.samples)))
                            : requested;
    }
    void Event(const char* event, int requested, int actual) {
        if (enabled)
            Emit("event=" + std::string(event) + " note=" + std::to_string(current) +
                 " requested=" + std::to_string(requested) + " actual=" + std::to_string(actual));
    }
    void Queue(int queued, int waitMs) {
        if (!enabled)
            return;
        minQueue = std::min(minQueue, queued);
        maxWaitMs = std::max(maxWaitMs, waitMs);
        if (submitted && queued == 0)
            ++emptyQueue; // Observations, not a backend underrun counter.
    }
    void Native(const int16_t* pcm, size_t stereoFrames) {
        if (!enabled)
            return;
        output[0].Add(pcm, stereoFrames, 2);
        output[1].Add(pcm + 1, stereoFrames, 2);
    }
    void Final(const int16_t* pcm, size_t stereoFrames, bool muted) {
        if (!enabled)
            return;
        output[2].Add(pcm, stereoFrames, 2);
        output[3].Add(pcm + 1, stereoFrames, 2);
        const size_t remaining = 32000 * 2 * 20 - capture.size();
        const size_t samples = std::min(remaining, stereoFrames * 2);
        capture.insert(capture.end(), pcm, pcm + samples);
        frames += stereoFrames;
        windowFrames += stereoFrames;
        ++submitted;
        if (windowFrames >= 32000) {
            Emit("outputMuted=" + std::to_string(muted));
            Flush();
        }
    }
    void Flush() {
        for (auto& slot : notes)
            FlushSlot(slot);
        static const char* names[] = { "native-L", "native-R", "device-L", "device-R" };
        for (size_t i = 0; i < output.size(); ++i) {
            if (output[i].count)
                // Preserve output evidence even if many note changes/events used
                // the detail budget. These four summaries remain bounded.
                sink(std::string("stage=") + names[i] + ' ' + output[i].Describe());
            const auto previous = output[i].previous;
            const auto hasPrevious = output[i].hasPrevious;
            output[i] = {};
            output[i].previous = previous;
            output[i].hasPrevious = hasPrevious;
        }
        sink("coverage frames=" + std::to_string(frames) +
             " minQueuedFrames=" + std::to_string(minQueue == INT32_MAX ? -1 : minQueue) +
             " emptyQueueObservations=" + std::to_string(emptyQueue) +
             " maxWorkerWakeGapMs=" + std::to_string(maxWaitMs) + " suppressedRecords=" + std::to_string(suppressed));
        lines = 0;
        suppressed = 0;
        windowFrames = 0;
        minQueue = INT32_MAX;
        emptyQueue = 0;
        maxWaitMs = 0;
    }
};
} // namespace MMAudioTrace
