// Production grass draw bodies with only GPU/matrix/light/hook boundaries replaced.
#include "global.h"
#include "2s2h/BenPort.h"
#include "2s2h/ShipUtils.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/Enhancements/Audio/MMWeather.h"
#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"
#include "overlays/actors/ovl_En_Kusa/z_en_kusa.h"
#include "overlays/actors/ovl_Obj_Grass/z_obj_grass.h"
#include "objects/object_kusa/object_kusa.h"
#include "objects/gameplay_field_keep/gameplay_field_keep.h"
#include "overlays/ovl_Obj_Grass/ovl_Obj_Grass.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#undef OPEN_DISPS_PORT_HELPERS
#undef CLOSE_DISPS_PORT_HELPERS
#define OPEN_DISPS_PORT_HELPERS(gfxCtx)
#define CLOSE_DISPS_PORT_HELPERS(gfxCtx)

static PlayState play;
static GraphicsContext gfx;
static Gfx opaque[128], translucent[128];
static int season = SEASON_AUTUMN, eligible = 1, allowDraw = 1, sway;
static int replaceFirstDraw = 0, groupedDrawCalls = 0;
static Mtx matrix;
static MtxF D_80936AD8[8];

int MMWeather_SeasonForPlay(const PlayState* current) {
    assert(current == &play);
    return eligible ? season : -1;
}
void Gfx_SetupDL25_Opa(GraphicsContext* current) {
    assert(current == &gfx);
}
void Gfx_SetupDL25_Xlu(GraphicsContext* current) {
    assert(current == &gfx);
}
void gSPDisplayList(Gfx* command, Gfx* list) {
    __gSPDisplayList(command, list);
}
void Gfx_DrawDListOpa(PlayState* current, Gfx* list) {
    gSPDisplayList(current->state.gfxCtx->polyOpa.p++, list);
}
Mtx* Matrix_Finalize(GraphicsContext* current) {
    assert(current == &gfx);
    return &matrix;
}
void Matrix_SetTranslateRotateYXZ(f32 x, f32 y, f32 z, Vec3s* rotation) {
    (void)x;
    (void)y;
    (void)z;
    (void)rotation;
}
void Matrix_Scale(f32 x, f32 y, f32 z, MatrixMode mode) {
    (void)x;
    (void)y;
    (void)z;
    (void)mode;
}
void FrameInterpolation_RecordOpenChild(const void* child, int epoch) {
    (void)child;
    (void)epoch;
}
void FrameInterpolation_RecordCloseChild(void) {
}
void Ship_ExtendedCullingActorAdjustProjectedZ(Actor* actor) {
    (void)actor;
}
void Ship_ExtendedCullingActorRestoreProjectedPos(PlayState* current, Actor* actor) {
    (void)current;
    (void)actor;
}
bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) {
    assert(flag == VB_OBJGRASS_OPA_DRAW_BE_OVERRIDDEN || flag == VB_OBJGRASS_XLU_DRAW_BE_OVERRIDDEN);
    assert(result);
    if (replaceFirstDraw && groupedDrawCalls++ == 0) {
        // The production Road randomizer leaf wrappers disable tint before
        // their flower. Record that hook boundary, then draw a native neighbor.
        if (flag == VB_OBJGRASS_OPA_DRAW_BE_OVERRIDDEN) {
            gSPGrayscale(gfx.polyOpa.p++, false);
        } else {
            gSPGrayscale(gfx.polyXlu.p++, false);
        }
        return false;
    }
    return allowDraw;
}
Lights* LightContext_NewLights(LightContext* context, GraphicsContext* current) {
    static Lights lights;
    (void)context;
    assert(current == &gfx);
    return &lights;
}
void Lights_BindAll(Lights* lights, LightNode* node, Vec3f* pos, PlayState* current) {
    (void)lights;
    (void)node;
    (void)pos;
    assert(current == &play);
}
void Lights_Draw(Lights* lights, GraphicsContext* current) {
    (void)lights;
    assert(current == &gfx);
}
void EnKusa_ApplySway(MtxF* current) {
    (void)current;
    ++sway;
}
void EnKusa_WaitForInteract(EnKusa* actor, PlayState* current) {
    (void)actor;
    (void)current;
}
void ObjGrass_OverrideMatrixCurrent(MtxF* current) {
    (void)current;
}

