#include "soh/Enhancements/debugger/FrameTimingProbe.h"
#include "fast/RenderCostProbe.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <ship/Context.h>
#include <spdlog/async_logger.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>
#include <thread>

int main(int argc, char** argv) {
    assert(argc == 2);
    std::filesystem::create_directories(argv[1]);
    std::filesystem::current_path(argv[1]);
    auto* context = Ship::Context::CreateUninitializedInstance("FrameTimingOutputTest", "test", "unused.json");
    assert(context->InitLogging(spdlog::level::off, spdlog::level::off));
    auto gameLogger = context->GetLogger();
    const auto logPath = Ship::Context::GetPathRelativeToAppDirectory("logs/FrameTimingOutputTest.log");
    auto sink = gameLogger->sinks().back();
    auto decoy = std::make_shared<spdlog::logger>("module-local", std::make_shared<spdlog::sinks::null_sink_mt>());
    spdlog::set_default_logger(decoy);
    assert(spdlog::default_logger() != gameLogger);
    gameLogger->set_level(spdlog::level::off);

    // Exercise the production adapter and the same asynchronous rotating-file
    // sink used by release builds, with normal game logging disabled.
    FrameTimingContext title{ -1, 0, 0, 1, 0, 60 };
    FrameTiming_BeginFrame(title, 1);
    FrameTiming_EndFrame(title, 1);
    FrameTimingContext play{ 82, 0, 0, 1, 0, 60 };
    for (auto level : { spdlog::level::off, spdlog::level::warn }) {
        gameLogger->set_level(level);
        FrameTiming_BeginFrame(play, 1);
        FrameTiming_RecordConfiguration({ { "test_configuration", true } });
        FrameTiming_RecordAttempt({ { "id", 1 },
                                    { "presented", false },
                                    { "fraction", .333 },
                                    { "pacing", { { "reason", "scheduler_late" } } } });
        FrameTiming_RecordAttempt(
            { { "id", 2 }, { "presented", true }, { "fraction", .667 }, { "gpu", { { "status", "pending" } } } });
        FrameTiming_Count("actors.drawn", 12);
        auto named = FrameTiming_BeginSpan();
        FrameTiming_EndNamedSpan("draw", "room", named);
        auto span = FrameTiming_BeginSpan();
        FrameTiming_EndSpan(FRAME_TIMING_PLAY_UPDATE, span);
        FrameTiming_AddDuration(FRAME_TIMING_DRAW_PRESENT, 1000000);
        FrameTiming_AddDuration(FRAME_TIMING_PRESENT, 500000);
        std::this_thread::sleep_for(std::chrono::milliseconds(1050));
        FrameTiming_EndFrame(play, 1);
        Fast::RenderCostProbe render;
        FrameTiming_LogRenderCost(render.Report(), play); // Unsampled reports must not be emitted.
        render.BeginFrame(true, 1000);
        render.SyncDepth(1);
        render.SetResource("objects/young_din/head");
        render.AddCommand(4, 5, "G_TRI1", 2000000);
        {
            Fast::RenderTextureScope texture(&render, "textures/young_din/hair");
            render.AddUpload(4096, 500000);
        }
        render.MutableReport().backend = "Direct3D 11";
        render.MutableReport().gameTick = 42;
        render.MutableReport().interpolationIndex = 1;
        render.MutableReport().interpolationT = 0.5f;
        render.MutableReport().renderWidth = 1280;
        render.MutableReport().renderHeight = 720;
        render.MutableReport().cacheMisses = 1;
        render.MutableReport().altRenderLookup = true;
        render.MutableReport().vertexLookups = { 500, 2 };
        render.MutableReport().displayListLookups = { 30, 1 };
        render.EndFrame(3001000);
        FrameTiming_LogRenderCost(render.Report(), play);
        assert(gameLogger->level() == level);      // Preserve the user's normal logging preference.
        assert(spdlog::default_logger() == decoy); // Keep module-local logging unchanged.
    }
    FrameTiming_BeginFrame(play, 0);
    FrameTiming_EndFrame(play, 0);
    FrameTiming_Shutdown();
    // The real Windows integration target destroys the shared engine Context,
    // draining its async writer before either the probe DLL or engine unloads.
    std::weak_ptr<spdlog::sinks::sink> sinkLifetime = sink;
    Ship::Context::DestroyInstance();
    spdlog::drop_all();
    gameLogger.reset();
    sink.reset();
    assert(sinkLifetime.expired()); // No probe-owned formatter may survive game DLL teardown.

    std::ifstream log(logPath);
    std::string line;
    int samples = 0;
    int renderSamples = 0;
    int flightSamples = 0;
    bool armed = false;
    bool disabled = false;
    while (std::getline(log, line)) {
        const std::string flightMarker = "[FrameFlightRecorder] ";
        if (const auto pos = line.find(flightMarker); pos != std::string::npos) {
            ++flightSamples;
            const auto capture = nlohmann::json::parse(line.substr(pos + flightMarker.size()));
            assert(capture.at("attempts").size() == 2);
            assert(capture.at("attempts")[0].at("presented") == false);
            assert(capture.at("attempts")[1].at("gpu").at("status") == "pending");
            assert(capture.at("attempts")[0].at("tick_id") == capture.at("ticks")[0].at("id"));
            assert(capture.at("ticks")[0].at("trace").at("counts").at("actors.drawn") == 12);
            assert(capture.at("ticks")[0].at("phases_ms").contains("play_update"));
            assert(capture.at("ticks")[0].at("wall_ms").get<double>() >= 1000.0);
            assert(capture.at("ticks")[0].contains("thread_cpu_ms"));
            continue;
        }
        const std::string renderMarker = "[RenderCostProbe] ";
        const auto renderStart = line.find(renderMarker);
        if (renderStart != std::string::npos) {
            ++renderSamples;
            const auto cost = nlohmann::json::parse(line.substr(renderStart + renderMarker.size()));
            assert(cost.at("scene") == play.scene);
            assert(cost.at("sample_every") == 31);
            assert(cost.at("game_tick") == 42);
            assert(cost.at("interpolation_index") == 1);
            assert(cost.at("interpolation_t") == 0.5);
            assert(cost.at("backend") == "Direct3D 11");
            assert(cost.at("render_width") == 1280);
            assert(cost.at("total_ms") == 3.0);
            assert(cost.at("command_sum_ms") == 2.0);
            assert(cost.at("non_command_ms") == 1.0);
            assert(cost.at("opcodes").at(0).at("name") == "G_TRI1");
            assert(cost.at("top_resources").at(0).at("path") == "objects/young_din/head");
            assert(cost.at("top_texture_uploads").at(0).at("path") == "textures/young_din/hair");
            assert(cost.at("top_texture_uploads").at(0).at("bytes") == 4096);
            assert(cost.at("cache").at("misses") == 1);
            const auto& lookup = cost.at("alt_render_lookup");
            assert(lookup.at("enabled") == true);
            assert(lookup.at("vertex_hits") == 500 && lookup.at("vertex_fallbacks") == 2);
            assert(lookup.at("display_list_hits") == 30 && lookup.at("display_list_fallbacks") == 1);
            assert(cost.at("scopes").at("upload").at("ms") == 0.5);
            continue;
        }
        const auto marker = line.find("[FrameTimingProbe] ");
        if (marker == std::string::npos) {
            continue;
        }
        const auto record = nlohmann::json::parse(line.substr(marker + 19));
        assert(record.at("schema") == 1);
        const auto event = record.at("event").get<std::string>();
        if (event == "startup") {
            armed = record.at("enabled").get<bool>();
            assert(record.at("phase_count") == FRAME_TIMING_PHASE_COUNT);
            assert(record.at("normal_log_level_independent").get<bool>());
        } else if (event == "state") {
            disabled = !record.at("enabled").get<bool>();
        } else if (event == "sample") {
            ++samples;
            assert(record.at("scene") == play.scene);
            assert(record.at("target_fps") == 60);
            assert(record.at("game_ticks") == 1);
            assert(record.at("presented_frames") == 1);
            assert(record.at("present_rate").get<double>() > 0);
            const auto& phases = record.at("phases");
            assert(phases.size() == FRAME_TIMING_PHASE_COUNT);
            assert(phases.at("play_update").at("calls") == 1);
            assert(phases.at("present").at("mean_ms_per_tick") == 0.5);
        }
    }
    assert(armed);
    assert(disabled);
    assert(samples == 2);
    assert(renderSamples == 2);
    assert(flightSamples == 2);

    // A fresh Context in the same process must announce itself and bind to its
    // new sink instead of retaining the old file, pool, or enabled state.
    context = Ship::Context::CreateUninitializedInstance("FrameTimingOutputRestart", "test", "unused.json");
    assert(context->InitLogging(spdlog::level::off, spdlog::level::off));
    const auto nextPath = Ship::Context::GetPathRelativeToAppDirectory("logs/FrameTimingOutputRestart.log");
    gameLogger = context->GetLogger();
    sink = gameLogger->sinks().back();
    gameLogger->set_level(spdlog::level::off);
    spdlog::set_default_logger(decoy);
    FrameTiming_BeginFrame(title, 1);
    FrameTiming_EndFrame(title, 1);
    FrameTiming_Shutdown();
    Ship::Context::DestroyInstance();
    spdlog::drop_all();
    sinkLifetime = sink;
    gameLogger.reset();
    sink.reset();
    assert(sinkLifetime.expired());
    std::ifstream nextLog(nextPath);
    assert(std::getline(nextLog, line));
    const auto restarted = nlohmann::json::parse(line.substr(line.find("[FrameTimingProbe] ") + 19));
    assert(restarted.at("event") == "startup");
    assert(restarted.at("enabled").get<bool>());
    std::cout << "Production timing output: Context file logger, separate default, all phases, Warn/Off, teardown and "
                 "restart passed\n";
}
