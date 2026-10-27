#!/usr/bin/env python3
"""Run portal/save-load and local shop/forest arrivals through native fade code.

The real Play_Init selection, instance initialization and fade arithmetic execute
with native engine types. Disk loading, transition factory dispatch and the GPU
are boundaries; rendered smoothness and Djipi art still require an in-game check.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import block_from

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--game", choices=("mm", "soh"))
parser.add_argument("--source-ref", help="Compare a committed portal implementation against the current native behavior")
args = parser.parse_args()


def function(source, signature):
    return block_from(source, source.index(signature))


def native_fade(game):
    mm = game == "mm"
    play = (ROOT / game / "src/code/z_play.c").read_text()
    fade = (ROOT / game / "src/code/z_fbdemo_fade.c").read_text()
    library = (ROOT / game / "src/code/z_lib.c").read_text()
    pointer = "self" if mm else "play"
    start = play.index("    " + ("this" if mm else "play") + "->transitionTrigger = TRANS_TRIGGER_END;",
                       play.index("void Play_Init("))
    end = play.index("    " + ("TransitionFade_Init" if mm else "ShrinkWindow_Init"), start)
    selection = re.sub(r"\bthis\b", "self", play[start:end])
    start = play.index("case TRANS_MODE_INSTANCE_INIT:")
    end = play.index("case TRANS_MODE_INSTANCE_RUNNING:", start)
    instance = re.sub(r"\bthis\b", "self", play[start:end])
    definitions = fade[fade.index("typedef enum TransitionFadeDirection"):fade.index("static Gfx")] if mm else ""
    definitions += "\n".join(re.sub(r"\bthis\b", "self", function(fade, signature)) for signature in (
        "void TransitionFade_Start(", "void* TransitionFade_Init(", "void TransitionFade_Destroy(",
        "void TransitionFade_Update(", "s32 TransitionFade_IsDone(", "void TransitionFade_SetColor(",
        "void TransitionFade_SetType("))
    data = "instanceData" if mm else "data"
    color = "color" if mm else "fadeColor"
    return r'''
#include "regs.h"
#include <cassert>
#include <initializer_list>
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
#define VT_COL(...) ""
#define VT_RST ""
''' + ("static RegEditor editor{};\nRegEditor* gRegEditor = &editor;\n" if mm else
       "static GameInfo gameInfo{};\nGameInfo* gGameInfo = &gameInfo;\n") + re.sub(
           r"\bthis\b", "self", function(library, "s32 Math_StepToS(")) + definitions + r'''
static void SelectNativeArrival(PlayState* POINTER) {
    int sceneLayer = 0, baseSceneLayer = 0;
SELECTION
}
static void InitializeNativeFade(PlayState* POINTER) {
    switch (TRANS_MODE_INSTANCE_INIT) {
INSTANCE
        default: break;
    }
}
static void CheckArrivalFade() {
    PlayState destination{};
    auto* play = &destination;
    SelectNativeArrival(play);
    require(play->transitionTrigger == TRANS_TRIGGER_END && play->transitionType == TRANS_TYPE_FADE_BLACK,
            "arrival must select a native black fade after loading the destination, never instant or a stale effect");
    require(gSaveContext.nextTransitionType == TRANS_NEXT_TYPE_DEFAULT,
            "Play_Init must consume the arrival override exactly once");
    auto& ctx = play->transitionCtx;
    ctx.transitionType = play->transitionType;
    ctx.init = TransitionFade_Init; ctx.setType = TransitionFade_SetType;
    ctx.setColor = TransitionFade_SetColor; ctx.start = TransitionFade_Start;
    InitializeNativeFade(play);
    auto* fade = reinterpret_cast<TransitionFade*>(&ctx.DATA);
    require(fade->COLOR.a == 255 && !TransitionFade_IsDone(fade),
            "the first native arrival frame must fully cover the destination");
    int previous = 255, intermediate = 0;
    for (int frame = 0; frame < 100 && !TransitionFade_IsDone(fade); ++frame) {
        TransitionFade_Update(fade, 3);
        require(fade->COLOR.a <= previous, "native arrival alpha must decrease monotonically");
        if (fade->COLOR.a > 0 && fade->COLOR.a < 255) ++intermediate;
        previous = fade->COLOR.a;
    }
    require(TransitionFade_IsDone(fade) && fade->COLOR.a == 0 && intermediate >= 5,
            "native arrival must reveal the destination over multiple frames and finish clear");
}
'''.replace("POINTER", pointer).replace("SELECTION", selection).replace("INSTANCE\n", instance + "\n").replace(
        "DATA", data).replace("COLOR", color)


def portal_source(path):
    if args.source_ref:
        return subprocess.check_output(["git", "show", args.source_ref + ":" + path], cwd=ROOT).decode("utf-8-sig")
    return (ROOT / path).read_text(encoding="utf-8-sig")


port = portal_source("mm/2s2h/BenPort.cpp")
oot = portal_source("soh/soh/OTRGlobals.cpp")

with tempfile.TemporaryDirectory(prefix="forest-native-arrival-") as temporary:
    for game, host in (("mm", port), ("soh", oot)):
        if args.game and args.game != game:
            continue
        mm = game == "mm"
        fixture = (ROOT / "tests/lost_woods_entrance" / ("mm_arrival_test.cpp" if mm else "oot_arrival_test.cpp"))
        source = fixture.read_text().split("int main()")[0]
        # These older fixtures model disk I/O with memset-only initialization.
        # Mirror the real initializer's pending-effect default before loading.
        common = (ROOT / game / "src/code/z_common_data.c").read_text()
        default = re.search(r"gSaveContext.nextTransitionType\s*=\s*[^;]+;", common)[0]
        source = source.replace("std::memset(&gSaveContext, 0, sizeof(gSaveContext));",
                                "std::memset(&gSaveContext, 0, sizeof(gSaveContext)); " + default)
        arrival = "MM_ApplyComboPortalArrival" if mm else "SOH_ApplyComboPortalArrival"
        source = source.replace("/* ARRIVAL_BODY */", function(host, 'extern "C" void ' + arrival + '('))
        title = (ROOT / game / "src/code/title_setup.c").read_text()
        boot = function(title, "void Setup_InitImpl(" if mm else "void TitleSetup_InitImpl(")
        source = source.replace("/* BOOT_BODY */", re.sub(r"\bthis\b", "self", boot))
        title_owner = "MM" if mm else "SOH"
        for operation, result in (("Request", "void"), ("Consume", "int")):
            source += "\n" + function(title, result + " Combo_" + title_owner + "_" + operation + "CrossGameTitleCard(") + "\n"
        source = source.replace("/* ROUTE_BODY */", "\n".join(function(host, signature) for signature in (
            "static void Combo_CancelLostWoodsPassage(", "static void Combo_ClearReturnRequests(",
            'extern "C" COMBO_EXPORT void MM_SetComboEntryIsResume(',
            'extern "C" COMBO_EXPORT void MM_SetComboPortalRoute(')) if mm else "")
        if mm:
            table = (ROOT / "mm/src/code/z_scene_table.c").read_text()
            for name in ("sLostWoodsEntrance0", "sWoodsOfMysteryEntrance0"):
                source += re.search(r"static EntranceTableEntry " + name + r"\[\] = \{.*?\n\};", table, re.S)[0]
            source += r'''
extern "C" s32 Entrance_GetTransitionFlags(u16 entrance) {
    return (entrance >> 9) == ENTR_SCENE_LOST_WOODS ? sLostWoodsEntrance0[0].flags : sWoodsOfMysteryEntrance0[0].flags;
}
'''
        else:
            table = (ROOT / "soh/src/code/z_scene_table.c").read_text()
            source += table[table.index("#define DEFINE_ENTRANCE("):table.index("#undef DEFINE_ENTRANCE")]
            source += "\n#undef DEFINE_ENTRANCE\n"
        source += native_fade(game)
        market = (ROOT / ("mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp" if mm else
                          "soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp")).read_text()
        forest = (ROOT / ("mm/2s2h/Enhancements/Gameplay/ComboSoloForest.cpp" if mm else
                          "soh/soh/Enhancements/Gameplay/ComboSoloForest.cpp")).read_text()
        source += function(market, "void Transition(") + "\n" + function(forest, "void ForestTransition(")
        source += r'''
int main() {
    for (int route : {COMBO_PORTAL_LOST_WOODS, COMBO_PORTAL_OPENING_FOREST, MIRROR_ROUTE}) {
        for (int slot = 0; slot < 3; ++slot) {
BOOT
            CheckArrivalFade();
        }
    }
    // A fresh route must replace a stale/instant effect from a previous load.
    for (int route : {COMBO_PORTAL_LOST_WOODS, COMBO_PORTAL_OPENING_FOREST, MIRROR_ROUTE}) {
        gSaveContext.nextTransitionType = TRANS_TYPE_INSTANT;
STALE_EFFECT
        CheckArrivalFade();
    }
    // A missing/stale forest request must not own another arrival's effect.
GUARDS
    for (bool shop : {false, true}) {
        PlayState departure{};
        CONSUME_TITLE(); // discard the earlier boot request before testing a local transition
        if (shop) Transition(&departure, 0); else ForestTransition(&departure, 0);
        require(CONSUME_TITLE() && !CONSUME_TITLE(),
                "explicit local shop/forest routes must request exactly one destination title");
        require(departure.transitionTrigger == TRANS_TRIGGER_START &&
                departure.transitionType == TRANS_TYPE_FADE_BLACK,
                "local shop/forest routes must still fade out natively");
        CheckArrivalFade();
    }
    std::puts("PASS: native portal/save-load and shop/solo-forest arrivals start black, reveal gradually, consume once and preserve other arrivals");
}
'''.replace("CONSUME_TITLE", "Combo_" + title_owner + "_ConsumeCrossGameTitleCard").replace(
    "MIRROR_ROUTE", "COMBO_PORTAL_MAGIC_MIRROR" if mm else "COMBO_PORTAL_OPENING_FOREST").replace("BOOT", r'''
            SetupState state{};
            gComboStartFileNum = slot;
            MM_SetComboEntryIsResume(0);
            MM_SetComboPortalRoute(route);
            Setup_InitImpl(&state);
''' if mm else r'''
            GameState state{};
            gComboReturnFileNum = slot;
            sComboReturnPortalRoute = route;
            TitleSetup_InitImpl(&state);
''').replace("STALE_EFFECT", r'''
        MM_SetComboPortalRoute(route);
        MM_ApplyComboPortalArrival();
''' if mm else r'''
        sComboReturnPortalRoute = route;
        SOH_ApplyComboPortalArrival(1);
''').replace("GUARDS", r'''
    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_WHITE;
    MM_SetComboPortalRoute(COMBO_PORTAL_MARKET);
    MM_ApplyComboPortalArrival();
    require(gSaveContext.nextTransitionType == TRANS_TYPE_FADE_WHITE, "ordinary/resume route must retain its native effect");
''' if mm else r'''
    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_WHITE;
    sComboReturnPortalRoute = COMBO_PORTAL_OPENING_FOREST;
    SOH_ApplyComboPortalArrival(0);
    require(gSaveContext.nextTransitionType == TRANS_TYPE_FADE_WHITE &&
            sComboReturnPortalRoute == COMBO_PORTAL_MARKET,
            "a superseded forest route must not replace a composed/Farore arrival's effect");
''')
        source = '#include "ComboGameMode.h"\nextern "C" { int gComboGameMode = COMBO_GAME_BOTH; }\n' + source
        cpp, binary = Path(temporary) / (game + ".cpp"), Path(temporary) / game
        cpp.write_text(source)
        includes = [game, game + ("/2s2h" if mm else "/soh"), game + "/include", game + "/include/PR",
                    game + "/src", game + "/assets", "libultraship/include", "combo"]
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DCOMBO_BUILD", "-DF3DEX_GBI_2",
                        "-DFMT_HEADER_ONLY", "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                        *["-I" + str(ROOT / path) for path in includes], str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
