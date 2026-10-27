#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
#include <string>
#include "mods/items/logic/weapon_upgrades.h"
#include "mods/extended_equipment.h"
#include "mods/equipment/nei_equipment_presentation.h"
#include "../../combo/NeiHeldSword.h"
#include "../../combo/render/sword_element_aura_profiles.h"
#ifdef NEI_EQUIPMENT_MM
#include "mods/forms/custom_forms.h"
#endif

static std::set<std::string> local, donor, shipped, selected;
static bool altOn, equipped = true, customBody, adult;
static bool vanillaHeldSwords;
static Gfx hand[1]{}, original[1]{}, native[1]{};
static Gfx legacyBlade[1]{}, legacyHilt[1]{};
extern "C" {
void DinFireSword_SetHandModel(Player*, void*, int) {}
void DinFireSword_CopyHandModel(Player*, void*, void*) {}
SaveContext gSaveContext{};
u8 ResourceMgr_FileExists(const char* path) { return local.contains(path); }
u8 ResourceMgr_FileAltExists(const char*) { return 0; }
bool ResourceMgr_IsAltAssetsEnabled() { return altOn; }
int ResourceMgr_IsModAsset(const char* path) { return selected.contains(path); }
int ResourceMgr_IsModAssetForGame(const char*, const char* path) { return selected.contains(path); }
int32_t OOT_NeiEnsureGiBaseOwner() { return 1; }
int32_t OOT_NeiResourceExists(const char* path) {
    const std::string p = path;
    const std::string prefix = "__OTR__@oot-gi-base:";
    return p.starts_with(prefix) ? shipped.contains("__OTR__" + p.substr(prefix.size())) : donor.contains(p);
}
int OOT_NeiResourceIsMod(const char* path) { return selected.contains(path); }
#ifdef NEI_EQUIPMENT_MM
#include "native_equipment_data.inc"
s32 AdultLink_UsesAdultPresentation(const Player*) { return adult; }
u8 Player_IsCustomLinkModel(Player*) { return customBody; }
#else
u8 Player_IsCustomLinkModel() { return customBody; }
#endif
u8 FourSword_IsEquipped() { return equipped; }
s32 CVarGetInteger(const char* name, s32 fallback) {
    return !std::strcmp(name, "gEnhancements.SkijerNEI.UseVanillaHeldSwords") ? vanillaHeldSwords : fallback;
}
u8 WeaponUpgrade_KokiriLevel() { return 0; }
u8 WeaponUpgrade_HasGilded() { return 0; }
u8 WeaponUpgrade_HasTrueMaster() { return 0; }
u8 WeaponUpgrade_HasGreatFairy() { return 0; }
void* MmAssets_LoadResource(const char*) { return nullptr; }
Gfx* ResourceMgr_LoadGfxByName(const char* path) {
    if (std::strstr(path, "gNeiFourSwordBladeDL")) return legacyBlade;
    if (std::strstr(path, "gNeiFourSwordHiltDL")) return legacyHilt;
    return std::strstr(path, "Closed") ? hand : native;
}
void gSPDisplayList(Gfx* command, Gfx* display) { __gSPDisplayList(command, display); }

#define FOURSWORD_BLADE_DL "__OTR__objects/object_nei_four_sword/gNeiFourSwordBladeDL"
#define FOURSWORD_HILT_DL "__OTR__objects/object_nei_four_sword/gNeiFourSwordHiltDL"
#include "four_getter.inc"
#include "held_selector.inc"
}

static Gfx perDraw[32][8];
static unsigned allocations;
static Gfx* allocateCompound(size_t size) {
    assert(size == 8 * sizeof(Gfx) && allocations < 32);
    return perDraw[allocations++];
}
#ifdef NEI_EQUIPMENT_MM
extern "C" {
u8 Trident_GoldenArmor() { return 0; }
s32 CustomForms_ActiveForm() { return CUSTOM_FORM_NONE; }
u8 ExtEquip_ShouldHideSwordDL() { return 0; }
s32 BossRemains_IsOdolwaWorn() { return 0; }
s32 BossRemains_IsGohtWorn() { return 0; }
}
static int sPlayerLod;
static bool sIsMod, sIsForm, sIsChildRig;
static Gfx* sDL_LHClosed = hand;
static Gfx* gPlayerLeftHandClosedDLs[2 * PLAYER_FORM_MAX];
#undef GRAPH_ALLOC
#define GRAPH_ALLOC(context, size) allocateCompound(size)
#else
static Gfx* gPlayerLeftHandClosedDLs[4] = {hand,hand,hand,hand};
static int sDListsLodOffset, sLeftHandType;
static Color_RGB8 sPlayerBodyEnvColor = {30,105,27};
static Gfx* DinFireSword_HandDL(PlayState*, Player*, void*, u8, u8, u8) {
    return equipped ? nullptr : native;
}
#define Player_ResolveLimbDLForDummyOrLocal(path) (path)
#define Graph_Alloc(context, size) allocateCompound(size)
#endif
#include "four_limb.inc"

