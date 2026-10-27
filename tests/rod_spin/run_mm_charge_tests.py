"""Verify the unchanged custom MM rod charge threshold and release path.

Execute production raw-input polling, equip update, handler and charge functions
with real MM structures. Unrelated projectile, audio, graphics, first-person and
attack-setup calls are engine boundaries; native sword charge is tested beside
this fixture by native_mm_test.c.
"""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import functions
from run_mm_nei_tests import flags


def main():
    helper = functions((ROOT / "mm/mods/items/helpers/equip_helper.c").read_text(),
                       ("ItemInput_CheckOtherButtons", "ItemInput_Update", "ItemInput_CheckDamage", "ItemEquip_Update"))
    common = r'''
#include "mods/items/custom_items.h"
#include "mods/items/helpers/equip_helper.h"
#include "mods/items/logic/item_rod_common.h"
#include "mods/extended_player.h"
#include "functions.h"
#include "macros.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
CustomItemState gCustomItemState;
static u16 sStowedItemButtons[256];
static u16 equippedButton;
static int consumes, releasedMwa, releases, sparks;
u16 ItemInput_GetEquippedButton(u8 item, PlayState* play) { return equippedButton; }
u8 Pacci_UltrahandModeActive(void) { return 0; }
u8 ItemInput_IsBlockedEx(Player* player, PlayState* play, u8 skip) { return 0; }
void Actor_PlaySfx(Actor* actor, u16 id) {}
void AudioSfx_StopById(u32 id) {}
void ItemMagic_Consume(PlayState* play, s16 amount) { consumes += amount; }
void RodCommon_PreserveChargeSparkCadence(PlayState* play, Vec3f* pos, RodColor* color) { ++sparks; }
void func_80837948(PlayState* play, Player* player, s32 mwa) {
    assert(mwa >= PLAYER_MWA_SPIN_ATTACK_1H && mwa <= PLAYER_MWA_BIG_SPIN_2H);
    releasedMwa = mwa; ++releases;
}
'''
    with tempfile.TemporaryDirectory(prefix="rod-custom-charge-") as td:
        directory = Path(td)
        for low in ("fire", "ice", "light"):
            cap = low.capitalize()
            stem = "s" if low == "fire" else "s" + cap
            equip = stem + "EquipState"
            raw = (ROOT / f"mm/mods/items/logic/item_rod_{low}.c").read_text()
            names = [f"{cap}Rod_{name}" for name in
                     ("CanCharge", "StartCharge", "UpdateCharge", "ReleaseCharge", "CancelCharge")]
            names.append(f"Handle_{cap}Rod")
            body = functions(raw, names)
            prefix = common + f'\n#include "mods/items/logic/item_rod_{low}.h"\n'
            prefix += "\n".join(re.findall(r'^#define \w+ gCustomItemState\.\w+.*$', raw, re.M)) + "\n"
            prefix += f'''
static ItemEquipState {equip};
static s8 {stem}PrevInvinc;
static u8 {stem}LastSwingType, {stem}ChargeButtonHeld;
static s16 {stem}ChargeHoldCounter;
static RodColor s{cap}RodColor;
static void {cap}Rod_UpdateProjectile(Player* p, PlayState* play) {{}}
static void {cap}Rod_Update{dict(fire='Flamethrower', ice='IceWave', light='LightBeam')[low]}(Player* p, PlayState* play) {{}}
static void {cap}Rod_EnterFirstPerson(Player* p, PlayState* play) {{ assert(0); }}
static void {cap}Rod_UpdateFirstPerson(Player* p, PlayState* play, ItemInputState* input) {{ assert(0); }}
static void {cap}Rod_OnEquip(PlayState* play, Player* p) {{ {low}RodActive = 1; }}
static void {cap}Rod_OnUnequip(PlayState* play, Player* p) {{ {low}RodActive = 0; }}
static u8 {cap}Rod_CheckBackfire(Player* p, PlayState* play, s16 cost, u8 chance) {{ return 0; }}
static void {cap}Rod_ProcessSwing(Player* p, PlayState* play) {{}}
static void {cap}Rod_StopSpin{cap}(void) {{ {low}RodSpinActive = 0; }}
'''
            prefix += "\n".join(helper.values()) + "\n" + "\n".join(body[name] for name in names)
            checks = f'''
int main(void) {{
    Player p = {{0}}; PlayState play = {{0}};
    p.actor.bgCheckFlags = BGCHECKFLAG_GROUND;
    p.heldItemAction = PLAYER_IA_ROD_{low.upper()};
    const u16 buttons[] = {{ BTN_B, BTN_CRIGHT, BTN_DLEFT }};
    for (unsigned b = 0; b < ARRAY_COUNT(buttons); ++b) {{
        equippedButton = buttons[b];
        for (int heldFrames = 1; heldFrames <= 10; heldFrames += (heldFrames == 1 ? 8 : 1)) {{
            memset(&gCustomItemState, 0, sizeof(gCustomItemState));
            {equip}.isEquipped = 1; {low}RodActive = 1;
            {stem}ChargeButtonHeld = {stem}ChargeHoldCounter = 0;
            consumes = releases = sparks = 0;
            for (int frame = 0; frame < heldFrames; ++frame) {{
                ++play.gameplayFrames;
                play.state.input[0].cur.button = equippedButton;
                play.state.input[0].press.button = frame == 0 ? equippedButton : 0;
                Handle_{cap}Rod(&p, &play);
            }}
            assert({low}RodCharging == (heldFrames == 10));
            if (heldFrames == 10) {{
                for (int frame = 0; frame < 6; ++frame) {{
                    ++play.gameplayFrames; play.state.input[0].press.button = 0;
                    Handle_{cap}Rod(&p, &play);
                }}
                assert({low}RodChargeLevel >= {low.upper()}_ROD_CHARGE_MIN && sparks > 0);
            }}
            ++play.gameplayFrames;
            play.state.input[0].cur.button = play.state.input[0].press.button = 0;
            Handle_{cap}Rod(&p, &play);
            assert(!{low}RodCharging);
            assert(releases == (heldFrames == 10));
            assert(consumes == (heldFrames == 10 ? {low.upper()}_ROD_MAGIC_SPIN_SMALL : 0));
            if (releases) assert(releasedMwa == PLAYER_MWA_SPIN_ATTACK_1H);
        }}
        // Preserve the existing full charge threshold and deliberate 1H release.
        {equip}.isEquipped = 1; {low}RodActive = 1;
        {stem}ChargeButtonHeld = {stem}ChargeHoldCounter = 0;
        consumes = releases = 0;
        for (int frame = 0; frame < 70; ++frame) {{
            ++play.gameplayFrames;
            play.state.input[0].cur.button = equippedButton;
            play.state.input[0].press.button = frame == 0 ? equippedButton : 0;
            Handle_{cap}Rod(&p, &play);
        }}
        assert({low}RodCharging && {low}RodChargeLevel >= {low.upper()}_ROD_CHARGE_BIG);
        ++play.gameplayFrames;
        play.state.input[0].cur.button = play.state.input[0].press.button = 0;
        Handle_{cap}Rod(&p, &play);
        assert(!{low}RodCharging && releases == 1 && releasedMwa == PLAYER_MWA_BIG_SPIN_1H);
        assert(consumes == {low.upper()}_ROD_MAGIC_SPIN_BIG);
    }}
    puts("PASS MM {cap}: raw B/C/D input, 1/9-frame taps, 10-frame charge, small/big release and existing magic costs/1H animations");
    return 0;
}}
'''
            unit = directory / f"{low}.c"
            unit.write_text(prefix + "\n" + checks)
            binary = directory / low
            extra = ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-g"] if "--sanitize" in sys.argv else []
            subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *flags(),
                            *extra, "-Werror=implicit-function-declaration", str(unit), "-o", str(binary)], check=True)
            subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
