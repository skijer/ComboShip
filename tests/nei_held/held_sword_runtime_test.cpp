#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
#include <string>
#include "mods/items/logic/weapon_upgrades.h"
#include "mods/extended_equipment.h"
#include "../../combo/render/sword_element_aura_profiles.h"
#ifdef NEI_EQUIPMENT_MM
#include "mods/forms/custom_forms.h"
#endif
#include "../../combo/NeiHeldSword.h"

static std::set<std::string> local, donor, shipped, alternate, localMods, foreignMods;
static bool altOn = false, bodySelected = false, fourEquipped = false, adult = false;
static bool adultMode = false, shippedOwnerReady = true;
static int vanillaHeldSwords = -1; // -1 leaves the CVar absent, exercising the production default.
static u8 kokiriLevel = 0, trueMaster = 0, greatFairy = 0;
static bool gildedLook = true, fairyLook = true;
static Gfx hand[1]{}, nativeBlade[1]{}, nativeHilt[1]{}, original[1]{};
static int registeredAura = -1;
static void* registeredHand = nullptr;
extern "C" {
void DinFireSword_SetHandModel(Player*, void* handDL, int model) { registeredHand = handDL; registeredAura = model; }
void DinFireSword_CopyHandModel(Player*, void* sourceDL, void* destinationDL) {
    if (registeredHand == sourceDL) registeredHand = destinationDL;
}
SaveContext gSaveContext{};
u8 ResourceMgr_FileExists(const char* p) { return local.contains(p); }
u8 ResourceMgr_FileAltExists(const char* p) { return alternate.contains(p); }
bool ResourceMgr_IsAltAssetsEnabled() { return altOn; }
int ResourceMgr_IsModAsset(const char* p) { return localMods.contains(p) || (altOn && alternate.contains(p)); }
int ResourceMgr_IsModAssetForGame(const char*, const char* p) { return foreignMods.contains(p); }
int32_t OOT_NeiEnsureGiBaseOwner() { return shippedOwnerReady; }
int32_t OOT_NeiResourceExists(const char* p) {
    const char* prefix = "__OTR__@oot-gi-base:";
    if (std::string(p).starts_with(prefix))
        return shippedOwnerReady && shipped.contains(std::string("__OTR__") + (p + std::strlen(prefix)));
    return donor.contains(p);
}
#ifdef NEI_EQUIPMENT_MM
u16 gEquipMasks[] = {0x000f, 0x00f0, 0x0f00, 0xf000};
u8 gEquipShifts[] = {0, 4, 8, 12};
Gfx* gPlayerLeftHandClosedDLs[2 * PLAYER_FORM_MAX]{};
s32 CustomForms_ActiveForm() { return CUSTOM_FORM_NONE; }
s32 BossRemains_IsOdolwaWorn() { return 0; }
s32 BossRemains_IsGohtWorn() { return 0; }
u8 Trident_GoldenArmor() { return 0; }
Gfx* ResourceMgr_LoadGfxByName(const char*) { return hand; }
s32 AdultLink_IsActive() { return adult || adultMode; }
s32 AdultLink_UsesAdultPresentation(const Player*) { return adult; }
u8 Player_IsCustomLinkModel(Player*) { return bodySelected; }
#else
u8 Player_IsCustomLinkModel() { return bodySelected; }
#endif
void gSPDisplayList(Gfx* command, Gfx* display) { __gSPDisplayList(command, display); }
s32 CVarGetInteger(const char* name, s32 fallback) {
    if (!std::strcmp(name, "gEnhancements.SkijerNEI.UseVanillaHeldSwords"))
        return vanillaHeldSwords < 0 ? fallback : vanillaHeldSwords;
    if (std::strstr(name, "GildedUsesGildedLook")) return gildedLook;
    if (std::strstr(name, "BgsUsesGfsLook")) return fairyLook;
    return fallback;
}
u8 WeaponUpgrade_KokiriLevel() { return kokiriLevel; }
u8 WeaponUpgrade_HasGilded() { return kokiriLevel == 2; }
u8 WeaponUpgrade_HasTrueMaster() { return trueMaster; }
u8 WeaponUpgrade_HasGreatFairy() { return greatFairy; }
u8 FourSword_IsEquipped() { return fourEquipped; }
void* MmAssets_LoadResource(const char* p) {
    if (!local.contains(p)) return nullptr;
    return std::strstr(p, "Handle") ? nativeHilt : nativeBlade;
}
u8 FourSword_HeldSwordDL(void** blade, void** hilt) {
    if (!fourEquipped) return 0;
#ifdef NEI_EQUIPMENT_MM
    const int frame = NEI_HELD_SWORD_MM_HUMAN;
#else
    const int frame = gSaveContext.linkAge;
#endif
    *blade = NeiHeldSword_ModelDL(NEI_HELD_SWORD_FOUR, frame);
    *hilt = nullptr;
    return *blade != nullptr;
}
#ifdef NEI_EQUIPMENT_MM
u8 FourSword_HeldSwordDLForFrame(void** blade, void** hilt, int frame) {
    if (!fourEquipped) return 0;
    *blade = NeiHeldSword_ModelDL(NEI_HELD_SWORD_FOUR, frame);
    *hilt = nullptr;
    return *blade != nullptr;
}
#endif
}
#include "held_sword_bindings.inc"
#ifdef NEI_EQUIPMENT_MM
static int sPlayerLod = 0, nativeAllocations = 0;
static Gfx nativeFrameCompound[8];
static Gfx* nativeAllocate(size_t size) {
    assert(size == sizeof(nativeFrameCompound) && nativeAllocations++ == 0);
    return nativeFrameCompound;
}
#undef GRAPH_ALLOC
#define GRAPH_ALLOC(context, size) nativeAllocate(size)
#include "held_sword_native_bindings.inc"
#endif

