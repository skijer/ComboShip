"""Check real MM audio translation units, then run focused function fixtures."""
import pathlib
import re
import os
import shlex
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parents[2]
FUNCTIONS = {
    "load.c": ["AudioLoad_IsFontLoadComplete", "AudioLoad_IsSeqLoadComplete",
               "AudioLoad_SetFontLoadStatus", "AudioLoad_SetSeqLoadStatus",
               "AudioLoad_GetFontsForSequence", "AudioLoad_SyncInitSeqPlayerInternal",
               "AudioLoad_SyncLoad"],
    "heap.c": ["AudioHeap_ReleaseNotesForFont", "AudioHeap_Alloc",
               "AudioHeap_SearchPermanentCache", "AudioHeap_AllocPermanent"],
    "playback.c": ["AudioPlayback_GetInstrumentInner"],
    "seqplayer.c": ["AudioScript_SequencePlayerSetupChannels", "AudioScript_SelectChannelFont"],
    "synthesis.c": [],  # Full translation-unit check; decoder tests exercise the streamed-sample branch.
    "../sequence.c": ["AudioSeq_ResolveSequence", "AudioSeq_StartSequence", "AudioSeq_QueueSeqCmd"],
}


def function_body(source, name):
    match = re.search(r"^(?:static\s+)?[\w* ]+\b" + re.escape(name) + r"\([^;]*?\)\s*\{", source, re.M)
    if match is None:
        raise RuntimeError(f"cannot locate production function {name}")
    depth = 0
    for pos in range(source.find("{", match.start()), len(source)):
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
            if depth == 0:
                return match.start(), pos + 1
    raise RuntimeError(f"unterminated production function {name}")


def check_translation_units(build, compiler):
    # Fixtures define globals themselves and can hide missing declarations.
    # Compile the complete sources with game headers before using those fixtures.
    args = [compiler, "-std=gnu11", "-fsyntax-only", "-DNDEBUG",
            "-DF3DEX_GBI_2", "-DCOMBO_BUILD", "-DMM_BUILD_DLL",
            "-DCONTROLLERBUTTONS_T=uint32_t", "-DNON_EQUIVALENT", "-DNON_MATCHING",
            "-Wno-error", "-Wno-int-conversion", "-Wno-incompatible-pointer-types"]
    args += ["-I" + str(ROOT / path) for path in
             ("mm/include", "mm/include/PR", "mm/src", "mm", "mm/2s2h", "mm/assets",
              "libultraship/include", "libultraship/src", "combo")]
    failed = False
    for filename in FUNCTIONS:
        path = (ROOT / "mm/src/audio/lib" / filename).resolve()
        result = subprocess.run([*args, str(path)], capture_output=True, text=True)
        (build / (path.stem + "_syntax.log")).write_text(result.stdout + result.stderr)
        if result.returncode:
            failed = True
            sys.stderr.write(result.stdout + result.stderr)
        else:
            print("PASS real-header audio syntax:", path.relative_to(ROOT), flush=True)
    if failed:
        raise SystemExit(1)


def main(build, compiler="cc"):
    build = pathlib.Path(build).resolve()
    build.mkdir(parents=True, exist_ok=True)
    check_translation_units(build, compiler)
    bodies = []
    for filename, names in FUNCTIONS.items():
        path = (ROOT / "mm/src/audio/lib" / filename).resolve()
        source = path.read_text()
        for name in names:
            start, end = function_body(source, name)
            line = source.count("\n", 0, start) + 1
            bodies.append(f'#line {line} "{path}"\n{source[start:end]}\n')
    (build / "mm_audio_runtime_functions.inc").write_text("\n".join(bodies))
    common = [compiler, "-std=c11", "-g", "-DNDEBUG", "-Wall", "-Werror",
              "-Wno-unused-variable", "-Wno-unused-but-set-variable",
              *shlex.split(os.environ.get("MM_AUDIO_TEST_CFLAGS", "")),
              "-I" + str(ROOT / "mm/tests"), "-I" + str(ROOT / "mm/include"),
              "-I" + str(ROOT / "libultraship/include")]
    storage_exe = build / "mm_audio_font_id_test"
    subprocess.run([*common, str(ROOT / "mm/tests/audio_font_id_test.c"), "-o", str(storage_exe)], check=True)
    subprocess.run([str(storage_exe)], check=True)
    exe = build / "mm_audio_stream_runtime_test"
    subprocess.run([*common, "-I" + str(build),
                    str(ROOT / "mm/tests/audio_stream_runtime_test.c"), "-o", str(exe)], check=True)
    subprocess.run([str(exe)], check=True)


if __name__ == "__main__":
    main(*sys.argv[1:])
