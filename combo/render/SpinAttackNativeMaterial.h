#ifndef COMBO_SPIN_ATTACK_NATIVE_MATERIAL_H
#define COMBO_SPIN_ATTACK_NATIVE_MATERIAL_H
#include <stddef.h>
#include <stdint.h>

// Native GBI types must already be included by the host. Clone into frame-owned
// storage: no resource patch, global texture substitution or pointer cache.
static inline size_t CwSpinMaterial_Clone(Gfx* out, size_t capacity, const Gfx* source, size_t count, uint64_t texture0,
                                          uint64_t texture1, uint32_t primary, uint32_t environment) {
    if (!out || !source || count == 0 || count > 1024 || !texture0 || !texture1)
        return 0;
    size_t written = 0, textures = 0;
    for (size_t i = 0; i < count;) {
        uint32_t w0 = (uint32_t)source[i].words.w0;
        uint8_t op = (uint8_t)(w0 >> 24);
        size_t length = (op == 0x20 || op == 0x32 || op == 0x33) ? 2 : 1;
        size_t needed = op == 0xFD ? 2 : length;
        if (i + length > count || written + needed > capacity)
            return 0;
        switch (op) {
            case 0x20:
            case 0xFD: // Texture image by hash or native pointer.
            case 0x32:
            case 0x33: // Vertex resource or trace marker.
            case 0x01:
            case 0x03:
            case 0x05:
            case 0x06: // Native vertices/culling/triangles.
            case 0xD7:
            case 0xD9:
            case 0xDF: // Texture/geometry/end.
            case 0xE2:
            case 0xE3:
            case 0xE6:
            case 0xE7:
            case 0xE8:
            case 0xF2:
            case 0xF3:
            case 0xF4:
            case 0xF5:
            case 0xFA:
            case 0xFB:
            case 0xFC:
                break;
            case 0xDE:
                // Preserve the native scroll segment calls, but do not rewrite
                // an uninspected custom child graph or a matrix-bearing branch.
                if (source[i].words.w1 != 0x08000000 && source[i].words.w1 != 0x08000001 &&
                    source[i].words.w1 != 0x09000000 && source[i].words.w1 != 0x09000001)
                    return 0;
                break;
            default:
                return 0;
        }
        out[written] = source[i];
        if (op == 0x20 || op == 0xFD) {
            if (textures >= 2)
                return 0;
            uint64_t hash = textures++ == 0 ? texture0 : texture1;
            out[written].words.w0 = (w0 & 0x00FFFFFFu) | 0x20000000u;
            out[written].words.w1 = 0;
            out[written + 1] = (Gfx){ 0 };
            out[written + 1].words.w0 = (uint32_t)(hash >> 32);
            out[written + 1].words.w1 = (uint32_t)hash;
        } else if (op == 0xFA || op == 0xFB) {
            uint32_t rgb = op == 0xFA ? primary : environment;
            out[written].words.w1 = (rgb << 8) | (source[i].words.w1 & 255);
        } else if (op == 0xFC) {
            // Material tint uses both existing cosmetic channels, even if a
            // native/custom shape's old shader used only PRIMITIVE. Geometry,
            // UVs and all eight alpha-mux fields remain unchanged.
            uint32_t rgb0 = (15u << 20) | (31u << 15) | (15u << 5) | 31u;
            uint32_t rgb1 = (15u << 28) | (15u << 24) | (7u << 15) | (7u << 6);
            out[written].words.w0 = (w0 & ~rgb0) | (3u << 20) | (8u << 15) | (3u << 5) | 8u;
            out[written].words.w1 =
                ((uint32_t)source[i].words.w1 & ~rgb1) | (5u << 28) | (5u << 24) | (5u << 15) | (5u << 6);
        } else if (length == 2) {
            out[written + 1] = source[i + 1];
        }
        written += needed;
        i += length;
        if (op == 0xDF)
            return i == count && textures != 0 ? written : 0;
    }
    return 0;
}
#endif
