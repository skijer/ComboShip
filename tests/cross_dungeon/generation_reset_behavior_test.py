#!/usr/bin/env python3
"""Exercise reset and finalize against a live generation worker, without game assets."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from

parser = argparse.ArgumentParser()
parser.add_argument('--sanitize', action='store_true')
args = parser.parse_args()
if args.sanitize:
    os.environ['ASAN_OPTIONS'] = 'detect_leaks=0'
host = (ROOT / 'soh/soh/OTRGlobals.cpp').read_text()
launcher = (ROOT / 'combo/ComboShip.cpp').read_text()
register_cancel = block_from(host, host.index('extern "C" COMBO_EXPORT void SOH_SetOnComboCancelCallback('))
reset = block_from(host, host.index('bool Combo_HandleReset(void)'))
poll = block_from(launcher, launcher.index('static int Combo_PollFinalize()'))
marker = 'static void Combo_CancelGenerate()'
cancel = block_from(launcher, launcher.index(marker))
source = r'''
#define COMBO_BUILD 1
#include "ComboGameMode.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#include "gui/ComboGenProgress.h"
#include <atomic>
#include <cassert>
#include <iostream>
#include <thread>
#define COMBO_EXPORT
static ComboRando::ComboGenProgress g_ComboProgress;
static std::thread g_GenerateThread;
static std::atomic<bool> g_GenerateBusy{false}, g_ComboPendingFinalize{false};
static std::atomic<bool> workerExited{false};
struct WorkerCleanup {
    ~WorkerCleanup() { workerExited=true; }
};
static bool generating=false, foregroundOot=true, sComboResetPending=false, seedGenerated=true;
static int finalizes=0;
static bool IsRandoGenerating() { return generating; }
static void SetComboRandoGenerating(bool value) { generating=value; }
static void WaitForRandoGeneration() {}
static bool Combo_OotIsForeground() { return foregroundOot; }
static void* Combo_ResolveSym(const char*, const char*) { return nullptr; }
static void SetSeedGenerated(uint8_t value) { seedGenerated=value; }
static void (*SOH_SetSeedGenerated)(uint8_t)=SetSeedGenerated;
static void Combo_FinalizeGenerate() {
    assert(!g_GenerateBusy.load() && "finalize must wait for worker cleanup");
    assert(workerExited.load() && "finalize must join the worker before mutating native state");
    assert(!g_GenerateThread.joinable());
    ++finalizes;
}
''' + cancel + r'''
static void (*gComboCancelGenerationCallback)()=nullptr;
''' + register_cancel + '\n' + reset + '\n' + poll + r'''
int main() {
    SOH_SetOnComboCancelCallback(Combo_CancelGenerate);
    generating=true; g_ComboProgress.running=true; g_ComboProgress.done=false;
    g_GenerateBusy=true; g_ComboPendingFinalize=false;
    g_GenerateThread=std::thread([] {
        WorkerCleanup cleanup;
        while (!g_ComboProgress.cancel.load()) std::this_thread::yield();
        // The real worker restores its temporary cross map before it exits.
        g_ComboProgress.done=true; g_GenerateBusy=false;
    });
    assert(!Combo_HandleReset()); // OoT's normal reset can now run safely.
    assert(workerExited && !g_GenerateThread.joinable());
    assert(!generating && !g_GenerateBusy && !g_ComboPendingFinalize && !seedGenerated);
    assert(!g_ComboProgress.running && !g_ComboProgress.success && finalizes==0);
    // Success is published before the worker's final scope cleanup. Do not apply it early.
    g_GenerateBusy=true; g_ComboPendingFinalize=true; workerExited=false;
    assert(Combo_PollFinalize()==0 && finalizes==0 && g_ComboPendingFinalize);
    g_GenerateThread=std::thread([] { WorkerCleanup cleanup; g_GenerateBusy=false; });
    while (g_GenerateBusy.load()) std::this_thread::yield();
    assert(Combo_PollFinalize()==1 && finalizes==1 && !g_ComboPendingFinalize);
    // Startup COMBO_AUTOGEN_SEED uses a synchronous fill with no progress pointer.
    g_ComboProgress.done=false; g_ComboPendingFinalize=true;
    assert(Combo_PollFinalize()==1 && finalizes==2);
    // A completed, unfinalized result is discarded by Reset as well.
    generating=true; g_ComboPendingFinalize=true; seedGenerated=true;
    g_ComboProgress.success=true; g_ComboProgress.running=true;
    assert(!Combo_HandleReset());
    assert(!g_ComboPendingFinalize && !seedGenerated && !g_ComboProgress.success);
    assert(finalizes==2 && !generating);
    g_GenerateBusy=true; workerExited=false; seedGenerated=true;
    g_GenerateThread=std::thread([] { WorkerCleanup cleanup; g_GenerateBusy=false; });
    while (g_GenerateBusy.load()) std::this_thread::yield();
    assert(Combo_PollFinalize()==1 && !g_GenerateThread.joinable() && !seedGenerated);
    assert(finalizes==2); // Failed generation also finishes cleanup before unlocking the UI.
    generating=true; gComboCancelGenerationCallback=nullptr;
    assert(Combo_HandleReset() && generating); // Mixed-version fallback must not reset under a worker.
    generating=false; foregroundOot=false;
    assert(Combo_HandleReset() && sComboResetPending); // Existing MM return path survives.
    std::cout<<"Generation lifetime: reset joins worker, discards pending result, and unlocks file select; finalize waits for cleanup\n";
}
'''
with tempfile.TemporaryDirectory(prefix='generation-reset-') as tmp:
    path = Path(tmp)
    (path / 'test.cpp').write_text(source)
    cmd = ['c++', '-std=c++20', '-pthread', '-I' + str(ROOT / 'combo')]
    if args.sanitize:
        cmd += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g']
    subprocess.run(cmd + [str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
