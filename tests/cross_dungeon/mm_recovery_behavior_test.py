#!/usr/bin/env python3
"""Exercise native Stone Tower arrival normalization and its initialization boundary."""
import argparse
import os
import pathlib
import re
import subprocess
import sys
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument("--sanitize", action="store_true")
args = parser.parse_args()

root = pathlib.Path(__file__).resolve().parents[2]
sys.path.insert(0, str(root / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from

source = (root / 'mm/src/code/z_play.c').read_text()
match = re.search(r'static void Combo_NormalizeStoneTowerArrival\([^;{}]*\)\s*\{', source)
assert match, 'Stone Tower cross arrival normalization is missing'
helper = block_from(source, match.start())
init = source[source.index('void Play_Init(GameState*'):]
assert init.index('Actor_InitContext(') < init.index('Combo_NormalizeStoneTowerArrival(this)')
assert init.index('Combo_NormalizeStoneTowerArrival(this)') < init.index('while (!Room_ProcessRoomRequest(')
assert init.count('Combo_NormalizeStoneTowerArrival(this)') == 1
program = r'''
#include <cassert>
#include <cstdint>
struct PlayState { int sceneId; uint32_t flags; };
enum { SCENE_F40, SCENE_F41, SCENE_INISIE_N, SCENE_INISIE_R, SCENE_OTHER };
static bool enabled = false;
int Combo_MM_CrossDungeonRuntimeEnabled() { return enabled; }
void Flags_SetSwitch(PlayState* play, int flag) { play->flags |= uint32_t{1} << flag; }
void Flags_UnsetSwitch(PlayState* play, int flag) { play->flags &= ~(uint32_t{1} << flag); }
''' + helper + r'''
int main() {
    constexpr uint32_t inversion = uint32_t{1} << 0x14;
    constexpr uint32_t other = 0x80000003;
    PlayState play{SCENE_F41, other};
    Combo_NormalizeStoneTowerArrival(&play);
    assert(play.flags == other); // Off preserves the exact native state.
    enabled = true;
    Combo_NormalizeStoneTowerArrival(&play);
    assert(play.flags == (other | inversion));
    Combo_NormalizeStoneTowerArrival(&play);
    assert(play.flags == (other | inversion)); // Re-entry is idempotent.
    play.sceneId = SCENE_F40;
    Combo_NormalizeStoneTowerArrival(&play);
    assert(play.flags == other);
    for (int scene : {SCENE_INISIE_N, SCENE_INISIE_R, SCENE_OTHER}) {
        play = {scene, other | inversion};
        Combo_NormalizeStoneTowerArrival(&play);
        assert(play.flags == (other | inversion));
    }
    play = {SCENE_F40, other};
    Flags_SetSwitch(&play, 0x14); // Gameplay may still request intentional inversion.
    assert(play.flags == (other | inversion));
}
'''
# initializer_list supports the parallel non-exterior cases above.
program = '#include <initializer_list>\n' + program
with tempfile.TemporaryDirectory(prefix='stone-tower-recovery-') as temp:
    path = pathlib.Path(temp)
    (path / 'test.cpp').write_text(program)
    sanitizer = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g'] if args.sanitize else []
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror', *sanitizer,
                    str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('PASS native Stone Tower exterior normalization, off/interior preservation, init ordering')
