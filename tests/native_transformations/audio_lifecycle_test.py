#!/usr/bin/env python3
"""Exercise the actual native-mask controller's audio lifetime boundary.

Default: read the production controller and form entry points from this repo.
Restricted/no-checkout mode: --source-json - accepts {"native": str, "form": str}
on stdin. Sources remain in memory and reach the compiler through stdin only.
--patch applies a supplied unified controller patch in memory for RED/GREEN work.
Animation, camera/light services and queued audio ownership are explicit stubs;
this verifies controller decisions and retained-pointer cleanup, not PCM output.
"""
from pathlib import Path
import argparse
import json
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
NATIVE_PATH = "soh/mods/transformation_masks/mm_native_transform.inc.h"
FORM_PATH = "soh/mods/transformation_masks/mm_player_form.cpp"


def function(source, name):
    match = re.search(r"^[^\n{};]+\b" + re.escape(name) + r"\([^;{}]*\)\s*\{", source, re.M)
    if not match:
        raise AssertionError("Missing complete production function: " + name)
    cursor, depth = match.end(), 1
    while depth:
        depth += (source[cursor] == "{") - (source[cursor] == "}")
        cursor += 1
    return source[match.start():cursor]


def apply_patch(source, patch):
    """Strict single-file unified patch application without writing source."""
    old = source.splitlines(keepends=True)
    result, cursor, touched = [], 0, False
    lines = patch.splitlines(keepends=True)
    i = 0
    selected = False
    while i < len(lines):
        line = lines[i]
        if line.startswith("+++ "):
            selected = line[4:].strip().removeprefix("b/") == NATIVE_PATH
            i += 1
            continue
        match = re.match(r"@@ -(\d+)(?:,\d+)? \+\d+(?:,\d+)? @@", line)
        if not selected or not match:
            i += 1
            continue
        start = int(match.group(1)) - 1
        assert start >= cursor, "Overlapping patch hunks"
        result.extend(old[cursor:start])
        cursor = start
        touched = True
        i += 1
        while i < len(lines) and lines[i][:1] in (" ", "+", "-"):
            hunk = lines[i]
            if hunk.startswith(("--- ", "+++ ")):
                break
            if hunk[0] != "+":
                assert cursor < len(old) and old[cursor] == hunk[1:], (
                    "Patch context mismatch at source line " + str(cursor + 1))
                cursor += 1
            if hunk[0] != "-":
                result.append(hunk[1:])
            i += 1
    assert touched, "Patch has no controller hunks"
    result.extend(old[cursor:])
    return "".join(result)


