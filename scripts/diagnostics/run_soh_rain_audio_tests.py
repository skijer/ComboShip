#!/usr/bin/env python3
"""Run OoT seasonal rain against the real weather mixer and native SFX engine."""
import argparse
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
# Match global_outdoor_rain_audio_test in soh/CMakeLists.txt. Only resource,
# settings and sequence-command boundaries are fixtures; the audio code is real.
SOURCES = (
    "soh/tests/global_outdoor_rain_audio_test.cpp",
    "soh/tests/weather_sfx_engine_fixture.c",
    "soh/soh/Enhancements/audio/GlobalOutdoorRain.cpp",
    "soh/soh/Enhancements/audio/WeatherSamplePlayer.cpp",
    "soh/src/code/concurrent_weather_audio.c",
    "soh/src/code/code_800F7260.c",
    "soh/src/code/audio_sound_params.c",
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    cc = shlex.split(os.environ.get("CC", "cc"))
    cxx = shlex.split(os.environ.get("CXX", "c++"))
    sanitizer = ["-fsanitize=address,undefined,bounds", "-fno-sanitize-recover=all",
                 "-fno-omit-frame-pointer", "-fno-pie"] if args.sanitize else []
    common = ["-g", "-O1", "-DNDEBUG", *sanitizer,
              "-I" + str(ROOT / "soh/tests/weather_audio_stubs"),
              "-I" + str(ROOT / "soh/src"), "-I" + str(ROOT / "soh"),
              '-DCVAR_PREFIX_SETTING="gSettings"', '-DCVAR_PREFIX_AUDIO="gAudioEditor"']
    with tempfile.TemporaryDirectory(prefix="soh-rain-audio-") as directory:
        build = Path(directory)
        objects = []
        for source in SOURCES:
            path = ROOT / source
            cpp = path.suffix == ".cpp"
            obj = build / (path.name + ".o")
            subprocess.run([*(cxx if cpp else cc), "-std=c++20" if cpp else "-std=c11",
                            *common, "-c", str(path), "-o", str(obj)], cwd=ROOT, check=True)
            objects.append(str(obj))
        binary = build / "global_outdoor_rain_audio_test"
        subprocess.run([*cxx, *sanitizer, *(["-no-pie"] if args.sanitize else []),
                        *objects, "-lm", "-o", str(binary)], cwd=ROOT, check=True)
        subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=30)
    print("PASS OoT rain audio: seasonal/native storm handoffs, volume, thunder, native SFX and teardown")


if __name__ == "__main__":
    main()