static int frame() {
#ifdef NEI_EQUIPMENT_MM
    return adult ? NEI_HELD_SWORD_OOT_ADULT : NEI_HELD_SWORD_MM_HUMAN;
#else
    return gSaveContext.linkAge;
#endif
}
static void allResources() {
    local.clear(); donor.clear(); alternate.clear(); localMods.clear(); foreignMods.clear();
    for (int sword = 0; sword < NEI_HELD_SWORD_COUNT; ++sword) {
        for (const char* const* p = sNeiHeldSwordResources[sword]; *p; ++p) local.insert(*p);
        for (int f = 0; f < NEI_HELD_SWORD_FRAME_COUNT; ++f)
            for (const char* p : sNeiHeldSwordPaths[sword][f]) local.insert(p);
    }
    shipped = donor = local;
    altOn = bodySelected = fourEquipped = adult = false;
    adultMode = false;
    vanillaHeldSwords = -1;
    shippedOwnerReady = true;
    kokiriLevel = trueMaster = greatFairy = 0;
    gildedLook = fairyLook = true;
    gSaveContext = {};
}
static void swordHand(Player& player, int action, int item) {
    player = {};
    player.actor.scale.y = .01f;
    player.heldItemAction = static_cast<decltype(player.heldItemAction)>(action);
    player.heldItemId = item;
#ifdef NEI_EQUIPMENT_MM
    player.transformation = PLAYER_FORM_HUMAN;
    player.leftHandType = action == PLAYER_IA_SWORD_BIGGORON || action == PLAYER_IA_SWORD_TWO_HANDED
                             ? PLAYER_MODELTYPE_LH_TWO_HAND_SWORD
                             : PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;
#else
    player.leftHandType = action == PLAYER_IA_SWORD_BIGGORON ? PLAYER_MODELTYPE_LH_BGS : PLAYER_MODELTYPE_LH_SWORD;
    gSaveContext.swordHealth = 8.f;
    gSaveContext.bgsFlag = 1;
#endif
}
static void expectAuthored(Player& player, int sword) {
    const Player before = player;
    Gfx* selected = original;
    assert(WeaponUpgrade_ApplyHeldSwordDL(&selected, hand, &player, 12, 34, 56) &&
           "Vanilla sword still lacks its redesigned progressive held model");
    assert(std::memcmp(&player, &before, sizeof(player)) == 0);
    assert(selected != original && (selected[0].words.w0 >> 24) == G_DL);
    const int aura = sword == NEI_HELD_SWORD_FOUR ? -1
                     : sword == NEI_HELD_SWORD_GIANTS_KNIFE
                         ? SWORD_AURA_GIANTS_KNIFE
                     : sword == NEI_HELD_SWORD_BROKEN_GIANTS_KNIFE
                         ? SWORD_AURA_AUTHORED_BROKEN_GIANTS_KNIFE
                         : sword;
    assert(registeredHand == selected && registeredAura == aura);
    const Gfx* mesh = reinterpret_cast<const Gfx*>(selected[0].words.w1);
    assert((mesh[0].words.w0 >> 24) == G_DL_OTR_FILEPATH);
    std::string want = sNeiHeldSwordPaths[sword][frame()][0];
    if (!altOn) want = std::string("__OTR__@oot-gi-base:") + want.substr(7);
#ifdef NEI_EQUIPMENT_MM
    else want = std::string("__OTR__@oot:") + want.substr(7);
#endif
    assert(std::string(reinterpret_cast<const char*>(mesh[0].words.w1)) == want);
    assert(selected[1].words.w1 == reinterpret_cast<uintptr_t>(hand));
    assert((selected[2].words.w0 >> 24) == G_RDPPIPESYNC);
    assert((selected[3].words.w0 >> 24) == G_SETENVCOLOR && selected[3].words.w1 == 0x0c223800);
    assert((selected[5].words.w0 >> 24) == G_ENDDL);
}
static void expectUntouched(Player& player) {
    const Player before = player;
    Gfx* selected = original;
    const auto applied = WeaponUpgrade_ApplyHeldSwordDL(&selected, hand, &player, 12, 34, 56);
    if (applied) {
        std::cerr << "Unexpected sword override: item=" << int(player.heldItemId)
                  << " action=" << int(player.heldItemAction) << " four=" << fourEquipped
                  << " Alt=" << altOn << " local mods=" << localMods.size()
                  << " foreign mods=" << foreignMods.size() << "\n";
    }
    assert(!applied);
    assert(selected == original && std::memcmp(&player, &before, sizeof(player)) == 0);
}

