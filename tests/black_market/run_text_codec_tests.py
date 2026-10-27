#!/usr/bin/env python3
"""Execute each actual MarketText with its native formatter, decoder and font.

Native engine layouts, merchandise catalogue/ID policy, metadata rows, formatter,
decoder and glyph loader are production code. Hook registration, save services,
graphics allocation and item-icon services are boundary doubles; no game runs.
"""
import argparse
import os
import re
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def browsing_text_adapter():
    source = (ROOT / 'combo/BlackMarketBrowsingNative.h').read_text()
    return ('\n#include <array>\nstatic bool sBrowseTextboxSaved;\n'
            'static std::array<s16, 6> sBrowseTextboxLayout{};\n' +
            function(source, 'void ContinueBrowsingText('))


def render_controls(folder, host, source, flags):
    def case(label, following):
        begin = source.index("case " + label + ":")
        return source[begin:source.index("case " + following + ":", begin)]
    mm = host == "mm"
    body = r'''
#include "global.h"
#include "message_data_static.h"
#include "vt.h"
#include <stdlib.h>
static void Market_EndIcon(Font* font, u16 icon) { (void)font; (void)icon; }
static void Market_Ocarina(PlayState* play) { (void)play; abort(); }
static void Market_Action(PlayState* play, u16 action) { (void)play; (void)action; }
static void Market_Audio(u16 sound, ...) { (void)sound; }
#define Font_LoadMessageBoxEndIcon Market_EndIcon
#define Font_LoadMessageBoxIcon Market_EndIcon
#define Message_HandleOcarina Market_Ocarina
#define Interface_SetAButtonDoAction Market_Action
#define Interface_SetDoAction Market_Action
#define Audio_PlaySfx Market_Audio
#define Audio_PlaySfxGeneral Market_Audio
'''
    if not mm:
        body += '#include "soh/Enhancements/game-interactor/GameInteractor_Hooks.h"\n'
        body += 'Vec3f gSfxDefaultPos; f32 gSfxDefaultFreqAndVolScale; s8 gSfxDefaultReverb;\n'
    else:
        body = body.replace('#include "message_data_static.h"',
                            '#include "message_data_static.h"\n#include "message_data_fmt_nes.h"')
    decoded = "msgCtx->decodedBuffer.schar" if mm else "msgCtx->msgBufDecoded"
    body += "void MarketTest_RenderControls(PlayState* play) {\n"
    body += "MessageContext* msgCtx = &play->msgCtx; Font* font = &msgCtx->font;\nGfx* gfx = NULL; Gfx** gfxP = &gfx;\n"
    body += f"for (u16 i = 0; i <= msgCtx->decodedTextLen; ++i) {{ u16 character = (u8){decoded}[i];\nswitch(character) {{\n"
    body += case("MESSAGE_TWO_CHOICE", "MESSAGE_THREE_CHOICE")
    body += case("MESSAGE_CONTINUE" if mm else "MESSAGE_END", "MESSAGE_PERSISTENT" if mm else "MESSAGE_OCARINA")
    body += case("MESSAGE_EVENT", "MESSAGE_EVENT2" if mm else "MESSAGE_NAME")
    body += "default: " + ("" if mm else "if (character == MESSAGE_COLOR) ++i; ") + "break;\n}\n}\n}\n"
    lifecycle = (ROOT / ("mm/src/code/z_message.c" if mm else "soh/src/code/z_message_PAL.c")).read_text()
    if not mm:
        # Keep the whole native Start/Continue/Close wrappers. Their OpenText
        # boundary loads actual MarketText and runs the native common reset;
        # native scene/HUD/table selection is outside this focused codec test.
        native_open = function(lifecycle, "void Message_OpenText(")
        reset = native_open[native_open.index("    msgCtx->choiceNum = msgCtx->textUnskippable"):native_open.rindex("}")]
        body += "extern void MarketTest_LoadMarketText(PlayState*, u16);\n"
        body += "static void MarketTest_Open(PlayState* play, u16 textId) {\nMessageContext* msgCtx = &play->msgCtx;\n"
        body += "MarketTest_LoadMarketText(play, textId);\nmsgCtx->textId = textId;\n" + reset + "}\n"
        body += "#define Message_OpenText MarketTest_Open\n"
        body += "\n".join(function(lifecycle, signature) for signature in
                          ("void Message_StartTextbox(", "void Message_ContinueTextbox(", "void Message_CloseTextbox("))
    # Execute the exact native closing timer and common reset block. Song,
    # notebook, heart-container and HUD side branches are separate services;
    # the fixture uses ordinary market IDs with those inactive.
    closing = lifecycle[lifecycle.index("case MSGMODE_TEXT_CLOSING:", lifecycle.index("void Message_Update(")):]
    prefix_end = closing.index("            if (sLastPlayedSong" if mm else "            if ((msgCtx->textId")
    reset_begin = closing.index("            msgCtx->msgLength = 0;")
    reset_end = closing.index("            if (EQ_MAX_QUEST_HEART_PIECE_COUNT)" if mm else
                              "            if ((s32)(gSaveContext.inventory.questItems", reset_begin)
    body += "\nvoid MarketTest_CloseTick(PlayState* play) {\nMessageContext* msgCtx = &play->msgCtx;\n"
    body += "PauseContext* pauseCtx = &play->pauseCtx; InterfaceContext* interfaceCtx = &play->interfaceCtx;\n"
    body += "switch (msgCtx->msgMode) {\n" + closing[:prefix_end] + closing[reset_begin:reset_end]
    body += "break; default: abort(); }\n}\n"
    path = folder / (host + "_render_controls.c")
    path.write_text(body)
    obj = path.with_suffix(".o")
    subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *flags, "-c", str(path), "-o", str(obj)], check=True)
    return str(obj)


