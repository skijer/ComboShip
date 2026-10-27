#!/usr/bin/env python3
"""Exercise the production elemental-arrow receipt encoder and native pickup adapters."""
import os
from pathlib import Path
import shlex
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def game_flags(game, combo, cpp=False):
    result = ["-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0", "-DNON_EQUIVALENT", "-DNON_MATCHING",
              "-DCONTROLLERBUTTONS_T=uint32_t", "-DMM_BUILD_DLL" if game == "mm" else "-DSOH_BUILD_DLL"]
    if combo:
        result += ["-DCOMBO_BUILD"]
    for file in ("CMake/lus-cvars.cmake", "CMake/soh-cvars.cmake" if game == "soh" else "CMake/2ship-cvars.cmake"):
        for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / file).read_text()):
            result += [f'-D{key}="{value}"']
    includes = (f"{game}/include", f"{game}/include/PR", f"{game}/src", game, f"{game}/assets", f"{game}/mods",
                f"{game}/2s2h" if game == "mm" else "soh/soh", "libultraship/include", "libultraship/src",
                "combo", "combo/menu", "combo/gui", "torch/lib/n64graphics")
    result += ["-I" + str(ROOT / path) for path in includes]
    if cpp:
        result += ["-DIMGUI_DEFINE_MATH_OPERATORS"]
    else:
        result += ["-Werror=implicit-function-declaration", "-Wno-int-conversion", "-Wno-incompatible-pointer-types",
                   "-Wno-discarded-qualifiers"]
    return result


def run_compile(command):
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(" ".join(command) + "\n" + result.stdout + result.stderr)
    return result.stderr.count("warning:")


def block(source, marker):
    begin = source.index("{", source.index(marker))
    depth, end = 1, begin + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[begin:end]


flags = ["-std=c++20", "-Wall", "-Wextra", "-Werror"]
sanitize = []
if "--sanitize" in sys.argv:
    sanitize = ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    flags += sanitize
