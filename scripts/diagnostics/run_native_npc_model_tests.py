"""Compile real native/cutscene/static actor files and exercise model routing."""
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    flags = [os.environ.get("CC", "cc"), "-std=gnu2x", "-O2", "-Werror=implicit-function-declaration",
             "-Wno-incompatible-pointer-types", "-Wno-int-conversion", "-Wno-discarded-qualifiers",
             "-DLOG_LEVEL_GAME_PRINTS=0", "-DF3DEX_GBI_2"]
    flags += ["-I"+str(ROOT/path) for path in
              ["soh/include", "soh/src", "soh/assets", "soh", "soh/mods", "libultraship/include", "combo"]]
    for file in ["CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"]:
        for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT/file).read_text()):
            flags.append(f'-D{key}="{value}"')
    with tempfile.TemporaryDirectory(prefix="native-npc-models-") as folder:
        output = Path(folder)
        test = output/"routing"
        subprocess.run([*flags, str(ROOT/"soh/tests/native_npc_models_routing_test.c"), "-o", str(test)], check=True)
        subprocess.run([str(test)], check=True)
        pose = output/"pose-resources"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-I"+str(ROOT/"soh"), "-I"+str(ROOT/"combo"),
                        str(ROOT/"soh/tests/native_npc_pose_resources_test.cpp"), "-o", str(pose)], check=True)
        subprocess.run([str(pose)], check=True)
        bridge = output/"pose-bridge"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-DF3DEX_GBI_2",
                        *["-I"+str(ROOT/p) for p in ["soh/include", "soh", "libultraship/include", "combo"]],
                        str(ROOT/"soh/tests/native_npc_pose_bridge_test.cpp"),
                        str(ROOT/"soh/soh/Enhancements/cosmetics/NativeNpcPose.cpp"),
                        str(ROOT/"soh/soh/resource/type/Array.cpp"),
                        str(ROOT/"soh/soh/resource/type/Animation.cpp"), "-o", str(bridge)], check=True)
        subprocess.run([str(bridge)], check=True)
        sample = output/"pose-sampler"
        subprocess.run([*flags, "-ffunction-sections", "-fdata-sections",
                        str(ROOT/"soh/tests/native_npc_pose_sampling_test.c"),
                        str(ROOT/"soh/src/code/z_skelanime.c"), "-Wl,--gc-sections", "-lm", "-o", str(sample)], check=True)
        subprocess.run([str(sample)], check=True)
        draw_scale = output/"draw-scale"
        subprocess.run([*flags, "-DCOMBO_BUILD", "-ffunction-sections", "-fdata-sections",
                        str(ROOT/"soh/tests/native_npc_draw_scale_test.c"),
                        str(ROOT/"soh/src/code/z_skelanime.c"), "-Wl,--gc-sections", "-lm", "-o", str(draw_scale)], check=True)
        subprocess.run([str(draw_scale)], check=True)
        for actor in range(4):
            tracking = output/f"malon-tracking-{actor}"
            subprocess.run([*flags, "-ffunction-sections", "-fdata-sections", f"-DMALON_TRACKING_ACTOR={actor}",
                            str(ROOT/"soh/tests/native_npc_malon_tracking_test.c"),
                            str(ROOT/"soh/src/overlays/actors/ovl_En_Viewer/static_story_actor.c"),
                            "-Wl,--gc-sections", "-lm", "-o", str(tracking)], check=True)
            subprocess.run([str(tracking)], check=True)
        credits = output/"credits-routing"
        subprocess.run([*flags,"-ffunction-sections","-fdata-sections",
                        str(ROOT/"soh/tests/native_npc_credits_routing_test.c"),
                        str(ROOT/"soh/src/overlays/actors/ovl_Demo_Ec/z_demo_ec.c"),
                        "-Wl,--gc-sections","-lm","-o",str(credits)],check=True)
        subprocess.run([str(credits)],check=True)
        for actor in ["En_Sa", "Demo_Sa", "En_Ko", "En_Ma1", "En_Ma2", "En_Ma3", "En_Viewer", "Demo_Ec"]:
            path = ROOT/f"soh/src/overlays/actors/ovl_{actor}/z_{actor.lower()}.c"
            subprocess.run([*flags, "-c", str(path), "-o", str(output/f"{actor}.o")], check=True)
            print("PASS real actor compilation:", actor, flush=True)
        path = ROOT/"soh/src/overlays/actors/ovl_En_Viewer/static_story_kokiri.c"
        subprocess.run([*flags, "-c", str(path), "-o", str(output/"static-kokiri.o")], check=True)
        print("PASS real actor compilation: static_story_kokiri", flush=True)


if __name__ == "__main__":
    main()
