#include "FrameTimingProbe.h"
#include "FrameFlightRecorder.h"
#include <ship/diagnostics/PerformanceTrace.h>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <time.h>
#endif
#include "fast/RenderCostProbe.h"

#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>
#include <ship/Context.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace {
thread_local FrameTiming::Recorder recorder;
std::shared_ptr<spdlog::logger> diagnosticLogger;
int previousEnabled = -1;
FrameTiming::FlightRecorder flight;
uint64_t tickId = 0, tickStart = 0, tickCpuStart = 0, flightStart = 0;
uint64_t previousLogNanos = 0;
FrameTimingContext tickContext{};
std::string flightLogPath;
bool configurationLogged = false;
uint64_t flightErrors = 0;
uint64_t captureStart = 0, captureBytes = 0;
bool captureLimit = false;
uint64_t contextId = 0;
constexpr uint64_t MaxCaptureBytes = 256ULL * 1024 * 1024;
constexpr uint64_t MaxCaptureNanos = 15ULL * 60 * 1000000000;

uint64_t ThreadCpuNow() {
#if defined(_WIN32)
    FILETIME creation, exit, kernel, user;
    if (!GetThreadTimes(GetCurrentThread(), &creation, &exit, &kernel, &user))
        return 0;
    auto value = [](FILETIME t) { return (uint64_t(t.dwHighDateTime) << 32) | t.dwLowDateTime; };
    return (value(kernel) + value(user)) * 100;
#elif defined(CLOCK_THREAD_CPUTIME_ID)
    timespec value{};
    if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &value) != 0)
        return 0;
    return uint64_t(value.tv_sec) * 1000000000ULL + value.tv_nsec;
#else
    return 0;
#endif
}

uint64_t Now() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

constexpr const char* names[] = { "tick_build",        "play_update",      "play_draw",        "frame_hooks",
                                  "cosmetics",         "mm_asset_lookup",  "audio_mutex_wait", "window_events",
                                  "interpolation",     "draw_and_present", "frame_ready",      "render_setup",
                                  "graphics_commands", "gui_finish",       "present" };
static_assert(sizeof(names) / sizeof(names[0]) == FRAME_TIMING_PHASE_COUNT);

const std::shared_ptr<spdlog::logger>& DiagnosticLogger() {
    // Reuse the application's existing sinks and asynchronous writer, but give
    // this diagnostic its own level. A saved Warn/Off setting must not hide the
    // capture, and normal resource logging must retain the user's preference.
    if (!diagnosticLogger) {
        // Windows statically links spdlog into each game/engine DLL. The game
        // DLL's default logger is not the engine Context's file logger.
        diagnosticLogger = Ship::Context::GetRawInstance()->GetLogger()->clone("OoTFrameTimingProbe");
        flightLogPath =
            Ship::Context::GetPathRelativeToAppDirectory("logs/OoT-RenderFlight-" + std::to_string(Now()) + ".log");
        auto captureSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(flightLogPath, false);
        captureSink->set_pattern("%v");
        diagnosticLogger->sinks().push_back(captureSink);
        diagnosticLogger->set_level(spdlog::level::info);
        diagnosticLogger->flush_on(spdlog::level::info);
    }
    return diagnosticLogger;
}

void Emit(const char* marker, const nlohmann::json& record) {
    const auto serialized = record.dump();
    if (captureBytes + serialized.size() > MaxCaptureBytes) {
        if (!captureLimit)
            DiagnosticLogger()->info(
                "[RenderFlightState] "
                "{{\"event\":\"capture_limit\",\"reason\":\"256_MiB\",\"discarded_record_bytes\":{}}}",
                serialized.size());
        captureLimit = true;
        return;
    }
    captureBytes += serialized.size();
    DiagnosticLogger()->info("[{}] {}", marker, serialized);
}
void DrainTrace(const char* reason) {
    auto tail = Ship::PerformanceTrace::EndTick();
    if (tail.is_null()) {
        ++flightErrors;
        return;
    }
    if (tail.value("aggregate_keys", 0ULL) || tail.value("phase_keys", 0ULL) || tail.value("actor_keys", 0ULL) ||
        tail.value("outstanding_scopes", 0ULL) || !tail.value("counts", nlohmann::json::object()).empty()) {
        Emit("RenderFlightTail",
             { { "reason", reason },
               { "trace", std::move(tail) },
               { "incomplete_tail_possible", true },
               { "note", "No wait for background workers; late or queued work may finish after capture closes" } });
    }
}

