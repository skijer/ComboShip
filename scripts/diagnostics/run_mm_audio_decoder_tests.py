"""Exercise MM's actual Ogg helpers with libogg/libvorbis and generated audio."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import sys

sys.dont_write_bytecode = True
from run_mm_audio_runtime_test import function_body

ROOT = Path(__file__).resolve().parents[2]


def main():
    source = (ROOT / "mm/2s2h/resource/importer/AudioSampleFactory.cpp").read_text()
    declarations = source[source.index("struct OggFileData {"):source.index("static size_t VorbisReadCallback")]
    names = ("VorbisReadCallback", "VorbisSeekCallback", "VorbisCloseCallback", "VorbisTellCallback", "GetOggType")
    bodies = [declarations]
    for name in names:
        start, end = function_body(source, name)
        bodies.append(source[start:end])
    start = source.index("static const ov_callbacks vorbisCallbacks")
    bodies.append(source[start:source.index("};", start) + 2])
    start, end = function_body(source, "OggDecoderWorker")
    bodies.append(source[start:end])
    cppflags = os.environ.get("MM_AUDIO_DECODER_CPPFLAGS")
    libs = os.environ.get("MM_AUDIO_DECODER_LIBS")
    # The fixture calls libvorbis directly. Shared vorbisfile/vorbisenc pkg-config
    # flags do not include their private libvorbis dependency on Ubuntu.
    packages = ["vorbisfile", "vorbisenc", "vorbis", "ogg"]
    if cppflags is None:
        cppflags = subprocess.check_output(["pkg-config", "--cflags", *packages], text=True)
    if libs is None:
        libs = subprocess.check_output(["pkg-config", "--libs", *packages], text=True)
    with tempfile.TemporaryDirectory(prefix="mm-audio-decoder-") as temporary:
        build = Path(temporary)
        (build / "audio_decoder_production.inc").write_text("\n".join(bodies))
        synthesis = (ROOT / "mm/src/audio/lib/synthesis.c").read_text()
        start = synthesis.index("                    case CODEC_OPUS:")
        end = synthesis.index("                    default:", start)
        (build / "audio_stream_playback_production.inc").write_text(synthesis[start:end])
        mixer = (ROOT / "mm/2s2h/mixer.c").read_text()
        start, end = function_body(mixer, "aOPUSdecImpl")
        (build / "audio_opus_production.inc").write_text(mixer[start:end])
        binary = build / "audio_decoder_test"
        command = [*shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-g", "-Wall", "-Wextra",
                   "-Wno-unused-variable", "-I" + str(build), "-I" + str(ROOT / "mm/include"),
                   "-I" + str(ROOT / "libultraship/include"), *shlex.split(cppflags),
                   *shlex.split(os.environ.get("MM_AUDIO_DECODER_TEST_FLAGS", "")),
                   str(ROOT / "mm/tests/audio_decoder_test.cpp"), *shlex.split(libs), "-o", str(binary)]
        subprocess.run(command, check=True)
        failures = []
        for case in ("invalid-header", "empty-header", "truncated-header", "bad-checksum", "short-packet",
                     "valid-vorbis", "valid-opus", "worker-invalid", "worker-invalid-vorbis", "worker-read-error", "worker-vorbis",
                     "worker-opus", "callback-bounds", "playback-null-s16", "playback-null-opus", "playback-ready",
                     "opus-open-failure", "opus-seek-failure", "opus-read-error", "opus-ready", "opus-eof"):
            result = subprocess.run([str(binary), case], capture_output=True, text=True)
            if result.returncode:
                failures.append(case)
                print(f"FAIL {case}: exit {result.returncode}\n{result.stdout}{result.stderr}", flush=True)
            else:
                print(result.stdout.strip(), flush=True)
        if failures:
            raise SystemExit("MM decoder regressions failed: " + ", ".join(failures))


if __name__ == "__main__":
    main()
