"""Execute actual host init/charge branches and audited damage selection."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_din_fire_tests import FLAGS as MM_FLAGS


def function(source, name):
    match = re.search(r'^[A-Za-z_][^\n;{}]*\b' + re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    if match is None:
        raise ValueError("Missing production spin definition: " + name)
    start = match.start()
    brace = source.index("{", start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def declaration(source, name):
    match = re.search(r'static [^;{}]*\b' + name + r'\s*(?:\[[^]]*\])?\s*=\s*\{', source, re.S)
    if not match:
        raise ValueError("Missing native constant: " + name)
    brace = source.index("{", match.start())
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():source.index(";", end) + 1]


def main():
    oot = ["-std=gnu2x", "-O1", "-g", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0",
           "-Werror=implicit-function-declaration"]
    oot += ["-I" + str(ROOT / p) for p in ("soh/include", "soh/src", "soh/assets", "soh", "soh/soh",
                                         "libultraship/include", "libultraship/src", "combo")]
    for path in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
        for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / path).read_text()):
            oot.append(f'-D{key}="{value}"')
    variants = [("soh", "solo", oot), ("soh", "combo", [*oot, "-DCOMBO_BUILD"]),
                ("mm", "solo", [f for f in MM_FLAGS if f not in ("-DCOMBO_BUILD", "-DMM_BUILD_DLL")]),
                ("mm", "combo", MM_FLAGS)]
    env = os.environ.copy()
    env["ASAN_OPTIONS"] = env.get("ASAN_OPTIONS", "") + ":detect_leaks=0"
    with tempfile.TemporaryDirectory(prefix="spin-native-") as directory:
        temp = Path(directory)
        for host, mode, flags in variants:
            source = (ROOT / host / "src/overlays/actors/ovl_En_M_Thunder/z_en_m_thunder.c").read_text()
            constants = [declaration(source, "sCylinderInit")]
            if host == "soh":
                constants += [re.search(r'^#define EN_M_THUNDER_SWORD_BEAM_FLAG .*', source, re.M).group(0)]
                constants += [declaration(source, name) for name in ("sSpinAttackDmgFlags", "sJumpAttackDmgFlags", "sSfxIds")]
            else:
                constants += ["#define ENMTHUNDER_TYPE_MAX 4", re.search(r'typedef enum \{.*?\} EnMThunderSubType;', source, re.S).group(0),
                              declaration(source, "sDamages"), declaration(source, "sChargingSfxIds")]
            names = ["EnMThunder_InitStyle", "EnMThunder_PreviewStyle", "EnMThunder_ReleaseStyle", "EnMThunder_VisualStyle"]
            if host == "soh": names += ["EnMThunder_SetupAction"]
            names += ["EnMThunder_Init", "EnMThunder_ChargingSpinAttack" if host == "soh" else "EnMThunder_Charge",
                      "EnMThunder_Destroy"]
            (temp / "spin_actor_production.inc").write_text("\n".join(constants + [function(source, n) for n in names]))
            selected = [*flags, "-I" + directory, "-ffunction-sections", "-fdata-sections"]
            if host == "mm": selected += ["-DHOST_MM"]
            if "--sanitize" in sys.argv:
                selected += ["-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
            binary = temp / (host + "-" + mode)
            subprocess.run([os.environ.get("CC", "cc"), *selected, "tests/spin_style/native_actor_test.c",
                            f"{host}/src/code/din_fire_sword.c", f"{host}/src/code/sword_element.c", "-Wl,--gc-sections",
                            "-lm", "-o", str(binary)], cwd=ROOT, check=True)
            subprocess.run([str(binary)], env=env, check=True)
            # Compile the entire actual actor, including its rendering path;
            # extracting only state branches cannot prove native TU integration.
            subprocess.run([os.environ.get("CC", "cc"), *selected, "-c",
                            f"{host}/src/overlays/actors/ovl_En_M_Thunder/z_en_m_thunder.c",
                            "-o", str(temp / (host + "-" + mode + ".o"))], cwd=ROOT, check=True)
            print("PASS full native actor compilation", host, mode, flush=True)


if __name__ == "__main__":
    main()