#include "autumn_grass_production.inc"

static void Begin(void) {
    gfx.polyOpa.p = opaque;
    gfx.polyXlu.p = translucent;
}
static void Check(const Gfx* first, const Gfx* end, const void* leafList, const Gfx* native, size_t count,
                  u32 expectedColor, unsigned expectedMode) {
    int tinted = 0, drawn = 0, colors = 0;
    size_t sourceIndex = 0;
    for (const Gfx* command = first; command < end; ++command) {
        const unsigned op = command->words.w0 >> 24;
        if (op == G_SETGRAYSCALE) {
            tinted = command->words.w1 != 0;
            assert(!tinted || command->words.w1 == expectedMode);
            continue;
        }
        if (op == G_SETINTENSITY) {
            assert(command->words.w1 == expectedColor);
            ++colors;
            continue;
        }
        if (op == G_DL && command->words.w1 == (uintptr_t)leafList) {
            assert(tinted);
            ++drawn;
        }
        assert(sourceIndex < count && !memcmp(command, &native[sourceIndex++], sizeof(Gfx)));
    }
    assert(!tinted && drawn == 1 && colors == 1 && sourceIndex == count);
}

static void Kusa(void) {
    EnKusa grass = { 0 };
    grass.actor.projectedPos.z = 500;
    for (int bush = 0; bush <= 1; ++bush)
        for (int fade = 0; fade <= bush; ++fade) {
            grass.actor.projectedPos.z = fade ? 1250 : 500;
            void (*draw)(Actor*, PlayState*) = bush ? EnKusa_DrawBush : EnKusa_DrawGrass;
            season = SEASON_OFF;
            Begin();
            draw(&grass.actor, &play);
            Gfx native[128];
            const Gfx* start = fade ? translucent : opaque;
            const size_t count = (fade ? gfx.polyXlu.p : gfx.polyOpa.p) - start;
            memcpy(native, start, count * sizeof(Gfx));
            season = SEASON_AUTUMN;
            Begin();
            draw(&grass.actor, &play);
            Check(fade ? translucent : opaque, fade ? gfx.polyXlu.p : gfx.polyOpa.p,
                  !bush  ? gKusaSproutDL
                  : fade ? gKusaBushType2DL
                         : gKusaBushType1DL,
                  native, count, 0xF26846FF, 2);
            eligible = 0;
            Begin();
            draw(&grass.actor, &play);
            assert((size_t)((fade ? gfx.polyXlu.p : gfx.polyOpa.p) - (fade ? translucent : opaque)) == count);
            assert(!memcmp(native, fade ? translucent : opaque, count * sizeof(Gfx)));
            eligible = 1;
        }
    grass.isCut = 1;
    Begin();
    EnKusa_DrawGrass(&grass.actor, &play);
    assert(gfx.polyOpa.p - opaque == 1 && opaque[0].words.w1 == (uintptr_t)gKusaStumpDL);
    puts("PASS cuttable grass/bush OPA and faded XLU keep original alpha/materials; stumps and ineligible play "
         "unchanged");
}

