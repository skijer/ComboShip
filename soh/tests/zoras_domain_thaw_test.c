#include "global.h"
#include "mods/extended_inventory.h"
#include "overlays/actors/ovl_Bg_Spot07_Taki/z_bg_spot07_taki.h"
#include "scenes/overworld/spot07/spot07_scene.h"
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

SaveContext gSaveContext;
PlayState* gPlayState;
int zoraWaterLook;
static int alwaysThawed, legacyIce, registrations, deletions, alive[2], causticY;
static PlayState play;
static GraphicsContext gfx;
static Gfx opaque[64], translucent[64], scroll[16];
static Mtx matrix;
static CollisionHeader nativeCollision;
static WaterBox waterBoxes[2];
static SaveContext saveBefore;
static NeiSaveData nei, neiBefore;
static const void* registeredHeader;
const void* sZorasDomainEntranceTextures[] = { gZorasDomainDayEntranceTex, gZorasDomainNightEntranceTex };

int32_t CVarGetInteger(const char* name, int32_t fallback) {
    if (!strcmp(name, CVAR_ENHANCEMENT("AlwaysThawedZorasDomain"))) {
        return alwaysThawed;
    }
    if (!strcmp(name, CVAR_ENHANCEMENT("ZorasDomainIce"))) {
        return legacyIce;
    }
    return fallback;
}
NeiSaveData* Nei_Save(void) {
    return &nei;
}
u16 Nei_GetOwnedItem(u8 slot) {
    return ITEM_NONE;
}
u8 Seasons_RandoMode(void) {
    return NEI_SEASONS_INDIVIDUAL;
}
void Seasons_UpdateGates(void) {
}
void Actor_ProcessInitChain(Actor* actor, InitChainEntry* chain) {
    actor->scale.x = actor->scale.y = actor->scale.z = 0.1f;
}
void CollisionHeader_GetVirtual(void* resource, CollisionHeader** result) {
    registeredHeader = resource;
    *result = &nativeCollision;
}
s32 DynaPoly_SetBgActor(PlayState* current, DynaCollisionContext* dyna, Actor* actor, CollisionHeader* header) {
    REQUIRE(current == &play && dyna == &play.colCtx.dyna && header == &nativeCollision);
    REQUIRE(actor->params == 0 || actor->params == 1);
    REQUIRE(!alive[actor->params]);
    alive[actor->params] = 1;
    ++registrations;
    return actor->params;
}
void DynaPoly_DeleteBgActor(PlayState* current, DynaCollisionContext* dyna, s32 id) {
    REQUIRE(current == &play && dyna == &play.colCtx.dyna);
    if (id == BGACTOR_NEG_ONE || id == BG_ACTOR_MAX) {
        return;
    }
    REQUIRE(id >= 0 && id < 2 && alive[id]);
    alive[id] = 0;
    ++deletions;
}
void FrameInterpolation_RecordOpenChild(const void* child, int line) {
}
void FrameInterpolation_RecordCloseChild(void) {
}
void Gfx_SetupDL_25Opa(GraphicsContext* current) {
    REQUIRE(current == &gfx);
}
void Gfx_SetupDL_25Xlu(GraphicsContext* current) {
    REQUIRE(current == &gfx);
}
Mtx* Matrix_NewMtx(GraphicsContext* current, char* file, s32 line) {
    REQUIRE(current == &gfx);
    return &matrix;
}
void gSPDisplayList(Gfx* command, Gfx* list) {
    __gSPDisplayList(command, list);
}
void gSPSegment(void* command, int segment, uintptr_t base) {
    __gSPSegment((Gfx*)command, segment, base);
}
Gfx* Gfx_TwoTexScrollEx(GraphicsContext* current, s32 tile1, u32 x1, u32 y1, s32 width1, s32 height1, s32 tile2, u32 x2,
                        u32 y2, s32 width2, s32 height2, s32 dx1, s32 dy1, s32 dx2, s32 dy2) {
    REQUIRE(current == &gfx);
    if (width1 == 64 && height1 == 32 && width2 == 64 && height2 == 32) {
        causticY = y2;
    }
    return scroll;
}