void FlushFlight(const char* reason) {
    if (flight.Empty())
        return;
    const uint64_t start = Now();
    auto record = flight.Take(reason);
    record["diagnostic_errors"] = flightErrors;
    const auto pool = Ship::Context::GetRawInstance()->GetLogThreadPool();
    record["logger_queue_depth_before_emit"] = pool ? nlohmann::json(pool->queue_size()) : nlohmann::json(nullptr);
    record["logger_overruns_total"] = pool ? nlohmann::json(pool->overrun_counter()) : nlohmann::json(nullptr);
    record["previous_log_emit_ms"] = previousLogNanos / 1000000.0;
    Emit("FrameFlightRecorder", record);
    previousLogNanos = Now() - start;
    flightStart = Now();
}

void ReportOutputState(bool enabled) {
    if (previousEnabled == static_cast<int>(enabled)) {
        return;
    }
    DiagnosticLogger(); // create the complete-session capture before announcing its path
    const nlohmann::json state = {
        { "event", previousEnabled == -1 ? "startup" : "state" },
        { "schema", 1 },
        { "enabled", enabled },
        { "phase_count", FRAME_TIMING_PHASE_COUNT },
        { "interval_ms", 1000 },
        { "normal_log_level_independent", true },
        { "scope", "all OoT gameplay scenes; no slow-frame threshold" },
        { "flight_schema", 1 },
        { "complete_capture_file", flightLogPath },
        { "flight_capture", "all ticks and render attempts; transition and shutdown flush" },
        { "gpu", "DX11 asynchronous timestamp queries; pending/disjoint/errors explicit" },
        { "presentation", "CPU submission intervals and backend statistics; not physical display timestamps" },
        { "bounds",
          { { "attempts_per_batch", 1024 },
            { "ticks_per_batch", 128 },
            { "capture_bytes", MaxCaptureBytes },
            { "capture_seconds", 900 } } },
    };
    Emit("FrameTimingProbe", state);
    previousEnabled = static_cast<int>(enabled);
}
} // namespace

extern "C" void FrameTiming_Shutdown(void) {
    // Release shared sinks while both game DLLs (which may own their formatter
    // vtables) remain mapped. Context teardown drains the async queue afterward.
    try {
        DrainTrace("shutdown");
        FlushFlight("shutdown");
    } catch (...) { ++flightErrors; }
    recorder = {};
    tickStart = tickCpuStart = flightStart = previousLogNanos = 0;
    configurationLogged = false;
    captureStart = captureBytes = 0;
    captureLimit = false;
    if (diagnosticLogger) {
        diagnosticLogger->flush();
        diagnosticLogger.reset();
    }
    previousEnabled = -1;
}

extern "C" void FrameTiming_BeginFrame(FrameTimingContext context, int enabled) {
    try {
        if (enabled && context.scene >= 0 && !captureStart)
            captureStart = Now();
        if (enabled && captureStart && !captureLimit && Now() - captureStart >= MaxCaptureNanos) {
            Emit("RenderFlightState", { { "event", "capture_limit" }, { "reason", "15_minutes" } });
            captureLimit = true;
        }
        enabled = enabled && !captureLimit;
        const bool wasActive = previousEnabled == 1;
        ReportOutputState(enabled != 0);
        const bool changed = context.scene != tickContext.scene || context.room != tickContext.room ||
                             context.age != tickContext.age || context.altAssets != tickContext.altAssets ||
                             context.paused != tickContext.paused || context.targetFps != tickContext.targetFps;
        if (!enabled || changed) {
            if (!enabled && wasActive)
                DrainTrace("disabled");
            FlushFlight(!enabled ? "disabled" : "context_change");
        }
        if (changed || contextId == 0)
            ++contextId;
        tickContext = context;
        tickStart = enabled && context.scene >= 0 ? Now() : 0;
        recorder.BeginFrame(context, enabled != 0, tickStart);
        if (recorder.Active()) {
            ++tickId;
            if (!flightStart)
                flightStart = tickStart;
            tickCpuStart = ThreadCpuNow();
            Ship::PerformanceTrace::BeginTick(tickId);
        }

    } catch (...) {
        ++flightErrors;
        recorder = {};
        (void)Ship::PerformanceTrace::EndTick();
    }
}

