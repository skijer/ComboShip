#!/usr/bin/env python3
"""Execute real MM chest draw/category paths, then compile the complete C actor."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

from run_time_pedestal_tests import functions

ROOT = Path(__file__).resolve().parents[2]


def main():
    actor_path = ROOT / "mm/src/overlays/actors/ovl_En_Box/z_en_box.c"
    actor = functions(actor_path.read_text())
    helpers = functions((ROOT / "mm/src/code/z_actor.c").read_text(), {"Actor_IsSmallChest"})
    selected = [helpers["Actor_IsSmallChest"]]
    selected += [actor[name] for name in ("EnBox_PostLimbDraw", "EnBox_SetRenderMode1",
                                        "EnBox_SetRenderMode2", "EnBox_SetRenderMode3", "EnBox_Draw")]
    rando = (ROOT / "mm/2s2h/Rando/ActorBehavior/EnBox.cpp").read_text()
    rando_functions = functions(rando.replace('extern "C" int ', "int "))
    rando_selected = [body for name, body in rando_functions.items() if name in
                      {"MMChest_GetRandoItemType", "EnBox_RandoPostLimbDraw", "EnBox_RandoDraw"}]
    rando_arrays = "\n".join(re.findall(r"^static Gfx gBoxChest\w+CopyDL\[\d+\];", rando, re.M))
    treasure_map = re.search(r"std::vector<std::vector<RandoCheckId>> treasureGameMap = \{.*?\n\};", rando, re.S)[0]
    hook_start = rando.index("{", rando.index("COND_ID_HOOK(ShouldActorInit"))
    hook_end, depth = hook_start + 1, 1
    while depth:
        depth += (rando[hook_end] == "{") - (rando[hook_end] == "}")
        hook_end += 1
    init_hook = "static void RunRandoShouldActorInit(Actor* actor, bool* should) " + rando[hook_start:hook_end]
    flags = ["-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-DCONTROLLERBUTTONS_T=uint32_t",
             "-DNON_EQUIVALENT", "-DNON_MATCHING", "-O1", "-g", "-fsanitize=undefined",
             "-fno-sanitize-recover=undefined"]
    flags += ["-I" + str(ROOT / p) for p in ("mm/include", "mm/include/PR", "mm/src", "mm",
               "mm/2s2h", "mm/assets", "libultraship/include", "libultraship/src", "combo")]
    cc = shlex.split(os.environ.get("CC", "cc"))
    cxx = shlex.split(os.environ.get("CXX", "c++"))
    with tempfile.TemporaryDirectory(prefix="mm-chest-contents-") as directory:
        folder = Path(directory)
        extra = ROOT / "mm/2s2h/Enhancements/Graphics/ChestContents.c"
        preamble = '#include "src/overlays/actors/ovl_En_Box/z_en_box.h"\n'
        preamble += '#include "2s2h/Enhancements/Graphics/ChestContents.h"\n'
        native = folder / "chest_draw.c"
        native.write_text(preamble + "\n".join(selected))
        (folder / "chest_rando_production.inc").write_text(
            rando_arrays + "\n" + treasure_map + "\n" + "\n".join(rando_selected) + "\n" + init_hook)
        objects = []
        for source in [native, extra]:
            target = folder / (source.stem + ".o")
            subprocess.run([*cc, "-std=gnu11", *flags, "-Werror=implicit-function-declaration",
                            "-Wno-int-conversion", "-Wno-incompatible-pointer-types", "-c", str(source),
                            "-o", str(target)], check=True)
            objects.append(str(target))
        binary = folder / "chest_contents_test"
        subprocess.run([*cxx, "-std=c++20", *flags, "-I" + directory,
                        str(ROOT / "mm/tests/chest_contents_test.cpp"), *objects, "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        subprocess.run([*cc, "-std=gnu11", *flags, "-Werror=implicit-function-declaration",
                        "-Wno-int-conversion", "-Wno-incompatible-pointer-types", "-fsyntax-only",
                        str(actor_path)], check=True)
        print("PASS: complete MM chest actor compiles with real integration headers")


if __name__ == "__main__":
    main()