static void GroupedGrass(void) {
    static ObjGrass grass;
    grass.activeGrassGroups = 1;
    grass.grassGroups[0].count = 1;
    grass.grassGroups[0].flags = OBJ_GRASS_GROUP_DRAW;
    ObjGrassElement* element = &grass.grassGroups[0].elements[0];
    element->flags = OBJ_GRASS_ELEM_DRAW;
    for (int fade = 0; fade <= 1; ++fade) {
        element->alpha = fade ? 96 : 255;
        void (*draw)(Actor*, PlayState*) = fade ? ObjGrass_DrawXlu : ObjGrass_DrawOpa;
        season = SEASON_OFF;
        Begin();
        draw(&grass.actor, &play);
        Gfx native[128];
        const Gfx* start = fade ? translucent : opaque;
        const size_t count = (fade ? gfx.polyXlu.p : gfx.polyOpa.p) - start;
        memcpy(native, start, count * sizeof(Gfx));
        season = SEASON_AUTUMN;
        Begin();
        draw(&grass.actor, &play);
        Check(fade ? translucent : opaque, fade ? gfx.polyXlu.p : gfx.polyOpa.p, gObjGrass_D_809AAAE0, native, count,
              0xF26846FF, 2);
        assert(element->alpha == (fade ? 96 : 255));
        allowDraw = 0;
        Begin();
        draw(&grass.actor, &play);
        for (const Gfx* command = fade ? translucent : opaque; command < (fade ? gfx.polyXlu.p : gfx.polyOpa.p);
             ++command)
            assert(command->words.w1 != (uintptr_t)gObjGrass_D_809AAAE0);
        allowDraw = 1;
    }
    puts("PASS grouped grass OPA/XLU scoped color, unchanged fade alpha and existing replacement hook routing");
}

static void Spring(void) {
    EnKusa grass = { 0 };
    ObjGrass grouped = { 0 };
    grouped.activeGrassGroups = 1;
    grouped.grassGroups[0].count = 1;
    grouped.grassGroups[0].flags = OBJ_GRASS_GROUP_DRAW;
    grouped.grassGroups[0].elements[0].flags = OBJ_GRASS_ELEM_DRAW;
    struct {
        void (*draw)(Actor*, PlayState*);
        Actor* actor;
        const void* list;
        int fade;
    } cases[] = {
        { EnKusa_DrawGrass, &grass.actor, gKusaSproutDL, 0 },
        { EnKusa_DrawBush, &grass.actor, gKusaBushType1DL, 0 },
        { EnKusa_DrawBush, &grass.actor, gKusaBushType2DL, 1 },
        { ObjGrass_DrawOpa, &grouped.actor, gObjGrass_D_809AAAE0, 0 },
        { ObjGrass_DrawXlu, &grouped.actor, gObjGrass_D_809AAAE0, 1 },
    };
    for (unsigned i = 0; i < ARRAY_COUNT(cases); ++i) {
        grass.actor.projectedPos.z = cases[i].fade ? 1250 : 500;
        grouped.grassGroups[0].elements[0].alpha = cases[i].fade ? 96 : 255;
        for (unsigned scene = 0; scene < 2; ++scene) {
            play.sceneId = scene ? SCENE_00KEIKOKU : SCENE_BACKTOWN;
            season = SEASON_OFF;
            Begin();
            cases[i].draw(cases[i].actor, &play);
            const Gfx* start = cases[i].fade ? translucent : opaque;
            const size_t count = (cases[i].fade ? gfx.polyXlu.p : gfx.polyOpa.p) - start;
            Gfx native[128];
            memcpy(native, start, count * sizeof(Gfx));
            season = SEASON_SPRING;
            Begin();
            cases[i].draw(cases[i].actor, &play);
            Check(cases[i].fade ? translucent : opaque, cases[i].fade ? gfx.polyXlu.p : gfx.polyOpa.p, cases[i].list,
                  native, count, 0xF6C1D9FF, 3);
            assert(grouped.grassGroups[0].elements[0].alpha == (cases[i].fade ? 96 : 255));
            for (int restore = 0; restore < 3; ++restore) {
                season = restore == 0 ? SEASON_SUMMER : restore == 1 ? SEASON_OFF : SEASON_SPRING;
                eligible = restore != 2;
                Begin();
                cases[i].draw(cases[i].actor, &play);
                start = cases[i].fade ? translucent : opaque;
                assert((size_t)((cases[i].fade ? gfx.polyXlu.p : gfx.polyOpa.p) - start) == count);
                assert(!memcmp(native, start, count * sizeof(Gfx)));
            }
            eligible = 1;
        }
    }
    grass.isCut = 1;
    season = SEASON_SPRING;
    Begin();
    EnKusa_DrawGrass(&grass.actor, &play);
    assert(gfx.polyOpa.p - opaque == 1 && opaque[0].words.w1 == (uintptr_t)gKusaStumpDL);
    // Replacements can suppress both grouped draw paths. Seasonal state must
    // still be disabled before the next actor, with the original hook intact.
    allowDraw = 0;
    for (int fade = 0; fade <= 1; ++fade) {
        grouped.grassGroups[0].elements[0].alpha = fade ? 96 : 255;
        Begin();
        (fade ? ObjGrass_DrawXlu : ObjGrass_DrawOpa)(&grouped.actor, &play);
        const Gfx* start = fade ? translucent : opaque;
        const Gfx* end = fade ? gfx.polyXlu.p : gfx.polyOpa.p;
        unsigned mode = 0;
        for (const Gfx* cmd = start; cmd < end; ++cmd) {
            if ((cmd->words.w0 >> 24) == G_SETGRAYSCALE)
                mode = cmd->words.w1;
            assert(cmd->words.w1 != (uintptr_t)gObjGrass_D_809AAAE0);
        }
        assert(mode == 0);
    }
    allowDraw = 1;
    puts(
        "PASS spring grass/bush OPA/XLU readable blossom blush, exact native fade alpha, replacement hooks, stumps and "
        "summer/Off/eligibility cleanup");
}

