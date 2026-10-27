"""Execute production chooser/render/native-input paths with both engines' real headers.

The fixtures replace engine services (audio, config, save-selection API, printer and player refresh),
not menu behavior. Every changed translation unit also compiles in standalone and Combo modes.
These checks do not establish in-game visual/audio acceptance.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def flags(host, combo=False):
    paths = [f"{host}/include", f"{host}/src", f"{host}/assets", host, "libultraship/include", "combo", "combo/menu"]
    result = ["-std=gnu2x", "-O1", "-g", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
              "-ffunction-sections", "-fdata-sections", "-Werror=implicit-function-declaration",
              "-Wno-incompatible-pointer-types", "-Wno-int-conversion"]
    result += ["-I" + str(ROOT / p) for p in paths]
    if host == "mm":
        result += ["-DSWORD_KALEIDO_MM", "-I" + str(ROOT / "mm/2s2h"),
                   "-include", str(ROOT / "mm/include/global.h")]
    else:
        for path in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
            for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()):
                result.append(f'-D{key}="{value}"')
    if combo:
        result += ["-DCOMBO_BUILD"]
    overlay = "overlays/misc/ovl_kaleido_scope" if host == "soh" else "overlays/kaleido_scope/ovl_kaleido_scope"
    result += ["-I" + str(ROOT / host / "src" / overlay)]
    return result


def compile_source(args):
    # Existing headers emit legacy macro/const warnings; retain their diagnostics on failure.
    result = subprocess.run([os.environ.get("CC", "cc"), *args], cwd=ROOT, text=True, capture_output=True)
    if result.returncode:
        print(result.stdout, end="")
        print(result.stderr, end="")
        result.check_returncode()


def main():
    with tempfile.TemporaryDirectory(prefix="sword-element-kaleido-") as td:
        for host in ("soh", "mm"):
            overlay = ROOT / host / "src" / ("overlays/misc/ovl_kaleido_scope" if host == "soh" else "overlays/kaleido_scope/ovl_kaleido_scope")
            for combo in (False, True):
                mode = "combo" if combo else "standalone"
                for fixture in ("test", "integration_test", "render_test"):
                    binary = Path(td) / f"{host}-{mode}-{fixture}"
                    compile_source([*flags(host, combo), str(ROOT / f"tests/sword_element_kaleido_{fixture}.c"),
                                    str(overlay / "sword_element_kaleido.c"), "-Wl,--gc-sections", "-lm", "-o", str(binary)])
                    subprocess.run([str(binary)], check=True)
                for source in ("z_kaleido_equipment.c", "sword_element_kaleido.c",
                               "z_kaleido_scope_PAL.c" if host == "soh" else "z_kaleido_scope_NES.c"):
                    compile_source([*flags(host, combo), "-fsyntax-only", str(overlay / source)])
                print(f"{host} {mode}: chooser, native equip/input, render commands and all changed TUs PASS", flush=True)


if __name__ == "__main__":
    main()
