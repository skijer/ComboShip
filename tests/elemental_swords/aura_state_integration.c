// Real selection core and real deferred aura renderer in one executable.
// Rename only the old fixture's selector/config boundaries before including it.
#define SwordElement_Get AuraBoundaryGet
#define SwordElement_HasSelection AuraBoundaryHasSelection
#define SwordElement_Active AuraBoundaryActive
#define SwordElement_IsAvailable AuraBoundaryAvailable
#define SwordElement_Override AuraBoundaryOverride
#define CVarGetInteger AuraBoundaryCVar
#ifdef SWORD_AURA_TEST_MM
#include "../../mm/tests/din_fire_fixture.h"
#define AURA_INVENTORY gSaveContext.save.saveInfo.inventory
#define AURA_MAGIC gSaveContext.save.saveInfo.playerData.magic
#else
#define main LegacyAuraRenderMain
#include "../../soh/tests/din_fire_sword_test.c"
#undef main
#include "mods/nei_save.h"
static NeiSaveData nei;
NeiSaveData* Nei_Save(void) { return &nei; }
#define AURA_INVENTORY gSaveContext.inventory
#define AURA_MAGIC gSaveContext.magic
#endif
#undef SwordElement_Get
#undef SwordElement_HasSelection
#undef SwordElement_Active
#undef SwordElement_IsAvailable
#undef SwordElement_Override
#undef CVarGetInteger
#include "sword_element.h"

static int gate[4];
int32_t CVarGetInteger(const char* key, int32_t fallback) {
    static const char* keys[] = { "", "gEnhancements.DinFireSwordDamage", "gEnhancements.IceSwordDamage",
                                  "gEnhancements.LightSwordDamage" };
    for (int i = 1; i < 4; ++i) if (!strcmp(key, keys[i])) return gate[i];
    return AuraBoundaryCVar(key, fallback);
}
static void CoreAuraSetup(void) {
#ifdef SWORD_AURA_TEST_MM
    Setup(); swordOption = alt = assets = 0;
#else
    setup(); enabled = alt = assets = 0;
    player.leftHandType = PLAYER_MODELTYPE_LH_SWORD;
#endif
    memset(&nei, 0, sizeof(nei));
    memset(gate, 0, sizeof(gate));
    memset(AURA_INVENTORY.items, ITEM_NONE, sizeof(AURA_INVENTORY.items));
    player.heldItemId = ITEM_SWORD_KOKIRI;
    player.itemAction = player.heldItemAction = PLAYER_IA_SWORD_KOKIRI;
    AURA_MAGIC = 37;
    resourceQueries = 0;
}
static size_t CoreAuraDraw(void) {
    ++play.gameplayFrames;
    DinFireSword_Update(&play, &player);
#ifdef SWORD_AURA_TEST_MM
    ResetBuffers();
#else
    gfx.polyOpa.p = opa; gfx.polyXlu.p = xlu; matrices = 0;
#endif
    Player before = player;
    SaveContext save = gSaveContext;
    DinFireSword_BeginPlayerDraw(&play, &player);
    DinFireSword_Draw(&play, &player);
    DinFireSword_DrawAfterPlayer(&play, &player);
    REQUIRE(!memcmp(&before, &player, sizeof(player)) && !memcmp(&save, &gSaveContext, sizeof(save)));
    REQUIRE(resourceQueries == 0 && AURA_MAGIC == 37);
    return (gfx.polyOpa.p - opa) + (gfx.polyXlu.p - xlu);
}
int main(void) {
    const u8 slots[] = { 0, SLOT_ARROW_FIRE, SLOT_ARROW_ICE, SLOT_ARROW_LIGHT };
    const u8 arrows[] = { ITEM_NONE, ITEM_ARROW_FIRE, ITEM_ARROW_ICE, ITEM_ARROW_LIGHT };
    for (int element = 1; element < 4; ++element) {
        CoreAuraSetup();
        REQUIRE(!SwordElement_Set(ITEM_SWORD_KOKIRI, element));
        REQUIRE(CoreAuraDraw() == 0);
        AURA_INVENTORY.items[slots[element]] = arrows[element];
        REQUIRE(SwordElement_Set(ITEM_SWORD_KOKIRI, element));
        REQUIRE(CoreAuraDraw() > 0);
        // The arrow grants a normal imbue with the cheat switch off.
        REQUIRE(CoreAuraDraw() > 0);
        AURA_INVENTORY.items[slots[element]] = ITEM_NONE;
        REQUIRE(CoreAuraDraw() == 0);
        REQUIRE(SwordElement_Get(ITEM_SWORD_KOKIRI) == element);
        gate[element] = 1;
        REQUIRE(CoreAuraDraw() > 0);
        gate[element] = 0;
        REQUIRE(CoreAuraDraw() == 0);
        AURA_INVENTORY.items[slots[element]] = arrows[element];
        REQUIRE(CoreAuraDraw() > 0);
        REQUIRE(SwordElement_Set(ITEM_SWORD_KOKIRI, SWORD_ELEMENT_NONE));
#ifdef SWORD_AURA_TEST_MM
        swordOption = alt = assets = 1;
#else
        enabled = alt = assets = 1;
#endif
        REQUIRE(CoreAuraDraw() == 0);
        gate[element] = 1;
        REQUIRE(CoreAuraDraw() > 0);
        REQUIRE(SwordElement_Get(ITEM_SWORD_KOKIRI) == SWORD_ELEMENT_NONE);
    }
    puts("PASS real selector/aura: independent arrow unlocks and cheat overrides, saved None, no pack/Alt dependency or magic cost");
    return 0;
}