static void expectCombinedHandPriority(Player& player, int sword, const char* path) {
    for (int appearance : {0, 1}) {
        vanillaHeldSwords = appearance;
        for (bool alt : {false, true}) {
            altOn = alt;
            localMods.insert(path); expectUntouched(player); localMods.clear();
            foreignMods.insert(path); expectUntouched(player); foreignMods.clear();
        }
    }
    vanillaHeldSwords = -1;
    // An inactive Alt hand does not own the draw; a live toggle immediately does.
    alternate.insert(path); altOn = false; expectAuthored(player, sword);
    altOn = true; expectUntouched(player); alternate.clear(); altOn = false;
}

static void expectFourSeparateFromGenericSwordMods(Player& player, const char* path) {
    for (bool alt : {false, true}) {
        altOn = alt;
        localMods.insert(path); expectAuthored(player, NEI_HELD_SWORD_FOUR); localMods.clear();
        foreignMods.insert(path); expectAuthored(player, NEI_HELD_SWORD_FOUR); foreignMods.clear();
    }
    alternate.insert(path);
    altOn = false; expectAuthored(player, NEI_HELD_SWORD_FOUR);
    altOn = true; expectAuthored(player, NEI_HELD_SWORD_FOUR);
    alternate.clear(); altOn = false;
}

static void expectAppearanceToggle(Player& player, int sword, int nativeAura = -1, bool separateHilt = false) {
    const SaveContext saveBefore = gSaveContext;
    for (int appearance : {0, 1, 0, 1, 0}) {
        vanillaHeldSwords = appearance;
        if (!appearance) {
            expectAuthored(player, sword);
        } else if (nativeAura < 0) {
            expectUntouched(player);
        } else {
            const Player before = player;
            Gfx* selected = original;
            assert(WeaponUpgrade_ApplyHeldSwordDL(&selected, hand, &player, 12, 34, 56));
            assert(std::memcmp(&player, &before, sizeof(player)) == 0);
            assert(selected[0].words.w1 == reinterpret_cast<uintptr_t>(nativeBlade));
            const int handIndex = separateHilt ? 2 : 1;
            if (separateHilt) assert(selected[1].words.w1 == reinterpret_cast<uintptr_t>(nativeHilt));
            assert(selected[handIndex].words.w1 == reinterpret_cast<uintptr_t>(hand));
            assert((selected[handIndex + 1].words.w0 >> 24) == G_RDPPIPESYNC);
            assert((selected[handIndex + 2].words.w0 >> 24) == G_SETENVCOLOR &&
                   selected[handIndex + 2].words.w1 == 0x0c223800);
            assert((selected[handIndex + 4].words.w0 >> 24) == G_ENDDL);
            assert(registeredHand == selected && registeredAura == nativeAura &&
                   "Native progressive blades must retain their own elemental aura profile");
        }
        assert(std::memcmp(&gSaveContext, &saveBefore, sizeof(gSaveContext)) == 0);
    }
}