def run_mm(folder):
    source = (ROOT / "mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp").read_text()
    custom = (ROOT / "mm/2s2h/CustomMessage/CustomMessage.cpp").read_text()
    items = (ROOT / "mm/2s2h/Rando/StaticData/Items.cpp").read_text()
    ids = (ROOT / "mm/2s2h/Enhancements/Gameplay/BlackMarketItems.cpp").read_text()
    helpers = "\n".join(function(custom, signature) for signature in (
        "void CustomMessage::Replace(", "void CustomMessage::ReplaceColorChars(",
        "void CustomMessage::AddLineBreaks(", "void CustomMessage::EnsureMessageEnd(",
        "void CustomMessage::LoadCustomMessageIntoFont(",
    ))
    catalogue = (ROOT / "combo/rando/BlackMarket.h").read_text()
    keys = set(re.findall(r'"(RI_[A-Z_0-9]+)"', catalogue)) | {"RI_UNKNOWN"}
    rows = [line.strip() for line in items.splitlines()
            if (match := re.match(r"\s*RI\((RI_[A-Z_0-9]+),", line)) and match[1] in keys]
    metadata = "namespace Rando::StaticData {\n" + items[items.index("#define RI("):items.index("// clang-format off")]
    metadata += "\nstd::map<RandoItemId, RandoStaticItem> Items = {\n" + "\n".join(rows) + "\n};\n#undef RI\n"
    metadata += function(items, "RandoItemId GetItemIdFromName(") + "\n}\nnamespace ComboBlackMarketItems {\n"
    metadata += function(ids, "RandoItemId Id(") + "\n}\n"
    fixture = (ROOT / "tests/black_market/market_mm_text_codec_test.cpp").read_text()
    fixture = fixture.replace("/* CUSTOM_MESSAGE_HELPERS */", helpers).replace("/* NATIVE_METADATA */", metadata)
    fixture = fixture.replace("/* MARKET_TEXT */", function(source, "const Item* Offer(") + "\n" +
                              function(source, "void MarketText("))
    includes = [ROOT, *[ROOT / "mm" / p for p in ("", "include", "include/PR", "src", "assets", "mods", "2s2h")],
                *[ROOT / p for p in ("combo", "combo/menu", "libultraship/include", "libultraship")]]
    flags = common_flags(includes) + ["-DMM_BUILD_DLL"]
    units = []
    c_sources = [ROOT / "mm/src/code" / (name + ".c")
                 for name in ("z_message", "z_message_nes", "z_message_staff", "z_kanfont")]
    # Message_OpenText resets the real optional-title context; link its C owner.
    c_sources.append(ROOT / "mm/2s2h/Enhancements/Graphics/OotLocationTitleCard.c")
    for c_source in c_sources:
        obj = folder / (c_source.stem + ".o")
        subprocess.run([os.environ.get("CC", "cc"), "-std=gnu17", *flags, "-c",
                        str(c_source), "-o", str(obj)], check=True)
        units.append(str(obj))
    units.append(render_controls(folder, "mm", (ROOT / "mm/src/code/z_message_nes.c").read_text(), flags))
    compile_and_run(folder, "mm", fixture, flags, units)


def common_flags(includes):
    return ["-g", "-O1", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2", "-DFMT_HEADER_ONLY",
            "-DCONTROLLERBUTTONS_T=uint32_t", "-DLOG_LEVEL_GAME_PRINTS=6",
            "-ffunction-sections", "-fdata-sections", "-fsanitize=address,undefined",
            "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie",
            *["-I" + str(path) for path in includes]]


