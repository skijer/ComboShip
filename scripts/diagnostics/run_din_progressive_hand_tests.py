"""Exercise the production final OoT hand stage after equipment/PAK hooks."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def main():
    source = (ROOT / "soh/src/code/z_player_lib.c").read_text()
    start = source.index("    GameInteractor_Should(VB_PLAYER_OVERRIDE_LIMB_DRAW")
    end = source.index("    // The Switch Hook shares", start)
    stage = source[start:end]
    fixture = r'''
#define main ExistingRendererTests
#include "tests/din_fire_sword_test.c"
#undef main
#include "mods/items/logic/weapon_upgrades.h"
// This fixture selects ordinary Din swords; active Four is tested separately.
u8 WeaponUpgrade_ApplyHeldSwordDL(Gfx** dList, void* hand, Player* player, u8 r, u8 g, u8 b) {
    REQUIRE(0 && "Inactive Four Sword must not build a compound in the Din fixture");
    return 0;
}
static Gfx brokenMesh[1], pakMesh[1];
static Gfx* chosenPak;
static s32 sLeftHandType, sDListsLodOffset;
static Color_RGB8 sPlayerBodyEnvColor = { 12, 34, 56 };
Gfx* gPlayerLeftHandClosedDLs[] = { hand, hand, hand, hand };
// External hooks supply their selected mesh; the production order determines
// whether the final result is still fitted to the fire shell.
#define GameInteractor_Should(...) (*dList = brokenMesh)
#define PakLoader_GetEquipDL(...) chosenPak
#define PAK_DL_STUB ((Gfx*)-1)
#define Player_ResolveLimbDLForDummyOrLocal(path) (path)
static void FinalHand(PlayState* play, Player* this, Gfx** dList, u8 mayDrawProgressiveFire) {
    s32 limbIndex = PLAYER_LIMB_L_HAND;
''' + stage + r'''
}
int main(void) {
    setup();
    renderFairyUpgrade = 1;
    player.heldItemAction = PLAYER_IA_SWORD_BIGGORON;
    player.leftHandType = PLAYER_MODELTYPE_LH_BGS;
    gSaveContext.swordHealth = 0;
    sLeftHandType = PLAYER_MODELTYPE_LH_OPEN;
    Gfx* selected = brokenMesh;
    FinalHand(&play, &player, &selected, 1);
    REQUIRE(selected == handDL && containsLayer(selected, selected + 6, swordBlade));
    REQUIRE(sLeftHandType == PLAYER_MODELTYPE_LH_BGS);
    // No fire option: the native hook's selected mesh remains authoritative.
    enabled = 0;
    FinalHand(&play, &player, &selected, 1);
    REQUIRE(selected == brokenMesh);
    enabled = 1;
    // A hidden hand must not be reintroduced by the final fire stage.
    FinalHand(&play, &player, &selected, 0);
    REQUIRE(selected == brokenMesh);
    // The enabled progressive fire option selects its fitted blade after PAKs.
    pakActive = 1;
    chosenPak = pakMesh;
    sLeftHandType = PLAYER_MODELTYPE_LH_OPEN;
    FinalHand(&play, &player, &selected, 1);
    REQUIRE(selected == handDL && sLeftHandType == PLAYER_MODELTYPE_LH_BGS);
    enabled = 0;
    FinalHand(&play, &player, &selected, 1);
    REQUIRE(selected == pakMesh);
    enabled = 1;
    chosenPak = PAK_DL_STUB;
    sLeftHandType = PLAYER_MODELTYPE_LH_OPEN;
    FinalHand(&play, &player, &selected, 1);
    REQUIRE(selected == NULL && sLeftHandType == PLAYER_MODELTYPE_LH_OPEN);
    puts("PASS OoT final hand: fitted GFS after CustomEquipment/PAK; disabled and hidden-hand fallbacks");
}
'''
    flags = ["-std=gnu2x", "-O1", "-DNDEBUG", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
             "-Werror=implicit-function-declaration"]
    flags += ["-I" + str(ROOT / p) for p in
              ("soh/include", "soh/src", "soh/assets", "soh", "libultraship/include")]
    for path in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
        for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()):
            flags.append(f'-D{key}="{value}"')
    with tempfile.TemporaryDirectory(prefix="din-progressive-final-hand-") as temp:
        source_path = Path(temp) / "final_hand.c"
        source_path.write_text(fixture)
        binary = Path(temp) / "test"
        subprocess.run([os.environ.get("CC", "cc"), *flags, str(source_path),
                        "soh/src/code/din_fire_sword.c", "-lm", "-o", str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], check=True)

if __name__ == "__main__":
    main()