extern "C" FrameTimingSpan FrameTiming_BeginSpan(void) {
    return recorder.Active() ? recorder.BeginSpan(Now()) : FrameTimingSpan{};
}

extern "C" int FrameTiming_IsActive(void) {
    return recorder.Active();
}

extern "C" void FrameTiming_AddDuration(FrameTimingPhase phase, uint64_t nanos) {
    recorder.AddDuration(phase, nanos);
}

extern "C" void FrameTiming_EndSpan(FrameTimingPhase phase, FrameTimingSpan span) {
    if (span.epoch != 0 && recorder.Active()) {
        recorder.EndSpan(phase, span, Now());
    }
}

extern "C" void FrameTiming_EndFrame(FrameTimingContext context, int enabled) {
    try {
        if (!recorder.Active()) {
            return;
        }
        const auto end = Now();
        const auto cpuEnd = ThreadCpuNow();
        nlohmann::json tickPhases = nlohmann::json::object();
        for (size_t i = 0; i < FRAME_TIMING_PHASE_COUNT; ++i)
            tickPhases[names[i]] = recorder.CurrentPhases()[i] / 1000000.0;
        const auto traceStart = Now();
        auto trace = Ship::PerformanceTrace::EndTick();
        const auto traceEnd = Now();
        nlohmann::json tick = { { "id", tickId },
                                { "start_ns", tickStart },
                                { "end_ns", end },
                                { "context_id", contextId },
                                { "wall_ms", (end - tickStart) / 1000000.0 },
                                { "thread_cpu_ms", tickCpuStart && cpuEnd >= tickCpuStart
                                                       ? nlohmann::json((cpuEnd - tickCpuStart) / 1000000.0)
                                                       : nlohmann::json(nullptr) },
                                { "scene", tickContext.scene },
                                { "room", tickContext.room },
                                { "age", tickContext.age },
                                { "alt_assets", tickContext.altAssets != 0 },
                                { "paused", tickContext.paused != 0 },
                                { "target_fps", tickContext.targetFps },
                                { "end_scene", context.scene },
                                { "end_room", context.room },
                                { "phases_ms", std::move(tickPhases) },
                                { "trace", std::move(trace) },
                                { "trace_finalize_ms", (traceEnd - traceStart) / 1000000.0 } };
        tick["post_tick_diagnostic_ms"] = (Now() - end) / 1000000.0;
        flight.Tick(std::move(tick));
        if (end - flightStart >= 1000000000ULL)
            FlushFlight("interval");
        auto report = recorder.EndFrame(end, context, enabled != 0);
        if (!report) {
            return;
        }
        // Formatting and logging occur after the timed tick, once per second.
        const double perTickMillis = 1.0 / (1000000.0 * report->gameTicks);
        nlohmann::json phases = nlohmann::json::object();
        for (size_t i = 0; i < FRAME_TIMING_PHASE_COUNT; ++i) {
            phases[names[i]] = {
                { "mean_ms_per_tick", report->phaseNanos[i] * perTickMillis },
                { "max_ms_per_tick", report->phaseMaxNanos[i] / 1000000.0 },
                { "calls", report->calls[i] },
            };
        }
        const nlohmann::json record = {
            { "event", "sample" },
            { "schema", 1 },
            { "scene", report->context.scene },
            { "room", report->context.room },
            { "age", report->context.age },
            { "alt_assets", report->context.altAssets != 0 },
            { "paused", report->context.paused != 0 },
            { "target_fps", report->context.targetFps },
            { "capture_ms", report->captureNanos / 1000000.0 },
            { "game_ticks", report->gameTicks },
            { "graphics_calls", report->calls[FRAME_TIMING_DRAW_PRESENT] },
            { "presented_frames", report->calls[FRAME_TIMING_PRESENT] },
            { "present_rate", report->calls[FRAME_TIMING_PRESENT] * (1000000000.0 / report->captureNanos) },
            { "mean_tick_ms", report->tickNanos * perTickMillis },
            { "max_tick_ms", report->tickMaxNanos / 1000000.0 },
            { "between_ticks_ms", (report->captureNanos - report->tickNanos) / 1000000.0 },
            { "phases", std::move(phases) },
            { "semantics",
              "inclusive elapsed scopes; nested phases overlap; draw_and_present includes pacing and GPU waits" },
        };
        Emit("FrameTimingProbe", record);

    } catch (...) {
        ++flightErrors;
        recorder = {};
        (void)Ship::PerformanceTrace::EndTick();
    }
}

