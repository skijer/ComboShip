#!/usr/bin/env python3
"""Exercise the production OoT Chateau draw with native/Alt and missing assets.

Only graphics submission and archive availability are substituted. The actual
draw function is compiled, including its donor fallback and per-pass selection.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'soh/soh/Enhancements/randomizer/draw.cpp').read_text()
start = source.index('void Randomizer_DrawChateauRomani(')
begin = source.index('{', start)
end, depth = begin + 1, 1
while depth:
    depth += (source[end] == '{') - (source[end] == '}')
    end += 1
function = source[start:end]
fixture = r'''
#include <cassert>
#include <cstdio>
#include <string>
#include <vector>
struct PlayState { struct { void* gfxCtx; } state; };
struct GetItemEntry {};
struct Gfx {};
bool alt, donor, opaque, glass;
int phase;
int shimmerCalls;
int opaDisp, xluDisp;
std::vector<std::string> calls;
bool MmAssets_IsAvailable() { return donor; }
bool ResourceMgr_IsAltAssetsEnabled() { return alt; }
void NeiGi_DrawChateauShimmer(PlayState*) { ++shimmerCalls; }
bool ResourceMgr_FileExists(const char* path) {
    std::string p(path);
    return p.find("OpaqueDL") != std::string::npos ? opaque : glass;
}
void Gfx_SetupDL_25Opa(void*) { phase = 0; }
void Gfx_SetupDL_25Xlu(void*) { phase = 1; }
void gSPDisplayList(int, Gfx* path) {
    calls.push_back(std::to_string(phase) + ":host:" + (const char*)path);
}
#define POLY_OPA_DISP opaDisp
#define POLY_XLU_DISP xluDisp
#define ALIGN_ASSET(n)
#define OPEN_DISPS(x)
#define CLOSE_DISPS(x)
#define gSPMatrix(...)
#define GSP_MM_DL(disp, path) calls.push_back(std::to_string(phase) + ":donor:" + path)
const char* gGiChateauRomaniBottleEmptyDL = "native-opaque";
const char* gGiChateauRomaniBottleDL = "native-glass";
PRODUCTION
int main() {
    PlayState play{};
    for (int a = 0; a < 2; a++)
    for (int d = 0; d < 2; d++)
    for (int o = 0; o < 2; o++)
    for (int g = 0; g < 2; g++) {
        alt = a; donor = d; opaque = o; glass = g; calls.clear(); shimmerCalls = 0;
        Randomizer_DrawChateauRomani(&play, nullptr);
        const bool use3ds = a && o && g;
        std::vector<std::string> expected;
        if (use3ds) expected = {
            "0:host:__OTR__objects/combo_chateau_romani_3ds/OpaqueDL",
            "1:host:__OTR__objects/combo_chateau_romani_3ds/GlassDL"};
        else if (d) expected = {"0:donor:native-opaque", "1:donor:native-glass"};
        if (calls != expected) {
            fprintf(stderr, "FAIL Chateau route Alt=%d donor=%d opaque=%d glass=%d\n", a,d,o,g);
            return 1;
        }
        assert(shimmerCalls == int(use3ds || d));
    }
    // A live Alt toggle must not retain a donor/custom pointer from an earlier draw.
    donor = opaque = glass = true;
    for (bool enabled : {true, false, true}) {
        alt = enabled; calls.clear(); Randomizer_DrawChateauRomani(&play, nullptr);
        assert(calls.size() == 2);
        assert((calls[0].find(":host:") != std::string::npos) == enabled);
    }
    puts("PASS Chateau production draw: 16 availability cases, both passes, Alt ON/OFF/ON");
}
'''.replace('PRODUCTION', function)
with tempfile.TemporaryDirectory(prefix='chateau-3ds-draw-') as temp:
    path = Path(temp)
    (path / 'test.cpp').write_text(fixture)
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-parameter', str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
