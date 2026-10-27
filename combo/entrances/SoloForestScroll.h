#pragma once
#include <stdint.h>
// Generated from FINAL TexAnim by tools/black_market/final_forest.py.
// Native MM negates serialized Y velocity; these values already match it.
namespace SoloForestScroll {
struct Layer {
    int8_t x, y;
    uint8_t width, height;
};
struct Binding {
    uint8_t segment, count;
    Layer layers[2];
};
inline constexpr Binding kBindings[] = {
    // Segment 0x08
    { 8, 2, { { 0, 0, 32, 32 }, { 0, -1, 32, 64 } } },
    // Segment 0x09
    { 9, 2, { { 0, 0, 32, 32 }, { -1, 0, 32, 64 } } },
    // Segment 0x0A
    { 10, 1, { { -1, -1, 64, 64 }, { 0, 0, 0, 0 } } },
    // Segment 0x0B
    { 11, 1, { { 1, 1, 64, 64 }, { 0, 0, 0, 0 } } },
    // Segment 0x0C
    { 12, 1, { { -1, 1, 64, 64 }, { 0, 0, 0, 0 } } },
    // Segment 0x0D
    { 13, 1, { { 0, -1, 64, 64 }, { 0, 0, 0, 0 } } },
};
inline constexpr const char* kMarker = "custom/skiejer_forest/final_scroll_v1";
} // namespace SoloForestScroll