static void Winter(void) {
    EnKusa grass = { 0 };
    ObjGrass grouped = { 0 };
    grouped.activeGrassGroups = 1;
    grouped.grassGroups[0].count = 1;
    grouped.grassGroups[0].flags = OBJ_GRASS_GROUP_DRAW;
    grouped.grassGroups[0].elements[0].flags = OBJ_GRASS_ELEM_DRAW;
    struct {
        void (*draw)(Actor*, PlayState*);
        Actor* actor;
        const void* list;
        int fade;
    } cases[] = {
        { EnKusa_DrawGrass, &grass.actor, gKusaSproutDL, 0 },
        { EnKusa_DrawBush, &grass.actor, gKusaBushType1DL, 0 },
        { EnKusa_DrawBush, &grass.actor, gKusaBushType2DL, 1 },
        { ObjGrass_DrawOpa, &grouped.actor, gObjGrass_D_809AAAE0, 0 },
        { ObjGrass_DrawXlu, &grouped.actor, gObjGrass_D_809AAAE0, 1 },
    };
    for (unsigned i = 0; i < ARRAY_COUNT(cases); ++i) {
        grass.actor.projectedPos.z = cases[i].fade ? 1250 : 500;
        grouped.grassGroups[0].elements[0].alpha = cases[i].fade ? 96 : 255;
        play.sceneId = SCENE_00KEIKOKU;
        season = SEASON_OFF;
        Begin();
        cases[i].draw(cases[i].actor, &play);
        const Gfx* start = cases[i].fade ? translucent : opaque;
        const size_t count = (cases[i].fade ? gfx.polyXlu.p : gfx.polyOpa.p) - start;
        Gfx native[128];
        memcpy(native, start, count * sizeof(Gfx));
        const s16 scenes[] = { SCENE_00KEIKOKU,  SCENE_BACKTOWN,    SCENE_24KEMONOMITI,
                               SCENE_20SICHITAI, SCENE_20SICHITAI2, SCENE_21MITURINMAE };
        for (unsigned scene = 0; scene < ARRAY_COUNT(scenes); ++scene) {
            play.sceneId = scenes[scene];
            season = SEASON_WINTER;
            Begin();
            cases[i].draw(cases[i].actor, &play);
            Check(cases[i].fade ? translucent : opaque, cases[i].fade ? gfx.polyXlu.p : gfx.polyOpa.p, cases[i].list,
                  native, count, 0xDCECF5FF, 3);
            eligible = 0;
            Begin();
            cases[i].draw(cases[i].actor, &play);
            start = cases[i].fade ? translucent : opaque;
            assert((size_t)((cases[i].fade ? gfx.polyXlu.p : gfx.polyOpa.p) - start) == count);
            assert(!memcmp(native, start, count * sizeof(Gfx)));
            eligible = 1;
        }
    }
    play.sceneId = SCENE_00KEIKOKU;
    grass.isCut = 1;
    Begin();
    EnKusa_DrawGrass(&grass.actor, &play);
    assert(gfx.polyOpa.p - opaque == 1 && opaque[0].words.w1 == (uintptr_t)gKusaStumpDL);
    puts("PASS winter grass/bush OPA/XLU frost across adjacent outdoors; exact native commands, fade alpha and stumps");
}

