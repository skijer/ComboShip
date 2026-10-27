#include "soh/Enhancements/debugger/FrameTimingProbe.h"
#include <cassert>
#include <iostream>

using namespace FrameTiming;
static constexpr uint64_t ms = 1000000;
static const FrameTimingContext context = { 83, 0, 1, 1, 0, 60 };

static void AggregatesTicksAndMultipleRendersWithoutCountingGapsAsWork() {
    Recorder recorder;
    for (uint64_t start : { 0ULL, 600ULL }) {
        recorder.BeginFrame(context, true, start * ms);
        for (uint64_t offset : { 0ULL, 100ULL, 200ULL }) {
            auto token = recorder.BeginSpan((start + offset) * ms);
            recorder.EndSpan(FRAME_TIMING_DRAW_PRESENT, token, (start + offset + 100) * ms);
        }
        auto report = recorder.EndFrame((start + 400) * ms, context, true);
        if (start == 0) {
            assert(!report);
        } else {
            assert(report);
            assert(report->gameTicks == 2);
            assert(report->captureNanos == 1000 * ms);
            assert(report->tickNanos == 800 * ms);
            assert(report->tickMaxNanos == 400 * ms);
            assert(report->phaseNanos[FRAME_TIMING_DRAW_PRESENT] == 600 * ms);
            assert(report->phaseMaxNanos[FRAME_TIMING_DRAW_PRESENT] == 300 * ms);
            assert(report->calls[FRAME_TIMING_DRAW_PRESENT] == 6);
        }
    }
}

static void DisabledCollectionDiscardsPartialWindowsAndRejectsOldScopes() {
    Recorder recorder;
    recorder.BeginFrame(context, true, 0);
    auto old = recorder.BeginSpan(1 * ms);
    assert(!recorder.EndFrame(400 * ms, context, true));
    recorder.BeginFrame(context, false, 500 * ms);
    assert(!recorder.Active());
    assert(recorder.BeginSpan(600 * ms).epoch == 0);
    assert(!recorder.EndFrame(2000 * ms, context, true));
    recorder.BeginFrame(context, true, 3000 * ms);
    recorder.EndSpan(FRAME_TIMING_COSMETICS, old, 3050 * ms);
    auto report = recorder.EndFrame(4000 * ms, context, true);
    assert(report && report->gameTicks == 1);
    assert(report->captureNanos == 1000 * ms);
    assert(report->calls[FRAME_TIMING_COSMETICS] == 0);
}

static void ContextChangesDiscardPreviousScenesAndPauseStates() {
    Recorder recorder;
    recorder.BeginFrame(context, true, 0);
    assert(!recorder.EndFrame(800 * ms, context, true));
    auto next = context;
    next.scene = 82;
    recorder.BeginFrame(next, true, 850 * ms);
    assert(!recorder.EndFrame(1000 * ms, next, true));
    next.paused = 1;
    recorder.BeginFrame(next, true, 1050 * ms);
    auto report = recorder.EndFrame(2050 * ms, next, true);
    assert(report && report->gameTicks == 1);
    assert(report->context.scene == 82 && report->context.paused == 1);
    assert(report->captureNanos == 1000 * ms);
    next.scene = -1;
    recorder.BeginFrame(next, true, 2100 * ms);
    assert(!recorder.Active());
    assert(!recorder.EndFrame(4000 * ms, next, true));
}

