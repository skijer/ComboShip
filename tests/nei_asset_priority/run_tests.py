"""Archive provenance, Alt selection and stock companion regression checks."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def native_function(source, name):
    start = source.index('extern "C" int ' + name + '(')
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


with tempfile.TemporaryDirectory(prefix="nei-priority-") as directory:
    binary = str(Path(directory) / "priority")
    subprocess.run(["c++", "-std=c++20", str(ROOT / "tests/nei_asset_priority/priority_test.cpp"), "-o", binary], check=True)
    subprocess.run([binary], check=True)
    # Compile the real MM bridge in its native macro environment. A SoH-only
    # CVAR_ENHANCEMENT definition would hide a production MM compile failure.
    source = (ROOT / 'mm/2s2h/BenPort.cpp').read_text()
    menu = (ROOT / 'mm/2s2h/BenGui/BenMenu.cpp').read_text()
    setting = re.search(r'\.CVar\("([^"]*\.DinFireSword)"\)', menu).group(1)
    unit = Path(directory) / 'mm-bridge.cpp'
    unit.write_text('''#include <cassert>
#include <cstring>
static bool enabled = false;
static int expectedPresentation = 0;
static const char* expectedSetting = "''' + setting + '''";
static int CVarGetInteger(const char* name, int fallback) {
    assert(std::strcmp(name, expectedSetting) == 0);
    assert(fallback == 0);
    return enabled;
}
namespace NeiAssetPriority {
static int GetDinSwordGiProfile(const char* native, const char* owner, const char* path, bool fire) {
    assert(std::strcmp(native, "mm") == 0);
    assert(std::strcmp(owner, "oot") == 0);
    assert(std::strcmp(path, "sword") == 0);
    return fire ? 2 : 0;
}
static bool GetGiModelFit(const char* native, const char* owner, const char* path,
                          float scale, float tilt, int presentation, float* fit, int profile) {
    assert(std::strcmp(native, "mm") == 0 && std::strcmp(owner, "oot") == 0);
    assert(std::strcmp(path, "sword") == 0 && scale == 1.0f && tilt == 0.0f);
    assert(presentation == expectedPresentation);
    assert(profile == (enabled ? 2 : 0));
    fit[0] = 3.0f; fit[1] = 4.0f;
    return true;
}
}
''' + '\n'.join(native_function(source, name) for name in
    ('ResourceMgr_GetGiModelFitForGame', 'ResourceMgr_GetDinSwordGiProfileForGame')) + '''
int main() {
    for (bool fire : {false, true}) {
        enabled = fire;
        float fit[2]{};
        assert(ResourceMgr_GetDinSwordGiProfileForGame("oot", "sword") == (fire ? 2 : 0));
        for (int presentation = 0; presentation <= 3; ++presentation) {
            expectedPresentation = presentation;
            assert(ResourceMgr_GetGiModelFitForGame("oot", "sword", 1.0f, 0.0f, presentation, fit) == 1);
            assert(fit[0] == 3.0f && fit[1] == 4.0f);
        }
    }
}
'''.replace('for (bool fire : {false, true})', 'for (int fire = 0; fire <= 1; ++fire)'))
    native_binary = str(Path(directory) / 'mm-bridge')
    subprocess.run(['c++', '-std=c++20', '-Wall', '-Wextra', '-Werror', str(unit), '-o', native_binary], check=True)
    subprocess.run([native_binary], check=True)
print("PASS: base/Alt mod provenance, exact stock/companion paths, live selection changes")
print("PASS: native MM GI bridge compiles without SoH macros, follows its menu setting and retains all four framing contexts")
subprocess.run([sys.executable, str(ROOT / 'tests/nei_asset_priority/run_abi_tests.py')], check=True)