static void testVanillaHeldSetting() {
    for (bool alt : {false, true}) {
        allResources();
        altOn = alt;
        // An unset setting and explicit false both preserve every authored
        // sword/frame. Already-populated wrappers must not latch that decision.
        for (int sword = 0; sword < NEI_HELD_SWORD_COUNT; ++sword) {
            for (int rig = 0; rig < NEI_HELD_SWORD_FRAME_COUNT; ++rig) {
                vanillaHeldSwords = -1;
                assert(NeiHeldSword_ModelDL(sword, rig));
                for (int appearance : {1, 0, 1, 0}) {
                    vanillaHeldSwords = appearance;
                    assert(bool(NeiHeldSword_ModelDL(sword, rig)) == !appearance &&
                           "The live vanilla preference did not bypass the authored held sword");
                }
            }
        }
        for (const char* path : {
                 "__OTR__objects/gameplay_keep/gRazorSwordBladeDL",
                 "__OTR__objects/gameplay_keep/gRazorSwordHandleDL",
                 "__OTR__objects/object_link_child/gLinkHumanGildedSwordBladeDL",
                 "__OTR__objects/object_link_child/gLinkHumanGildedSwordHandleDL",
                 "__OTR__objects/object_link_child/gLinkHumanGreatFairysSwordDL"})
            local.insert(path);
        for (bool adultFrame : {false, true}) {
            adult = adultFrame;
#ifndef NEI_EQUIPMENT_MM
            gSaveContext.linkAge = adult ? LINK_AGE_ADULT : LINK_AGE_CHILD;
#endif
            Player player{};
            swordHand(player, PLAYER_IA_SWORD_KOKIRI, ITEM_SWORD_KOKIRI);
#ifdef NEI_EQUIPMENT_MM
            const int kokiri = NEI_HELD_SWORD_MM_KOKIRI;
#else
            const int kokiri = NEI_HELD_SWORD_KOKIRI;
#endif
            expectAppearanceToggle(player, kokiri);
            kokiriLevel = 1;
            expectAppearanceToggle(player, NEI_HELD_SWORD_RAZOR, SWORD_AURA_NATIVE_MM_RAZOR, true);
            kokiriLevel = 2;
            expectAppearanceToggle(player, NEI_HELD_SWORD_GILDED, SWORD_AURA_NATIVE_MM_GILDED, true);
            gildedLook = false;
            expectAppearanceToggle(player, NEI_HELD_SWORD_RAZOR, SWORD_AURA_NATIVE_MM_RAZOR, true);
            gildedLook = true;
            kokiriLevel = 0;
            swordHand(player, PLAYER_IA_SWORD_MASTER, ITEM_SWORD_MASTER);
            expectAppearanceToggle(player, NEI_HELD_SWORD_MASTER);
            trueMaster = 1;
            expectAppearanceToggle(player, NEI_HELD_SWORD_TRUE_MASTER);
            trueMaster = 0;
            swordHand(player, PLAYER_IA_SWORD_BIGGORON, ITEM_SWORD_BGS);
            expectAppearanceToggle(player, NEI_HELD_SWORD_BIGGORON);
            greatFairy = 1;
            expectAppearanceToggle(player, NEI_HELD_SWORD_GREAT_FAIRY, SWORD_AURA_NATIVE_MM_FAIRY);
            fairyLook = false;
            expectAppearanceToggle(player, NEI_HELD_SWORD_BIGGORON);
            fairyLook = true;
            greatFairy = 0;
#ifdef NEI_EQUIPMENT_MM
            swordHand(player, PLAYER_IA_SWORD_RAZOR, ITEM_SWORD_RAZOR);
            expectAppearanceToggle(player, NEI_HELD_SWORD_RAZOR);
            swordHand(player, PLAYER_IA_SWORD_GILDED, ITEM_SWORD_GILDED);
            expectAppearanceToggle(player, NEI_HELD_SWORD_GILDED);
            swordHand(player, PLAYER_IA_SWORD_TWO_HANDED, ITEM_SWORD_GREAT_FAIRY);
            expectAppearanceToggle(player, NEI_HELD_SWORD_GREAT_FAIRY);
#else
            swordHand(player, PLAYER_IA_SWORD_BIGGORON, ITEM_SWORD_KNIFE);
            gSaveContext.swordHealth = 0;
            expectAppearanceToggle(player, NEI_HELD_SWORD_BROKEN_GIANTS_KNIFE);
#endif
            swordHand(player, PLAYER_IA_SWORD_KOKIRI, ITEM_SWORD_KOKIRI);
            for (int appearance : {0, 1}) {
                vanillaHeldSwords = appearance;
                bodySelected = true;
                expectUntouched(player);
                bodySelected = false;
                localMods.insert("__OTR__objects/object_custom_equip/gCustomKokiriSwordDL");
                expectUntouched(player);
                localMods.clear();
            }
        }
    }
    allResources();
}

