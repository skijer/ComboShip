#!/usr/bin/env python3
"""Fresh production frame capture checks; requires only a C++20 compiler."""
from pathlib import Path
import os
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parents[2]
FLAGS = ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-g", "-fsanitize=address,undefined",
         "-fno-sanitize-recover=all", "-no-pie", "-I" + str(ROOT)]
ENV = {**os.environ, "ASAN_OPTIONS": "detect_leaks=0"}
with tempfile.TemporaryDirectory(prefix="market-frame-") as temporary:
    binary = Path(temporary) / "capture"
    subprocess.run([os.environ.get("CXX", "c++"), *FLAGS,
                    str(ROOT / "tests/black_market/frame_capture_test.cpp"),
                    str(ROOT / "tests/black_market/frame_report_other_tu.cpp"), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env=ENV)
print("PASS market frame: asymmetry, selected scale/rotation/lift, corner union, disabled/nested scopes, no leak, cross TU")

sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import functions
renderer = functions((ROOT / "soh/soh/Enhancements/randomizer/NeiGiMeshRenderer.inc").read_text())
oot = functions((ROOT / "soh/soh/Enhancements/randomizer/draw.cpp").read_text())
mm = functions((ROOT / "mm/2s2h/Rando/DrawItem.cpp").read_text())
prefix = (ROOT / "tests/black_market/frame_fallback_test.cpp").read_text().split("/* PRODUCTION_REPORTERS */")[0]
reporters = "\n".join(renderer[name] for name in ("NeiGi_ReportBlackMarketBounds", "NeiGi_ReportBlackMarketModel"))
generic = prefix + reporters + "\n#define Matrix_RotateZF Matrix_RotateZ\nnamespace Oot {\n" + oot["DrawCustomItemDiamond"]
generic += "\n}\nnamespace Mm {\n" + mm["DrawNeiRealOpa"] + "\n}\n"
generic += (ROOT / "tests/black_market/frame_draw_test.cpp").read_text()
with tempfile.TemporaryDirectory(prefix="market-frame-draw-") as temporary:
    path=Path(temporary)/"draw.cpp";path.write_text(generic);binary=Path(temporary)/"draw"
    subprocess.run([os.environ.get("CXX", "c++"), *FLAGS, "-Wno-unused-parameter", str(path), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env=ENV)
source = (ROOT / "tests/black_market/frame_fallback_test.cpp").read_text()
source = source.replace("/* PRODUCTION_REPORTERS */", "\n".join(renderer[name] for name in
                        ("NeiGi_ReportBlackMarketBounds", "NeiGi_ReportBlackMarketModel")))
source = source.replace("/* PRODUCTION_OOT */", "\n".join(oot[name] for name in
    ("Randomizer_DrawExtMagicCape", "Randomizer_DrawExtPegasusAnklet", "Randomizer_DrawExtClimbBoots",
     "Randomizer_DrawExtRocBoots", "Randomizer_DrawExtTrident", "DrawMmWeaponGi", "Randomizer_DrawExtFourSwordPresentation")))
source = source.replace("/* PRODUCTION_MM */", "\n".join(mm[name] for name in
    ("DrawOotTunicTint", "DrawOotMagicSpellTint", "DrawOotNeiSpellReal", "DrawOotExtPegasusAnklet", "DrawOotExtClimbBoots",
     "DrawOotExtRocBoots", "DrawOotExtTrident")))
with tempfile.TemporaryDirectory(prefix="market-frame-fallback-") as temporary:
    path=Path(temporary)/"fallback.cpp";path.write_text(source);binary=Path(temporary)/"fallback"
    subprocess.run([os.environ.get("CXX", "c++"), *FLAGS, "-DCOMBO_BUILD", "-Wno-unused-parameter",
                    str(path), "-o", str(binary)], check=True)
    for case in ("oot-cape", "mm-cloth", "mm-spell", "mm-spell-fallback", "oot-pegasus", "oot-climb", "oot-roc",
                 "oot-trident", "mm-pegasus", "mm-climb", "mm-roc", "mm-trident", "oot-four"):
        subprocess.run([str(binary), case], check=True, env=ENV)
