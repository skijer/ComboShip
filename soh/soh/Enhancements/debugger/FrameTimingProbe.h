#pragma once

#include <stdint.h>

typedef enum FrameTimingPhase {
    FRAME_TIMING_TICK_BUILD,
    FRAME_TIMING_PLAY_UPDATE,
    FRAME_TIMING_PLAY_DRAW,
    FRAME_TIMING_FRAME_HOOKS,
    FRAME_TIMING_COSMETICS,
    FRAME_TIMING_MM_ASSETS,
    FRAME_TIMING_AUDIO_WAIT,
    FRAME_TIMING_WINDOW_EVENTS,
    FRAME_TIMING_INTERPOLATION,
    FRAME_TIMING_DRAW_PRESENT,
    FRAME_TIMING_FRAME_READY,
    FRAME_TIMING_RENDER_SETUP,
    FRAME_TIMING_GRAPHICS_COMMANDS,
    FRAME_TIMING_GUI_FINISH,
    FRAME_TIMING_PRESENT,
    FRAME_TIMING_PHASE_COUNT
} FrameTimingPhase;

typedef struct FrameTimingContext {
    int scene;
    int room;
    int age;
    int altAssets;
    int paused;
    int targetFps;
} FrameTimingContext;

typedef struct FrameTimingSpan {
    uint64_t epoch;
    uint64_t start;
} FrameTimingSpan;

#ifdef __cplusplus
extern "C" {
#endif
void FrameTiming_BeginFrame(FrameTimingContext context, int enabled);
int FrameTiming_IsActive(void);
FrameTimingSpan FrameTiming_BeginSpan(void);
void FrameTiming_EndSpan(FrameTimingPhase phase, FrameTimingSpan span);
void FrameTiming_AddDuration(FrameTimingPhase phase, uint64_t nanos);
void FrameTiming_EndFrame(FrameTimingContext context, int enabled);
void FrameTiming_EndNamedSpan(const char* category, const char* name, FrameTimingSpan span);
void FrameTiming_Count(const char* name, uint64_t amount);
void FrameTiming_Shutdown(void);
#ifdef __cplusplus
}

#include <algorithm>
#include <array>
#include <optional>
#include <nlohmann/json_fwd.hpp>
extern "C" void FrameTiming_RecordAttempt(const nlohmann::json& attempt);
extern "C" uint64_t FrameTiming_TickId();
extern "C" int FrameTiming_NeedsConfiguration();
extern "C" void FrameTiming_RecordConfiguration(const nlohmann::json& configuration);

namespace Fast {
struct RenderCostReport;
}
nlohmann::json FrameTiming_RenderSummary(const Fast::RenderCostReport& report);
extern "C" void FrameTiming_LogRenderCost(const Fast::RenderCostReport& report, FrameTimingContext context);

namespace FrameTiming {
// Inclusive elapsed durations, not exclusive CPU or GPU utilization. Scope
// totals overlap (for example Cosmetics inside FrameHooks inside TickBuild).
struct Report {
    FrameTimingContext context{};
    uint64_t captureNanos = 0;
    uint64_t gameTicks = 0;
    uint64_t tickNanos = 0;
    uint64_t tickMaxNanos = 0;
    std::array<uint64_t, FRAME_TIMING_PHASE_COUNT> phaseNanos{};
    std::array<uint64_t, FRAME_TIMING_PHASE_COUNT> phaseMaxNanos{};
    std::array<uint64_t, FRAME_TIMING_PHASE_COUNT> calls{};
};

// Explicit timestamps keep accounting deterministic in tests. The adapter
// supplies steady_clock timestamps; no allocation or locking occurs here.
class Recorder {
  public:
    const auto& CurrentPhases() const {
        return frameNanos;
    }
    bool Active() const {
        return active;
    }

    void BeginFrame(FrameTimingContext context, bool enabled, uint64_t now) {
        active = false;
        ++epoch;
        if (!enabled || context.scene < 0) {
            windowOpen = false;
            window = {};
            return;
        }
        if (!windowOpen || !SameContext(window.context, context)) {
            window = {};
            window.context = context;
            windowStart = now;
            windowOpen = true;
        }
        frameNanos = {};
        frameCalls = {};
        frameStart = now;
        active = true;
    }

    FrameTimingSpan BeginSpan(uint64_t now) const {
        return active ? FrameTimingSpan{ epoch, now } : FrameTimingSpan{};
    }

    void EndSpan(FrameTimingPhase phase, FrameTimingSpan span, uint64_t now) {
        if (!active || span.epoch == 0 || span.epoch != epoch || now < span.start || phase < 0 ||
            phase >= FRAME_TIMING_PHASE_COUNT) {
            return;
        }
        AddDuration(phase, now - span.start);
    }

    void AddDuration(FrameTimingPhase phase, uint64_t nanos) {
        if (!active || phase < 0 || phase >= FRAME_TIMING_PHASE_COUNT) {
            return;
        }
        frameNanos[phase] += nanos;
        ++frameCalls[phase];
    }

    std::optional<Report> EndFrame(uint64_t now, FrameTimingContext context, bool enabled) {
        if (!active) {
            return std::nullopt;
        }
        active = false;
        // Input, game update or the GUI can change tags during this very tick.
        // Reject it before it can close a report window under obsolete tags.
        if (!enabled || !SameContext(window.context, context)) {
            windowOpen = false;
            window = {};
            return std::nullopt;
        }
        const uint64_t elapsed = now - frameStart;
        ++window.gameTicks;
        window.tickNanos += elapsed;
        window.tickMaxNanos = std::max(window.tickMaxNanos, elapsed);
        for (size_t i = 0; i < FRAME_TIMING_PHASE_COUNT; ++i) {
            window.phaseNanos[i] += frameNanos[i];
            window.phaseMaxNanos[i] = std::max(window.phaseMaxNanos[i], frameNanos[i]);
            window.calls[i] += frameCalls[i];
        }
        if (now - windowStart < 1000000000ULL) {
            return std::nullopt;
        }
        window.captureNanos = now - windowStart;
        windowOpen = false;
        return window;
    }

  private:
    static bool SameContext(const FrameTimingContext& a, const FrameTimingContext& b) {
        return a.scene == b.scene && a.room == b.room && a.age == b.age && a.altAssets == b.altAssets &&
               a.paused == b.paused && a.targetFps == b.targetFps;
    }
    bool active = false;
    bool windowOpen = false;
    uint64_t epoch = 0;
    uint64_t windowStart = 0;
    uint64_t frameStart = 0;
    Report window;
    std::array<uint64_t, FRAME_TIMING_PHASE_COUNT> frameNanos{};
    std::array<uint64_t, FRAME_TIMING_PHASE_COUNT> frameCalls{};
};

class Scope {
  public:
    explicit Scope(FrameTimingPhase phase) : phase(phase), span(FrameTiming_BeginSpan()) {
    }
    ~Scope() {
        FrameTiming_EndSpan(phase, span);
    }
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

  private:
    FrameTimingPhase phase;
    FrameTimingSpan span;
};
} // namespace FrameTiming
#endif
