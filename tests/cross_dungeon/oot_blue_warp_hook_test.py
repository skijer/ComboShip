#!/usr/bin/env python3
"""Compile the real blue-warp hook portably and exercise both build modes."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--compiler', default=os.environ.get('CXX', 'g++'))
parser.add_argument('--msvc', action='store_true')
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
if args.msvc and args.sanitize:
    parser.error('--sanitize requires the GCC/Clang build mode')

ROOT = Path(__file__).resolve().parents[2]
production = (ROOT / 'soh/soh/Enhancements/TimeSavers/SkipCutscene/Story/SkipBlueWarp.cpp').read_text()
header = (ROOT / 'soh/soh/Enhancements/game-interactor/GameInteractor.h').read_text()
macro = re.search(r'^#define REGISTER_VB_SHOULD\(flag, body\).*?(?=\n\n)', header, re.M | re.S).group()
start = production.index('void RegisterShouldPlayBlueWarp()')
opening = production.index('{', start)
depth, end = 1, opening + 1
while depth:
    depth += (production[end] == '{') - (production[end] == '}')
    end += 1
hooks = production[start:end]
policy = production[production.index('static bool sEnteredBlueWarp'):production.index('extern void TimeSaverQueueItem')]
names = sorted(set(re.findall(r'\b(?:ENTR_|RSK_|CHAMBER_CS_|EVENTCHKINF_|VB_|GAMEMODE_|TIMER_STATE_)[A-Z0-9_]+', hooks + policy)))
prefix = r'''
#include <cassert>
#include <cstdarg>
using GIVanillaBehavior = int;
''' + 'enum { ' + ','.join(names) + ' };\n' + r'''
static bool isRando = false, isBossRush = false, crossEnabled = false;
static bool settings[1000]{};
static int storySkip = 0, warpOverrides = 0, completeNotifications = 0;
static struct {
    int gameMode = 0, entranceIndex = 0, cutsceneIndex = 0, chamberCutsceneNum = 0;
    int timerState = 0, dayTime = 0, skyboxTime = 0, nightFlag = 0;
} gSaveContext;
#define IS_RANDO isRando
#define IS_BOSS_RUSH isBossRush
#define RAND_GET_OPTION(option) settings[option]
#define CVAR_ENHANCEMENT(name) name
static int CVarGetInteger(const char*, int) { return storySkip; }
static bool Combo_SOH_CrossBossEnabled() { return false; }
static void Flags_SetEventChkInf(int) {}
static void Entrance_OverrideBlueWarp() { ++warpOverrides; }
struct GameInteractor {
    struct OnVanillaBehavior {};
    using Hook = void (*)(GIVanillaBehavior, bool*, va_list);
    Hook callbacks[1000]{};
    static GameInteractor* Instance;
    template<typename T> void RegisterGameHookForID(int id, Hook hook) { callbacks[id] = hook; }
};
static GameInteractor interactor;
GameInteractor* GameInteractor::Instance = &interactor;
#ifdef COMBO_BUILD
extern "C" int Combo_SOH_CrossDungeonRuntimeEnabled() { return crossEnabled; }
extern "C" int Combo_SOH_CompleteCrossDungeonTransition() { ++completeNotifications; return 0; }
#endif
static void Invoke(int id, bool* should, ...) {
    va_list args;
    va_start(args, should);
    interactor.callbacks[id](id, should, args);
    va_end(args);
}
'''
main = r'''
int main() {
    RegisterShouldPlayBlueWarp();
    for (int mask = 0; mask < 32; ++mask) {
        isRando = bool(mask & 1);
        settings[RSK_SHUFFLE_DUNGEON_ENTRANCES] = bool(mask & 2);
        settings[RSK_SHUFFLE_BOSS_ENTRANCES] = bool(mask & 4);
        crossEnabled = bool(mask & 8);
        storySkip = bool(mask & 16);
        bool nativeShuffle = bool(mask & 6);
#ifdef COMBO_BUILD
        bool crossShuffle = crossEnabled;
#else
        bool crossShuffle = false;
#endif
        bool routeOverride = isRando && (nativeShuffle || crossShuffle);
        bool skip = storySkip || routeOverride;
        gSaveContext = {};
        gSaveContext.entranceIndex = ENTR_KOKIRI_FOREST_0;
        gSaveContext.cutsceneIndex = 0xFFF1;
        gSaveContext.timerState = TIMER_STATE_ENV_HAZARD_TICK;
        gSaveContext.nightFlag = 1;
        bool should = true;
        warpOverrides = 0;
        completeNotifications = 0;
        Invoke(VB_PLAY_BLUE_WARP_CS, &should);
        Invoke(VB_PLAY_TRANSITION_CS, &should);
        assert(should == !skip && "native and cross-game shuffles must force the blue-warp skip");
        assert(warpOverrides == int(routeOverride) && "only active randomizer routing overrides the exit");
#ifdef COMBO_BUILD
        assert(completeNotifications == int(routeOverride) && "native-init redirects acknowledge the completed source sequence");
#else
        assert(completeNotifications == 0);
#endif
        assert(!sEnteredBlueWarp && "transition must consume the blue-warp observation");
        if (skip) {
            assert(gSaveContext.entranceIndex == ENTR_KOKIRI_FOREST_DEKU_TREE_BLUE_WARP);
            assert(gSaveContext.cutsceneIndex == 0 && gSaveContext.timerState == TIMER_STATE_OFF);
            assert(gSaveContext.dayTime == 0x8000 && gSaveContext.nightFlag == 0);
        } else {
            assert(gSaveContext.entranceIndex == ENTR_KOKIRI_FOREST_0);
            assert(gSaveContext.cutsceneIndex == 0xFFF1);
        }
    }
    // Credits and boss rush must preserve their native transition even with routing/skip enabled.
    isRando = true;
    crossEnabled = true;
    storySkip = 1;
    for (int mode = 0; mode < 2; ++mode) {
        isBossRush = mode == 0;
        gSaveContext = {};
        gSaveContext.gameMode = mode == 1 ? GAMEMODE_END_CREDITS : 0;
        gSaveContext.entranceIndex = ENTR_KOKIRI_FOREST_0;
        gSaveContext.cutsceneIndex = 0xFFF1;
        bool should = true;
        warpOverrides = 0;
        completeNotifications = 0;
        Invoke(VB_PLAY_TRANSITION_CS, &should);
        assert(should && warpOverrides == 0 && gSaveContext.cutsceneIndex == 0xFFF1);
        assert(completeNotifications == 0);
    }
}
'''

sanitize = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g'] if args.sanitize else []
if sanitize:
    os.environ['ASAN_OPTIONS'] = 'detect_leaks=0'
with tempfile.TemporaryDirectory(prefix='oot-blue-warp-hook-') as temporary:
    directory = Path(temporary)
    source = directory / 'hook.cpp'
    source.write_text(prefix + macro + '\n' + policy + hooks + main)
    for combo in [False, True]:
        name = 'combo' if combo else 'standalone'
        binary = directory / (name + ('.exe' if args.msvc else ''))
        if args.msvc:
            command = [args.compiler, '/nologo', '/std:c++20', '/EHsc', '/W3', '/WX', '/wd4100',
                       *(['/DCOMBO_BUILD'] if combo else []), str(source),
                       '/Fo' + str(directory / (name + '.obj')), '/Fe' + str(binary)]
        else:
            command = [args.compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-Wpedantic',
                       '-Wno-unused-parameter', *sanitize, *(['-DCOMBO_BUILD'] if combo else []),
                       str(source), '-o', str(binary)]
        subprocess.run(command, cwd=directory, check=True)
        subprocess.run([str(binary)], check=True)
        print(f'PASS: {binary.name} production blue-warp hook portability and 34 routing cases')
