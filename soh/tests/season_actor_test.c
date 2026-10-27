// Production draw bodies; only the GPU, matrix, interpolation and hook boundaries
// are fixtures. Assert the command stream, including the untouched trunk/stump.
#include "global.h"
#include "overlays/actors/ovl_En_Kusa/z_en_kusa.h"
#include "overlays/actors/ovl_En_Wood02/z_en_wood02.h"
#include "objects/gameplay_field_keep/gameplay_field_keep.h"
#include "objects/object_kusa/object_kusa.h"
#include "objects/object_wood02/object_wood02.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "soh/Enhancements/audio/SeasonWeather.h"
#if __has_include("soh/Enhancements/Graphics/SeasonFoliageActor.h")
#include "soh/Enhancements/Graphics/SeasonFoliageActor.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUIRE(condition)                                               \
    do {                                                                 \
        if (!(condition)) {                                              \
            fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #condition); \
            exit(1);                                                     \
        }                                                                \
    } while (0)

static PlayState play;
static GraphicsContext gfx;
static Gfx opaque[64], translucent[64];
static Mtx matrix;
static int season = -1, eligible = 1;

int32_t SeasonWeather_SeasonForPlay(const PlayState* current) {
    REQUIRE(current == &play);
    return eligible ? season : -1;
}
void FrameInterpolation_RecordOpenChild(const void* child, int line) {
    (void)child;
    (void)line;
}
void FrameInterpolation_RecordCloseChild(void) {
}
void Gfx_SetupDL_25Opa(GraphicsContext* current) {
    REQUIRE(current == &gfx);
}
void Gfx_SetupDL_25Xlu(GraphicsContext* current) {
    REQUIRE(current == &gfx);
}
void gSPDisplayList(Gfx* command, Gfx* list) {
    __gSPDisplayList(command, list);
}
void Gfx_DrawDListOpa(PlayState* current, Gfx* list) {
    gSPDisplayList(current->state.gfxCtx->polyOpa.p++, list);
}
Mtx* Matrix_NewMtx(GraphicsContext* current, char* file, s32 line) {
    (void)file;
    (void)line;
    REQUIRE(current == &gfx);
    return &matrix;
}
bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) {
    REQUIRE(flag == VB_TREE_SETUP_DRAW);
    // Shuffled-tree crates can be emitted here. They must see no active tint.
    for (Gfx* first = opaque; first < gfx.polyOpa.p; ++first)
        REQUIRE((first->words.w0 >> 24) != G_SETGRAYSCALE || first->words.w1 == 0);
    return result;
}

#include "season_actor_production.inc"

static void Begin(void) {
    gfx.polyOpa.p = opaque;
    gfx.polyXlu.p = translucent;
}
static void Check(const Gfx* first, const Gfx* end, const void* list, int expected, const Gfx* native,
                  size_t nativeCount, u32 color, unsigned mode) {
    size_t index = 0;
    int active = 0, draws = 0, colors = 0;
    for (const Gfx* cmd = first; cmd < end; ++cmd) {
        const unsigned op = cmd->words.w0 >> 24;
        if (op == G_SETGRAYSCALE) {
            active = cmd->words.w1 != 0;
            REQUIRE(!active || cmd->words.w1 == mode);
            continue;
        }
        if (op == G_SETINTENSITY) {
            REQUIRE(cmd->words.w1 == color);
            ++colors;
            continue;
        }
        if (op == G_DL && cmd->words.w1 == (uintptr_t)list) {
            REQUIRE(active == expected);
            ++draws;
        }
        REQUIRE(index < nativeCount && !memcmp(cmd, &native[index++], sizeof(Gfx)));
    }
    REQUIRE(!active && draws == 1 && colors == expected && index == nativeCount);
}
static void Grass(void) {
    for (int type = 0; type < 3; ++type) {
        for (int cut = 0; cut < 2; ++cut) {
            EnKusa actor = { 0 };
            actor.actor.params = type;
            actor.actor.flags = cut ? ACTOR_FLAG_GRASS_DESTROYED : 0;
            const void* list = cut ? object_kusa_DL_0002E0 : type ? object_kusa_DL_000140 : gFieldBushDL;
            season = -1;
            Begin();
            EnKusa_Draw(&actor.actor, &play);
            Gfx native[64];
            const size_t count = gfx.polyOpa.p - opaque;
            memcpy(native, opaque, count * sizeof(Gfx));
            for (int current = 0; current < 4; ++current) {
                season = current;
                Begin();
                EnKusa_Draw(&actor.actor, &play);
                const u32 color = current == 0 ? 0xF6C1D9FF : current == 2 ? 0xF26846FF : 0xDCECF5FF;
                const unsigned mode = current == 2 ? 2 : 3;
                Check(opaque, gfx.polyOpa.p, list, !cut && current != 1, native, count, color, mode);
            }
            season = 2;
            eligible = 0;
            Begin();
            EnKusa_Draw(&actor.actor, &play);
            REQUIRE(gfx.polyOpa.p - opaque == count && !memcmp(opaque, native, count * sizeof(Gfx)));
            eligible = 1;
        }
    }
}
static void Trees(void) {
    for (int type = 0; type <= 5; ++type) {
        EnWood02 actor = { 0 };
        actor.drawType = type;
        actor.actor.params = type < 3 ? WOOD_TREE_CONICAL_SMALL : WOOD_BUSH_GREEN_SMALL;
        const void* list = type < 3 ? D_80B3BF70[type] : D_80B3BF54[type];
        season = -1;
        Begin();
        EnWood02_Draw(&actor.actor, &play);
        Gfx nativeOpa[64], nativeXlu[64];
        size_t opaCount = gfx.polyOpa.p - opaque, xluCount = gfx.polyXlu.p - translucent;
        memcpy(nativeOpa, opaque, opaCount * sizeof(Gfx));
        memcpy(nativeXlu, translucent, xluCount * sizeof(Gfx));
        for (int current = 0; current < 4; ++current) {
            season = current;
            Begin();
            EnWood02_Draw(&actor.actor, &play);
            REQUIRE(gfx.polyOpa.p - opaque == opaCount && !memcmp(opaque, nativeOpa, opaCount * sizeof(Gfx)));
            const u32 color = current == 0   ? 0xF6C1D9FF
                              : current == 2 ? (type < 3 ? 0xEF484CFF : 0xF26846FF)
                                             : (type < 3 ? 0xE2EEE9FF : 0xDCECF5FF);
            Check(translucent, gfx.polyXlu.p, list, current != 1 && type <= 3, nativeXlu, xluCount, color,
                  current == 0   ? 3
                  : current == 2 ? 2
                                 : 3);
        }
    }
    for (int type = WOOD_LEAF_GREEN; type <= WOOD_LEAF_YELLOW; ++type) {
        EnWood02 actor = { 0 };
        actor.actor.params = type;
        for (int current = -1; current < 4; ++current) {
            season = current;
            Begin();
            EnWood02_Draw(&actor.actor, &play);
            for (Gfx* cmd = opaque; cmd < gfx.polyOpa.p; ++cmd)
                REQUIRE((cmd->words.w0 >> 24) != G_SETGRAYSCALE);
        }
    }
}
int main(void) {
    play.state.gfxCtx = &gfx;
    Grass();
    Trees();
    puts("PASS actual OoT grass/tree draw command scopes: palettes, native restore, trunk/stump/marker scope");
}
