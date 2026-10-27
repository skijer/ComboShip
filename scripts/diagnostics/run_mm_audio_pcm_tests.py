"""Compare MM's production streamed PCM loads/resampler with SoH's mixer."""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
from run_mm_audio_runtime_test import function_body

ROOT = Path(__file__).resolve().parents[2]
FUNCTIONS = ["clamp16", "aClearBufferImpl", "aLoadBufferImpl", "aSetBufferImpl", "aResampleImpl"]


def mixer_fixture(game, build):
    path = ROOT / ("mm/2s2h/mixer.c" if game == "mm" else "soh/soh/mixer.c")
    source = path.read_text()
    names = FUNCTIONS.copy()
    if game == "mm" and "void aLoadBufferExactImpl(" in source:
        names.append("aLoadBufferExactImpl")
    # Separate translation units retain each game's real DMEM layout/table.
    bodies = ["#include <assert.h>\n#include <stddef.h>\n#include <string.h>",
              '#include "combo/audio/MMAudioTraceBridge.h"',
              '#include "audio/soundfont.h"']
    bodies += [f"#define {name} {game}_{name}" for name in names if name != "clamp16"]
    bodies += ['#define aOPUSdecImpl mm_test_opus', '#include "mixer.h"',
               source[source.index("#define ROUND_UP_64"):source.index("static void aMixImplSSE2")]]
    for name in names:
        start, end = function_body(source, name)
        bodies.append(source[start:end])
    bodies.append(f"""
#define DMEM_UNCOMPRESSED_NOTE 0x570
#define SAMPLES_PER_FRAME 16
#define SAMPLE_SIZE 2
void {game}_reset(void) {{ memset(&rspa, 0, sizeof(rspa)); }}
unsigned char* {game}_buffer(void) {{ return BUF_U8(DMEM_UNCOMPRESSED_NOTE); }}
void {game}_resample(int count, uint16_t pitch, int16_t* state, int init, int16_t* output) {{
    aSetBufferImpl(0, DMEM_UNCOMPRESSED_NOTE, 0xB00, count * 2);
    aResampleImpl(init ? A_INIT : A_CONTINUE, pitch, state);
    memcpy(output, BUF_S16(0xB00), count * 2);
}}
""")
    if game == "soh":
        bodies.append("""
void soh_load(Sample* sample, int position, int count) {
    aClearBufferImpl(DMEM_UNCOMPRESSED_NOTE, (count + SAMPLES_PER_FRAME) * SAMPLE_SIZE);
    size_t remaining = sample->size - position * SAMPLE_SIZE;
    size_t bytes = count * SAMPLE_SIZE;
    if (bytes > remaining) bytes = remaining;
    aLoadBufferImpl(sample->sampleAddr + position * SAMPLE_SIZE, DMEM_UNCOMPRESSED_NOTE, bytes);
}
""")
    else:
        synthesis = (ROOT / "mm/src/audio/lib/synthesis.c").read_text()
        start = synthesis.index("                    case CODEC_OPUS:")
        end = synthesis.index("                    default:", start)
        bodies.append("""
static unsigned opusCalls;
void aOPUSdecImpl(void* source, uint16_t dest, uint16_t bytes, struct OggOpusFile** state,
                  int32_t position, uint32_t size) { ++opusCalls; }
unsigned mm_opus_calls(void) { return opusCalls; }
void mm_load_native(const void* source, int bytes) {
    aLoadBufferImpl(source, DMEM_UNCOMPRESSED_NOTE, bytes);
}
#define AudioSynth_ClearBuffer(pkt, addr, bytes) aClearBufferImpl(addr, bytes)
void mm_load(Sample* sample, int position, int count) {
    struct { int32_t samplePosInt; struct OggOpusFile* opusFile; } state = { position, NULL };
    __typeof__(state)* synthState = &state;
    unsigned char* sampleAddr = sample->sampleAddr;
    int numSamplesToLoadAdj = count;
    int numSamplesProcessed = 0;
    int dmemUncompressedAddrOffset1 = 0;
    int flags = -1;
    int skipBytes = 1;
    switch (sample->codec) {
""")
        bodies.append(synthesis[start:end])
        bodies.append("""
        default: assert(0);
    }
skip:
    assert(flags == A_CONTINUE && skipBytes == 0);
    assert(numSamplesProcessed == count && dmemUncompressedAddrOffset1 == count);
}
""")
    result = build / f"{game}_audio_pcm_production.c"
    result.write_text("\n".join(bodies))
    return result


def main():
    with tempfile.TemporaryDirectory(prefix="mm-audio-pcm-") as temporary:
        build = Path(temporary)
        sources = [mixer_fixture(game, build) for game in ("mm", "soh")]
        binary = build / "audio_pcm_test"
        command = [*shlex.split(os.environ.get("CC", "cc")), "-std=gnu11", "-g", "-Wall", "-Wextra",
                   "-Werror", "-Wno-unused-parameter", "-Wno-sign-compare",
                   *shlex.split(os.environ.get("MM_AUDIO_PCM_CFLAGS", "")),
                   "-I" + str(ROOT), "-I" + str(ROOT / "mm/include"), "-I" + str(ROOT / "mm/2s2h"),
                   "-I" + str(ROOT / "libultraship/include"), *map(str, sources),
                   str(ROOT / "mm/tests/audio_pcm_test.c"), "-lm", "-o", str(binary)]
        subprocess.run(command, check=True)
        subprocess.run([str(binary), *sys.argv[1:]], check=True)


if __name__ == "__main__":
    main()
