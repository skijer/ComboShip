#!/usr/bin/env python3
"""Exercise the actual shared mirror placement and both native travel adapters."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def block(source, marker):
    start = source.index(marker)
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def main():
    assert (ROOT / "combo/gameplay/MagicMirrorAnchor.h").exists(), "Mirror cannot follow the raised stump yet"
    with tempfile.TemporaryDirectory(prefix="magic-mirror-") as directory:
        binary = Path(directory) / "anchor"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie", "-I" + str(ROOT),
                        str(ROOT / "tests/magic_mirror/anchor_test.cpp"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
        assert (ROOT / "combo/gameplay/MagicMirrorRuntime.inc.h").exists(), "Mirror recall is not implemented yet"
        for host in ("soh", "mm"):
            adapter = (ROOT / host / ("soh" if host == "soh" else "2s2h") /
                       "Enhancements/Gameplay/MagicMirror.cpp").read_text()
            (Path(directory) / "native_mirror_sound.inc.h").write_text("\n".join(
                block(adapter, "void " + name + "(") for name in ("PlayActivationChime", "PlayDepartureCue")))
            # Run each engine's real once-animation advancement and dispatcher.
            native = (ROOT / host / "src/code/z_skelanime.c").read_text()
            prefix = "LinkAnimation" if host == "soh" else "PlayerAnimation"
            bodies = []
            for suffix in ("Once", "Update"):
                match = re.search(r"s32\s+" + prefix + "_" + suffix + r"\([^;{}]*\)\s*\{", native)
                assert match, prefix + "_" + suffix
                start = match.start()
                end = match.end()
                depth = 1
                while depth:
                    depth += (native[end] == "{") - (native[end] == "}")
                    end += 1
                bodies.append(native[start:end])
            (Path(directory) / "native_animation.inc.h").write_text("\n".join(bodies))
            message = (ROOT / host / "src/code" /
                       ("z_message_PAL.c" if host == "soh" else "z_message.c")).read_text()
            message_bodies = []
            for name in ("Message_GetState", "Message_ShouldAdvance", "Message_CloseTextbox"):
                match = re.search(r"(?:u8|bool|void) " + name + r"\(", message)
                body = block(message, match[0])
                body = body.replace(name + "(", "Native" + name + "(", 1)
                # Audio, settings and the MM behavior hook are explicit boundary doubles.
                body = re.sub(r"Audio_PlaySfx(?:General)?\([^;]*;", "TestMessageSfx();", body)
                body = re.sub(r'CVarGetInteger\([^;]*?\) != 0', 'false', body)
                body = body.replace('GameInteractor_Should(VB_MSG_ADVANCE, shouldAdvance)', 'shouldAdvance')
                body = body.replace('CVarGetInteger("gEnhancements.Dialogue.FastText", 0)', 'false')
                message_bodies.append(body)
            (Path(directory) / "native_message.inc.h").write_text("\n".join(message_bodies))
            player = (ROOT / host / "src/overlays/actors/ovl_player_actor/z_player.c").read_text()
            if host == "mm":
                start = player.index("    if (play->actorCtx.isOverrideInputOn && (this == GET_PLAYER(play)))")
                end = player.index("    // SoH z_player.c:", start)
            else:
                start = player.index("        if (this->stateFlags1 & (PLAYER_STATE1_INPUT_DISABLED | PLAYER_STATE1_IN_CUTSCENE))")
                end = player.index("        // Gust jar:", start)
            pad = re.sub(r"\bthis\b", "current", player[start:end])
            for name in ("CustomItems_BlocksMovement", "func_8082DA90", "TransformMasks_IsTransformed",
                         "MmMaskWear_IsKamaroDancing", "WolfLinkHost_RestorePlayerInput",
                         "GameInteractor_ExecuteOnPassPlayerInputs", "TransformMasks_FilterB"):
                pad = pad.replace(name, "Test" + name)
            pad = "Input ReadNativePromptInput(Player* current, PlayState* play) { Input input{};\n" + pad + "\nreturn input; }"
            wolf = (ROOT / host / "mods/transformation_masks/wolf_link_form.cpp").read_text()
            read = block(wolf, "static WolfInput ReadInput(").replace("ReadInput(", "TestWolfReadInput(")
            for name in ("Lib_GetControlStickData", "func_80077D10"):
                read = read.replace(name, "TestGetStickData")
            read = read.replace("Camera_GetInputDirYaw(GET_ACTIVE_CAM(play))", "TestCameraYaw(play)")
            aliases = ""
            if host == "mm":
                compat = (ROOT / "mm/mods/nei_oot_compat.h").read_text()
                for name, value in re.findall(r"^#define\s+(PLAYER_STATE1_\w+)\s+(.+)$", compat, re.M):
                    if name in read:
                        aliases += f"#ifndef {name}\n#define {name} {value}\n#endif\n"
            mario_path = "soh/expansions/sm64/sm64_mario.c" if host == "soh" else "mm/2s2h/expansions/sm64/sm64_mario.c"
            mario = (ROOT / mario_path).read_text()
            start = mario.index("#ifdef COMBO_BUILD", mario.index("void Sm64Mario_Update("))
            end = mario.index("#endif", start) + len("#endif")
            mario_gate = ("bool NativeMarioReadsPad(PlayState* play, Player* player) {\n" +
                          mario[start:end].replace("return;", "return false;") + "\nreturn true;\n}")
            (Path(directory) / "native_prompt_input.inc.h").write_text(
                pad + "\n" + aliases + block(wolf, "struct WolfInput {") + ";\n" + read + "\n" + mario_gate)
            includes = [ROOT, ROOT / host, ROOT / host / "include", ROOT / host / "include/PR",
                        ROOT / host / "src", ROOT / host / "assets", ROOT / host / "mods",
                        ROOT / host / ("soh" if host == "soh" else "2s2h"), ROOT / "libultraship/include",
                        ROOT / "combo", ROOT / "combo/menu"]
            target = Path(directory) / host
            subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-w", "-DF3DEX_GBI_2", "-DCOMBO_BUILD",
                            *(["-DMIRROR_OOT"] if host == "soh" else []), "-ffunction-sections", "-fdata-sections",
                            "-Wl,--gc-sections", "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                            "-I" + directory, *["-I" + str(p) for p in includes],
                            str(ROOT / "tests/magic_mirror/native_test.cpp"),
                            "-o", str(target)], check=True)
            subprocess.run([str(target)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})

if __name__ == "__main__":
    main()