FIXTURE = r'''
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>
using u8=uint8_t; using u16=uint16_t; using u32=uint32_t;
using s8=int8_t; using s16=int16_t; using s32=int32_t; using f32=float;
struct Vec3f {f32 x=0,y=0,z=0;}; struct Vec3s {s16 x=0,y=0,z=0;};
struct VecSph {f32 r=0; s16 pitch=0,yaw=0;};
struct LinkAnimationHeader {int frames;};
struct SkelAnime {LinkAnimationHeader* animation=nullptr;f32 curFrame=0,playSpeed=0,startFrame=0,endFrame=0;u8 mode=0;};
struct Actor {Vec3f projectedPos,velocity;f32 yDistToWater=0;};
struct Player {Actor actor;u32 stateFlags1=0,stateFlags3=0;f32 linearVelocity=0;Vec3s headLimbRot,upperLimbRot;SkelAnime skelAnime;};
struct LightInfo {}; struct LightNode {}; struct Gfx {};
struct Input {struct {u16 button=0;} press;struct {s16 stick_x=0,stick_y=0;} cur;};
struct PlayState {struct {Input input[1];} state;struct {s16 adjAmbientColor[3]{},adjLight1Color[3]{},adjFogColor[3]{},adjFogNear=0;} envCtx;int lightCtx=0;s16 activeCamera=0;Player* player=nullptr;};
enum MmPlayerTransformation {MM_PLAYER_FORM_FIERCE_DEITY,MM_PLAYER_FORM_GORON,MM_PLAYER_FORM_ZORA,MM_PLAYER_FORM_DEKU,MM_PLAYER_FORM_HUMAN};
enum {MMFORM_STATE_INACTIVE,MMFORM_STATE_TRANSFORMING,MMFORM_STATE_ACTIVE,MMFORM_STATE_DETRANSFORMING};
constexpr u32 PLAYER_STATE1_INPUT_DISABLED=1,PLAYER_STATE1_IN_CUTSCENE=2,PLAYER_STATE1_IN_WATER=4,PLAYER_STATE3_PAUSE_ACTION_FUNC=8;
constexpr int SUBCAM_NONE=-1,CAM_STAT_ACTIVE=1,ANIMMODE_ONCE=0,ANIMMODE_LOOP=1;
constexpr u16 BTN_A=1,BTN_B=2,BTN_CUP=4,BTN_CDOWN=8,BTN_CLEFT=16,BTN_CRIGHT=32;
constexpr u16 MM_NA_SE_PL_PUT_OUT_ITEM=0x0877,MM_NA_SE_IT_SET_TRANSFORM_MASK=0x1856,
 MM_NA_SE_PL_FREEZE_S=0x0874,MM_NA_SE_IT_TRANSFORM_MASK_BROKEN=0x1858,
 MM_NA_SE_PL_TRANSFORM_VOICE=0x09aa,MM_NA_SE_PL_FACE_CHANGE=0x09a4,
 MM_NA_SE_EV_LIGHTNING_HARD=0x2912,MM_NA_SE_SY_TRANSFORM_MASK_FLASH=0x484f;
#define GET_PLAYER(p) ((p)->player)
#define MIN(a,b) std::min((a),(b))
#define MAX(a,b) std::max((a),(b))
#define ABS(x) std::abs(x)
#define CHECK_BTN_ANY(a,b) ((a)&(b))
#define MMFORM_IS_ZORA_SWIM() 0
#define ZORA_SWIM_THRESHOLD 20
struct {int state=0;MmPlayerTransformation currentForm=MM_PLAYER_FORM_HUMAN,targetForm=MM_PLAYER_FORM_DEKU;s16 cutsceneTimer=0,cutscenePhase=0,flashAlpha=0;u8 skeletonLoaded=0;SkelAnime formSkelAnime;LinkAnimationHeader* idleAnim=nullptr;} gFormState;
static u8 sForceInstantTransform=0;
static Player player;static PlayState play;static PlayState* gPlayState=&play;
static int frame=0,cameraClears=0,lightClears=0,loads=0;
static bool loadFailure=false;
static LinkAnimationHeader maskOn{66},maskLoop{3},maskOff{10},formOff{35},idle{72};
static LightNode light;
struct Event {u16 id;int frame;};
static std::vector<Event> sounds;
static std::vector<u16> stopped;
// The real bank's PCM/stop semantics have a separate real-DSP regression.
// This boundary models both pending and active retained position references.
static std::map<u16,Vec3f*> pending,active;
static s32 MmSfx_PlayAtPos(u16 id,Vec3f* pos) {sounds.push_back({id,frame});pending[id]=pos;return 1;}
static void MmSfx_PlayTransformFlash() {MmSfx_PlayAtPos(MM_NA_SE_SY_TRANSFORM_MASK_FLASH,nullptr);}
static void MmSfx_Stop(u16 id) {stopped.push_back(id);pending.erase(id);active.erase(id);}
static void BankFrame() {active.insert(pending.begin(),pending.end());pending.clear();}
static void LightContext_RemoveLight(PlayState*,int*,LightNode*) {lightClears++;}
static void Play_ClearCamera(PlayState*,s16) {cameraClears++;}
static void Play_ChangeCameraStatus(PlayState* p,s16 id,int) {p->activeCamera=id;}
static void MmForm_RestoreEquips(PlayState*) {}
static void MmForm_SaveAndRestrictEquips(PlayState*) {}
static void ExtEquip_RestoreFromTransform() {}
static void ExtEquip_UnequipForTransform() {}
static void MmForm_UpdateNativeMaskCamera(Player*,PlayState*) {}
static void MmForm_UpdateNativeMaskLighting(Player*,PlayState*) {}
static void Player_SetBootData(PlayState*,Player*) {}
static void MmForm_ApplyFormProperties(Player*,MmPlayerTransformation) {}
static void MmForm_RestoreOotState(Player*) {}
static void MmForm_EnterSwimIdle(Player*,PlayState*) {}
static s16 Animation_GetLastFrame(LinkAnimationHeader* a) {return a->frames-1;}
static void LinkAnimation_Change(PlayState*,SkelAnime* s,LinkAnimationHeader* a,f32 speed,f32 start,f32 end,u8 mode,f32) {s->animation=a;s->curFrame=start;s->playSpeed=speed;s->startFrame=start;s->endFrame=end;s->mode=mode;}
static void LinkAnimation_PlayLoop(PlayState* p,SkelAnime* s,LinkAnimationHeader* a) {LinkAnimation_Change(p,s,a,1,0,Animation_GetLastFrame(a),ANIMMODE_LOOP,0);}
static bool LinkAnimation_Update(PlayState*,SkelAnime* s) {f32 next=s->curFrame+s->playSpeed*1.5f;if(s->mode==ANIMMODE_LOOP) {s->curFrame=next>s->endFrame?s->startFrame:next;return false;}if(s->playSpeed<0) {s->curFrame=std::max(next,s->endFrame);return next<=s->endFrame;}s->curFrame=std::min(next,s->endFrame);return next>=s->endFrame;}
static bool LinkAnimation_OnFrame(SkelAnime* s,f32 target) {return s->curFrame>=target&&s->curFrame-s->playSpeed*1.5f<target;}
static bool Math_StepToF(f32* value,f32 target,f32 step) {*value+=std::clamp(target-*value,-step,step);return *value==target;}
/* NATIVE_STRUCTS */
static bool MmForm_BeginNativeMaskCutscene(Player* p,PlayState* scene,u8 removing) {
 if(gFormState.cutsceneTimer!=0)return false;
 sNativeMaskCs={};sNativeMaskCs.active=1;sNativeMaskCs.owner=scene;sNativeMaskCs.removing=removing;
 sNativeMaskCs.cameraId=1;sNativeMaskCs.previousCamera=0;sNativeMaskCs.lightNode=&light;scene->activeCamera=1;
 sNativeMaskCs.ownedFlags=(PLAYER_STATE1_INPUT_DISABLED|PLAYER_STATE1_IN_CUTSCENE)&~p->stateFlags1;
 sNativeMaskCs.maskOn=&maskOn;sNativeMaskCs.maskLoop=&maskLoop;sNativeMaskCs.maskOff=&maskOff;sNativeMaskCs.formOff=&formOff;
 auto skel=removing?&gFormState.formSkelAnime:&p->skelAnime;
 auto animation=removing?&formOff:&maskOn;
 LinkAnimation_Change(scene,skel,animation,2.0f/3.0f,0,Animation_GetLastFrame(animation),ANIMMODE_ONCE,0);
 return true;
}
static bool MmForm_LoadFormSkeleton(PlayState* p,MmPlayerTransformation) {loads++;if(loadFailure)return false;gFormState.skeletonLoaded=1;gFormState.idleAnim=&idle;LinkAnimation_PlayLoop(p,&gFormState.formSkelAnime,&idle);return true;}
/* PRODUCTION_FUNCTIONS */
static void InitEntry() {auto* player=&::player;auto* play=&::play;/* INIT_ENTRY */}
static void DeathEntry() {/* DEATH_ENTRY */}
static void ResetEntry() {/* RESET_ENTRY */}
static void Begin(bool removing=false) {
 player={};play={};play.player=&player;gPlayState=&play;gFormState={};
 gFormState.currentForm=removing?MM_PLAYER_FORM_DEKU:MM_PLAYER_FORM_HUMAN;
 gFormState.targetForm=removing?MM_PLAYER_FORM_HUMAN:MM_PLAYER_FORM_DEKU;
 gFormState.state=removing?MMFORM_STATE_DETRANSFORMING:MMFORM_STATE_TRANSFORMING;
 gFormState.skeletonLoaded=removing;gFormState.idleAnim=&idle;
 frame=cameraClears=lightClears=loads=0;loadFailure=false;sForceInstantTransform=0;
 sounds.clear();stopped.clear();pending.clear();active.clear();sNativeMaskCs={};
}
static void Tick(bool removing=false) {frame++;MmForm_UpdateNativeMaskCutscene(&player,&play,removing);BankFrame();}
static void Complete(bool removing=false) {
 Begin(removing);play.state.input[0].cur.stick_x=30;
 while((!sNativeMaskCs.active||gFormState.cutscenePhase!=2)&&frame<150)Tick(removing);
 while(sNativeMaskCs.active&&frame<180)Tick(removing);
 assert(!sNativeMaskCs.active&&gFormState.cutscenePhase==2);
 assert(play.activeCamera==0&&cameraClears==1&&lightClears==1);
 assert(!(player.stateFlags1&(PLAYER_STATE1_INPUT_DISABLED|PLAYER_STATE1_IN_CUTSCENE)));
 assert(!(player.stateFlags3&PLAYER_STATE3_PAUSE_ACTION_FUNC));
 assert(stopped.empty()&&"normal completion must let native cue tails finish");
 assert(active.count(MM_NA_SE_SY_TRANSFORM_MASK_FLASH));
 if(!removing)assert(active.count(MM_NA_SE_EV_LIGHTNING_HARD));
}
static void AssertStopped() {
 for(u16 id:{MM_NA_SE_PL_PUT_OUT_ITEM,MM_NA_SE_IT_SET_TRANSFORM_MASK,MM_NA_SE_PL_FREEZE_S,MM_NA_SE_IT_TRANSFORM_MASK_BROKEN,MM_NA_SE_PL_TRANSFORM_VOICE,MM_NA_SE_PL_FACE_CHANGE,MM_NA_SE_EV_LIGHTNING_HARD,MM_NA_SE_SY_TRANSFORM_MASK_FLASH})
   assert(std::count(stopped.begin(),stopped.end(),id)>=1);
 assert(pending.empty()&&active.empty());
}
int main(int argc,char** argv) {
 assert(argc==2);std::string which=argv[1];
 if(which=="normal-on"||which=="normal-off")Complete(which=="normal-off");
 else if(which.rfind("post-",0)==0) {
  Complete();const int cameras=cameraClears,lights=lightClears;
  // A late queued reference must also be removed, even after active was cleared.
  pending[MM_NA_SE_PL_TRANSFORM_VOICE]=&player.actor.projectedPos;
  if(which=="post-destroy")MmForm_OnPlayDestroy(&play,&player);
  else if(which=="post-init")InitEntry();
  else if(which=="post-death")DeathEntry();
  else if(which=="post-reset")ResetEntry();
  else if(which=="post-null-death") {gPlayState=nullptr;DeathEntry();}
  else if(which=="post-address-reuse") {play.envCtx.adjFogNear=37;InitEntry();assert(play.envCtx.adjFogNear==37);}
  else assert(false);
  AssertStopped();assert(cameraClears==cameras&&lightClears==lights);
 } else if(which=="skip") {
  Begin();while(frame<5)Tick();play.state.input[0].press.button=BTN_A;Tick();play.state.input[0].press.button=0;
  while(sNativeMaskCs.active&&frame<80)Tick();
  assert(!sNativeMaskCs.active&&frame<80);
  for(u16 id:{MM_NA_SE_PL_PUT_OUT_ITEM,MM_NA_SE_IT_SET_TRANSFORM_MASK,MM_NA_SE_PL_FREEZE_S,MM_NA_SE_IT_TRANSFORM_MASK_BROKEN,MM_NA_SE_PL_TRANSFORM_VOICE,MM_NA_SE_PL_FACE_CHANGE,MM_NA_SE_EV_LIGHTNING_HARD})assert(std::count(stopped.begin(),stopped.end(),id)==1);
  assert(std::count(stopped.begin(),stopped.end(),MM_NA_SE_SY_TRANSFORM_MASK_FLASH)==0);
  assert(active.count(MM_NA_SE_SY_TRANSFORM_MASK_FLASH));
  MmForm_OnPlayDestroy(&play,&player);AssertStopped();
 } else if(which=="failure") {
  Begin();loadFailure=true;while((frame==0||sNativeMaskCs.active)&&frame<130)Tick();
  assert(loads==1&&!sNativeMaskCs.active&&gFormState.state==MMFORM_STATE_INACTIVE);AssertStopped();
 } else if(which.rfind("abort-",0)==0) {
  Begin();while(frame<34)Tick();pending[MM_NA_SE_PL_FACE_CHANGE]=&player.actor.projectedPos;
  if(which=="abort-destroy")MmForm_OnPlayDestroy(&play,&player);
  else if(which=="abort-init")InitEntry();
  else if(which=="abort-death")DeathEntry();
  else if(which=="abort-reset")ResetEntry();
  else if(which=="abort-null-death") {gPlayState=nullptr;DeathEntry();}
  else assert(false);
  assert(!sNativeMaskCs.active);AssertStopped();
 } else assert(false);
 std::printf("PASS %s\\n",argv[1]);
}
'''


