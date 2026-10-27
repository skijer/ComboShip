#pragma once
#include <cstdint>
#include <nlohmann/json.hpp>

namespace FrameTiming {
// Bounded, ordered raw observations. Percentiles are computed off the game thread
// by the analysis tool, without losing the intervals behind an FPS average.
class FlightRecorder {
  public:
    static constexpr size_t MaxAttempts = 1024;
    static constexpr size_t MaxTicks = 128;
    void Attempt(nlohmann::json value) {
        if (attempts.size() < MaxAttempts)
            attempts.push_back(std::move(value));
        else
            ++attemptOverflow;
    }
    void Tick(nlohmann::json value) {
        if (ticks.size() < MaxTicks)
            ticks.push_back(std::move(value));
        else
            ++tickOverflow;
    }
    bool Empty() const {
        return attempts.empty() && ticks.empty();
    }
    nlohmann::json Take(const char* reason) {
        nlohmann::json result = { { "schema", 1 },
                                  { "reason", reason },
                                  { "attempts", std::move(attempts) },
                                  { "ticks", std::move(ticks) },
                                  { "attempt_overflow", attemptOverflow },
                                  { "tick_overflow", tickOverflow } };
        attempts = nlohmann::json::array();
        ticks = nlohmann::json::array();
        attemptOverflow = tickOverflow = 0;
        return result;
    }

  private:
    nlohmann::json attempts = nlohmann::json::array(), ticks = nlohmann::json::array();
    uint64_t attemptOverflow = 0, tickOverflow = 0;
};
} // namespace FrameTiming