static void CarriedGrass(void) {
    Actor actor = { 0 };
    play.sceneId = SCENE_24KEMONOMITI;
    season = SEASON_OFF;
    Begin();
    ObjGrassCarry_Draw(&actor, &play);
    Gfx native[128];
    const size_t count = gfx.polyOpa.p - opaque;
    memcpy(native, opaque, count * sizeof(Gfx));
    for (int selected = SEASON_SPRING; selected <= SEASON_WINTER; ++selected) {
        season = selected;
        Begin();
        ObjGrassCarry_Draw(&actor, &play);
        if (selected == SEASON_SUMMER) {
            assert((size_t)(gfx.polyOpa.p - opaque) == count && !memcmp(native, opaque, count * sizeof(Gfx)));
        } else {
            Check(opaque, gfx.polyOpa.p, gKusaBushType1DL, native, count,
                  selected == SEASON_SPRING   ? 0xF6C1D9FF
                  : selected == SEASON_WINTER ? 0xDCECF5FF
                                              : 0xF26846FF,
                  selected == SEASON_AUTUMN ? 2 : 3);
        }
    }
    eligible = 0;
    Begin();
    ObjGrassCarry_Draw(&actor, &play);
    assert((size_t)(gfx.polyOpa.p - opaque) == count && !memcmp(native, opaque, count * sizeof(Gfx)));
    eligible = 1;
    play.sceneId = SCENE_00KEIKOKU;
    Begin();
    ObjGrassCarry_Draw(&actor, &play);
    assert((size_t)(gfx.polyOpa.p - opaque) == count && !memcmp(native, opaque, count * sizeof(Gfx)));
    puts("PASS Road carried grass retains native/Alt draw identity and actor palettes; Summer/Off/story exact");
}

static void MixedGroupedGrass(void) {
    ObjGrass grass = { 0 };
    grass.activeGrassGroups = 1;
    grass.grassGroups[0].count = 2;
    grass.grassGroups[0].flags = OBJ_GRASS_GROUP_DRAW;
    play.sceneId = SCENE_24KEMONOMITI;
    for (int fade = 0; fade <= 1; ++fade) {
        for (int i = 0; i < 2; ++i) {
            grass.grassGroups[0].elements[i].flags = OBJ_GRASS_ELEM_DRAW;
            grass.grassGroups[0].elements[i].alpha = fade ? 96 : 255;
        }
        for (int selected = SEASON_SPRING; selected <= SEASON_WINTER; ++selected) {
            if (selected == SEASON_SUMMER)
                continue;
            season = selected;
            replaceFirstDraw = 1;
            groupedDrawCalls = 0;
            Begin();
            (fade ? ObjGrass_DrawXlu : ObjGrass_DrawOpa)(&grass.actor, &play);
            const Gfx* start = fade ? translucent : opaque;
            const Gfx* end = fade ? gfx.polyXlu.p : gfx.polyOpa.p;
            int tinted = 0, nativeDraws = 0;
            for (const Gfx* cmd = start; cmd < end; ++cmd) {
                const unsigned op = cmd->words.w0 >> 24;
                if (op == G_SETGRAYSCALE)
                    tinted = cmd->words.w1;
                if (op == G_DL && cmd->words.w1 == (uintptr_t)gObjGrass_D_809AAAE0) {
                    assert(tinted == (selected == SEASON_AUTUMN ? 2 : 3));
                    ++nativeDraws;
                }
            }
            assert(groupedDrawCalls == 2 && nativeDraws == 1 && !tinted);
        }
    }
    replaceFirstDraw = 0;
    puts("PASS Road mixed randomizer/native grass batches: leaf tint returns before the next OPA/XLU element");
}

int main(void) {
    play.state.gfxCtx = &gfx;
    MixedGroupedGrass();
    CarriedGrass();
    Spring();
    Winter();
    Kusa();
    GroupedGrass();
}