def fixture(native, form):
    # Only the actual production structs/functions listed here enter the test TU.
    structs = native[native.index("struct MmNativeMaskCamera"):native.index("static u8 MmForm_IsNativeMaskForm")]
    names = ("MmForm_IsNativeMaskForm", "MmForm_StopNativeMaskSfx", "MmForm_RestoreNativeMaskLighting",
             "MmForm_EndNativeMaskCutscene", "MmForm_InterruptNativeMaskForScene", "MmForm_OnPlayDestroy",
             "MmForm_CancelNativeMaskCutscene", "MmForm_UpdateNativeMaskCutscene")
    result = FIXTURE.replace("/* NATIVE_STRUCTS */", structs)
    result = result.replace("/* PRODUCTION_FUNCTIONS */", "\n".join(function(native, name) for name in names))
    for marker, name in (("INIT", "MmForm_Init"), ("DEATH", "MmForm_OnDeath"), ("RESET", "MmForm_Reset")):
        body = function(form, name).split("{", 1)[1].split("MmForm_StopGoronRollSfx();", 1)[0]
        assert "MmForm_" in body and "NativeMask" in body
        result = result.replace("/* " + marker + "_ENTRY */", body)
    # Static call-graph guard: no per-frame call to the teardown entry points.
    assert len(re.findall(r"MmForm_InterruptNativeMaskForScene\s*\(", form)) == 1
    assert len(re.findall(r"MmForm_CancelNativeMaskCutscene\s*\(", form)) == 2
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-json", help="JSON source input, or - for stdin")
    parser.add_argument("--patch", type=Path)
    parser.add_argument("--report", type=Path)
    args = parser.parse_args()
    if args.source_json:
        payload = json.loads(sys.stdin.readline()) if args.source_json == "-" else json.loads(Path(args.source_json).read_text())
        native, form = payload["native"], payload["form"]
    else:
        native = (ROOT / NATIVE_PATH).read_text()
        form = (ROOT / FORM_PATH).read_text()
    if args.patch:
        native = apply_patch(native, args.patch.read_text())
    source = fixture(native, form)
    cases = ("normal-on", "normal-off", "post-destroy", "post-init", "post-death", "post-reset",
             "post-null-death", "post-address-reuse", "skip", "failure", "abort-destroy", "abort-init",
             "abort-death", "abort-reset", "abort-null-death")
    report = {"boundary": "exact controller functions; stub animation/services/audio ownership", "hosts": []}
    failures = []
    with tempfile.TemporaryDirectory(prefix="native-mask-audio-lifecycle-") as temporary:
        for combined in (False, True):
            host = "ComboShip OoT" if combined else "solo SoH"
            binary = Path(temporary) / ("combo" if combined else "solo")
            command = [os.environ.get("CXX", "c++"), "-std=c++20", "-O1", "-g", "-Wall", "-Wextra",
                       "-Wno-unused-function", "-Wno-unused-variable", "-fsanitize=address,undefined",
                       "-fno-sanitize-recover=all", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
            if combined:
                command += ["-DCOMBO_BUILD", "-DSOH_BUILD_DLL"]
            command += ["-x", "c++", "-", "-o", str(binary)]
            built = subprocess.run(command, input=source, text=True, capture_output=True)
            if built.returncode:
                sys.stderr.write(built.stderr)
                raise SystemExit(built.returncode)
            outcomes = []
            for case in cases:
                run = subprocess.run([str(binary), case], text=True, capture_output=True,
                                     env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
                outcome = {"case": case, "passed": run.returncode == 0}
                if run.returncode:
                    outcome["stderr"] = run.stderr[-2000:]
                    failures.append(host + ":" + case)
                outcomes.append(outcome)
            report["hosts"].append({"host": host, "outcomes": outcomes})
            print(json.dumps({"host": host, "passed": sum(x["passed"] for x in outcomes), "cases": len(cases)}), flush=True)
    if args.report:
        args.report.write_text(json.dumps(report, indent=2) + "\n")
    if failures:
        print("FAIL native audio lifecycle: " + ", ".join(failures), file=sys.stderr)
        raise SystemExit(1)
    print("PASS native audio lifecycle: 30 scenarios across solo SoH and ComboShip")


if __name__ == "__main__":
    main()
