#!/usr/bin/env python3
"""Run OoT Blast Mask behavior against the native formatter, codec and player UI.

ROM services (graphics, icon loading and actor allocation) are boundary doubles.
Production text, message formatting, NES decoding, font loading, A arbitration,
input filtering and bomb initialization execute without a running game.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/black_market"))
from run_text_codec_tests import common_flags, compile_and_run, function, render_controls


def flags():
    includes = [ROOT, *[ROOT / "soh" / p for p in
                ("include", "include/PR", "", "soh", "src", "assets", "mods")],
                *[ROOT / p for p in ("combo", "combo/menu", "libultraship/include", "libultraship")]]
    includes += [Path(path) for path in os.environ.get("COMBO_NEI_FIXTURE_CPATH", "/usr/include/imgui").split(os.pathsep) if path]
    return common_flags(includes) + ['-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"']


def codec(folder):
    native = (ROOT / "soh/src/code/z_message_PAL.c").read_text()
    source = (ROOT / "soh/mods/extended_player.c").read_text()
    row = re.search(r"Randomizer_DrawMmMask, RG_MM_MASK_BLAST,\s*(.*?)\s*\},", source, re.S)
    if not row:
        raise RuntimeError("Missing Blast Mask receipt row")
    fixture = (ROOT / "tests/blast_mask/tutorial_test.cpp").read_text()
    fixture = fixture.replace("/* TUTORIAL_TEXT */", "static const char* tutorials[] = {" + row[1] + "};")
    util = (ROOT / "soh/soh/util.cpp").read_text()
    fixture = fixture.replace("/* NATIVE_COPY */", function(util, "size_t SohUtils::CopyStringToCharBuffer("))
    decoder = r'''
#include "global.h"
#include "message_data_static.h"
#include "ComboItemReceiptPresentation.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "textures/message_static/message_static.h"
#include "textures/message_texture_static/message_texture_static.h"
#include <stdlib.h>
#include <string.h>
static u16 sTextBoxNum, sNextTextId;
static u8 sTextboxSkipped;
static bool sDisplayNextMessageAsEnglish;
static s16 sTextIsCredits, sTextFade;
static CwItemReceiptPresentation sItemReceiptPresentation;
static void Message_ApplyItemReceiptLayout(PlayState* play) { (void)play; }
static int Message_HasItemReceiptIcon(void) { return 0; }
static void Message_DecodeJPN(PlayState* play) { (void)play; abort(); }
static void Message_LoadItemIcon(PlayState* play, u16 item, s16 y) {
    (void)y; if(item != ITEM_MM_MASK_BLAST) abort();
    // The native icon loader also consumes the icon operand and sets choiceNum.
    // Keep those codec side effects; only resource copying is outside this test.
    ++play->msgCtx.msgBufPos;
    play->msgCtx.choiceNum = 1;
}
bool Message_DecodeName(PlayState* play, s16* decoded, s32* texture) { (void)play; (void)decoded; (void)texture; abort(); }
void osSyncPrintfUnused(const char* format, ...) { (void)format; }
void gSPInvalidateTexCache(Gfx* packet, uintptr_t texture) { (void)packet; (void)texture; }
bool GameInteractor_Should(GIVanillaBehavior behavior, uint32_t result, ...) { (void)behavior; return result; }
'''
    decoder += function(native, "void Message_Decode(PlayState*") + "\n" + function(native, "u8 Message_GetState(")
    c_path = folder / "decoder.c"
    c_path.write_text(decoder)
    units = []
    options = flags()
    for path in (c_path, ROOT / "soh/src/code/z_kanfont.c",
                 ROOT / "soh/soh/Enhancements/custom-message/CustomMessageManager.cpp"):
        obj = folder / (path.stem + ".o")
        compiler = os.environ.get("CXX", "c++") if path.suffix == ".cpp" else os.environ.get("CC", "cc")
        standard = "-std=c++20" if path.suffix == ".cpp" else "-std=gnu2x"
        subprocess.run([compiler, standard, *options, "-c", str(path), "-o", str(obj)], check=True)
        units.append(str(obj))
    # The shared render fixture's lifecycle hooks are unused and removed by GC.
    units.append(render_controls(folder, "oot", native, options))
    compile_and_run(folder, "tutorial", fixture, options, units)


def action(folder):
    player = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
    interface = (ROOT / "soh/src/code/z_parameter.c").read_text()
    transform = (ROOT / "soh/mods/transformation_masks/transformation_masks.c").read_text()
    mask = (ROOT / "soh/mods/transformation_masks/mm_mask_wear.cpp").read_text()
    fixture = (ROOT / "tests/blast_mask/action_test.cpp").read_text()
    table = re.search(r"static const char\* actionsTbl\[\] = \{.*?\n\};", interface, re.S)[0]
    interface_code = table + "\n" + "\n".join(function(interface, signature) for signature in (
        "void Interface_LoadActionLabel(", "void Interface_SetDoAction("))
    player_code = "\n".join(function(player, signature) for signature in (
        "s32 Player_FriendlyLockOnOrParallel(", "int Player_IsZTargeting(", "void Player_UpdateInterface("))
    player_lib = (ROOT / "soh/src/code/z_player_lib.c").read_text()
    player_code = function(player_lib, "s32 Player_InBlockingCsMode(") + "\n" + player_code
    # C's player-local identifier 'this' is reserved in the C++ fixture.
    player_code = re.sub(r"\bthis\b", "player", player_code)
    helpers = function(transform, "u8 TransformMasks_AButtonIsOffered(")
    for signature in ("extern \"C\" s32 MmMaskWear_CanUseBlastA(",
                      "extern \"C\" void MmMaskWear_FilterBlastA("):
        if signature in mask:
            helpers += "\n" + function(mask, signature)
    helpers += "\n" + function(transform, "void TransformMasks_FilterB(")
    signature = "static void MmMaskWear_UpdateBlast("
    if signature in mask:
        blast_update = function(mask, signature)
    else:
        begin = mask.index("{", mask.index("case 2: // Blast Mask", mask.index('extern "C" void MmMaskWear_Update(')))
        depth, end = 1, begin + 1
        while depth:
            depth += (mask[end] == "{") - (mask[end] == "}")
            end += 1
        body = re.sub(r"\s*break;\s*$", "", mask[begin + 1:end - 1])
        blast_update = "static void MmMaskWear_UpdateBlast(PlayState* play, Player* player) {" + body + "}\n"
    fixture = fixture.replace("/* INTERFACE */", interface_code).replace("/* PLAYER_INTERFACE */", player_code)
    fixture = fixture.replace("/* BLAST_HELPERS */", helpers).replace("/* BLAST_UPDATE */", blast_update)
    # Execute the C interface code as C, preserving its original conversions.
    interface_path = folder / "interface.c"
    interface_path.write_text('#include "global.h"\n#include "textures/do_action_static/do_action_static.h"\n'
                              '#include "assets/soh_assets.h"\n#include <string.h>\n'
                              '#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"\n' + interface_code)
    obj = folder / "interface.o"
    subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *flags(), "-c", str(interface_path), "-o", str(obj)], check=True)
    compile_and_run(folder, "action", fixture.replace(interface_code, ""), flags(), [str(obj)])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--part", choices=("codec", "action", "all"), default="all")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="blast-mask-") as directory:
        folder = Path(directory)
        if args.part in ("codec", "all"):
            codec(folder)
        if args.part in ("action", "all"):
            action(folder)


if __name__ == "__main__":
    main()