def run_oot(folder):
    source = (ROOT / "soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp").read_text()
    ids = (ROOT / "soh/soh/Enhancements/Gameplay/ComboBlackMarketItems.cpp").read_text()
    items = (ROOT / "soh/soh/Enhancements/randomizer/item_list.cpp").read_text()
    native = (ROOT / "soh/src/code/z_message_PAL.c").read_text()
    util = (ROOT / "soh/soh/util.cpp").read_text()
    id_function = function(ids, "RandomizerGet Id(")
    keys = set(re.findall(r"\bRG_[A-Z_0-9]+\b", id_function)) - {"RG_NONE"}
    names = dict(re.findall(r"itemTable\[(RG_[A-Z_0-9]+)\]\s*=\s*Item\(\s*\1\s*,\s*(Text\{[^}]*\})", items))
    assert keys <= names.keys(), keys - names.keys()
    metadata = "static std::map<RandomizerGet, Rando::Item> metadata;\n"
    metadata += "Rando::Item& Rando::StaticData::RetrieveItem(RandomizerGet id) { return metadata[id]; }\n"
    metadata += "namespace ComboBlackMarketOotItems { using ComboRando::BlackMarket::Item;\n" + id_function + "\n}\n"
    metadata += "static void InitMetadata() {\n" + "\n".join(
        f"metadata[{key}] = Rando::Item({key}, {names[key]}, ITEMTYPE_ITEM, 0, false, LOGIC_NONE, RHT_NONE, ITEM_CATEGORY_MAJOR);"
        for key in sorted(keys)) + "\n}\n"
    fixture = (ROOT / "tests/black_market/market_oot_text_codec_test.cpp").read_text()
    fixture = fixture.replace("/* NATIVE_METADATA */", metadata)
    fixture = fixture.replace("/* NATIVE_COPY */", function(util, "size_t SohUtils::CopyStringToCharBuffer("))
    fixture = fixture.replace("/* MARKET_TEXT */", function(source, "const Item* Offer(") + "\n" +
                              function(source, "void MarketText(") + browsing_text_adapter())
    # Retain the production decoder exactly. The item-icon/layout and Japanese
    # services are unused boundaries for this icon-less NES merchandise path.
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
static void Message_LoadItemIcon(PlayState* play, u16 item, s16 y) { (void)play; (void)item; (void)y; abort(); }
bool Message_DecodeName(PlayState* play, s16* decoded, s32* texture) { (void)play; (void)decoded; (void)texture; abort(); }
void osSyncPrintfUnused(const char* format, ...) { (void)format; }
void gSPInvalidateTexCache(Gfx* packet, uintptr_t texture) { (void)packet; (void)texture; }
bool GameInteractor_Should(GIVanillaBehavior behavior, uint32_t result, ...) { (void)behavior; return result; }
'''
    decoder += function(native, "void Message_Decode(PlayState*") + "\n" + function(native, "u8 Message_GetState(")
    c_path = folder / "oot_decoder.c"
    c_path.write_text(decoder)
    includes = [ROOT, *[ROOT / "soh" / p for p in ("include", "include/PR", "", "soh", "src", "assets", "mods")],
                *[ROOT / p for p in ("combo", "combo/menu", "libultraship/include", "libultraship")]]
    # Native item metadata reaches ImGui and ResourceManager headers. This gate
    # runs outside the legacy NEI runner's CPATH scope, but uses the same pins.
    includes += [Path(path) for path in os.environ.get(
        "COMBO_NEI_FIXTURE_CPATH", "/usr/include/imgui").split(os.pathsep) if path]
    flags = common_flags(includes) + ['-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"', '-DBLACK_MARKET_BROWSING_OOT']
    units = []
    for path in (c_path, ROOT / "soh/src/code/z_kanfont.c",
                 ROOT / "soh/soh/Enhancements/custom-message/CustomMessageManager.cpp",
                 ROOT / "soh/soh/Enhancements/custom-message/text.cpp",
                 ROOT / "soh/soh/Enhancements/randomizer/item.cpp"):
        obj = folder / ("oot_" + path.stem + ".o")
        compiler = os.environ.get("CXX", "c++") if path.suffix == ".cpp" else os.environ.get("CC", "cc")
        standard = "-std=c++20" if path.suffix == ".cpp" else "-std=gnu2x"
        subprocess.run([compiler, standard, *flags, "-c", str(path), "-o", str(obj)], check=True)
        units.append(str(obj))
    units.append(render_controls(folder, "oot", native, flags))
    compile_and_run(folder, "oot", fixture, flags, units)


def compile_and_run(folder, host, fixture, flags, units):
    cpp = folder / (host + ".cpp")
    cpp.write_text(fixture)
    binary = folder / host
    subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", *flags, str(cpp),
                    *units, "-Wl,--gc-sections", "-ldl", "-no-pie", "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, timeout=20,
                   env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--game", choices=("mm", "oot", "both"), default="both")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="market-text-codec-") as directory:
        folder = Path(directory)
        if args.game in ("mm", "both"):
            run_mm(folder)
        if args.game in ("oot", "both"):
            run_oot(folder)