extern "C" void FrameTiming_LogRenderCost(const Fast::RenderCostReport& report, FrameTimingContext context) {
    if (!report.sampled || (report.totalNanos < 1000000000ULL / std::max(1, context.targetFps) &&
                            (report.frameIndex - 1) % Fast::RenderCostProbe::SampleEvery != 0)) {
        return;
    }
    auto counter = [](const Fast::RenderCostCounter& cost) {
        return nlohmann::json{ { "calls", cost.calls }, { "ms", cost.nanos / 1000000.0 } };
    };
    uint64_t commandNanos = 0;
    nlohmann::json opcodes = nlohmann::json::array();
    for (size_t i = 0; i < report.opcodes.size(); ++i) {
        const auto& cost = report.opcodes[i];
        if (cost.calls == 0) {
            continue;
        }
        commandNanos += cost.nanos;
        auto entry = counter(cost);
        entry["ucode"] = i / 256;
        entry["opcode"] = i % 256;
        entry["name"] = cost.name ? cost.name : "<unknown>";
        opcodes.push_back(std::move(entry));
    }
    // Sort indices only. Reports keep no resource ownership and remain unchanged.
    std::vector<size_t> order;
    for (size_t i = 0; i < report.resources.size(); ++i) {
        if (report.resources[i].commands.calls != 0) {
            order.push_back(i);
        }
    }
    std::sort(order.begin(), order.end(), [&](size_t a, size_t b) {
        return report.resources[a].commands.nanos > report.resources[b].commands.nanos;
    });
    nlohmann::json resources = nlohmann::json::array();
    for (size_t i = 0; i < std::min<size_t>(12, order.size()); ++i) {
        const auto& cost = report.resources[order[i]];
        auto entry = counter(cost.commands);
        entry["path"] = cost.path;
        resources.push_back(std::move(entry));
    }
    order.clear();
    for (size_t i = 0; i < report.textures.size(); ++i) {
        if (report.textures[i].upload.calls != 0) {
            order.push_back(i);
        }
    }
    std::sort(order.begin(), order.end(),
              [&](size_t a, size_t b) { return report.textures[a].upload.nanos > report.textures[b].upload.nanos; });
    nlohmann::json textures = nlohmann::json::array();
    for (size_t i = 0; i < std::min<size_t>(12, order.size()); ++i) {
        const auto& cost = report.textures[order[i]];
        auto entry = counter(cost.upload);
        entry["path"] = cost.path;
        entry["bytes"] = cost.bytes;
        textures.push_back(std::move(entry));
    }
    const nlohmann::json record = {
        { "event", "sample" },
        { "schema", 1 },
        { "scene", context.scene },
        { "room", context.room },
        { "age", context.age },
        { "alt_assets", context.altAssets != 0 },
        { "paused", context.paused != 0 },
        { "target_fps", context.targetFps },
        { "frame_index", report.frameIndex },
        { "game_tick", report.gameTick },
        { "attempt_id", report.attemptId },
        { "tick_id", tickId },
        { "interpolation_index", report.interpolationIndex },
        { "interpolation_t", report.interpolationT },
        { "sample_every", Fast::RenderCostProbe::SampleEvery },
        { "backend", report.backend },
        { "render_width", report.renderWidth },
        { "render_height", report.renderHeight },
        { "window_width", report.windowWidth },
        { "window_height", report.windowHeight },
        { "msaa", report.msaa },
        { "total_ms", report.totalNanos / 1000000.0 },
        { "command_sum_ms", commandNanos / 1000000.0 },
        { "non_command_ms", (report.totalNanos >= commandNanos ? report.totalNanos - commandNanos : 0) / 1000000.0 },
        { "opcodes", std::move(opcodes) },
        { "top_resources", std::move(resources) },
        { "top_texture_uploads", std::move(textures) },
        { "resource_overflow", report.resourceOverflow },
        { "scopes",
          { { "texture_import", counter(report.textureImport) },
            { "upload", counter(report.upload) },
            { "driver_draw", counter(report.driverDraw) },
            { "shader_create", counter(report.shaderCreate) },
            { "framebuffer_setup", counter(report.framebufferSetup) },
            { "framebuffer_finish", counter(report.framebufferFinish) } } },
        { "cache",
          { { "hits", report.cacheHits },
            { "misses", report.cacheMisses },
            { "evictions", report.cacheEvictions },
            { "entries", report.cacheEntries },
            { "clears_total", report.cacheClearsTotal },
            { "deletes_total", report.cacheDeletesTotal } } },
        { "alt_render_lookup",
          { { "enabled", report.altRenderLookup },
            { "vertex_hits", report.vertexLookups.hits },
            { "vertex_fallbacks", report.vertexLookups.fallbacks },
            { "display_list_hits", report.displayListLookups.hits },
            { "display_list_fallbacks", report.displayListLookups.fallbacks } } },
        { "upload_bytes", report.uploadBytes },
        { "vertices", report.vertices },
        { "triangles", report.triangles },
        { "submitted_triangles", report.submittedTriangles },
        { "semantics", "sampled CPU elapsed including driver waits, not GPU timestamps; nested scopes overlap; "
                       "resource times are own commands, excluding child lists; total includes profiling overhead; "
                       "cache clears/deletes are cumulative since interpreter initialization" },
    };
    Emit("RenderCostProbe", record);
}

