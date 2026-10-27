"""Exercise the real OoT left-hand injection seam, including clone lifetime."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/nei_held"))
from run_articulated_tests import flags
from run_held_sword_tests import function

source = (ROOT / "soh/src/code/z_player_lib.c").read_text()
begin = source.index("        // Hide Link's held-weapon DL")
end = source.index("        // Twilight clawshot mode", begin)
stage = re.sub(r"\bthis\b", "player", source[begin:end])
stage = stage.replace("*dList = ootHand;", "*dList = (Gfx*)ootHand;")
fixture = (ROOT / "tests/nei_held/held_sword_runtime_test.cpp").read_text().split("\nint main() {")[0] + r'''
#include <array>
static Gfx openHand[1]{}, closedHand[1]{};
Gfx* gPlayerLeftHandOpenDLs[] = {openHand, openHand, openHand, openHand};
Gfx* gPlayerLeftHandClosedDLs[] = {closedHand, closedHand, closedHand, closedHand};
static s32 sLeftHandType, sDListsLodOffset;
static Color_RGB8 sPlayerBodyEnvColor = {12,34,56};
static bool hideHeld = false, extOwns = false, transformed = false;
static std::array<std::array<Gfx,8>, 4> perDraw;
static unsigned allocations = 0;
static void* allocateCompound(GraphicsContext*, size_t size) {
    assert(size == 8*sizeof(Gfx) && allocations < perDraw.size());
    return perDraw[allocations++].data();
}
#define Graph_Alloc(context, size) allocateCompound(context, size)
#define Player_ResolveLimbDLForDummyOrLocal(path) (path)
#define GameInteractor_Should(...) hideHeld
#define ExtEquip_ShouldHideSwordDL() extOwns
#define TransformMasks_IsTransformedAny() transformed
static void applyStage(PlayState* play, Player* player, Gfx** dList) {
    bool gerudoHandled = false;
    s32 limbIndex = PLAYER_LIMB_L_HAND;
    u8 mayDrawProgressiveFire = false;
''' + stage + r'''
}
int main() {
    allResources();
    Player player{}; PlayState play{}; GraphicsContext graphics{};
    play.state.gfxCtx = &graphics;
    swordHand(player, PLAYER_IA_SWORD_KOKIRI, ITEM_SWORD_KOKIRI);
    sLeftHandType = player.leftHandType;
    Gfx* first = original;
    applyStage(&play, &player, &first);
    assert(first != original && first[1].words.w1 == reinterpret_cast<uintptr_t>(closedHand) &&
           "Authored held sword was attached to an open palm instead of the native closed fist");
    assert(sLeftHandType == player.leftHandType && "New held mesh changed the native sword hand classification");
    std::array<Gfx,8> snapshot{};
    std::memcpy(snapshot.data(), first, sizeof(snapshot));
    Gfx* second = original; sPlayerBodyEnvColor = {99,88,77};
    applyStage(&play, &player, &second);
    assert(first != second && "Player and clone share a mutable deferred compound");
    assert(std::memcmp(snapshot.data(), first, sizeof(snapshot)) == 0);
    for (int mode = 0; mode < 3; ++mode) {
        hideHeld = mode == 0; extOwns = mode == 1; transformed = mode == 2;
        sLeftHandType = PLAYER_MODELTYPE_LH_CLOSED;
        Gfx* selected = original;
        applyStage(&play, &player, &selected);
        assert(selected == original && "A hidden, transformed or separately-owned weapon received an authored blade");
    }
    std::cout << "PASS OoT sword limb: native fist/type, per-draw clone lifetime and hide/transformation/weapon ownership\n";
}
'''
with tempfile.TemporaryDirectory(prefix="nei-oot-sword-limb-") as temporary:
    directory = Path(temporary)
    weapon = (ROOT / "soh/mods/items/logic/weapon_upgrades.c").read_text()
    (directory / "held_sword_bindings.inc").write_text(function(weapon, "WeaponUpgrade_ApplyHeldSwordDL"))
    path = directory / "limb.cpp"
    path.write_text(fixture)
    binary = directory / "limb"
    subprocess.run([os.environ.get("CXX", "c++"), "-DCOMBO_BUILD", *flags(), "-I" + str(directory),
                    "-I" + str(ROOT / "tests/nei_held"), str(path), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