static void resources() {
    local.clear(); donor.clear(); shipped.clear(); selected.clear();
    for (int sword = 0; sword < NEI_HELD_SWORD_COUNT; ++sword) {
        for (const char* const* p = sNeiHeldSwordResources[sword]; *p; ++p) local.insert(*p);
        for (int frame = 0; frame < NEI_HELD_SWORD_FRAME_COUNT; ++frame)
            for (const char* p : sNeiHeldSwordPaths[sword][frame]) local.insert(p);
    }
    donor = shipped = local;
    customBody = false;
    vanillaHeldSwords = false;
    gSaveContext = {};
}

static int frame() {
#ifdef NEI_EQUIPMENT_MM
    return adult ? NEI_HELD_SWORD_OOT_ADULT : NEI_HELD_SWORD_MM_HUMAN;
#else
    return gSaveContext.linkAge;
#endif
}

static void expectFourMesh(Gfx* draw) {
    if (draw == original)
        std::cerr << "Four Sword caller declined: Alt=" << altOn << " frame=" << frame() << '\n';
    assert(draw && draw != original && "A caller skin gate prevented the Four Sword held selector");
    const Gfx* sword = reinterpret_cast<const Gfx*>(draw[0].words.w1);
    assert((sword[0].words.w0 >> 24) == G_DL_OTR_FILEPATH);
    std::string want = sNeiHeldSwordPaths[NEI_HELD_SWORD_FOUR][frame()][0];
    if (!altOn) want = "__OTR__@oot-gi-base:" + want.substr(7);
#ifdef NEI_EQUIPMENT_MM
    else want = "__OTR__@oot:" + want.substr(7);
#endif
    assert(std::string(reinterpret_cast<const char*>(sword[0].words.w1)) == want &&
           "Four Sword held draw lost the authored GI resource owner/frame");
    assert(draw[1].words.w1 == reinterpret_cast<uintptr_t>(hand));
}

static void expectFour(Player& player) {
    const Player before = player;
    Gfx* draw = original;
    const u8 applied = WeaponUpgrade_ApplyHeldSwordDL(&draw, hand, &player, 30, 105, 27);
    if (!applied)
        std::cerr << "Four Sword declined: Alt=" << altOn << " frame=" << frame()
                  << " body=" << customBody << " selected=" << (selected.empty() ? "none" : *selected.begin()) << '\n';
    assert(applied &&
           "The equipped Four Sword was suppressed by an unrelated native equipment/body mod");
    assert(!std::memcmp(&player, &before, sizeof(player)));
    expectFourMesh(draw);
}

static u8 getFour(void** blade, void** hilt) {
#ifdef NEI_EQUIPMENT_MM
    return FourSword_HeldSwordDLForFrame(blade, hilt, frame());
#else
    return FourSword_HeldSwordDL(blade, hilt);
#endif
}

