#!/usr/bin/env python3
"""Exercise the production resume paths against the shared crash-handler boundary."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts/diagnostics"))
from run_mm_nei_tests import flags


def resume_body(path, name):
    source = (ROOT / path).read_text(encoding="utf-8-sig")
    start = source.index('extern "C" COMBO_EXPORT void ' + name + '(')
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


HARNESS = r'''
#include <cassert>
#include <memory>
#include <libultraship/bridge/crashhandlerbridge.h>
#define COMBO_EXPORT
#define SPDLOG_INFO(...) ((void)0)
namespace spdlog::level { constexpr int trace = 0; }
namespace ItemGrantAudit { struct Scope { explicit Scope(const char*) {} }; }
namespace Ship {
struct Logger { void flush_on(int) {} };
struct Gui { void* GetImGuiContext() { return nullptr; } };
struct Window { virtual ~Window() = default; Gui gui; Gui* GetGui() { return &gui; } };
}
namespace Fast {
struct Fast3dWindow : Ship::Window { void SetIsRunning(bool) {} };
}
namespace Ship {
struct Context {
    Logger logger;
    std::shared_ptr<Window> window = std::make_shared<Fast::Fast3dWindow>();
    static Context* GetRawInstance() { static Context ctx; return &ctx; }
    Logger* GetLogger() { return &logger; }
    void SetResourceManager(int) {}
    std::shared_ptr<Window> GetWindow() { return window; }
};
}
namespace ImGui { void SetCurrentContext(void*) {} }
namespace BenGui { void ActivateMenu() {} }
using s32 = int;
static void (*activeCallback)(char*, size_t*) = nullptr;
extern "C" void CrashHandlerRegisterCallback(void (*callback)(char*, size_t*)) { activeCallback = callback; }
extern "C" void CrashHandler_PrintExt(char*, size_t*) {}
extern "C" void CrashHandler_PrintSohData(char*, size_t*) {}
static int sMMResourceManager, gComboStartFileNum, gComboReturnFileNum;
static bool sComboResetPending;
static struct { int fileNum; } gSaveContext = { 2 };
static int mmLoops, ootLoops, exitCalls, exitedSlot = -1;
void GameInteractor_ExecuteOnExitGame(int slot) {
    assert(activeCallback == CrashHandler_PrintSohData);
    ++exitCalls;
    exitedSlot = slot;
}
void OTRAudio_Init() { assert(activeCallback == CrashHandler_PrintExt); }
void MM_ResetSystemHeapForResume() {}
void MM_ResetFrameLoopForResume() {}
void SOH_ResetFrameLoopForResume() {}
void SOH_ReinitForResume() { assert(activeCallback == CrashHandler_PrintSohData); }
bool WindowIsRunning() { return true; }
void MM_RunGameLoop() { assert(activeCallback == CrashHandler_PrintExt); ++mmLoops; }
void SOH_RunGameLoop() { assert(activeCallback == CrashHandler_PrintSohData); ++ootLoops; }
'''


def main():
    with tempfile.TemporaryDirectory(prefix="combo-crash-owner-") as td:
        source = Path(td) / "owner.cpp"
        binary = Path(td) / "owner"
        source.write_text(HARNESS + "\n" +
                          resume_body("mm/2s2h/BenPort.cpp", "MM_ResumeGame") + "\n" +
                          resume_body("soh/soh/OTRGlobals.cpp", "SOH_ResumeGame") + r'''
int main() {
    activeCallback = CrashHandler_PrintSohData;
    for (int i = 0; i < 4; ++i) {
        MM_ResumeGame(i);
        assert(gComboStartFileNum == i);
        SOH_ResumeGame();
        assert(gComboReturnFileNum == gSaveContext.fileNum);
    }
    assert(mmLoops == 4 && ootLoops == 4 && exitCalls == 0);
    sComboResetPending = true;
    SOH_ResumeGame();
    assert(exitCalls == 1 && exitedSlot == 2 && gComboReturnFileNum == -1 && !sComboResetPending);
    sComboResetPending = true;
    gSaveContext.fileNum = 0xFF;
    SOH_ResumeGame();
    assert(exitCalls == 1 && gComboReturnFileNum == -1 && !sComboResetPending);
    assert(mmLoops == 4 && ootLoops == 6);
}
''')
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-O1",
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                        "-I" + str(ROOT / "libultraship/include"), str(source), "-o", str(binary)], check=True)
        env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0")
        subprocess.run([str(binary)], check=True, env=env)
        print("PASS production resume paths rebind shared crash reporter before audio and game loops")

        native = (ROOT / "mm/src/code/z_actor.c").read_text()
        start = native.index("static MMCrashActorContext sCrashActorContext;")
        end = native.index("\ntypedef struct {", start)
        actor_source = Path(td) / "actor_context.c"
        actor_binary = Path(td) / "actor_context"
        actor_source.write_text(r'''
#include "z64.h"
#include "crash_actor_context.h"
#include "din_fire_sword.h"
#include <assert.h>
#include <setjmp.h>
''' + native[start:end] + r'''
static PlayState play;
static Actor outerActor, innerActor;
static jmp_buf crashEscape;
static unsigned finishBegins, finishEnds;
static void CheckSnapshot(Actor* actor, PlayState* state) {
    const MMCrashActorContext* context = MM_GetCrashActorContext();
    assert(context->active && context->sceneId == state->sceneId);
    assert(context->room == state->roomCtx.curRoom.num);
    assert(context->actorId == actor->id && context->category == actor->category);
    assert(context->params == actor->params && context->actor == (uintptr_t)actor);
    assert(context->callback == (uintptr_t)actor->update);
}
void DinFireSword_FinishBeginActor(PlayState* state, Actor* actor) {
    CheckSnapshot(actor, state);
    ++finishBegins;
}
void DinFireSword_FinishEndActor(PlayState* state, Actor* actor) {
    CheckSnapshot(actor, state);
    ++finishEnds;
}
static void InnerUpdate(Actor* actor, PlayState* state) { CheckSnapshot(actor, state); }
static void OuterUpdate(Actor* actor, PlayState* state) {
    CheckSnapshot(actor, state);
    Actor_RunUpdateWithCrashContext(&innerActor, state);
    CheckSnapshot(actor, state);
}
static void FailedUpdate(Actor* actor, PlayState* state) {
    CheckSnapshot(actor, state);
    actor->params = 999;
    state->sceneId = 0;
    longjmp(crashEscape, 1);
}
int main(void) {
    play.sceneId = 109;
    play.roomCtx.curRoom.num = 3;
    outerActor.id = ACTOR_EN_ITEM00;
    outerActor.category = ACTORCAT_PROP;
    outerActor.params = -7;
    outerActor.update = OuterUpdate;
    innerActor.id = ACTOR_EN_WOOD02;
    innerActor.category = ACTORCAT_PROP;
    innerActor.params = 2;
    innerActor.update = InnerUpdate;
    assert(!MM_GetCrashActorContext()->active);
    Actor_RunUpdateWithCrashContext(&outerActor, &play);
    assert(!MM_GetCrashActorContext()->active);
    assert(finishBegins == 2 && finishEnds == 2);
    outerActor.update = FailedUpdate;
    if (!setjmp(crashEscape)) Actor_RunUpdateWithCrashContext(&outerActor, &play);
    const MMCrashActorContext* saved = MM_GetCrashActorContext();
    assert(saved->active && saved->sceneId == 109 && saved->room == 3);
    assert(saved->actorId == ACTOR_EN_ITEM00 && saved->params == -7);
    assert(saved->actor == (uintptr_t)&outerActor && saved->callback == (uintptr_t)FailedUpdate);
    assert(finishBegins == 3 && finishEnds == 2);
    return 0;
}
''')
        subprocess.run([os.environ.get("CC", "cc"), "-std=gnu11", *flags(),
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                        str(actor_source), "-o", str(actor_binary)], check=True)
        subprocess.run([str(actor_binary)], check=True, env=env)
        print("PASS real MM actor snapshot survives failed dispatch and restores nested/successful dispatch")


if __name__ == "__main__":
    main()
