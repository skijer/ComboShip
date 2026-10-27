#ifndef ELEMENTAL_SWORD_AURA_GBI_CHECKS_H
#define ELEMENTAL_SWORD_AURA_GBI_CHECKS_H
// Inspect the submitted native GBI graph and its real, in-process vertices.
typedef struct {
    int vertices, triangles, textures, matrices;
    int min[3], max[3];
    uintptr_t texture[8];
    int scroll;
} AuraGbi;

static void AuraWalk(AuraGbi* result, const Gfx* start, const Gfx* end, int depth) {
    REQUIRE(depth < 8);
    unsigned cache = 0;
    for (size_t offset = 0; offset < 4096; ++offset) {
        const Gfx* command = start + offset;
        if (end != NULL && command == end) break;
        unsigned op = command->words.w0 >> 24;
        if (op == G_ENDDL) return;
        if (op == G_DL) {
            const Gfx* child = (const Gfx*)command->words.w1;
            REQUIRE(child != NULL);
            AuraWalk(result, child, NULL, depth + 1);
        } else if (op == G_VTX) {
            int count = (command->words.w0 >> 12) & 0xFF;
            int first = ((command->words.w0 >> 1) & 0x7F) - count;
            const Vtx* vertices = (const Vtx*)command->words.w1;
            REQUIRE(vertices != NULL && count > 0 && count <= 32 && first >= 0 && first + count <= 32);
            for (int i = 0; i < count; ++i) {
                cache |= 1u << (first + i);
                for (int axis = 0; axis < 3; ++axis) {
                    int value = vertices[i].v.ob[axis];
                    if (value < result->min[axis]) result->min[axis] = value;
                    if (value > result->max[axis]) result->max[axis] = value;
                }
            }
            result->vertices += count;
        } else if (op == G_TRI1) {
            for (int shift = 0; shift < 24; shift += 8) {
                unsigned vertex = (command->words.w0 >> shift) & 0xFF;
                REQUIRE(!(vertex & 1) && vertex < 64 && (cache & (1u << (vertex / 2))));
            }
            ++result->triangles;
        } else if (op == G_TRI2) {
            for (int shift = 0; shift < 24; shift += 8) {
                unsigned a = (command->words.w0 >> shift) & 0xFF;
                unsigned b = (command->words.w1 >> shift) & 0xFF;
                REQUIRE(!(a & 1) && a < 64 && (cache & (1u << (a / 2))));
                REQUIRE(!(b & 1) && b < 64 && (cache & (1u << (b / 2))));
            }
            result->triangles += 2;
        } else if (op == G_SETTIMG) {
            REQUIRE(result->textures < 8);
            const u8* pixels = (const u8*)command->words.w1;
            REQUIRE(pixels != NULL && memcmp(pixels, "__OTR__", 7) != 0);
            result->texture[result->textures++] = (uintptr_t)pixels;
        } else if (op == G_MTX) {
            ++result->matrices;
        } else if (op == G_SETTILESIZE) {
            result->scroll = command->words.w0 & 0xFFF;
        }
    }
    REQUIRE(depth == 0); // Every child list must end in a native EndDisplayList.
}

static AuraGbi AuraInspect(Gfx* opa, Gfx* opaEnd, Gfx* xlu, Gfx* xluEnd) {
    AuraGbi result = { 0 };
    for (int axis = 0; axis < 3; ++axis) {
        result.min[axis] = 32767;
        result.max[axis] = -32768;
    }
    AuraWalk(&result, opa, opaEnd, 0);
    AuraWalk(&result, xlu, xluEnd, 0);
    return result;
}

static int AuraHasColor(Gfx* start, Gfx* end, unsigned opcode, u32 rgb) {
    for (Gfx* command = start; command < end; ++command)
        if ((command->words.w0 >> 24) == opcode && ((command->words.w1 >> 8) & 0xFFFFFF) == rgb) return 1;
    return 0;
}
#endif
