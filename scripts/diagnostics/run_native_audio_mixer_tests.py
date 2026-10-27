#!/usr/bin/env python3
"""Compile each complete game mixer and compare the actual SSE2 path to Q15 arithmetic."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    failures = 0
    with tempfile.TemporaryDirectory(prefix="native-audio-mixer-") as temporary:
        build = Path(temporary)
        (build / "opus").mkdir()
        (build / "opus/opus.h").write_text("#pragma once\n")
        # The entire mixer is compiled, but codec I/O is outside this native-mix
        # test. Abort if a tested path unexpectedly crosses that boundary.
        (build / "opusfile.h").write_text("""
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
typedef struct OggOpusFile OggOpusFile;
static inline OggOpusFile* op_open_memory(const unsigned char* p, size_t n, int* e) { abort(); }
static inline int op_pcm_seek(OggOpusFile* f, int64_t p) { abort(); }
static inline int op_read(OggOpusFile* f, int16_t* p, int n, int* l) { abort(); }
static inline void op_free(OggOpusFile* f) { abort(); }
""")
        for game, source in (("mm", "mm/2s2h/mixer.c"), ("soh", "soh/soh/mixer.c")):
            binary = build / game
            command = [*shlex.split(os.environ.get("CC", "cc")), "-std=gnu11", "-O2", "-g",
                       "-Wall", "-Wextra", "-Wno-unused-parameter", "-Wno-unused-variable",
                       "-Wno-unused-function", "-Wno-sign-compare", "-Wno-incompatible-pointer-types",
                       *shlex.split(os.environ.get("NATIVE_AUDIO_CFLAGS", "")),
                       '-DMIXER_SOURCE="' + str(ROOT / source) + '"', '-DGAME_NAME="' + game + '"',
                       "-I" + str(build), "-I" + str(ROOT / game / "include"),
                       "-I" + str(ROOT / "libultraship/include"),
                       str(ROOT / "tests/audio_mixer/mix_test.c"), "-lm", "-o", str(binary)]
            if game == "mm":
                command.insert(1, "-DTEST_MM_HILOGAIN")
            subprocess.run(command, check=True)
            failures += subprocess.run([str(binary)], timeout=30).returncode != 0
    raise SystemExit(bool(failures))


if __name__ == "__main__":
    main()
