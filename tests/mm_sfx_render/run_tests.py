#!/usr/bin/env python3
"""Exercise production MM sample rendering with the host's real DSP functions."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tests/goron_audio"))
from run_tests import function


def main():
    subprocess.run([sys.executable, "-B", ROOT / "tests/mm_sfx_render/clock_test.py"], check=True)
    subprocess.run([sys.executable, "-B", ROOT / "tests/native_transformations/audio_lifecycle_test.py"], check=True)
    source = (ROOT / "soh/soh/mixer.c").read_text()
    names = ("clamp16", "aClearBufferImpl", "aLoadBufferImpl", "aSaveBufferImpl",
             "aLoadADPCMImpl", "aSetBufferImpl", "aInterleaveImpl", "aDMEMMoveImpl",
             "aSetLoopImpl", "aADPCMdecImpl", "aResampleImpl", "aEnvSetup1Impl",
             "aEnvSetup2Impl", "aEnvMixerImpl", "aS8DecImpl", "aHiLoGainImpl", "aInterlImpl")
    with tempfile.TemporaryDirectory(prefix="mm-sfx-render-") as temporary:
        build = Path(temporary)
        preamble = source[source.index("#define ROUND_UP_64"):source.index("static void aMixImplSSE2")]
        mixer = build / "dsp.c"
        mixer.write_text('#include <stdint.h>\n#include <stdbool.h>\n#include <string.h>\n'
                         '#include "mixer.h"\n' + preamble + "\n" +
                         "\n".join(function(source, name) for name in names))
        subprocess.run([os.environ.get("CC", "cc"), "-std=gnu11", "-O1", "-g", "-DF3DEX_GBI_2",
                        "-fsanitize=address", "-fno-omit-frame-pointer", "-fno-pie",
                        "-I" + str(ROOT / "soh/soh"), "-I" + str(ROOT / "libultraship/include"),
                        "-c", str(mixer), "-o", str(build / "dsp.o")], check=True)
        binary = build / "sample_test"
        subprocess.run([os.environ.get("CXX", "c++"), "-std=gnu++20", "-O1", "-g",
                        "-fsanitize=address", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
                        "-I" + str(ROOT / "soh"),
                        str(ROOT / "tests/mm_sfx_render/sample_test.cpp"), str(build / "dsp.o"),
                        "-o", str(binary)], check=True)
        environment = dict(os.environ)
        environment.setdefault("ASAN_OPTIONS", "detect_leaks=0")
        for case in ("normal", "high-rate", "high-rate-odd", "high-rate-end", "high-rate-end-first",
                     "high-rate-loop", "high-rate-adpcm", "high-rate-small-adpcm"):
            subprocess.run([str(binary), case], check=True, env=environment)


if __name__ == "__main__":
    main()
