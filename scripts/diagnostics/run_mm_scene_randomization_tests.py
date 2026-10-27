"""Exercise scene randomization at the production editor/scene boundaries.

Extract unchanged function bodies, as in the existing audio runtime harness.
The fixture replaces rendering, config persistence and playback; it does not
claim to render the editors or run the game.
"""
import os
import pathlib
import re
import shlex
import subprocess
import tempfile

ROOT = pathlib.Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r'^(?:extern "C" )?[\w:* ]+\b' + re.escape(name) +
                      r'\([^;]*?\)\s*\{.*?^}', source, re.M | re.S)
    if match is None:
        raise RuntimeError(f"Cannot find production function {name}")
    return match[0]


def main():
    cosmetics = (ROOT / "mm/2s2h/BenGui/CosmeticEditor.cpp").read_text()
    audio = (ROOT / "mm/2s2h/Enhancements/Audio/AudioEditor.cpp").read_text()
    play = (ROOT / "mm/src/code/z_play.c").read_text()
    constants = re.findall(r'^const char\* kCosmetic\w+Cvar = .*?;', cosmetics, re.M)
    bodies = constants + [function(cosmetics, "CosmeticEditorApplyLinkSize"),
                          function(cosmetics, "CosmeticEditorWindow::InitElement")]
    if "AudioEditor_RandomizeOnSceneLoad()" in audio:
        bodies.append(function(audio, "AudioEditor_RandomizeOnSceneLoad"))
    # Execute the actual scene-audio initialization statements in their actual
    # order. A late shuffle must not pass by replaying the previous scene's song.
    start = play.index("    Interface_SetSceneRestrictions(this);")
    end = play.index("    gSaveContext.seqId = this->sceneSequences.seqId;", start)
    bodies.append("void InitializeSceneAudio(PlayState* thisx) {\n" +
                  "#define this thisx\n" + play[start:end] + "#undef this\n}\n")
    with tempfile.TemporaryDirectory(prefix="mm-scene-randomization-") as temporary:
        build = pathlib.Path(temporary)
        (build / "scene_randomization.inc").write_text("\n".join(bodies))
        executable = build / "scene_randomization_test"
        subprocess.run([
            *shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra",
            "-Werror", "-Wno-unused-parameter", "-I" + str(build),
            str(ROOT / "mm/tests/scene_randomization_test.cpp"), "-o", str(executable),
        ], check=True)
        subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    main()
