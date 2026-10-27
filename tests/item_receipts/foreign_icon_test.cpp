#include "combo/menu/ComboItemDrawABI.h"
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
/* ICON_OWNER_INCLUDE */
#ifdef COMBO_BUILD
extern "C" {
int gComboGameMode = COMBO_GAME_BOTH;
}
#endif
using s16 = int16_t;
using u8 = uint8_t;
using RandoCheckId = int;
namespace ComboRando {
constexpr int GAME_OOT = 0, GAME_MM = 1;
struct ForeignItem {
    int itemGame = GAME_OOT;
    bool trap = false;
    std::string itemName;
} item;
} // namespace ComboRando
namespace Rando::MiscBehavior {
const ComboRando::ForeignItem* MM_LookupForeign(RandoCheckId) { return &ComboRando::item; }
} // namespace Rando::MiscBehavior
namespace Rando {
uint8_t ComboForeignMessageIcon(RandoCheckId);
}
static bool donorReady = false;
static std::string stagedPath, requested;
static int width, height, ia8;
static uint8_t tint[3];
static CwItemIconInfo donorIcon{};
static int32_t DonorIcon(const char* name, CwItemIconInfo* icon) {
    requested = name;
    *icon = donorIcon;
    return 1;
}
void* Combo_ResolveSym(const char*, const char*) { return donorReady ? reinterpret_cast<void*>(DonorIcon) : nullptr; }
const char* ComboInternRoutedPathOOT(std::string path) {
    static std::string routed;
    routed = std::move(path);
    return routed.c_str();
}
void Message_StageCustomItemIconEx(void* tex, s16 w, s16 h, u8 isIA8) {
    stagedPath = static_cast<const char*>(tex);
    width = w;
    height = h;
    ia8 = isIA8;
    tint[0] = tint[1] = tint[2] = 255;
}
void Message_StageCustomItemIconTint(void* tex, s16 w, s16 h, u8 isIA8, u8 r, u8 g, u8 b) {
    Message_StageCustomItemIconEx(tex, w, h, isIA8);
    tint[0] = r;
    tint[1] = g;
    tint[2] = b;
}
/* FOREIGN_ICON_SELECTOR */

int main() {
    for (const char* alias : {"Shield of Ikana", "Ikana Mirror Shield", "Mirror Shield (MM)", "Ikana Shield",
                              "MM Mirror Shield", "Mirror Shield (Ikana)"}) {
        ComboRando::item.itemName = alias;
        stagedPath.clear();
        assert(Rando::ComboForeignMessageIcon(17) == 0xF5 && "cold Ikana alias lost its native MM icon");
        assert(stagedPath == COMBO_IKANA_SHIELD_ICON);
        assert(width == 32 && height == 32 && !ia8);
    }
    donorReady = true;
    ComboRando::item.itemName = "Bow";
    donorIcon = {"__OTR__textures/icon_item_static/gItemIconBowTex", 32, 32, 0, 0, {}};
    assert(Rando::ComboForeignMessageIcon(17) == 0xF5);
    assert(stagedPath == "__OTR__@oot:textures/icon_item_static/gItemIconBowTex");
    assert(tint[0] == 255 && tint[1] == 255 && tint[2] == 255);
    ComboRando::item.itemName = "Song of Healing";
    donorIcon = {"__OTR__textures/icon_item_static/gSongNoteTex", 16, 24, 1, 1, {255, 150, 230, 255}};
    assert(Rando::ComboForeignMessageIcon(17) == 0xF5);
    assert(width == 16 && height == 24 && ia8 && tint[0] == 255 && tint[1] == 150 && tint[2] == 230);
    ComboRando::item.itemName = "unknown borrowed name";
    donorIcon = {"__OTR__icon_item_static_yar/gItemIconMirrorShieldTex", 32, 32, 0, 0, {}};
    assert(Rando::ComboForeignMessageIcon(17) == 0xF5);
    assert(stagedPath == COMBO_IKANA_SHIELD_ICON);
    ComboRando::item.trap = true;
    stagedPath = "untouched";
    assert(Rando::ComboForeignMessageIcon(17) == 0xFE && stagedPath == "untouched");
    ComboRando::item.trap = false;
    for (int bad : {0, 65}) {
        donorIcon.width = bad;
        assert(Rando::ComboForeignMessageIcon(17) == 0xFE);
    }
    std::cout << "Foreign receipts preserve MM-owned Ikana aliases, donor clef tint and trap/dimension guards\n";
}
