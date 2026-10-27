#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include "RegionalActorParams.h"
namespace ComboRegionalActors {
struct Point {
    float x, y, z;
};
struct Row {
    int family, params;
    float across, front, lift;
};
/* Offsets are relative to the authoritative endpoint's approach direction.
 * Keep the central walking/return lane empty. Host adapters floor-fit each row. */
inline constexpr std::array<Row, 4> Rows(int route, bool lulu) {
    if (route == 5)
        return { { { 25, 0x7E1F, -90, 100, 0 }, { 25, 0x7E2F, 90, 100, 0 }, {}, {} } };
    // Entire native guitar loop stays inside the Lake vault and off the adult center lane.
    if (route == 6)
        return { { { 26, 0x7D11, 44, 180, 0 }, lulu ? Row{ 22, 0x7E0C, 64, 80, 0 } : Row{}, {}, {} } };
    if (route == 7)
        return { { { 27, 0x7D02, 150, 100, 12 }, {}, {}, {} } };
    return {};
}
struct State {
    int route = 0;
    std::array<uintptr_t, 4> handles{};
    std::array<bool, 4> attempted{};
};
template <class World> void Clear(State& state, World& world) {
    for (auto h : state.handles)
        if (world.Live(h))
            world.Kill(h);
    state = {};
}
template <class World>
void Update(State& state, World& world, bool ready, int route, Point anchor, short yaw, bool lulu) {
    if (!ready || route < 5 || route > 7) {
        Clear(state, world);
        return;
    }
    if (state.route != route)
        Clear(state, world);
    state.route = route;
    const float angle = yaw * (6.28318530717958647692f / 65536.0f), sn = std::sin(angle), cs = std::cos(angle);
    const auto rows = Rows(route, lulu);
    for (unsigned i = 0; i < rows.size(); i++) {
        const auto& row = rows[i];
        if (!row.family)
            continue;
        if (world.Authored(row.family, state)) {
            if (world.Live(state.handles[i]))
                world.Kill(state.handles[i]);
            state.handles[i] = 0;
            continue;
        }
        if (world.Live(state.handles[i]) || state.attempted[i])
            continue;
        Point pos{ anchor.x + sn * row.front + cs * row.across, anchor.y + 200,
                   anchor.z + cs * row.front - sn * row.across };
        if (!world.Floor(&pos))
            continue;
        pos.y += row.lift;
        state.attempted[i] = true;
        state.handles[i] = world.Spawn(row, pos, yaw);
    }
}
} // namespace ComboRegionalActors