#ifndef NEI_EQUIPMENT_MM
static void knifeStateChecks() {
  Player player{};
  for (int age : {LINK_AGE_ADULT, LINK_AGE_CHILD}) {
    for (int item : {ITEM_SWORD_BGS, ITEM_SWORD_KNIFE}) {
      for (bool alternateEnabled : {false, true}) {
        for (float health : {0.f, 8.f}) {
          allResources();
          altOn = alternateEnabled;
          gSaveContext.linkAge = age;
          swordHand(player, PLAYER_IA_SWORD_BIGGORON, item);
          gSaveContext.bgsFlag = 0;
          gSaveContext.swordHealth = health;
          const SaveContext saved = gSaveContext;
          const Player before = player;
          Gfx *selected = original;
          assert(WeaponUpgrade_ApplyHeldSwordDL(&selected, hand, &player, 12,
                                                34, 56));
          const Gfx *wrapper =
              reinterpret_cast<const Gfx *>(selected[0].words.w1);
          const std::string path =
              reinterpret_cast<const char *>(wrapper[0].words.w1);
          const char *wanted =
              health <= 0.f ? "/broken_giants_knife/" : "/giants_knife/";
          if (path.find(wanted) == std::string::npos)
            std::cerr << "Knife state drew " << path << " instead of " << wanted
                      << "\n";
          assert(path.find(wanted) != std::string::npos);
          assert(selected[1].words.w1 == reinterpret_cast<uintptr_t>(hand));
          assert(std::memcmp(&player, &before, sizeof(player)) == 0);
          assert(std::memcmp(&gSaveContext, &saved, sizeof(saved)) == 0);
          const int model = health <= 0.f ? NEI_HELD_SWORD_BROKEN_GIANTS_KNIFE
                                          : NEI_HELD_SWORD_GIANTS_KNIFE;
          expectAuthored(player, model);
          // No incomplete donor graph may replace the native hand.
          for (const char *const *p = sNeiHeldSwordResources[model]; *p; ++p) {
            local.erase(*p);
            donor.erase(*p);
            shipped.erase(*p);
            expectUntouched(player);
            local.insert(*p);
            donor.insert(*p);
            shipped.insert(*p);
          }
          const char *custom = health <= 0.f
                                   ? "__OTR__objects/object_custom_equip/"
                                     "gCustomBrokenLongswordDL"
                                   : "__OTR__objects/object_custom_equip/"
                                     "gCustomBreakableLongswordDL";
          expectCombinedHandPriority(player, model, custom);
          expectCombinedHandPriority(
              player, model,
              "__OTR__objects/object_custom_equip/gCustomLongswordDL");
          bodySelected = true;
          expectUntouched(player);
          bodySelected = false;
          greatFairy = 1;
          expectAuthored(player, NEI_HELD_SWORD_GREAT_FAIRY);
          fairyLook = false;
          expectAuthored(player, model);
          fairyLook = true;
          greatFairy = 0;
          fourEquipped = true;
          expectAuthored(player, NEI_HELD_SWORD_FOUR);
          fourEquipped = false;
          vanillaHeldSwords = 1;
          expectUntouched(player);
        }
      }
    }
  }
  allResources();
}
#endif

