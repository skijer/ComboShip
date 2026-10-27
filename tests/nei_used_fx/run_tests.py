import os
from pathlib import Path
import re
import subprocess
import tempfile
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import functions

def flags():
    result = ["-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0"]
    result += ["-I" + str(ROOT / p) for p in
               ("soh", "soh/include", "soh/src", "soh/assets", "soh/mods", "combo/menu", "libultraship/include")]
    for config in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
        for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / config).read_text()):
            result.append(f'-D{key}="{value}"')
    return result

if __name__ == "__main__" and "--budget-only" not in sys.argv:
    subprocess.run([sys.executable,str(ROOT/"tests/nei_used_fx/run_interpolation_test.py")],check=True)
    subprocess.run([sys.executable,str(ROOT/'tests/nei_used_fx/source_contract_test.py')],check=True)
    subprocess.run([sys.executable,str(ROOT/'tests/nei_used_fx/material_test.py')],check=True)
    subprocess.run([sys.executable,str(ROOT/'tests/nei_used_fx/attack_material_test.py')],check=True)
    subprocess.run([sys.executable,str(ROOT/'tests/nei_used_fx/cast_material_test.py')],check=True)
    subprocess.run([sys.executable,str(ROOT/'tests/nei_used_fx/flight_particle_test.py')],check=True)
    subprocess.run([sys.executable,str(ROOT/'tests/nei_used_fx/preserve_approved_test.py')],check=True)
    subprocess.run([sys.executable,str(ROOT/'tests/nei_used_fx/preserve_accepted_fire_test.py')],check=True)
if __name__ == "__main__":
    with tempfile.TemporaryDirectory(prefix="nei-used-fx-") as tmp:
        graph=functions((ROOT/'soh/src/code/graph.c').read_text())
        (Path(tmp)/'nei_gi_graph.inc').write_text(graph['Graph_OpenDisps']+'\n'+graph['Graph_CloseDisps'])
        # The budget test uses the established real-engine graphics fixture.
        # Its unrelated GI-dispatch regressions are stripped at link time.
        (Path(tmp)/'nei_gi_dispatch.inc').write_text(
            'void Player_DrawGetItemImpl(PlayState*, Player*, Vec3f*, s32);\n'
            'void EnGirlA_Draw(Actor*, PlayState*);\n')
        (Path(tmp)/'nei_gi_bounds.inc').write_text('')
        for host, adapter in (("soh", "soh/soh/Enhancements/Gameplay/MagicMirror.cpp"),
                              ("mm", "mm/2s2h/Enhancements/Gameplay/MagicMirror.cpp")):
            body = functions((ROOT / adapter).read_text())["DrawCastAura"]
            (Path(tmp) / ("mirror_cast_adapter_" + host + ".inc.h")).write_text(
                body.replace("DrawCastAura(", "MirrorCastAura(", 1))
        for name in (("budget",) if "--budget-only" in sys.argv else ("policy", "feedback_policy", "presentation", "budget")):
            binary = str(Path(tmp) / name)
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O2", *flags(), '-I'+tmp,
                            '-ffunction-sections','-fdata-sections',
                            str(ROOT / "tests/nei_used_fx" / (name + "_test.cpp")), '-Wl,--gc-sections',
                            "-o", binary], check=True)
            subprocess.run([binary], check=True)
        if "--budget-only" not in sys.argv:
            from run_mm_nei_tests import flags as mm_flags
            binary = str(Path(tmp) / "mm-presentation")
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O2", *mm_flags(),
                            "-I" + str(ROOT), "-I" + tmp, "-DNEI_USED_FX_MM",
                            str(ROOT / "tests/nei_used_fx/presentation_test.cpp"), "-o", binary], check=True)
            subprocess.run([binary], check=True)
        # Compile the public interface as C, just as the unity item sources use it.
        source = Path(tmp) / "abi.c"
        source.write_text('#include "soh/Enhancements/randomizer/NeiUsedMagicPresentation.h"\n')
        subprocess.run([os.environ.get("CC", "cc"), "-std=gnu2x", *flags(), "-fsyntax-only", str(source)], check=True)
