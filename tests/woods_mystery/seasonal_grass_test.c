// Run actual actor draw bodies through the established matrix/light/GPU seams.
#define main ExistingGrassTests
#include "autumn_grass_test.c"
#undef main

static void WoodsActors(void) {
    EnKusa grass = { 0 };
    ObjGrass grouped = { 0 };
    Actor carried = { 0 };
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
        { ObjGrassCarry_Draw, &carried, gKusaBushType1DL, 0 },
    };
    play.sceneId = SCENE_26SARUNOMORI;
    for (unsigned i = 0; i < ARRAY_COUNT(cases); ++i) {
        grass.actor.projectedPos.z = cases[i].fade ? 1250 : 500;
        grouped.grassGroups[0].elements[0].alpha = cases[i].fade ? 96 : 255;
        season = SEASON_OFF;
        Begin();
        cases[i].draw(cases[i].actor, &play);
        const Gfx* start = cases[i].fade ? translucent : opaque;
        size_t count = (cases[i].fade ? gfx.polyXlu.p : gfx.polyOpa.p) - start;
        Gfx native[128];
        memcpy(native, start, count * sizeof(Gfx));
        for (int room = 0; room < 9; ++room) {
            play.roomCtx.curRoom.num = room;
            for (int day = 1; day <= 3; ++day) {
                gSaveContext.save.day = day;
                for (int selected = SEASON_SPRING; selected <= SEASON_WINTER; ++selected) {
                    season = selected;
                    Begin();
                    cases[i].draw(cases[i].actor, &play);
                    start = cases[i].fade ? translucent : opaque;
                    const Gfx* end = cases[i].fade ? gfx.polyXlu.p : gfx.polyOpa.p;
                    if (selected == SEASON_SUMMER) {
                        assert((size_t)(end - start) == count && !memcmp(start, native, count * sizeof(Gfx)));
                    } else {
                        Check(start, end, cases[i].list, native, count,
                              selected == SEASON_SPRING ? 0xF6C1D9FF : selected == SEASON_WINTER ? 0xE5EDF3FF : 0xD99C45FF,
                              selected == SEASON_AUTUMN ? 1 : 3);
                    }
                }
            }
        }
        eligible = 0;
        Begin();
        cases[i].draw(cases[i].actor, &play);
        start = cases[i].fade ? translucent : opaque;
        assert(!memcmp(start, native, count * sizeof(Gfx)));
        eligible = 1;
    }
    grass.isCut = 1;
    Begin();
    EnKusa_DrawGrass(&grass.actor, &play);
    assert(gfx.polyOpa.p - opaque == 1 && opaque[0].words.w1 == (uintptr_t)gKusaStumpDL);
    grass.isCut = 0; // The same draw is used as the grass regrows.
    Begin();
    EnKusa_DrawGrass(&grass.actor, &play);
    assert(gfx.polyOpa.p - opaque > 1);
    puts("PASS Woods standing/bush/grouped/faded/carried/regrowing grass: all rooms/days, three palettes, exact fade/stump/native cleanup");
}

static void WoodsMixedGroupedGrass(void) {
    ObjGrass grass = { 0 };
    grass.activeGrassGroups = 1;
    grass.grassGroups[0].count = 2;
    grass.grassGroups[0].flags = OBJ_GRASS_GROUP_DRAW;
    play.sceneId = SCENE_26SARUNOMORI;
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
            int tinted = 0, nativeDraws = 0;
            for (const Gfx* cmd = fade ? translucent : opaque; cmd < (fade ? gfx.polyXlu.p : gfx.polyOpa.p); ++cmd) {
                if (cmd->words.w0 >> 24 == G_SETGRAYSCALE)
                    tinted = cmd->words.w1;
                if (cmd->words.w0 >> 24 == G_DL && cmd->words.w1 == (uintptr_t)gObjGrass_D_809AAAE0) {
                    assert(tinted == (selected == SEASON_AUTUMN ? 1 : 3));
                    ++nativeDraws;
                }
            }
            assert(groupedDrawCalls == 2 && nativeDraws == 1 && !tinted);
        }
    }
    replaceFirstDraw = 0;
    puts("PASS Woods mixed replacement/native grass batches: seasonal state resumes for the next OPA/XLU element");
}

SaveContext gSaveContext;
int main(void) {
    play.state.gfxCtx = &gfx;
    WoodsMixedGroupedGrass();
    WoodsActors();
    return 0;
}