#include "zoras_domain_actor.inc"
#include "zoras_domain_native.inc"

void SeasonScene_UpdateWater(PlayState* current);

static void Configure(int adult, int season, int thawed, int legacy) {
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    memset(&nei, 0, sizeof(nei));
    gSaveContext.linkAge = adult ? LINK_AGE_ADULT : LINK_AGE_CHILD;
    Nei_Save()->season = season < 0 ? SEASON_OFF : season;
    Nei_Save()->seasonsOwned = season < 0 ? 0 : NEI_SEASONS_MASK;
    Nei_Save()->seasonsRodOwned = season >= 0;
    gSaveContext.eventChkInf[3] = 0xA55A;
    gSaveContext.infTable[7] = 0x5AA5;
    alwaysThawed = thawed;
    legacyIce = legacy;
    registrations = deletions = 0;
    REQUIRE(!alive[0] && !alive[1]);
    play.sceneNum = SCENE_ZORAS_DOMAIN;
    saveBefore = gSaveContext;
    neiBefore = nei;
}

static int HasList(Gfx* begin, Gfx* end, const void* list) {
    for (; begin < end; ++begin) {
        if ((begin->words.w0 >> 24) == G_DL && begin->words.w1 == (uintptr_t)list) {
            return 1;
        }
    }
    return 0;
}

static void CheckDraw(BgSpot07Taki* actor, int frozen) {
    gfx.polyOpa.p = opaque;
    gfx.polyXlu.p = translucent;
    BgSpot07Taki_Draw(&actor->dyna.actor, &play);
    const int type = actor->dyna.actor.params;
    const void* water = type ? object_spot07_object_DL_000BE0 : object_spot07_object_DL_000460;
    const void* ice = type ? object_spot07_object_DL_003210 : object_spot07_object_DL_001CF0;
    REQUIRE(HasList(translucent, gfx.polyXlu.p, water) == !frozen);
    REQUIRE(HasList(opaque, gfx.polyOpa.p, ice) == frozen);
    REQUIRE(alive[type] == frozen);
    REQUIRE(!memcmp(&gSaveContext, &saveBefore, sizeof(gSaveContext)));
    REQUIRE(!memcmp(&nei, &neiBefore, sizeof(nei)));
}

static void ActorMatrix(void) {
    for (int adult = 0; adult <= 1; ++adult) {
        for (int season = -1; season <= SEASON_OFF; ++season) {
            for (int thawed = 1; thawed >= 0; --thawed) {
                for (int legacy = 0; legacy <= 2; ++legacy) {
                    Configure(adult, season, thawed, legacy);
                    for (int type = 0; type < 2; ++type) {
                        BgSpot07Taki actor = { 0 };
                        actor.dyna.actor.params = type;
                        int frozen = thawed                    ? 0
                                     : season == SEASON_WINTER ? 1
                                     : season == SEASON_SUMMER ? 0
                                     : legacy == 1             ? 1
                                     : legacy == 2             ? 0
                                                               : adult;
                        BgSpot07Taki_Init(&actor.dyna.actor, &play);
                        CheckDraw(&actor, frozen);
                        if (frozen) {
                            REQUIRE(registeredHeader == (const void*)(type ? object_spot07_object_Col_0038FC
                                                                           : object_spot07_object_Col_002590));
                        }
                        int before = registrations + deletions;
                        for (int frame = 0; frame < 5; ++frame) {
                            BgSpot07Taki_Update(&actor.dyna.actor, &play);
                            CheckDraw(&actor, frozen);
                        }
                        REQUIRE(registrations + deletions == before);
                        BgSpot07Taki_Destroy(&actor.dyna.actor, &play);
                    }
                    REQUIRE(!memcmp(&gSaveContext, &saveBefore, sizeof(gSaveContext)));
                    REQUIRE(!memcmp(&nei, &neiBefore, sizeof(nei)));
                    REQUIRE(registrations == deletions);
                }
            }
        }
    }
}

