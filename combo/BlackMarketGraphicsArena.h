#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace BlackMarketGraphicsArena {
// Native tail allocators do not report exhaustion. Check the whole optional
// draw before its first allocation, including transient OPEN/CLOSE_DISPS nodes.
// tailBytes includes each aligned vertex/matrix/scroll-list allocation;
// command counts are Gfx packets, including setup and restore commands.
template <class Graphics>
bool HasRoom(const Graphics* gfx, size_t tailBytes, size_t opaCommands, size_t xluCommands,
             size_t overlayCommands = 2) {
    if (!gfx || tailBytes > std::numeric_limits<size_t>::max() - 15)
        return false;
    tailBytes = (tailBytes + 15) & ~size_t(15);
    const auto fits = [](const auto& arena, size_t bytes, size_t commands, bool alignTail) {
        const uintptr_t head = reinterpret_cast<uintptr_t>(arena.p);
        uintptr_t tail = reinterpret_cast<uintptr_t>(arena.d);
        if (alignTail)
            tail &= ~uintptr_t(15);
        if (!head || !tail || tail < head || bytes > tail - head)
            return false;
        return commands <= (tail - head - bytes) / sizeof(*arena.p);
    };
    return fits(gfx->polyOpa, tailBytes, opaCommands, true) && fits(gfx->polyXlu, 0, xluCommands, false) &&
           fits(gfx->overlay, 0, overlayCommands, false);
}
} // namespace BlackMarketGraphicsArena
