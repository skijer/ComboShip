#!/usr/bin/env python3
"""Execute both native tunic fallback drawers into real GBI packets."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from run_time_pedestal_tests import functions

ROOT = Path(__file__).resolve().parents[2]
fixture = (ROOT / "tests/nei_gi/tunic_material_test.cpp").read_text()
mm = functions((ROOT / "mm/2s2h/Rando/DrawItem.cpp").read_text())
oot = functions((ROOT / "soh/soh/Enhancements/randomizer/draw.cpp").read_text())
flags = ["-std=c++20", "-DF3DEX_GBI_2", "-I" + str(ROOT), "-I" + str(ROOT / "libultraship/include")]
if "--sanitize" in sys.argv:
    flags += ["-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
with tempfile.TemporaryDirectory(prefix="tunic-material-") as td:
    for host, combo in (("mm", False), ("mm", True), ("oot", False), ("oot", True)):
        source = ROOT / ("mm/src/code/z_rcp.c" if host == "mm" else "soh/src/code/z_rcp.c")
        # Use the real native SETUPDL_25: it configures the combiner and fog
        # without setting primitive/environment colors. Do not mock that state.
        setup = re.search(r"/\* SETUPDL_25 \*/(.*?)\n\s*\},", source.read_text(), re.S)[1]
        if host == "mm":
            names = ("LoadNeiLegacyGfx", "DrawOotMedallion", "DrawOotGetItemOpaOpaTint", "DrawOotTunicTint",
                     "DrawOotExtSpiritBreastplate", "DrawOotExtChampionsTunic", "DrawOotExtSagesTunic")
            drawers = "\n".join(mm[name] for name in names)
        else:
            names = ("DrawCustomItemDiamondTint", "DrawCustomTunicTint", "Randomizer_DrawExtSpiritBreastplate",
                     "Randomizer_DrawExtChampionsTunic", "Randomizer_DrawExtSagesTunic")
            drawers = "\n".join(oot[name] for name in names if name in oot)
        text = fixture.replace("/* NATIVE_SETUP */", "Gfx setup25[] = {" + setup + "\n};")
        text = text.replace("/* PRODUCTION_DRAWERS */", drawers)
        path = Path(td) / f"{host}-{combo}.cpp"
        path.write_text(text)
        binary = path.with_suffix("")
        mode = (["-DTEST_OOT_TUNIC"] if host == "oot" else []) + (["-DCOMBO_BUILD"] if combo else [])
        subprocess.run([os.environ.get("CXX", "c++"), *flags, *mode, str(path), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
