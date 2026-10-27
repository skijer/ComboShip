"""Exercise production dungeon tint and bottle shimmer command streams."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]


def main():
    source = (ROOT / "mm/src/code/z_draw.c").read_text()
    names = ["GetItem_GetDungeonItemTint", "GetItem_GetDungeonKeyEmblemTint", "GetItem_GetDungeonKeyModel",
             "GetItem_TryDrawDungeonKey", "GetItem_DrawDungeonItem", "GetItem_BottleShimmerColor",
             "GetItem_DrawBottleShimmer", "GetItem_GetShimmerColor", "GetItem_DrawShimmer", "GetItem_Draw"]
    bodies = [function(source, name) for name in names]
    flags = ["-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL", "-DCONTROLLERBUTTONS_T=uint32_t",
             "-DNON_EQUIVALENT", "-DNON_MATCHING", "-Wno-int-conversion", "-Wno-incompatible-pointer-types"]
    flags += ["-I" + str(ROOT / p) for p in
              ("mm/include", "mm/include/PR", "mm/src", "mm", "mm/2s2h", "mm/assets",
               "libultraship/include", "libultraship/src", "combo", "combo/menu")]
    cc = shlex.split(os.environ.get("CC", "cc"))
    with tempfile.TemporaryDirectory(prefix="mm-item-visuals-") as temporary:
        build = Path(temporary)
        (build / "item_visuals_production.inc").write_text("\n".join(bodies))
        binary = build / "item_visuals_test"
        subprocess.run([*cc, "-std=gnu11", *flags, "-I" + str(build),
                        str(ROOT / "mm/tests/item_visuals_test.c"), "-lm", "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        result = subprocess.run([*cc, "-std=gnu11", *flags, "-Werror=implicit-function-declaration",
                                 "-fsyntax-only", str(ROOT / "mm/src/code/z_draw.c")], capture_output=True, text=True)
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        print("PASS real-header z_draw.c syntax")
        # Raw previews delegate to the concrete-award drawer. Both routes must
        # retain the owner hook before dispatch (including the old per-key copies).
        source = (ROOT / "mm/2s2h/Rando/DrawItem.cpp").read_text()
        preview = function(source, "Rando::DrawItem")
        assert "Rando::DrawResolvedItem(randoItemId, randoCheckId, actor);" in preview
        draw = function(source, "Rando::DrawResolvedItem")
        assert draw.index("DungeonItem_GetOwner") < draw.index("switch (randoItemId)")
        assert "GetItem_DrawDungeonItem" in draw
        print("PASS randomizer owner hook precedes dispatch")
        # Compile the native dispatcher labels with the real item enum. This
        # catches duplicate imported-season cases without the app's UI SDK.
        dispatch = '#include "Rando/Types.h"\n'
        for index, section in enumerate(draw.split('switch (randoItemId)')[1:]):
            labels = re.findall(r'^\s*case (RI_\w+):', section, re.M)
            dispatch += f'void CheckDispatch{index}(RandoItemId id) {{ switch(id) {{\n'
            dispatch += '\n'.join(f'case {label}: break;' for label in labels) + '\n} }\n'
        path = build / 'dispatch.cpp'
        path.write_text(dispatch)
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-DCOMBO_BUILD',
                        '-I' + str(ROOT / 'mm/2s2h'), '-fsyntax-only', str(path)], check=True)
        print("PASS real-enum native MM dispatcher compiles without duplicate season cases")


if __name__ == "__main__":
    main()
