// Production selection rules: arrows gate independently, swords retain separate
// saved choices, invalid inputs cannot mutate them, and enchantments use no magic.
#include "global.h"
#include "mods/nei_save.h"
#ifdef SWORD_ELEMENT_TEST_COMBAT
#include "din_fire_sword.h"
#include "soh/ResourceManagerHelpers.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int SwordElement_SlotForItem(uint16_t item) __attribute__((weak));
extern uint8_t SwordElement_IsAvailable(uint8_t element) __attribute__((weak));
extern uint8_t SwordElement_Get(uint16_t item) __attribute__((weak));
extern uint8_t SwordElement_HasSelection(uint16_t item) __attribute__((weak));
extern uint8_t SwordElement_Set(uint16_t item, uint8_t element) __attribute__((weak));
extern uint8_t SwordElement_ForItem(uint16_t item) __attribute__((weak));
extern uint8_t SwordElement_Active(PlayState* play, Player* player) __attribute__((weak));
extern uint8_t SwordElement_IsRegularSword(PlayState* play, Player* player) __attribute__((weak));

#define REQUIRE(c) do { if (!(c)) { fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #c); exit(1); } } while (0)
SaveContext gSaveContext;
static NeiSaveData nei;
static PlayState play;
static Player player;
static int enabled[4], kokiriLevel, trueMaster, fairy, transformed, otherOwner, bossOwner;
static int legacyEnabled, nativeAssets;

NeiSaveData* Nei_Save(void) { return &nei; }
int32_t CVarGetInteger(const char* key, int32_t fallback) {
    if (!strcmp(key, "gEnhancements.DinFireSwordDamage")) return enabled[1];
    if (!strcmp(key, "gEnhancements.IceSwordDamage")) return enabled[2];
    if (!strcmp(key, "gEnhancements.LightSwordDamage")) return enabled[3];
    if (!strcmp(key, "gEnhancements.DinFireSword")) return legacyEnabled;
    if (!strcmp(key, "gSettings.AltAssets")) return nativeAssets;
    return fallback;
}
u8 WeaponUpgrade_KokiriLevel(void) { return kokiriLevel; }
u8 WeaponUpgrade_HasTrueMaster(void) { return trueMaster; }
u8 WeaponUpgrade_HasGreatFairy(void) { return fairy; }
u8 ExtEquip_ShouldHideSwordDL(void) { return otherOwner; }
u8 FourSword_IsEquipped(void) { return otherOwner; }
s32 BossRemains_IsOdolwaWorn(void) { return bossOwner; }
s32 BossRemains_IsGohtWorn(void) { return 0; }
#ifdef SWORD_ELEMENT_TEST_MM
s32 CustomForms_ActiveForm(void) { return transformed; }
#define NO_CUSTOM_FORM -1
#define INVENTORY gSaveContext.save.saveInfo.inventory
#define MAGIC gSaveContext.save.saveInfo.playerData.magic
#else
u8 TransformMasks_IsTransformedAny(void) { return transformed; }
#define NO_CUSTOM_FORM 0
#define INVENTORY gSaveContext.inventory
#define MAGIC gSaveContext.magic
#endif

#ifdef SWORD_ELEMENT_TEST_COMBAT
uint8_t ResourceMgr_FileExists(const char* path) { return nativeAssets; }
Gfx* ResourceMgr_LoadGfxByName(const char* path) { static Gfx gfx[1]; return nativeAssets ? gfx : NULL; }
void* ResourceGetDataByName(const char* path) { static u8 data[1]; return nativeAssets ? data : NULL; }
#ifdef SWORD_ELEMENT_TEST_MM
bool ResourceMgr_IsAltAssetsEnabled(void) { return nativeAssets; }
s32 AdultLink_IsActive(void) { return 0; }
#define BASE_STRIKE DMG_SWORD
#define ICE_STRIKE DMG_ICE_ARROW
#define LIGHT_STRIKE DMG_LIGHT_ARROW
#define FIRE_STRIKE DMG_FIRE_ARROW
#define MELEE_FLAGS player.meleeWeaponQuads[0].elem.atDmgInfo.dmgFlags
#else
uint8_t GameInteractor_InvisibleLinkActive(void) { return 0; }
#define BASE_STRIKE DMG_SLASH_KOKIRI
#define ICE_STRIKE DMG_ARROW_ICE
#define LIGHT_STRIKE DMG_ARROW_LIGHT
#define FIRE_STRIKE DMG_ARROW_FIRE
#define MELEE_FLAGS player.meleeWeaponQuads[0].info.toucher.dmgFlags
#endif
#endif

