"""Execute the real Four Sword getter/held selector and MM GI ownership gate."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/nei_held"))
from run_articulated_tests import flags as oot_flags
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_nei_tests import flags as mm_flags
from run_mm_back_equipment_tests import function


def held(directory, host):
    equipment = (ROOT / host / "mods/equipment/behaviors/equip_foursword.c").read_text()
    names = (["FourSword_HeldSwordDLForFrame", "FourSword_HeldSwordDL"] if host == "mm"
             else ["FourSword_HeldSwordDL"])
    (directory / "four_getter.inc").write_text("\n".join(function(equipment, name) for name in names))
    weapon = (ROOT / host / "mods/items/logic/weapon_upgrades.c").read_text()
    (directory / "held_selector.inc").write_text(function(weapon, "WeaponUpgrade_ApplyHeldSwordDL"))
    if host == "mm":
        inventory = (ROOT / "mm/src/code/z_inventory.c").read_text()
        native_data = []
        for declaration in ("u16 gEquipMasks[]", "u8 gEquipShifts[]"):
            begin = inventory.index(declaration)
            end = inventory.index("};", begin) + 2
            native_data.append(inventory[begin:end])
        (directory / "native_equipment_data.inc").write_text("\n".join(native_data))
    if host == "soh":
        source = (ROOT / "soh/src/code/z_player_lib.c").read_text()
        begin = source.index("    // CustomEquipment can select a broken-knife")
        end = source.index("    // The Switch Hook shares", begin)
        stage = re.sub(r"\bthis\b", "player", source[begin:end])
        (directory / "four_limb.inc").write_text(
            "static void lateStage(PlayState* play, Player* player, Gfx** dList, bool mayDrawProgressiveFire) {\n"
            + stage + "\n}\n")
    else:
        source = (ROOT / "mm/src/code/z_player_lib.c").read_text()
        begin = source.index("            // Keep the native human fist and reuse")
        end = source.index("            // ⚠️", begin)
        native = source[begin:end]
        source = (ROOT / "mm/mods/items/logic/adult_link_render.cpp").read_text()
        begin = source.index("    if (limbIndex == PLAYER_LIMB_LEFT_HAND && *dList != nullptr")
        end = source.index("    if (*dList != NULL)", begin)
        overlay = source[begin:end]
        (directory / "four_limb.inc").write_text(
            "static void nativeStage(PlayState* play, Player* player, Gfx** dList) {\n"
            "    const bool handIsSpokenFor = false;\n" + native + "\n}\n"
            "static void adultStage(PlayState* play, Player* p, Gfx** dList) {\n"
            "    const int limbIndex = PLAYER_LIMB_LEFT_HAND;\n" + overlay + "\n}\n")
    binary = directory / host
    flags = (["-DCOMBO_BUILD", *oot_flags()] if host == "soh"
             else ["-std=c++20", "-DNEI_EQUIPMENT_MM", *mm_flags()])
    bridge = ([str(ROOT / "mm/2s2h/Rando/NeiResourceRouting.cpp"),
               "-Wl,--export-dynamic-symbol=OOT_NeiResourceExists",
               "-Wl,--export-dynamic-symbol=OOT_NeiResourceIsMod",
               "-Wl,--export-dynamic-symbol=OOT_NeiEnsureGiBaseOwner"] if host == "mm" else [])
    subprocess.run([os.environ.get("CXX", "c++"), *flags, "-w", "-I" + str(directory),
                    str(ROOT / "tests/nei_held/four_sword_visibility_test.cpp"), *bridge,
                    "-o", str(binary)], check=True)
    cases = ([sys.argv[2:]] if len(sys.argv) > 2
             else [[], ["body"], ["graph"], ["limb" if host == "mm" else "late"],
                   ["vanilla"], ["vanilla-missing"]])
    for case in cases:
        subprocess.run([str(binary), *case], check=True)


def gi(directory):
    source = (ROOT / "mm/2s2h/Rando/NeiGiPresentation.cpp").read_text()
    owner = (ROOT / "soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp").read_text()
    test = r'''
#include <cassert>
#include <cstring>
#include <set>
#include <string>
#include "mm/2s2h/Rando/Types.h"
#include "combo/menu/ComboItemDrawABI.h"
#include "soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
using NeiGi::Kind;
struct Vec3f { float x,y,z; };
struct Presentation {
    void (*draw)(); const char* opaque; const char* translucent;
    float scale; Kind effect; Vec3f effectCenter; bool alwaysShimmer;
    Kind shimmer=Kind::Neutral;
};
static bool altOn, din;
#define CVAR_ENHANCEMENT(name) name
#define CVAR_NEI_GI_EFFECTS "ItemEffects"
int CVarGetInteger(const char* name,int) { return !strcmp(name,"DinFireSword") && din; }
bool HasLegacyGiMod(const Presentation&,bool) { return false; }
bool HasRedesignGiMod(const Presentation&) { return false; }
bool HasResource(const char*) { return true; }
int OOT_NeiAltAssetsEnabled() { return altOn; }
int OOT_NeiResourceExists(const char*) { return true; }
void Randomizer_DrawCaneSomariaUpgrade() {}
const char* NeiGi_BaseSwordPath(const char* path) {
    static std::set<std::string> paths;
    return paths.insert(std::string("__OTR__@oot-gi-base:")+(path+7)).first->c_str();
}
static std::set<std::string> selected;
int ResourceMgr_IsIkanaShieldGiAvailable() {
    assert(false && "Four Sword fixture must not query the native shield pair"); return 0;
}
int ResourceMgr_IsModAssetForGame(const char* owner, const char* path) {
    assert(!strcmp(owner,"mm"));
    return selected.contains(path);
}
''' + function(source, "HasMmLegacyGiMod") + "\n" + function(owner, "SelectedSwordPath") + "\n" + r'''
bool HasSelectedSword(const Presentation& p,bool alt,bool (*available)(const char*)) {
    return SelectedSwordPath(p,alt,available) != nullptr;
}
''' + function(owner, "PresentationShimmer") + "\n" + function(owner, "NeiGi_FillCrossGameInfo") + r'''
int main() {
    for (const char* path : {"objects/object_gi_sword_1/gGiKokiriSwordGuardDL",
                             "objects/object_gi_sword_1/gGiKokiriSwordBladeHiltDL"}) {
        selected = {path};
        assert(HasMmLegacyGiMod(RI_SWORD_KOKIRI) && "Native Kokiri GI mods must keep priority");
        assert(!HasMmLegacyGiMod(RI_OOT_EXT_FOUR_SWORD) &&
               "A generic MM Kokiri/Din GI mod suppressed the authored OoT Four Sword");
    }
    selected.clear();
    assert(!HasMmLegacyGiMod(RI_OOT_EXT_FOUR_SWORD));
    const Presentation four{nullptr,"__OTR__objects/nei_gi_redesign/four_sword/gi_dl",nullptr,
                            1.f,Kind::FourSword,{},false};
    for (bool alt : {false,true}) for (bool fire : {false,true}) {
        altOn=alt; din=fire;
        CwItemDrawInfo info{};
        assert(!SelectedSwordPath(four,alt,HasResource));
        assert(NeiGi_FillCrossGameInfo(four,&info,alt));
        assert(info.drawKind==CW_DRAW_KIND_NEI_GI && info.dlistCount==1 && info.neiEffect==int(Kind::FourSword));
        const std::string owner=alt?"__OTR__":"__OTR__@oot-gi-base:";
        assert(std::string(info.dlists[0])==owner+"objects/nei_gi_redesign/four_sword/gi_dl");
        for (const char* slug : {"kokiri_sword","master_sword","biggoron_sword"}) {
            const std::string path=std::string("__OTR__objects/nei_gi_redesign/")+slug+"/gi_dl";
            Presentation ordinary=four; ordinary.opaque=path.c_str(); ordinary.alwaysShimmer=true;
            const char* chosen=SelectedSwordPath(ordinary,alt,HasResource);
            assert(bool(chosen)==alt && "Other selected sword variants lost their Alt priority");
            if(chosen) assert(bool(strstr(chosen,"/din_fire_sword/"))==fire);
        }
    }
}
'''
    path = directory / "gi.cpp"
    path.write_text(test)
    binary = directory / "gi"
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-I" + str(ROOT),
                    str(path), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
    print("PASS Four Sword GI: canonical authored owner in both Alt states; generic MM/Din variants retain only their own item")


if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="four-sword-visibility-") as temporary:
        directory = Path(temporary)
        mode = sys.argv[1] if len(sys.argv) > 1 else "all"
        if mode in {"all", "gi"}:
            gi(directory)
        if mode in {"all", "held"}:
            for host in ["soh", "mm"]:
                if sys.argv[2:] == ["limb"] and host == "soh":
                    continue
                if sys.argv[2:] == ["late"] and host == "mm":
                    continue
                held(directory, host)