extern "C" uint64_t FrameTiming_TickId() {
    return tickId;
}
extern "C" void FrameTiming_RecordAttempt(const nlohmann::json& value) {
    try {
        if (!recorder.Active())
            return;
        auto attempt = value;
        attempt["tick_id"] = tickId;
        attempt["context_id"] = contextId;
        attempt["scene"] = tickContext.scene;
        attempt["room"] = tickContext.room;
        attempt["age"] = tickContext.age;
        attempt["alt_assets"] = tickContext.altAssets != 0;
        attempt["paused"] = tickContext.paused != 0;
        attempt["target_fps"] = tickContext.targetFps;
        flight.Attempt(std::move(attempt));

    } catch (...) { ++flightErrors; }
}
extern "C" void FrameTiming_EndNamedSpan(const char* category, const char* name, FrameTimingSpan span) {
    if (recorder.Active() && span.epoch != 0 && span.epoch == recorder.BeginSpan(0).epoch) {
        Ship::PerformanceTrace::Record(category, name, span.start, Now());
    }
}
extern "C" void FrameTiming_Count(const char* name, uint64_t amount) {
    if (recorder.Active())
        Ship::PerformanceTrace::Count(name, amount);
}

extern "C" void FrameTiming_RecordConfiguration(const nlohmann::json& value) {
    try {
        if (!recorder.Active())
            return;
        auto configuration = value;
        configurationLogged = true;
        configuration["tick_id"] = tickId;
        Emit("RenderFlightConfiguration", configuration);

    } catch (...) { ++flightErrors; }
}

extern "C" int FrameTiming_NeedsConfiguration() {
    return recorder.Active() && !configurationLogged;
}

nlohmann::json FrameTiming_RenderSummary(const Fast::RenderCostReport& r) {
    nlohmann::json scopes = nlohmann::json::object();
    auto add = [&](const char* name, const Fast::RenderCostCounter& value) {
        scopes[name] = { { "calls", value.calls }, { "ms", value.nanos / 1000000.0 } };
    };
    add("driver_draw", r.driverDraw);
    add("texture_import", r.textureImport);
    add("upload", r.upload);
    add("shader_create", r.shaderCreate);
    add("framebuffer_setup", r.framebufferSetup);
    add("framebuffer_finish", r.framebufferFinish);
    return { { "scopes", std::move(scopes) },
             { "upload_bytes", r.uploadBytes },
             { "triangles", r.triangles },
             { "submitted_triangles", r.submittedTriangles },
             { "vertices", r.vertices },
             { "cache_hits", r.cacheHits },
             { "cache_misses", r.cacheMisses },
             { "cache_evictions", r.cacheEvictions },
             { "cache_entries", r.cacheEntries },
             { "cache_clears_total", r.cacheClearsTotal },
             { "cache_deletes_total", r.cacheDeletesTotal },
             { "resource_overflow", r.resourceOverflow },
             { "render_width", r.renderWidth },
             { "render_height", r.renderHeight },
             { "msaa", r.msaa } };
}
