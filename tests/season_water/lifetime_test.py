"""Run the production water backup functions and lifecycle registration.

The fixture compiles the actual functions from season_scene.cpp, with only
the engine types/event dispatcher supplied locally. Missing destruction cleanup
must fail by leaving the original water levels suppressed before cache release.
"""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile


def function(source, marker):
    start = source.index(marker)
    opening = source.index('{', start)
    depth = 1
    end = opening + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def run(source_path, sanitize):
    src = Path(source_path).read_text()
    begin = src.index('#define SEASON_WATER_GONE')
    end = src.index('typedef enum {', begin)
    production = src[begin:end] + '\n' + function(src, 'void RegisterSeasonSceneEffects()')
    harness = r'''
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <type_traits>
#include <vector>
using s16 = int16_t;
using s32 = int32_t;
struct WaterBox { s16 xMin, ySurface, zMin, xLength, zLength; uint32_t properties; };
static_assert(sizeof(WaterBox) == 16);
static_assert(offsetof(WaterBox, ySurface) == 2);
struct CollisionHeader { uint16_t numWaterBoxes; WaterBox* waterBoxes; };
#define MIN(a,b) std::min<s32>((a),(b))
#define COND_ID_HOOK(...) do {} while (0)
struct GameInteractor {
    struct OnPlayDrawEnd {};
    struct OnPlayDestroy {};
    inline static GameInteractor* Instance;
    std::vector<std::function<void()>> destroy;
    template<class Event, class Callback> void RegisterGameHook(Callback callback) {
        if constexpr (std::is_same_v<Event, OnPlayDestroy>) destroy.emplace_back(callback);
    }
    void EndPlay() { for (auto& callback : destroy) callback(); }
};
void SeasonScene_DrawGround() {}
''' + production + r'''
int main() {
    GameInteractor events;
    GameInteractor::Instance = &events;
    RegisterSeasonSceneEffects();
    auto* first = new WaterBox[6]{};
    const s16 heights[] = {-60, -60, -60, -60, -60, -315};
    for (int i = 0; i < 6; ++i) first[i].ySurface = heights[i];
    auto* oldHeader = new CollisionHeader{6, first};
    SeasonScene_SuppressWaterBoxes(oldHeader);
    SeasonScene_SuppressWaterBoxes(oldHeader); // Repeated frames retain originals.
    for (int i = 0; i < 6; ++i) assert(first[i].ySurface == -32000);
    events.EndPlay(); // Play_Destroy calls this before scene resources can retire.
    for (int i = 0; i < 6; ++i) {
        if (first[i].ySurface != heights[i]) {
            std::cerr << "FAIL: outgoing water still suppressed at Play destruction\n";
            return 1;
        }
    }
    delete oldHeader;
    delete[] first;
    // The production restore must now be harmless even after the old allocation
    // has been released. ASan catches retaining the stale pointer across reload.
    SeasonScene_RestoreWaterBoxes();
    events.EndPlay();
    auto* next = new WaterBox[2]{};
    next[0].ySurface = 120; next[1].ySurface = -85;
    auto* nextHeader = new CollisionHeader{2, next};
    SeasonScene_SuppressWaterBoxes(nextHeader);
    SeasonScene_RestoreWaterBoxes(); // Leaving Summer without a scene reload.
    assert(next[0].ySurface == 120 && next[1].ySurface == -85);
    SeasonScene_SuppressWaterBoxes(nextHeader);
    events.EndPlay();
    assert(next[0].ySurface == 120 && next[1].ySurface == -85);
    delete nextHeader;
    delete[] next;
    SeasonScene_RestoreWaterBoxes();
    std::cout << "PASS: registered destruction restores six original water levels; "
                 "reload, repeated cleanup, and later seasons use no freed memory\n";
}
'''
    with tempfile.TemporaryDirectory(prefix='field-water-lifetime-') as work:
        path = Path(work) / 'water_lifetime.cpp'
        binary = Path(work) / 'water_lifetime'
        path.write_text(harness)
        cmd = ['c++', '-std=c++20', '-O1', '-g', '-Wall', '-Wextra', '-Werror']
        if sanitize:
            cmd += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-no-pie']
        subprocess.run(cmd + [str(path), '-o', str(binary)], check=True)
        env = dict(os.environ)
        # LeakSanitizer cannot inspect other tasks in this managed runtime;
        # address/undefined-behavior checks remain active.
        env['ASAN_OPTIONS'] = 'detect_leaks=0'
        return subprocess.run([str(binary)], env=env).returncode


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('source'); parser.add_argument('--sanitize', action='store_true')
    args = parser.parse_args()
    raise SystemExit(run(args.source, args.sanitize))
