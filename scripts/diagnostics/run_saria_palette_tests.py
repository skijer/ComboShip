#!/usr/bin/env python3
"""Exercise the production TLUT uploader with native Saria palette sizes.

The actual resource payload is not available here. Dimensions come from the
native extraction metadata; the generated palette makes a dropped upper bank
observable without depending on renderer initialization or a graphics driver.
"""
from pathlib import Path
import argparse
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-ref", help="Read the renderer from this git ref to reproduce the original failure")
    args = parser.parse_args()
    renderer = (subprocess.check_output(["git", "show", args.baseline_ref + ":libultraship/src/fast/interpreter.cpp"],
                                        cwd=ROOT, text=True) if args.baseline_ref else
                (ROOT / "libultraship/src/fast/interpreter.cpp").read_text())
    start = renderer.index("void Interpreter::GfxDpLoadTlut(")
    end = renderer.index("\nvoid Interpreter::GfxDpLoadBlock(", start)
    production = renderer[start:end]
    metadata = (ROOT / "soh/assets/yml/ntsc_1-0/objects/object_sa.yml").read_text()
    sizes = []
    for name in ("gSariaEyeTLUT", "gSariaMouthTLUT"):
        block = metadata.split(name + ":\n", 1)[1].split("\n\n", 1)[0]
        sizes.append(int(re.search(r"width: (\d+)", block)[1]) *
                     int(re.search(r"height: (\d+)", block)[1]))
    assert sizes == [252, 232], sizes
    fixture = r'''
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#define SUPPORT_CHECK(x) do { if (!(x)) std::abort(); } while (0)
#define G_IM_SIZ_16b 2
struct State {
    struct { uint8_t siz; const uint8_t* addr; } texture_to_load{};
    struct { uint16_t tmem; } texture_tile[8]{};
    uint8_t palette_staging[2][256]{};
    const uint8_t* palettes[2]{};
    const uint8_t* palette_dram_addr[2]{};
};
struct Interpreter {
    State state;
    State* mRdp = &state;
    void GfxDpLoadTlut(uint8_t, uint32_t);
};
/* PRODUCTION */
static bool Check(unsigned tmem, unsigned count) {
    Interpreter renderer;
    // Exactly sized source: ASan catches any read beyond the requested TLUT.
    auto* source = new uint8_t[count * 2];
    for (unsigned i = 0; i < count * 2; ++i) source[i] = (i * 17 + 31) % 251;
    std::memset(renderer.state.palette_staging, 0xCC, 512);
    renderer.state.texture_to_load = { G_IM_SIZ_16b, source };
    renderer.state.texture_tile[7].tmem = tmem;
    renderer.GfxDpLoadTlut(7, count - 1);
    const unsigned offset = (tmem - 256) * 2;
    const unsigned copied = std::min(count * 2, 512 - offset);
    for (unsigned i = 0; i < 512; ++i) {
        const uint8_t expected = i >= offset && i < offset + copied ? source[i - offset] : 0xCC;
        if (renderer.state.palette_staging[i / 256][i % 256] != expected) {
            std::fprintf(stderr, "FAIL tmem=%u entries=%u byte=%u: upper palette bank or untouched bytes wrong\n",
                         tmem, count, i);
            delete[] source;
            return false;
        }
    }
    const unsigned firstBank = offset / 256;
    if (renderer.state.palettes[firstBank] != renderer.state.palette_staging[firstBank] ||
        renderer.state.palette_dram_addr[firstBank] != source) std::abort();
    if (offset < 256 && offset + copied > 256 &&
        (renderer.state.palettes[1] != renderer.state.palette_staging[1] ||
         renderer.state.palette_dram_addr[1] != source + 256 - offset)) std::abort();
    delete[] source;
    return true;
}
int main() {
    bool passed = true;
    // Full CI8 and CI4 subpalettes remain controls for the narrow partial-CI8 fix.
    passed &= Check(256, 256);
    passed &= Check(256, 16);
    passed &= Check(496, 16);
    for (unsigned count : { /* SARIA_COUNTS */ }) passed &= Check(256, count);
    passed &= Check(376, 32); // 64-byte upload crosses the bank boundary at byte 240.
    passed &= Check(504, 16); // Clamp at the end of palette memory, preserving source bounds.
    Interpreter low;
    const uint8_t raw[2] = { 0, 1 };
    low.state.texture_to_load = { G_IM_SIZ_16b, raw };
    low.state.texture_tile[7].tmem = 0;
    low.GfxDpLoadTlut(7, 0);
    if (low.state.palettes[1] != raw || low.state.palette_dram_addr[1] != raw) std::abort();
    if (!passed) return 1;
    std::puts("PASS production TLUT: Saria252/232, full256, CI4 banks, crossing offsets, bounds, raw fallback");
}
'''
    fixture = fixture.replace("/* PRODUCTION */", production).replace("/* SARIA_COUNTS */", ", ".join(map(str, sizes)))
    with tempfile.TemporaryDirectory(prefix="saria-tlut-") as directory:
        source = Path(directory) / "palette.cpp"
        source.write_text(fixture)
        binary = Path(directory) / "palette"
        subprocess.run(["c++", "-std=c++20", "-g", "-fsanitize=address,undefined", "-fno-pie", "-no-pie",
                        "-fno-sanitize-recover=all", str(source), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
