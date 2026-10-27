#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace BlackMarketVisual {
struct Point {
    float x, y, z;
};
struct Frame {
    Point minimum{}, maximum{};
    bool valid = false;
};

// A native graph-thread scope brackets one offer; nested previews restore it.
inline thread_local Frame* activeFrame = nullptr;
class Capture {
    Frame* previous;

  public:
    Frame frame;
    explicit Capture(bool enabled = true) : previous(activeFrame) {
        activeFrame = enabled ? &frame : nullptr;
    }
    ~Capture() {
        activeFrame = previous;
    }
    Capture(const Capture&) = delete;
    Capture& operator=(const Capture&) = delete;
};
inline bool Active() {
    return activeFrame != nullptr;
}

inline bool Finite(float value) {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return (bits & 0x7f800000u) != 0x7f800000u;
}
inline bool Finite(Point point) {
    return Finite(point.x) && Finite(point.y) && Finite(point.z);
}

template <class Transform> void Report(Point minimum, Point maximum, Transform transform) {
    if (!Active() || !Finite(minimum) || !Finite(maximum) || minimum.x > maximum.x || minimum.y > maximum.y ||
        minimum.z > maximum.z)
        return;
    Frame selected;
    for (unsigned corner = 0; corner < 8; ++corner) {
        const Point point = transform({ corner & 1 ? maximum.x : minimum.x, corner & 2 ? maximum.y : minimum.y,
                                        corner & 4 ? maximum.z : minimum.z });
        if (!Finite(point))
            return; // Publish a complete frame, never a partially transformed box.
        if (!selected.valid)
            selected = { point, point, true };
        else {
            selected.minimum = { std::min(selected.minimum.x, point.x), std::min(selected.minimum.y, point.y),
                                 std::min(selected.minimum.z, point.z) };
            selected.maximum = { std::max(selected.maximum.x, point.x), std::max(selected.maximum.y, point.y),
                                 std::max(selected.maximum.z, point.z) };
        }
    }
    if (!activeFrame->valid)
        *activeFrame = selected;
    else {
        activeFrame->minimum = { std::min(activeFrame->minimum.x, selected.minimum.x),
                                 std::min(activeFrame->minimum.y, selected.minimum.y),
                                 std::min(activeFrame->minimum.z, selected.minimum.z) };
        activeFrame->maximum = { std::max(activeFrame->maximum.x, selected.maximum.x),
                                 std::max(activeFrame->maximum.y, selected.maximum.y),
                                 std::max(activeFrame->maximum.z, selected.maximum.z) };
    }
}
} // namespace BlackMarketVisual
