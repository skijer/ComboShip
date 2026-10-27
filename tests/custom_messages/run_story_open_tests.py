#!/usr/bin/env python3
"""Exercise production story hooks through native OoT message opening/decoding."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/nei_item_stow"))
from run_ballchain_tests import flags as native_flags


def function(source, signature):
    start = source.index(signature)
    cursor, depth = source.index("{", start) + 1, 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[start:cursor]


def run(*args):
    subprocess.run([str(arg) for arg in args], cwd=ROOT, check=True,
                   env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


native = (ROOT / "soh/src/code/z_message_PAL.c").read_text()
hook = (ROOT / "soh/soh/Enhancements/game-interactor/GameInteractor_Hooks.cpp").read_text()
util = (ROOT / "soh/soh/util.cpp").read_text()
fixture = (ROOT / "tests/custom_messages/story_open_test.cpp").read_text()
fixture = fixture.replace("/* NATIVE_COPY */", function(util, "size_t SohUtils::CopyStringToCharBuffer("))
fixture = fixture.replace("/* NATIVE_OPEN_HOOK */", function(hook, "void GameInteractor_ExecuteOnOpenText("))
# Keep native opening/table selection/reset/decoding intact. Icon/receipt,
# Japanese glyph and graphics services are unused boundaries for these texts.
engine = r'''
#include "global.h"
#include "message_data_static.h"
#include "ComboItemReceiptPresentation.h"
#include "soh/OTRGlobals.h"
#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"
#include "textures/message_static/message_static.h"
#include "textures/message_texture_static/message_texture_static.h"
#include "vt.h"
#include <stdlib.h>
#include <string.h>
static u16 sTextBoxNum, sNextTextId;
static u8 sTextboxSkipped, sMessageStartFrameCount, sLastLanguage;
static bool sDisplayNextMessageAsEnglish;
static s16 sTextIsCredits, sTextFade, sMessageHasSetSfx;
static CwItemReceiptPresentation sItemReceiptPresentation;
extern const char* msgStaticTbl[];
extern MessageTableEntry *sNesMessageEntryTablePtr, *sGerMessageEntryTablePtr, *sFraMessageEntryTablePtr;
extern MessageTableEntry *sJpnMessageEntryTablePtr, *sStaffMessageEntryTablePtr;
static void Message_ApplyItemReceiptLayout(PlayState* play) { (void)play; }
static int Message_HasItemReceiptIcon(void) { return 0; }
static void Message_DecodeJPN(PlayState* play) { (void)play; abort(); }
static void Message_LoadItemIcon(PlayState* play, u16 item, s16 y) { (void)play; (void)item; (void)y; abort(); }
bool Message_DecodeName(PlayState* play, s16* decoded, s32* texture) { (void)play; (void)decoded; (void)texture; abort(); }
void Message_SetItemReceiptPresentation(const CwItemReceiptPresentation* value) {
    memset(&sItemReceiptPresentation, 0, sizeof(sItemReceiptPresentation));
    if (value) sItemReceiptPresentation = *value;
}
void Message_SetCapeVisibilityChoice(int enabled) { if (enabled) abort(); }
void osSyncPrintfUnused(const char* format, ...) { (void)format; }
void lusprintf(const char* file, int line, int level, const char* format, ...) {
    (void)file; (void)line; (void)level; (void)format;
}
void gSPInvalidateTexCache(Gfx* packet, uintptr_t texture) { (void)packet; (void)texture; }
Vec3f gSfxDefaultPos;
f32 gSfxDefaultFreqAndVolScale;
s8 gSfxDefaultReverb;
void Audio_PlaySfxGeneral(u16 id, Vec3f* pos, u8 token, f32* freq, f32* volume, s8* reverb) {
    (void)id; (void)pos; (void)token; (void)freq; (void)volume; (void)reverb;
}
'''
engine += "\n".join(function(native, signature) for signature in (
    "void Message_FindMessageJPN(", "void Message_FindMessage(", "void Message_FindCreditsMessage(",
    "void Message_OpenText(", "void Message_StartTextbox(", "void Message_ContinueTextbox(",
    "void Message_CloseTextbox(", "void Message_Decode(PlayState*"))

with tempfile.TemporaryDirectory(prefix="story-native-open-") as directory:
    work = Path(directory)
    (work / "spdlog").mkdir()
    (work / "spdlog/spdlog.h").write_text("#pragma once\n#define SPDLOG_DEBUG(...) ((void)0)\n")
    # The formatter only consumes gPlayState from gameconsole.h. Avoid pulling
    # the unrelated JSON/CVar console implementation into this native test.
    (work / "soh/Enhancements").mkdir(parents=True)
    (work / "soh/Enhancements/gameconsole.h").write_text(
        '#pragma once\n#include "z64.h"\n#ifdef __cplusplus\nextern "C" {\n#endif\n'
        'extern PlayState* gPlayState;\n#ifdef __cplusplus\n}\n#endif\n')
    cpp, c = work / "story.cpp", work / "open.c"
    cpp.write_text(fixture)
    c.write_text(engine)
    common = ["-I" + str(work), *[flag for flag in native_flags()
                                 if not flag.startswith("-std=") and flag != "-Wno-incompatible-pointer-types"]]
    common += ["-w", "-g", "-O0", "-ffunction-sections", "-fdata-sections", "-fsanitize=address,undefined",
               "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie",
               "-DCONTROLLERBUTTONS_T=uint32_t", "-I" + str(work), "-I" + str(ROOT / "combo"),
               "-I" + str(ROOT / "combo/menu")]
    for combo in (False, True):
        selected = [*common, *(["-DCOMBO_BUILD"] if combo else [])]
        units = []
        for source in (c, ROOT / "soh/src/code/z_kanfont.c",
                       ROOT / "soh/src/overlays/actors/ovl_En_Viewer/static_story_dialogue.c",
                       ROOT / "soh/soh/Enhancements/StaticStoryDialogue.cpp",
                       ROOT / "soh/soh/Enhancements/custom-message/CustomMessageManager.cpp"):
            obj = work / (source.stem + ".o")
            is_cpp = source.suffix == ".cpp"
            run(os.environ.get("CXX", "c++") if is_cpp else os.environ.get("CC", "cc"),
                "-std=c++20" if is_cpp else "-std=gnu2x", *selected, "-c", source, "-o", obj)
            units.append(obj)
        binary = work / ("story-combo" if combo else "story-solo")
        run(os.environ.get("CXX", "c++"), "-std=c++20", *selected, cpp, *units,
            "-Wl,--gc-sections", "-no-pie", "-o", binary)
        run(binary)