int main() {
#ifndef NEI_EQUIPMENT_MM
  knifeStateChecks();
#endif
  Player player{};
  allResources();
  swordHand(player, PLAYER_IA_SWORD_KOKIRI, ITEM_SWORD_KOKIRI);
#ifdef NEI_EQUIPMENT_MM
    const int kokiri = NEI_HELD_SWORD_MM_KOKIRI;
#else
    const int kokiri = NEI_HELD_SWORD_KOKIRI;
#endif
    expectAuthored(player, kokiri);
    // Every dependency is required; failed draws leave the native hand intact.
    for (const char* const* p = sNeiHeldSwordResources[kokiri]; *p; ++p) {
        local.erase(*p); donor.erase(*p); shipped.erase(*p); expectUntouched(player);
        local.insert(*p); donor.insert(*p); shipped.insert(*p);
    }
    for (const char* p : sNeiHeldSwordPaths[kokiri][frame()]) {
        local.erase(p); donor.erase(p); shipped.erase(p); expectUntouched(player);
        local.insert(p); donor.insert(p); shipped.insert(p);
    }
    // A MM-local collision cannot stand in for a missing OoT authored donor.
#ifdef NEI_EQUIPMENT_MM
    shipped.clear(); expectUntouched(player); shipped = local;
    altOn = true; donor.clear(); expectUntouched(player); donor = local; altOn = false;
#endif
    // Base-path GI-only mods cannot replace the promised shipped held mesh.
    localMods.insert(sNeiHeldSwordResources[kokiri][0]);
    expectAuthored(player, kokiri); localMods.clear();
    shippedOwnerReady = false; expectUntouched(player); shippedOwnerReady = true;
    // Selected native/Alt hand or generic equipment wins across live toggles.
    altOn = true;
    const char* custom = "__OTR__objects/object_custom_equip/gCustomKokiriSwordDL";
    alternate.insert(custom); expectUntouched(player);
    altOn = false; expectAuthored(player, kokiri);
    alternate.clear(); localMods.insert(custom); expectUntouched(player); localMods.clear();
    altOn = true; bodySelected = true; expectUntouched(player); bodySelected = false;
    player.leftHandType = PLAYER_MODELTYPE_LH_OPEN; expectUntouched(player);
    swordHand(player, PLAYER_IA_SWORD_KOKIRI, ITEM_SWORD_KOKIRI);
    kokiriLevel = 1; expectAuthored(player, NEI_HELD_SWORD_RAZOR);
    kokiriLevel = 2; expectAuthored(player, NEI_HELD_SWORD_GILDED);
    gildedLook = false; expectAuthored(player, NEI_HELD_SWORD_RAZOR); gildedLook = true;
    kokiriLevel = 0;
    swordHand(player, PLAYER_IA_SWORD_MASTER, ITEM_SWORD_MASTER);
    expectAuthored(player, NEI_HELD_SWORD_MASTER);
    trueMaster = 1; expectAuthored(player, NEI_HELD_SWORD_TRUE_MASTER);
    trueMaster = 0;
    swordHand(player, PLAYER_IA_SWORD_BIGGORON, ITEM_SWORD_BGS);
    expectAuthored(player, NEI_HELD_SWORD_BIGGORON);
    greatFairy = 1; expectAuthored(player, NEI_HELD_SWORD_GREAT_FAIRY);
    fairyLook = false; expectAuthored(player, NEI_HELD_SWORD_BIGGORON); fairyLook = true;
    fourEquipped = true; expectAuthored(player, NEI_HELD_SWORD_FOUR); fourEquipped = false;
    // The combined native hand path belongs to the physical age/rig. Adult
    // one-hand tiers use Master-hand DLs; child one-hand tiers use Kokiri hands.
    const char* adultOneHands[] = {
        "__OTR__objects/object_link_boy/gLinkAdultLeftHandHoldingMasterSwordNearDL",
        "__OTR__objects/object_link_boy/gLinkAdultLeftHandHoldingMasterSwordFarDL",
    };
#ifdef NEI_EQUIPMENT_MM
    swordHand(player, PLAYER_IA_SWORD_KOKIRI, ITEM_SWORD_KOKIRI);
    for (u8 level : {1, 2}) {
        kokiriLevel = level;
        const int model = level == 1 ? NEI_HELD_SWORD_RAZOR : NEI_HELD_SWORD_GILDED;
        // Native MM selects the combined source from its equipped sword nibble,
        // independently of the progressive blade chosen by the NEI upgrade.
        const char* nativeHands[] = {
            "__OTR__objects/object_link_child/gLinkHumanLeftHandHoldingKokiriSwordDL",
            "__OTR__objects/object_link_child/gLinkHumanLeftHandHoldingRazorSwordDL",
            "__OTR__objects/object_link_child/gLinkHumanLeftHandHoldingGildedSwordDL",
        };
        for (u16 equip = 1; equip <= 3; ++equip) {
            gSaveContext.save.saveInfo.equips.equipment = equip;
            expectCombinedHandPriority(player, model, nativeHands[equip - 1]);
        }
    }
    gSaveContext.save.saveInfo.equips.equipment = 0; kokiriLevel = 0;
    adult = true;
    swordHand(player, PLAYER_IA_SWORD_RAZOR, ITEM_SWORD_RAZOR);
    for (const char* path : adultOneHands) expectCombinedHandPriority(player, NEI_HELD_SWORD_RAZOR, path);
    swordHand(player, PLAYER_IA_SWORD_GILDED, ITEM_SWORD_GILDED);
    for (const char* path : adultOneHands) expectCombinedHandPriority(player, NEI_HELD_SWORD_GILDED, path);
    adult = false;
    swordHand(player, PLAYER_IA_SWORD_BIGGORON, ITEM_SWORD_BGS);
    fairyLook = false;
    for (u8 upgrade : {0, 1}) {
        greatFairy = upgrade;
        expectCombinedHandPriority(player, NEI_HELD_SWORD_BIGGORON,
            "__OTR__objects/object_link_child/gLinkHumanLeftHandHoldingGreatFairysSwordDL");
    }
    greatFairy = 1; fairyLook = true;
#else
    gSaveContext.linkAge = LINK_AGE_ADULT;
    swordHand(player, PLAYER_IA_SWORD_KOKIRI, ITEM_SWORD_KOKIRI);
    for (u8 level : {0, 1, 2}) {
        kokiriLevel = level;
        const int model = level == 0 ? NEI_HELD_SWORD_KOKIRI : level == 1 ? NEI_HELD_SWORD_RAZOR : NEI_HELD_SWORD_GILDED;
        for (const char* path : adultOneHands) expectCombinedHandPriority(player, model, path);
    }
    kokiriLevel = 0;
    gSaveContext.linkAge = LINK_AGE_CHILD;
    swordHand(player, PLAYER_IA_SWORD_MASTER, ITEM_SWORD_MASTER);
    for (const char* path : {
             "__OTR__objects/object_link_child/gLinkChildLeftFistAndKokiriSwordNearDL",
             "__OTR__objects/object_link_child/gLinkChildLeftFistAndKokiriSwordFarDL"})
        expectCombinedHandPriority(player, NEI_HELD_SWORD_MASTER, path);
    swordHand(player, PLAYER_IA_SWORD_BIGGORON, ITEM_SWORD_BGS);
    expectCombinedHandPriority(player, NEI_HELD_SWORD_GREAT_FAIRY,
                               "__OTR__objects/object_link_child/gLinkChildLeftHandHoldingMasterSwordDL");
    greatFairy = 0;
    expectCombinedHandPriority(player, NEI_HELD_SWORD_BIGGORON,
                               "__OTR__objects/object_link_child/gLinkChildLeftHandHoldingMasterSwordDL");
    gSaveContext.linkAge = LINK_AGE_ADULT;
    swordHand(player, PLAYER_IA_SWORD_KOKIRI, ITEM_SWORD_KOKIRI);
    localMods.insert("__OTR__objects/object_link_boy/gLinkAdultLeftHandHoldingBgsNearDL");
    expectAuthored(player, NEI_HELD_SWORD_KOKIRI); localMods.clear();
#endif
    swordHand(player, PLAYER_IA_SWORD_KOKIRI, ITEM_NET); expectUntouched(player);
    // A selected Four Sword slot does not own another item that happens to
    // reuse a sword action. Its own extended carrier remains a valid sword.
    fourEquipped = true; expectUntouched(player);
#ifdef NEI_EQUIPMENT_MM
    // Execute the actual native limb injection with the real handler. MM's Net
    // leaves ONE_HAND_SWORD on Player while replacing its DL with a closed fist.
    PlayState nativePlay{};
    nativePlay.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    const char* nativeHandPath = "__OTR__objects/object_link_child/gLinkHumanLeftHandClosedDL";
    local.insert(nativeHandPath);
    gPlayerLeftHandClosedDLs[PLAYER_FORM_HUMAN * 2] = (Gfx*)nativeHandPath;
    Gfx* nativeNet = hand;
    ApplyNativeHeldSwordStage(&nativePlay, &player, &nativeNet);
    assert(nativeNet == hand && nativeAllocations == 0);
#endif
    swordHand(player, PLAYER_IA_SWORD_MASTER, ITEM_NET); expectUntouched(player);
    swordHand(player, PLAYER_IA_SWORD_KOKIRI, ITEM_EXT_SWORD_2);
    expectAuthored(player, NEI_HELD_SWORD_FOUR);
#ifdef NEI_EQUIPMENT_MM
    Gfx* nativeFour = hand;
    const Player nativeBefore = player;
    ApplyNativeHeldSwordStage(&nativePlay, &player, &nativeFour);
    assert(nativeFour == nativeFrameCompound && nativeAllocations == 1 &&
           nativeFour[1].words.w1 == reinterpret_cast<uintptr_t>(hand) &&
           std::memcmp(&player, &nativeBefore, sizeof(player)) == 0);
    adult = true;
    for (const char* path : adultOneHands) expectFourSeparateFromGenericSwordMods(player, path);
    adult = false;
#else
    // The separate Four Sword item retains its authored body even if generic
    // child/adult sword equipment is replaced; its own pair mods are separate.
    gSaveContext.linkAge = LINK_AGE_CHILD;
    for (const char* path : adultOneHands) expectFourSeparateFromGenericSwordMods(player, path);
    gSaveContext.linkAge = LINK_AGE_ADULT;
#endif
    fourEquipped = false; expectUntouched(player);
#ifdef NEI_EQUIPMENT_MM
    swordHand(player, PLAYER_IA_SWORD_RAZOR, ITEM_SWORD_RAZOR); expectAuthored(player, NEI_HELD_SWORD_RAZOR);
    swordHand(player, PLAYER_IA_SWORD_GILDED, ITEM_SWORD_GILDED); expectAuthored(player, NEI_HELD_SWORD_GILDED);
    swordHand(player, PLAYER_IA_SWORD_TWO_HANDED, ITEM_SWORD_GREAT_FAIRY);
    expectAuthored(player, NEI_HELD_SWORD_GREAT_FAIRY);
    player.transformation = PLAYER_FORM_GORON; expectUntouched(player);
    player.transformation = PLAYER_FORM_FIERCE_DEITY; expectUntouched(player);
    swordHand(player, PLAYER_IA_SWORD_MASTER, ITEM_SWORD_MASTER); adult = true;
    expectAuthored(player, NEI_HELD_SWORD_MASTER);
    adult = false; adultMode = true;
    expectAuthored(player, NEI_HELD_SWORD_MASTER);
    fourEquipped = true;
    expectAuthored(player, NEI_HELD_SWORD_FOUR);
#else
    swordHand(player, PLAYER_IA_SWORD_BIGGORON, ITEM_SWORD_BGS);
    gSaveContext.swordHealth = 0; greatFairy = 0;
    fourEquipped = true; expectAuthored(player, NEI_HELD_SWORD_FOUR);
    fourEquipped = false;
    expectAuthored(player, NEI_HELD_SWORD_BROKEN_GIANTS_KNIFE);
    for (const char* path : {
             "__OTR__objects/object_link_boy/gLinkAdultHandHoldingBrokenGiantsKnifeDL",
             "__OTR__objects/object_link_boy/gLinkAdultHandHoldingBrokenGiantsKnifeFarDL"})
      expectCombinedHandPriority(player, NEI_HELD_SWORD_BROKEN_GIANTS_KNIFE,
                                 path);
    swordHand(player, PLAYER_IA_SWORD_BIGGORON, ITEM_SWORD_KNIFE);
    gSaveContext.swordHealth = 0;
    expectAuthored(player, NEI_HELD_SWORD_BROKEN_GIANTS_KNIFE);
    gSaveContext.swordHealth = 8;
    expectAuthored(player, NEI_HELD_SWORD_GIANTS_KNIFE);
    swordHand(player, PLAYER_IA_SWORD_KOKIRI, ITEM_SWORD_KOKIRI); gSaveContext.linkAge = LINK_AGE_CHILD;
    expectAuthored(player, NEI_HELD_SWORD_KOKIRI);
#endif
    testVanillaHeldSetting();
    std::cout << "PASS held sword selection: authored default, live vanilla toggle, native progressive auras, complete donor graphs, Alt/mod priority and untouched player/save state\n";
}
