"""Compare production authored Sage effects with the intact native fallback."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_time_pedestal_tests import functions

flags = ["-std=c++20", "-DF3DEX_GBI_2", "-DLOG_LEVEL_GAME_PRINTS=0"]
flags += ["-I" + str(ROOT / path) for path in
          ("soh", "soh/include", "soh/src", "soh/assets", "soh/mods",
           "libultraship/include", "combo/menu")]
for config in ("CMake/soh-cvars.cmake", "CMake/lus-cvars.cmake"):
    for key, value in re.findall(r'set\((CVAR_PREFIX_\w+)\s+"?([^\s"\)]+)', (ROOT / config).read_text()):
        flags.append(f'-D{key}="{value}"')
flags += ["-O1", "-g", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
          "-fno-pie", "-no-pie", "-ffunction-sections", "-fdata-sections"]
if "--fast-math" in sys.argv:
    flags.append("-ffast-math")

draw = functions((ROOT / "soh/soh/Enhancements/randomizer/draw.cpp").read_text())
graph = functions((ROOT / "soh/src/code/graph.c").read_text())
fixture = (ROOT / "tests/nei_gi/presentation_test.cpp").read_text()
# Keep the real presentation renderer and real fallback. Only the engine
# boundary comes from the existing arena fixture, with full pose recording.
fixture = fixture.replace("ORIGINAL(Randomizer_DrawExtSagesTunic)", "\n".join(
    draw[name] for name in ("DrawCustomItemDiamondTint", "DrawCustomTunicTint",
                           "Randomizer_DrawExtSagesTunic")))
fixture = fixture.replace(functions(fixture)["gSPDisplayList"],
                         "void gSPDisplayList(Gfx* p, Gfx* d) { gDma1p(p, G_DL_OTR_FILEPATH, d, 0, G_DL_PUSH); }")
fixture = fixture.replace("namespace Fixture {", '''
namespace ParticleProbe {
using Pose = std::array<float, 16>;
constexpr Pose Identity{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
Pose pose = Identity;
std::vector<Pose> stack, submitted;
Gfx setup26{};
void Multiply(const Pose& rhs) {
    Pose out{};
    for (int r=0;r<4;++r) for(int c=0;c<4;++c) for(int k=0;k<4;++k)
        out[r*4+c] += pose[r*4+k] * rhs[k*4+c];
    pose = out;
}
void Reset() { pose=Identity; stack.clear(); submitted.clear(); }
}
namespace Fixture {''', 1)
fixture = fixture.replace("  enabled = alt = dinSword", "  ParticleProbe::Reset();\n  enabled = alt = dinSword", 1)
fixture = fixture.replace("  Fixture::stack.emplace_back", "  ParticleProbe::stack.push_back(ParticleProbe::pose);\n  Fixture::stack.emplace_back", 1)
fixture = fixture.replace("  assert(!Fixture::stack.empty());", "  ParticleProbe::pose=ParticleProbe::stack.back();\n  ParticleProbe::stack.pop_back();\n  assert(!Fixture::stack.empty());", 1)
fixture = fixture.replace(functions(fixture)["Matrix_Scale"], '''
void Matrix_Scale(float x, float y, float z, uint8_t) {
    ParticleProbe::Multiply({x,0,0,0, 0,y,0,0, 0,0,z,0, 0,0,0,1});
    Fixture::matrix *= x;
}''')
fixture = fixture.replace(functions(fixture)["Matrix_RotateY"], '''
void Matrix_RotateY(float a, uint8_t) {
    float c=std::cos(a),s=std::sin(a);
    ParticleProbe::Multiply({c,0,s,0, 0,1,0,0, -s,0,c,0, 0,0,0,1});
}''')
fixture = fixture.replace(functions(fixture)["Matrix_Translate"], '''
void Matrix_Translate(float x, float y, float z, uint8_t mode) {
    if(mode==MTXMODE_NEW) { ParticleProbe::pose=ParticleProbe::Identity; Fixture::matrix=1; Fixture::matrixY=0; }
    ParticleProbe::Multiply({1,0,0,x, 0,1,0,y, 0,0,1,z, 0,0,0,1});
    Fixture::matrixY += y * Fixture::matrix;
}''')
fixture = fixture.replace("  ++Fixture::allocations;", "  ParticleProbe::submitted.push_back(ParticleProbe::pose);\n  ++Fixture::allocations;", 1)
fixture = fixture.replace(functions(fixture)["Gfx_SetupDL_26Opa"], '''
void Gfx_SetupDL_26Opa(GraphicsContext* ctx) {
    OPEN_DISPS(ctx);
    __gSPDisplayList(POLY_OPA_DISP++, &ParticleProbe::setup26);
    CLOSE_DISPS(ctx);
}''')
mm = functions((ROOT / "mm/2s2h/Rando/NeiGiPresentation.cpp").read_text())

with tempfile.TemporaryDirectory(prefix="sages-tunic-particles-") as temp:
    out = Path(temp)
    (out / "nei_gi_graph.inc").write_text(graph["Graph_OpenDisps"] + "\n" + graph["Graph_CloseDisps"])
    (out / "nei_gi_dispatch.inc").write_text("\n".join(
        draw[name] for name in ("Randomizer_DrawCaneSomariaUpgradeFlame", "Randomizer_DrawTrueMasterSwordFlame")))
    source = '''
#define NEI_GI_FIXTURE_BOUNDARY_ONLY
#include "ComboItemDrawABI.h"
#include "objects/object_gi_medal/object_gi_medal.h"
#include "objects/object_gi_clothes/object_gi_clothes.h"
#define Gfx_SetupDL26_Opa Gfx_SetupDL_26Opa
''' + fixture + '''
extern "C" { PlayState* gPlayState=&Fixture::play; }
void DrawOotSlateRuneFlame(u8,u8,u8) { assert(false && "Sage's Tunic borrowed a cane flame"); }
''' + mm["MM_DrawNeiGi"] + "\n" + (ROOT / "tests/nei_gi/sages_tunic_particles_test.inc").read_text()
    for host in ("oot", "mm"):
        path = out / (host + ".cpp")
        path.write_text(("#define NEI_GI_NATIVE_MM 1\n" if host == "mm" else "") + source)
        binary = path.with_suffix("")
        subprocess.run([os.environ.get("CXX", "c++"), *flags, "-I" + temp, str(path),
                        "-Wl,--gc-sections", "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
    print("PASS production Sage medallions: authored/native routes, exact fallback animation, settings and short arenas")
