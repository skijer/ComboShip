#ifndef COMBO_SPIN_ATTACK_ACCENT_NATIVE_H
#define COMBO_SPIN_ATTACK_ACCENT_NATIVE_H

// The native effect DL owns its texture sampling, UVs, geometry and blend mode.
// Localize its scroll call and colors in frame storage; never patch a resource
// or borrow segment 08 from the spin ribbon that is already being drawn.
static inline size_t CwSpinAccents_NativeMaterial(Gfx* out, const Gfx* source, size_t count, Gfx* scroll) {
    if (!source || count == 0 || count > 128)
        return 0;
    unsigned textures = 0, vertices = 0, triangles = 0, combines = 0;
    for (size_t i = 0; i < count;) {
        uint8_t op = (uint8_t)((uint32_t)source[i].words.w0 >> 24);
        size_t length = op == 0x20 || op == 0x32 || op == 0x33 ? 2 : 1;
        if (i + length > count)
            return 0;
        switch (op) {
            case 0x20:
                ++textures;
                break;
            case 0x32:
                ++vertices;
                break;
            case 0x05:
            case 0x06:
                ++triangles;
                break;
            case 0xFC:
                ++combines;
                break;
            case 0xDE:
                if (!scroll || (source[i].words.w1 != 0x08000000 && source[i].words.w1 != 0x08000001) ||
                    ((uint32_t)source[i].words.w0 & 0x00FFFFFFu) != 0)
                    return 0;
                break;
            case 0x33: // Native resource marker (two words).
            case 0xD7:
            case 0xD9:
            case 0xDF:
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
                break;
            default: // A replacement with an uninspected child/matrix is not safe to clone.
                return 0;
        }
        if (out) {
            memcpy(out + i, source + i, length * sizeof(Gfx));
            if (op == 0xDE)
                __gSPDisplayList(out + i, scroll);
            else if (op == 0xFA || op == 0xFB)
                gDPPipeSync(out + i); // The particle's color/alpha precedes this DL.
        }
        i += length;
        if (op == 0xDF)
            return i == count && textures && vertices && triangles && combines ? count : 0;
    }
    return 0;
}

static inline float CwSpinAccents_ColumnLength(float x, float y, float z) {
    return sqrtf(x * x + y * y + z * z);
}

// Keep the original particle origin in the parent's burst/blade frame. Native
// flame is upright and camera-facing, sparkle is a rotating full billboard,
// and ice retains a tumbling three-dimensional mesh. No CPU matrix is changed.
static inline void CwSpinAccents_NativeModel(MtxF* out, const MtxF* parent, const MtxF* billboard,
                                             const CwSpinAccentParticle* p, uint8_t element, float phase, size_t i) {
    memset(out, 0, sizeof(*out));
    out->ww = 1;
    out->xw = parent->xw + parent->xx * p->x + parent->xy * p->y + parent->xz * p->z;
    out->yw = parent->yw + parent->yx * p->x + parent->yy * p->y + parent->yz * p->z;
    out->zw = parent->zw + parent->zx * p->x + parent->zy * p->y + parent->zz * p->z;
    if (element == 2) {
        float yaw = phase * 1.9f + (float)i * 2.39996323f;
        float pitch = phase * 2.3f + (float)i * 0.73f;
        float cy = cosf(yaw), sy = sinf(yaw), cp = cosf(pitch), sp = sinf(pitch);
        float scale = p->size / 16.0f; // Native shard bounds: -11..16, -14..11, -13..13.
        out->xx = (parent->xx * cy - parent->xz * sy) * scale;
        out->yx = (parent->yx * cy - parent->yz * sy) * scale;
        out->zx = (parent->zx * cy - parent->zz * sy) * scale;
        out->xy = (parent->xx * sy * sp + parent->xy * cp + parent->xz * cy * sp) * scale;
        out->yy = (parent->yx * sy * sp + parent->yy * cp + parent->yz * cy * sp) * scale;
        out->zy = (parent->zx * sy * sp + parent->zy * cp + parent->zz * cy * sp) * scale;
        out->xz = (parent->xx * sy * cp - parent->xy * sp + parent->xz * cy * cp) * scale;
        out->yz = (parent->yx * sy * cp - parent->yy * sp + parent->yz * cy * cp) * scale;
        out->zz = (parent->zx * sy * cp - parent->zy * sp + parent->zz * cy * cp) * scale;
        return;
    }
    float sx = CwSpinAccents_ColumnLength(parent->xx, parent->yx, parent->zx);
    float sy = CwSpinAccents_ColumnLength(parent->xy, parent->yy, parent->zy);
    float sz = CwSpinAccents_ColumnLength(parent->xz, parent->yz, parent->zz);
    if (element == 1) {
        float right = sqrtf(billboard->xx * billboard->xx + billboard->zx * billboard->zx);
        float rx = right > 0.00001f ? billboard->xx / right : 1;
        float rz = right > 0.00001f ? billboard->zx / right : 0;
        float scale = p->size / 8000.0f; // Native flame is 8000 wide, 16000 high, with its origin at its foot.
        out->xx = rx * sx * scale;
        out->zx = rz * sx * scale;
        out->yy = sy * scale;
        out->xz = -rz * sz * scale;
        out->zz = rx * sz * scale;
        out->yw -= out->yy * 8000; // Center the native flame on the existing particle origin.
    } else {
        float angle = phase * 2.0f + (float)i * 1.7f;
        float c = cosf(angle), s = sinf(angle), scale = p->size / 14.0f;
        out->xx = (billboard->xx * c + billboard->xy * s) * sx * scale;
        out->yx = (billboard->yx * c + billboard->yy * s) * sx * scale;
        out->zx = (billboard->zx * c + billboard->zy * s) * sx * scale;
        out->xy = (billboard->xy * c - billboard->xx * s) * sy * scale;
        out->yy = (billboard->yy * c - billboard->yx * s) * sy * scale;
        out->zy = (billboard->zy * c - billboard->zx * s) * sy * scale;
        out->xz = billboard->xz * sz * scale;
        out->yz = billboard->yz * sz * scale;
        out->zz = billboard->zz * sz * scale;
    }
}

// Same two-tile scrolling pattern as the native EnFire/IcePiece effects, with
// frame-owned storage allocated by our caller so an allocation failure emits
// nothing. Each endpoint includes the native next-frame step for interpolation.
static inline void CwSpinAccents_NativeScroll(Gfx* dl, uint8_t element, uint32_t frame) {
    float y0 = element == 2 ? (float)(frame & 255) : 0;
    float y1 = (float)((element == 2 ? frame * 2u : 0u - frame * 20u) & (element == 2 ? 255u : 511u));
    int w0 = 32, h0 = element == 2 ? 16 : 64;
    int w1 = element == 2 ? 64 : 32, h1 = element == 2 ? 32 : 128;
    int step0 = element == 2 ? 1 : 0, step1 = element == 2 ? 2 : -20;
    gDPTileSync(dl);
    gDPSetTileSizeLerp(dl + 1, 0, 0, y0, (w0 - 1) * 4, y0 + (h0 - 1) * 4, 0, y0 + step0, (w0 - 1) * 4,
                       y0 + step0 + (h0 - 1) * 4);
    gDPSetTileSizeLerp(dl + 6, 1, 0, y1, (w1 - 1) * 4, y1 + (h1 - 1) * 4, 0, y1 + step1, (w1 - 1) * 4,
                       y1 + step1 + (h1 - 1) * 4);
    gSPEndDisplayList(dl + 11);
}
#endif