int main(void) {
    REQUIRE(SwordElement_IsRegularSword != NULL);
    REQUIRE(SwordElement_Set != NULL); // Fails against the unmodified baseline.
    memset(&nei, 0, sizeof(nei));
    memset(&gSaveContext, 0, sizeof(gSaveContext));
    memset(&play, 0, sizeof(play));
    memset(&player, 0, sizeof(player));
    transformed = NO_CUSTOM_FORM;
    memset(INVENTORY.items, ITEM_NONE, sizeof(INVENTORY.items));
    MAGIC = 37;
    // Normal progression is available with every cheat checkbox off.
    for (int e = 1; e < 4; ++e) enabled[e] = 0;
    REQUIRE(SwordElement_IsAvailable(0));
    REQUIRE(!SwordElement_IsAvailable(1));
    REQUIRE(!SwordElement_IsAvailable(2));
    REQUIRE(!SwordElement_IsAvailable(3));
    REQUIRE(!SwordElement_Set(ITEM_SWORD_KOKIRI, 1));
    REQUIRE(!SwordElement_HasSelection(ITEM_SWORD_KOKIRI));
    INVENTORY.items[SLOT_ARROW_FIRE] = ITEM_ARROW_FIRE;
    REQUIRE(SwordElement_IsAvailable(1));
    REQUIRE(!SwordElement_IsAvailable(2));
    REQUIRE(SwordElement_Set(ITEM_SWORD_KOKIRI, 1));
    REQUIRE(SwordElement_Get(ITEM_SWORD_KOKIRI) == 1);
    REQUIRE(SwordElement_ForItem(ITEM_SWORD_KOKIRI) == 1);
    REQUIRE(SwordElement_Get(ITEM_SWORD_MASTER) == 0);
    INVENTORY.items[SLOT_ARROW_ICE] = ITEM_ARROW_ICE;
    INVENTORY.items[SLOT_ARROW_LIGHT] = ITEM_ARROW_LIGHT;
    REQUIRE(SwordElement_Set(ITEM_SWORD_MASTER, 2));
    REQUIRE(SwordElement_Get(ITEM_SWORD_KOKIRI) == 1);
    REQUIRE(SwordElement_Get(ITEM_SWORD_MASTER) == 2);
    REQUIRE(!SwordElement_Set(ITEM_BOW, 3));
    REQUIRE(!SwordElement_Set(ITEM_SWORD_MASTER, 4));
    REQUIRE(!SwordElement_Set(ITEM_SWORD_MASTER, 255));
    REQUIRE(SwordElement_Get(ITEM_SWORD_MASTER) == 2);
    REQUIRE(SwordElement_ForItem(ITEM_SWORD_MASTER) == 2);
    REQUIRE(SwordElement_Get(ITEM_SWORD_MASTER) == 2);
    INVENTORY.items[SLOT_ARROW_ICE] = ITEM_NONE;
    REQUIRE(SwordElement_ForItem(ITEM_SWORD_MASTER) == 0);
    INVENTORY.items[SLOT_ARROW_ICE] = ITEM_ARROW_ICE;
    REQUIRE(SwordElement_ForItem(ITEM_SWORD_MASTER) == 2);
    REQUIRE(SwordElement_Set(ITEM_SWORD_MASTER, 0));
    REQUIRE(SwordElement_HasSelection(ITEM_SWORD_MASTER));
    REQUIRE(SwordElement_ForItem(ITEM_SWORD_MASTER) == 0);
    REQUIRE(!SwordElement_HasSelection(ITEM_BOW));
    kokiriLevel = 1;
    REQUIRE(SwordElement_Get(ITEM_SWORD_KOKIRI) == 1);
    REQUIRE(SwordElement_Set(ITEM_SWORD_KOKIRI, 2));
    kokiriLevel = 2;
    REQUIRE(SwordElement_Get(ITEM_SWORD_KOKIRI) == 2);
    REQUIRE(SwordElement_Set(ITEM_SWORD_KOKIRI, 3));
    kokiriLevel = 1;
    REQUIRE(SwordElement_Get(ITEM_SWORD_KOKIRI) == 2);
    kokiriLevel = 0;
    REQUIRE(SwordElement_Get(ITEM_SWORD_KOKIRI) == 1);
    trueMaster = 1;
    REQUIRE(SwordElement_Set(ITEM_SWORD_MASTER, 3));
    trueMaster = 0;
    REQUIRE(SwordElement_Get(ITEM_SWORD_MASTER) == 0);
    fairy = 1;
    REQUIRE(SwordElement_Set(ITEM_SWORD_BGS, 3));
    fairy = 0;
    REQUIRE(SwordElement_Get(ITEM_SWORD_BGS) == 0);

#ifdef SWORD_ELEMENT_TEST_MM
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    player.transformation = PLAYER_FORM_HUMAN;
#else
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head = &player.actor;
#endif
    player.actor.scale.y = 1.0f;
    player.heldItemId = ITEM_SWORD_KOKIRI;
    player.heldItemAction = PLAYER_IA_SWORD_KOKIRI;
    REQUIRE(SwordElement_IsRegularSword(&play, &player));
    player.heldItemId = ITEM_SWORD_MASTER;
    player.heldItemAction = PLAYER_IA_SWORD_MASTER;
    REQUIRE(SwordElement_Active(&play, &player) == 0);
    REQUIRE(SwordElement_IsRegularSword(&play, &player));
    player.heldItemId = ITEM_SWORD_KOKIRI;
    player.heldItemAction = PLAYER_IA_SWORD_KOKIRI;
    REQUIRE(SwordElement_Active(&play, &player) == 1);
    // An override works before its arrow is acquired and cannot rewrite the
    // saved per-sword choice. Disabling it restores that choice immediately.
    for (int element = 1; element < 4; ++element) {
        const u8 slots[] = { 0, SLOT_ARROW_FIRE, SLOT_ARROW_ICE, SLOT_ARROW_LIGHT };
        const u8 arrows[] = { ITEM_NONE, ITEM_ARROW_FIRE, ITEM_ARROW_ICE, ITEM_ARROW_LIGHT };
        INVENTORY.items[slots[element]] = ITEM_NONE;
        enabled[element] = 1;
        REQUIRE(!SwordElement_IsAvailable(element));
        REQUIRE(SwordElement_Active(&play, &player) == element);
        REQUIRE(SwordElement_Get(ITEM_SWORD_KOKIRI) == 1);
        REQUIRE(SwordElement_ForItem(ITEM_BOW) == 0);
        enabled[element] = 0;
        INVENTORY.items[slots[element]] = arrows[element];
        REQUIRE(SwordElement_Active(&play, &player) == 1);
    }
    Player remote = player;
    REQUIRE(!SwordElement_IsRegularSword(&play, &remote));
    REQUIRE(SwordElement_Active(&play, &remote) == 0);
    transformed = 1;
    REQUIRE(!SwordElement_IsRegularSword(&play, &player));
    REQUIRE(SwordElement_Active(&play, &player) == 0);
    transformed = NO_CUSTOM_FORM;
    otherOwner = 1;
    REQUIRE(SwordElement_Active(&play, &player) == 0);
    otherOwner = 0;
    bossOwner = 1;
    REQUIRE(SwordElement_Active(&play, &player) == 0);
    bossOwner = 0;
    player.heldItemId = ITEM_BOW;
    REQUIRE(!SwordElement_IsRegularSword(&play, &player));
    REQUIRE(SwordElement_Active(&play, &player) == 0);
    REQUIRE(MAGIC == 37);
#ifdef SWORD_ELEMENT_TEST_COMBAT
    player.heldItemId = ITEM_SWORD_KOKIRI;
    REQUIRE(DinFireSword_DamageFlags(&play, &player, BASE_STRIKE) == (BASE_STRIKE | FIRE_STRIKE));
    REQUIRE(SwordElement_Set(ITEM_SWORD_KOKIRI, 2));
    REQUIRE(DinFireSword_DamageFlags(&play, &player, BASE_STRIKE) == (BASE_STRIKE | ICE_STRIKE));
    MELEE_FLAGS = DinFireSword_SetDamageFlags(&play, &player, 0, BASE_STRIKE);
    REQUIRE(SwordElement_Set(ITEM_SWORD_KOKIRI, 3));
    DinFireSword_RefreshDamage(&play, &player);
    REQUIRE(MELEE_FLAGS == (BASE_STRIKE | LIGHT_STRIKE));
    enabled[3] = 0;
    DinFireSword_RefreshDamage(&play, &player);
    REQUIRE(MELEE_FLAGS == (BASE_STRIKE | LIGHT_STRIKE));
    INVENTORY.items[SLOT_ARROW_LIGHT] = ITEM_NONE;
    DinFireSword_RefreshDamage(&play, &player);
    REQUIRE(MELEE_FLAGS == BASE_STRIKE);
    INVENTORY.items[SLOT_ARROW_LIGHT] = ITEM_ARROW_LIGHT;
    REQUIRE(SwordElement_Set(ITEM_SWORD_KOKIRI, 0));
    REQUIRE(DinFireSword_DamageFlags(&play, &player, BASE_STRIKE) == BASE_STRIKE);
    // Cheat forcing is independent of arrow acquisition, Alt Assets and the
    // legacy Din appearance. Explicit None remains a saved normal choice.
    memset(&nei, 0, sizeof(nei));
    legacyEnabled = nativeAssets = 0;
#ifdef SWORD_ELEMENT_TEST_MM
    player.leftHandType = PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;
#else
    gSaveContext.linkAge = 1;
    player.leftHandType = PLAYER_MODELTYPE_LH_SWORD;
#endif
    INVENTORY.items[SLOT_ARROW_FIRE] = ITEM_NONE;
    enabled[1] = 1;
    REQUIRE(DinFireSword_DamageFlags(&play, &player, BASE_STRIKE) == (BASE_STRIKE | FIRE_STRIKE));
    REQUIRE(SwordElement_Get(ITEM_SWORD_KOKIRI) == 0);
    enabled[1] = 0;
    REQUIRE(DinFireSword_DamageFlags(&play, &player, BASE_STRIKE) == BASE_STRIKE);
    INVENTORY.items[SLOT_ARROW_FIRE] = ITEM_ARROW_FIRE;
    REQUIRE(DinFireSword_DamageFlags(&play, &player, BASE_STRIKE) == BASE_STRIKE);
    REQUIRE(SwordElement_Set(ITEM_SWORD_KOKIRI, 0));
    enabled[1] = 1;
    REQUIRE(DinFireSword_DamageFlags(&play, &player, BASE_STRIKE) == (BASE_STRIKE | FIRE_STRIKE));
    enabled[1] = 0;
    REQUIRE(DinFireSword_DamageFlags(&play, &player, BASE_STRIKE) == BASE_STRIKE);
    REQUIRE(MAGIC == 37);
    puts("PASS elemental sword core/combat: arrow unlock independent of options, asset-free cheats, saved None, unchanged sword flags, no magic");
#endif
    puts("PASS elemental sword state: arrow gating, independent swords/upgrades, no stacking, reversibility, owner isolation, zero magic cost");
    return 0;
}