static void LiveToggle(void) {
    Configure(1, SEASON_WINTER, 0, 0);
    BgSpot07Taki actors[2] = { 0 };
    for (int type = 0; type < 2; ++type) {
        actors[type].dyna.actor.params = type;
        BgSpot07Taki_Init(&actors[type].dyna.actor, &play);
    }
    for (int cycle = 0; cycle < 10; ++cycle) {
        for (int thawed = 1; thawed >= 0; --thawed) {
            alwaysThawed = thawed;
            for (int type = 0; type < 2; ++type) {
                BgSpot07Taki_Update(&actors[type].dyna.actor, &play);
                CheckDraw(&actors[type], !thawed);
            }
        }
    }
    for (int type = 0; type < 2; ++type) {
        BgSpot07Taki_Destroy(&actors[type].dyna.actor, &play);
    }
    REQUIRE(registrations == deletions);
}

static void WaterAndCaustics(void) {
    for (int thawed = 0; thawed <= 1; ++thawed) {
        for (int season = -1; season <= SEASON_OFF; ++season) {
            Configure(1, season, thawed, 0);
            waterBoxes[0].ySurface = 100;
            waterBoxes[1].ySurface = 877;
            nativeCollision.waterBoxes = waterBoxes;
            nativeCollision.numWaterBoxes = 2;
            play.colCtx.colHeader = &nativeCollision;
            SeasonScene_UpdateWater(&play);
            const int drained = !thawed && season == SEASON_SUMMER;
            REQUIRE(waterBoxes[0].ySurface == (drained ? -32000 : 100));
            REQUIRE(waterBoxes[1].ySurface == (drained ? -32000 : 877));
            REQUIRE(Seasons_WalksOnWater() == (!thawed && season == SEASON_WINTER));
            REQUIRE(zoraWaterLook == (thawed ? 0 : season == SEASON_WINTER ? 1 : season == SEASON_SUMMER ? 2 : 0));
            // Leave the Domain: normal seasonal physics and resources must resume.
            play.sceneNum = SCENE_LAKE_HYLIA;
            SeasonScene_UpdateWater(&play);
            REQUIRE(Seasons_WalksOnWater() == (season == SEASON_WINTER));
            REQUIRE(zoraWaterLook == (season == SEASON_WINTER ? 1 : season == SEASON_SUMMER ? 2 : 0));
            // Return: undo cached suppression even when it came from the same collision header.
            play.sceneNum = SCENE_ZORAS_DOMAIN;
            SeasonScene_UpdateWater(&play);
            REQUIRE(waterBoxes[0].ySurface == (drained ? -32000 : 100));
            REQUIRE(!memcmp(&gSaveContext, &saveBefore, sizeof(gSaveContext)));
            REQUIRE(!memcmp(&nei, &neiBefore, sizeof(nei)));
            Nei_Save()->season = SEASON_OFF;
            SeasonScene_UpdateWater(&play);
            REQUIRE(waterBoxes[0].ySurface == 100 && waterBoxes[1].ySurface == 877);
        }
    }
    Configure(1, -1, 0, 0);
    for (int frame = 0; frame < 300; frame += 23) {
        play.gameplayFrames = frame;
        for (int adult = 0; adult <= 1; ++adult) {
            gSaveContext.linkAge = adult ? LINK_AGE_ADULT : LINK_AGE_CHILD;
            for (int thawed = 0; thawed <= 1; ++thawed) {
                alwaysThawed = thawed;
                gfx.polyOpa.p = opaque;
                gfx.polyXlu.p = translucent;
                func_8009E730(&play);
                REQUIRE(causticY == (adult && !thawed ? 0 : 127 - frame % 128));
            }
        }
    }
}

int main(void) {
    gPlayState = &play;
    play.state.gfxCtx = &gfx;
    ActorMatrix();
    LiveToggle();
    WaterAndCaustics();
    puts("PASS age/season/legacy matrix, native water display lists, collision toggles, save immutability, caustics");
    return 0;
}