static void NestedSpansRemainInclusiveAndExpiredSpansAreIgnored() {
    Recorder recorder;
    recorder.BeginFrame(context, true, 0);
    auto expired = recorder.BeginSpan(0);
    recorder.EndFrame(100 * ms, context, true);
    recorder.BeginFrame(context, true, 200 * ms);
    auto outer = recorder.BeginSpan(200 * ms);
    auto inner = recorder.BeginSpan(250 * ms);
    recorder.EndSpan(FRAME_TIMING_COSMETICS, expired, 275 * ms);
    recorder.EndSpan(FRAME_TIMING_COSMETICS, inner, 300 * ms);
    recorder.EndSpan(FRAME_TIMING_TICK_BUILD, outer, 400 * ms);
    auto report = recorder.EndFrame(1000 * ms, context, true);
    assert(report && report->gameTicks == 2);
    assert(report->phaseNanos[FRAME_TIMING_TICK_BUILD] == 200 * ms);
    assert(report->phaseNanos[FRAME_TIMING_COSMETICS] == 50 * ms);
    assert(report->calls[FRAME_TIMING_COSMETICS] == 1);
    // A new report window must not reuse the previous totals.
    recorder.BeginFrame(context, true, 1100 * ms);
    report = recorder.EndFrame(2100 * ms, context, true);
    assert(report && report->gameTicks == 1);
    assert(report->phaseNanos[FRAME_TIMING_COSMETICS] == 0);
}

static void RendererDurationsCountOnlySuccessfulPresents() {
    Recorder recorder;
    recorder.AddDuration(FRAME_TIMING_PRESENT, 90 * ms);
    recorder.BeginFrame(context, true, 0);
    recorder.AddDuration(FRAME_TIMING_FRAME_READY, 2 * ms);
    recorder.AddDuration(FRAME_TIMING_PRESENT, 12 * ms);
    recorder.AddDuration(FRAME_TIMING_FRAME_READY, 3 * ms); // a dropped frame has no present
    recorder.AddDuration(FRAME_TIMING_FRAME_READY, 1 * ms);
    recorder.AddDuration(FRAME_TIMING_PRESENT, 13 * ms);
    auto report = recorder.EndFrame(1000 * ms, context, true);
    assert(report);
    assert(report->calls[FRAME_TIMING_FRAME_READY] == 3);
    assert(report->calls[FRAME_TIMING_PRESENT] == 2);
    assert(report->phaseNanos[FRAME_TIMING_PRESENT] == 25 * ms);
}

static void ChangesDuringTheReportingTickDiscardTheWholeWindow() {
    int FrameTimingContext::*fields[] = { &FrameTimingContext::scene,  &FrameTimingContext::room,
                                          &FrameTimingContext::age,    &FrameTimingContext::altAssets,
                                          &FrameTimingContext::paused, &FrameTimingContext::targetFps };
    for (auto field : fields) {
        Recorder recorder;
        recorder.BeginFrame(context, true, 0);
        assert(!recorder.EndFrame(800 * ms, context, true));
        recorder.BeginFrame(context, true, 900 * ms);
        auto changed = context;
        changed.*field = changed.*field == 1 ? 0 : changed.*field + 1;
        assert(!recorder.EndFrame(1100 * ms, changed, true));
        recorder.BeginFrame(changed, true, 1200 * ms);
        auto report = recorder.EndFrame(2200 * ms, changed, true);
        assert(report && report->gameTicks == 1 && report->captureNanos == 1000 * ms);
    }
    Recorder recorder;
    recorder.BeginFrame(context, true, 0);
    assert(!recorder.EndFrame(800 * ms, context, true));
    recorder.BeginFrame(context, true, 900 * ms);
    assert(!recorder.EndFrame(1100 * ms, context, false));
    recorder.BeginFrame(context, true, 1200 * ms);
    auto report = recorder.EndFrame(2200 * ms, context, true);
    assert(report && report->gameTicks == 1);
}

int main() {
    AggregatesTicksAndMultipleRendersWithoutCountingGapsAsWork();
    DisabledCollectionDiscardsPartialWindowsAndRejectsOldScopes();
    ContextChangesDiscardPreviousScenesAndPauseStates();
    NestedSpansRemainInclusiveAndExpiredSpansAreIgnored();
    RendererDurationsCountOnlySuccessfulPresents();
    ChangesDuringTheReportingTickDiscardTheWholeWindow();
    std::cout << "Frame timing: 6 behavioral tests passed\n";
}