with tempfile.TemporaryDirectory(prefix="sword-element-receipt-") as directory:
    build = Path(directory)
    widths = ""
    for file, name, output in (("soh/src/code/z_message_PAL.c", "sFontWidths", "ootFontWidths"),
                               ("mm/src/code/z_message_nes.c", "sNESFontWidths", "mmFontWidths")):
        source = (ROOT / file).read_text()
        initializer = re.search(r"f32 " + name + r"\[\d+\] = (\{.*?\n\});", source, re.S)[1]
        widths += "static const float " + output + "[] = " + initializer + ";\n"
    (build / "sword_element_font_widths.inc").write_text(widths)
    binary = build / "receipt"
    subprocess.run([*shlex.split(os.environ.get("CXX", "c++")), *flags,
                    "-I" + str(ROOT / "combo/menu"), "-I" + str(build),
                    str(ROOT / "tests/item_receipts/sword_element_receipt_test.cpp"),
                    "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
    for game in ("soh", "mm"):
        player = (ROOT / game / "src/overlays/actors/ovl_player_actor/z_player.c").read_text()
        signature = "s32 func_8084DFF4(" if game == "soh" else "s32 func_808482E0("
        receive = player[player.index(signature):]
        end = receive.index("        // Use this if we do have a getItemEntry") if game == "soh" else \
            receive.index("        if ((this->getItemId >= GI_MASK_DEKU)")
        receive = receive[:end].replace(signature, "s32 NativeReceive(", 1) + "    }\nreturn 0;\n}\n"
        catalog = ""
        if game == "mm":
            catalog = re.search(r"typedef struct GetItemEntry \{.*?\} GetItemEntry;", player, re.S)[0] + "\n"
            catalog += player[player.index("#define CHEST_ANIM_SHORT"):player.index("GetItemEntry sGetItemTable[")]
            catalog += "GetItemEntry sGetItemTable[GI_MAX - 1] = {\n"
            for name in ("FIRE", "ICE", "LIGHT"):
                row = re.search(r"// GI_ARROW_" + name + r"\n\s*(GET_ITEM\(.*?CHEST_ANIM_LONG\)),", player, re.S)[1]
                catalog += f"[GI_ARROW_{name} - 1] = {row},\n"
            catalog += "};\n"
        (build / "sword_element_native_pickup.inc").write_text(catalog + receive)
        for combo in (False, True):
            binary = build / (game + ("-combo" if combo else "-solo"))
            command = [*shlex.split(os.environ.get("CC", "cc")), "-std=gnu11", "-O1", *sanitize,
                       *game_flags(game, combo), "-I" + str(build),
                       *( ["-DSWORD_RECEIPT_MM"] if game == "mm" else [] ),
                       str(ROOT / "tests/item_receipts/sword_element_pickup_test.c"), "-o", str(binary)]
            run_compile(command)
            subprocess.run([str(binary)], check=True)
    catalog_source = (ROOT / "mm/2s2h/Rando/StaticData/Items.cpp").read_text()
    catalog = catalog_source[catalog_source.index("#define RI("):catalog_source.index("// clang-format off")]
    catalog += "\nnamespace Rando::StaticData { std::map<RandoItemId, RandoStaticItem> Items = {\n"
    for id in ("RI_UNKNOWN", "RI_ARROW_FIRE", "RI_ARROW_ICE", "RI_ARROW_LIGHT"):
        catalog += re.search(r"^\s*RI\(" + id + r",[^\n]*", catalog_source, re.M)[0] + "\n"
    catalog += "}; }\n#undef RI\n"
    font_source = (ROOT / "mm/src/code/z_message_nes.c").read_text()
    font = re.search(r"f32 sNESFontWidths\[160\] = (\{.*?\n\});", font_source, re.S)[1]
    catalog += 'extern "C" { float sNESFontWidths[160] = ' + font + "; }\n"
    (build / "sword_element_queue_catalog.inc").write_text(catalog)
    queue = (ROOT / "mm/2s2h/Rando/MiscBehavior/CheckQueue.cpp").read_text()
    fixture = (ROOT / "tests/item_receipts/sword_element_queue_test.cpp").read_text()
    fixture = fixture.replace("/* QUEUE_RECEIVE */", "void Receive(Actor* actor, PlayState* play) " + block(queue, ".giveItem ="))
    (build / "queue.cpp").write_text(fixture)
    for combo in (False, True):
        binary = build / ("queue-combo" if combo else "queue-solo")
        run_compile([*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-O1", *sanitize,
                     *game_flags("mm", combo, True), "-I" + str(build), "-ffunction-sections", "-fdata-sections",
                     str(build / "queue.cpp"), str(ROOT / "mm/2s2h/CustomMessage/CustomMessage.cpp"),
                     "-Wl,--gc-sections", "-o", str(binary)])
        subprocess.run([str(binary)], check=True)
    header_source = (ROOT / "mm/src/code/z_message.c").read_text()
    native_header = "\n".join(re.search(r"^#define " + name + r"[^\n]*", header_source, re.M)[0]
                              for name in ("MESSAGE_CUSTOM_ICON_ITEM", "MESSAGE_ITEM_NONE"))
    native_header += "\nvoid Message_DecodeHeader(PlayState* play) " + block(header_source, "void Message_DecodeHeader(")
    (build / "sword_element_native_header.inc").write_text(native_header)
    for combo in (False, True):
        binary = build / ("decoder-combo" if combo else "decoder-solo")
        run_compile([*shlex.split(os.environ.get("CC", "cc")), "-std=gnu11", "-O1", *sanitize,
                     "-fsanitize=undefined,bounds", "-fno-sanitize-recover=all", *game_flags("mm", combo),
                     "-I" + str(build), "-ffunction-sections", "-fdata-sections",
                     str(ROOT / "tests/item_receipts/sword_element_decoder_test.c"),
                     str(ROOT / "mm/src/code/z_message_nes.c"), "-Wl,--gc-sections", "-lm", "-o", str(binary)])
        subprocess.run([str(binary)], check=True)
        negative = subprocess.run([str(binary), "--four-line-negative-control"], capture_output=True, text=True)
        assert negative.returncode and "index 3 out of bounds" in negative.stderr, negative.stderr
        print("PASS native decoder negative control: a four-line MM tutorial fails the real row bound")
    if "--native" in sys.argv:
        for game, sources in (("soh", ("soh/src/overlays/actors/ovl_player_actor/z_player.c",)),
                              ("mm", ("mm/src/overlays/actors/ovl_player_actor/z_player.c",
                                      "mm/2s2h/CustomMessage/CustomMessage.cpp",
                                      "mm/2s2h/Rando/MiscBehavior/CheckQueue.cpp"))):
            for combo in (False, True):
                for index, source in enumerate(sources):
                    cpp = source.endswith(".cpp")
                    pch = []
                    if cpp:
                        for header in ("libultraship/bridge/consolevariablebridge.h", "ship/Context.h", "ship/window/Window.h"):
                            pch += ["-include", header]
                    warnings = run_compile([*shlex.split(os.environ.get("CXX" if cpp else "CC", "c++" if cpp else "cc")),
                                           "-std=gnu++20" if cpp else "-std=gnu11", *game_flags(game, combo, cpp), *pch,
                                           "-c", str(ROOT / source), "-o", str(build / f"{game}-{combo}-{index}.o")])
                    print(f"PASS full native object compile: {source} ({'combo' if combo else 'solo'}, {warnings} compiler warnings)")
