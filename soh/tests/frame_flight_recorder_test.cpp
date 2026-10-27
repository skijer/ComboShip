#include "soh/Enhancements/debugger/FrameFlightRecorder.h"
#include <cassert>
#include <cstdio>
int main() {
    FrameTiming::FlightRecorder flight;
    assert(flight.Empty());
    flight.Attempt({ { "id", 1 }, { "presented", false }, { "fraction", .333 }, { "gpu", nullptr } });
    flight.Attempt({ { "id", 2 }, { "presented", true }, { "fraction", .667 }, { "interval_ms", 83.0 } });
    flight.Tick({ { "id", 8 }, { "wall_ms", 458.0 }, { "phases", { { "graphics", 441.0 } } } });
    const auto first = flight.Take("transition");
    assert(first["attempts"].size() == 2 && first["ticks"].size() == 1);
    assert(first["attempts"][0]["presented"] == false);
    assert(first["attempts"][1]["interval_ms"] == 83.0);
    assert(first["ticks"][0]["phases"]["graphics"] == 441.0);
    assert(flight.Empty());
    for (size_t i = 0; i < FrameTiming::FlightRecorder::MaxAttempts + 2; ++i)
        flight.Attempt({ { "id", i } });
    for (size_t i = 0; i < FrameTiming::FlightRecorder::MaxTicks + 3; ++i)
        flight.Tick({ { "id", i } });
    const auto bounded = flight.Take("shutdown");
    assert(bounded["attempts"].size() == FrameTiming::FlightRecorder::MaxAttempts);
    assert(bounded["attempt_overflow"] == 2 && bounded["tick_overflow"] == 3);
    assert(flight.Take("empty")["attempt_overflow"] == 0);
    puts("Flight recorder: skips, ordering, stalls, transition flush, bounded overflow and reset passed");
}