int main(int argc, char** argv) {
    const bool bodyCase = argc > 1 && !std::strcmp(argv[1], "body");
    const bool limbCase = argc > 1 && !std::strcmp(argv[1], "limb");
    const bool lateCase = argc > 1 && !std::strcmp(argv[1], "late");
    const bool graphCase = argc > 1 && !std::strcmp(argv[1], "graph");
    const bool vanillaCase = argc > 1 && !std::strcmp(argv[1], "vanilla");
    const bool vanillaMissingCase = argc > 1 && !std::strcmp(argv[1], "vanilla-missing");
    for (bool alt : {false, true}) for (bool adultFrame : {false, true}) {
        resources(); altOn = alt; adult = adultFrame;
#ifndef NEI_EQUIPMENT_MM
        gSaveContext.linkAge = adult ? LINK_AGE_ADULT : LINK_AGE_CHILD;
#endif
        Player player{};
        player.actor.scale.y = .01f;
        player.heldItemAction = PLAYER_IA_SWORD_KOKIRI;
        player.heldItemId = ITEM_EXT_SWORD_2;
#ifdef NEI_EQUIPMENT_MM
        player.transformation = PLAYER_FORM_HUMAN;
        player.leftHandType = PLAYER_MODELTYPE_LH_ONE_HAND_SWORD;
#else
        player.leftHandType = PLAYER_MODELTYPE_LH_SWORD;
#endif
        expectFour(player);
        if (vanillaCase || vanillaMissingCase) {
            // Both authored alternatives are complete. Disabling only the
            // primary GI-derived sword must not select the second redesign.
            for (int model : {NEI_EQUIPMENT_FOUR_BLADE, NEI_EQUIPMENT_FOUR_HILT})
                for (const char* const* p = sNeiEquipmentResources[model]; *p; ++p)
                    local.insert(*p);
            if (vanillaCase) {
                local.insert(FOURSWORD_BLADE_DL);
                local.insert(FOURSWORD_HILT_DL);
            }
            donor = shipped = local;
            for (bool useVanilla : {true, false, true, false}) {
                vanillaHeldSwords = useVanilla;
                if (!useVanilla) {
                    expectFour(player);
                    continue;
                }
                void *blade = nullptr, *hilt = nullptr;
                if (vanillaMissingCase) {
                    assert(!getFour(&blade, &hilt) && !blade && !hilt &&
                           "Missing legacy resources must retain the native hand, not another authored Four Sword");
                    Gfx* draw = original;
                    assert(!WeaponUpgrade_ApplyHeldSwordDL(&draw, hand, &player, 30, 105, 27) && draw == original);
                    continue;
                }
                assert(getFour(&blade, &hilt) && blade == legacyBlade && hilt == legacyHilt &&
                       "The vanilla preference still selected an authored Four Sword fallback");
                for (bool body : {false, true}) {
                    customBody = body;
                    const Player before = player;
                    Gfx* draw = original;
                    assert(WeaponUpgrade_ApplyHeldSwordDL(&draw, hand, &player, 30, 105, 27));
                    assert(draw[0].words.w1 == reinterpret_cast<uintptr_t>(legacyBlade));
                    assert(draw[1].words.w1 == reinterpret_cast<uintptr_t>(legacyHilt));
                    assert(draw[2].words.w1 == reinterpret_cast<uintptr_t>(hand));
                    assert(!std::memcmp(&player, &before, sizeof(player)));
                }
                customBody = false;
            }
            if (vanillaCase) {
                // An override of either legacy half still owns the whole pair
                // in both settings, regardless of the complete authored graphs.
                for (bool useVanilla : {false, true}) {
                    vanillaHeldSwords = useVanilla;
                    for (const char* selectedHalf : {FOURSWORD_BLADE_DL, FOURSWORD_HILT_DL}) {
                        selected = {selectedHalf};
                        void *blade = nullptr, *hilt = nullptr;
                        assert(getFour(&blade, &hilt));
                        for (auto [draw, path] : {std::pair(blade, FOURSWORD_BLADE_DL),
                                                 std::pair(hilt, FOURSWORD_HILT_DL)}) {
                            const Gfx* list = static_cast<const Gfx*>(draw);
                            assert((list[0].words.w0 >> 24) == G_DL_OTR_FILEPATH);
                            assert(!std::strcmp(reinterpret_cast<const char*>(list[0].words.w1), path));
                        }
                    }
                }
            }
            continue;
        }
#ifndef NEI_EQUIPMENT_MM
        if (lateCase) {
            customBody = true;
            PlayState play{};
            GraphicsContext gfx{};
            play.state.gfxCtx = &gfx;
            const Player before = player;
            // The actual CustomEquipment/PAK hooks submit a generic combined
            // sword after the early injection. Four Sword must retain its model.
            for (Gfx* afterHook : {original, static_cast<Gfx*>(nullptr)}) {
                Gfx* draw = afterHook;
                lateStage(&play, &player, &draw, true);
                expectFourMesh(draw);
                assert(!std::memcmp(&player, &before, sizeof(player)));
            }
            Gfx* draw = original;
            lateStage(&play, &player, &draw, false);
            assert(draw == original); // Hidden/transformed hands still bypass the stage.
            equipped = false;
            lateStage(&play, &player, &draw, true);
            assert(draw == native); // The existing ordinary Din stage keeps its priority.
            draw = nullptr;
            lateStage(&play, &player, &draw, true);
            assert(draw == nullptr); // Ordinary weapons still honor an explicit empty PAK slot.
            equipped = true;
            continue;
        }
#endif
        if (graphCase) {
            // A missing blade, matrix, vertex buffer or material must never queue
            // a partially available sword; MM-local collisions cannot fill OoT.
            const auto testDependency = [&](const char* path) {
                void *blade = nullptr, *hilt = nullptr;
#ifdef NEI_EQUIPMENT_MM
                auto& owner = alt ? donor : shipped;
                owner.erase(path);
                assert(!getFour(&blade, &hilt) && "A MM-local file completed a missing OoT Four Sword graph");
                owner.insert(path);
#else
                local.erase(path); shipped.erase(path);
                assert(!getFour(&blade, &hilt) && "A partial Four Sword graph reached the renderer");
                local.insert(path); shipped.insert(path);
#endif
            };
            for (const char* const* p = sNeiHeldSwordResources[NEI_HELD_SWORD_FOUR]; *p; ++p) testDependency(*p);
            for (const char* path : sNeiHeldSwordPaths[NEI_HELD_SWORD_FOUR][frame()]) testDependency(path);
            expectFour(player);
            // Only an override of the actual Four Sword pair owns that pair.
            selected = {FOURSWORD_BLADE_DL};
            local.insert(FOURSWORD_BLADE_DL); local.insert(FOURSWORD_HILT_DL);
            donor.insert(FOURSWORD_BLADE_DL); donor.insert(FOURSWORD_HILT_DL);
            void *blade = nullptr, *hilt = nullptr;
            assert(getFour(&blade, &hilt) && blade && hilt);
            for (auto [draw, path] : {std::pair(blade, FOURSWORD_BLADE_DL), std::pair(hilt, FOURSWORD_HILT_DL)}) {
                const Gfx* list = static_cast<const Gfx*>(draw);
                assert((list[0].words.w0 >> 24) == G_DL_OTR_FILEPATH);
                assert(!std::strcmp(reinterpret_cast<const char*>(list[0].words.w1), path));
            }
            continue;
        }
#ifdef NEI_EQUIPMENT_MM
        if (limbCase) {
            customBody = sIsMod = true;
            const char* handPath = "__OTR__objects/object_link_child/gLinkHumanLeftHandClosedDL";
            local.insert(handPath);
            gPlayerLeftHandClosedDLs[PLAYER_FORM_HUMAN * 2] = reinterpret_cast<Gfx*>(const_cast<char*>(handPath));
            PlayState play{};
            GraphicsContext gfx{};
            play.state.gfxCtx = &gfx;
            play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
            const Player before = player;
            Gfx* draw = original;
            if (adult) adultStage(&play, &player, &draw);
            else nativeStage(&play, &player, &draw);
            expectFourMesh(draw);
            assert(!std::memcmp(&player, &before, sizeof(player)));
            continue;
        }
#endif
        if (bodyCase) {
            customBody = true;
            expectFour(player);
            // The same selected body still owns an ordinary sword.
            equipped = false;
            player.heldItemId = ITEM_SWORD_KOKIRI;
            Gfx* draw = original;
            assert(!WeaponUpgrade_ApplyHeldSwordDL(&draw, hand, &player, 30, 105, 27));
            equipped = true;
            continue;
        }
        // Din/custom/native hands own the regular sword family, never Four Sword.
        for (const char* path : {"__OTR__objects/object_custom_equip/gCustomKokiriSwordDL",
                                 "__OTR__objects/object_link_child/gLinkChildLeftFistAndKokiriSwordNearDL",
                                 "__OTR__objects/object_link_boy/gLinkAdultLeftHandHoldingMasterSwordNearDL",
                                 "__OTR__objects/object_link_child/gLinkHumanLeftHandHoldingKokiriSwordDL"}) {
            selected = {path};
            expectFour(player);
            player.heldItemId = ITEM_SWORD_KOKIRI;
            expectFour(player); // The earlier Four Sword equipment overlay still works.
            equipped = false;
            Gfx* draw = original;
            if (std::strstr(path, "gCustomKokiri"))
                assert(!WeaponUpgrade_ApplyHeldSwordDL(&draw, hand, &player, 30, 105, 27) && draw == original);
            equipped = true;
            player.heldItemId = ITEM_EXT_SWORD_2;
        }
    }
    std::cout << "PASS "
#ifdef NEI_EQUIPMENT_MM
              << "MM"
#else
              << "OoT"
#endif
              << " Four Sword held: real getter, child/adult and both Alt states, selected "
              << (vanillaCase ? "live vanilla/legacy pair" : vanillaMissingCase ? "missing vanilla resources" : graphCase ? "complete donor graph/own pair" : lateCase ? "late custom/PAK limb" : limbCase ? "native/adult limb" : bodyCase ? "body" : "native/Din equipment")
              << " preserves Four identity and ordinary mod priority\n";
}
