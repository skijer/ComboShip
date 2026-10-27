#!/usr/bin/env python3
"""Execute the native MM first/repeat completion bodies with a foreign logical owner."""
import ast
import pathlib
import subprocess
import tempfile
import os, sys
sanitize_flags = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g'] if '--sanitize' in sys.argv else []
if sanitize_flags:
    os.environ['ASAN_OPTIONS'] = 'detect_leaks=0'
root = pathlib.Path(__file__).resolve().parents[2]

def body(text, marker):
    start = text.index(marker)
    opening = text.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]
# Reuse the existing engine-boundary fixture without importing/running its test program.
fixture = ast.parse((root/'tests/cross_dungeon/mm_boss_warp_behavior_test.py').read_text())
prefix = next(ast.literal_eval(node.value) for node in fixture.body if isinstance(node,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='prefix' for t in node.targets))
prefix=prefix.replace('static int Combo_MM_CrossBossWarp(int) { return 0; }','')
prefix += r'''
#define COMBO_BUILD 1
static bool crossBoss=false; static int notified=-1;
static int Combo_MM_CrossBossEnabled() { return crossBoss; }
static int Combo_MM_CrossBossWarp(int scene) { if (!crossBoss) return 0; notified=scene; return 1; }
'''
skip = (root/'mm/2s2h/Enhancements/Cutscenes/StoryCutscenes/SkipGiantsChamber.cpp').read_text()
door = (root/'mm/src/overlays/actors/ovl_Door_Warp1/z_door_warp1.c').read_text()
functions = body(skip,'void HandleGiantsCutsceneSkip()')+'\n'+body(door,'void func_808BA10C(DoorWarp1* this, PlayState* play) {').replace('this','self')
main = r'''
int main() {
 const int scenes[]={SCENE_MITURIN_BS,SCENE_HAKUGIN_BS,SCENE_SEA_BS,SCENE_INISIE_BS};
 const int flags[]={WEEKEVENTREG_CLEARED_WOODFALL_TEMPLE,WEEKEVENTREG_CLEARED_SNOWHEAD_TEMPLE,WEEKEVENTREG_CLEARED_GREAT_BAY_TEMPLE,WEEKEVENTREG_CLEARED_STONE_TOWER_TEMPLE};
 const int escapes[]={ENTRANCE(WOODFALL_TEMPLE,1),ENTRANCE(MOUNTAIN_VILLAGE_SPRING,7),ENTRANCE(ZORA_CAPE,9),ENTRANCE(IKANA_CANYON,15)};
 for(int i=0;i<4;++i) {
  play={}; gSaveContext={}; play.sceneId=scenes[i]; gi.events.clear(); std::fill(std::begin(week),std::end(week),false);
  crossBoss=true; notified=-1; HandleGiantsCutsceneSkip();
  assert(notified==scenes[i] && "first warp must notify mapped source completion");
  assert(!week[flags[i]] && "physical boss must not clear its vanilla dungeon");
  assert(gi.events.size()==1 && std::get<GIEventTransition>(gi.events[0]).entrance==escapes[i]);
  for(bool physicalClear : {false,true}) {
   week[flags[i]]=physicalClear; notified=-1; DoorWarp1 actor{}; func_808BA10C(&actor,&play);
   assert(notified==scenes[i] && "repeat warp must reapply source cycle effects");
   assert(week[flags[i]]==physicalClear && "repeat warp must not leak target world effects");
   assert(play.nextEntrance==escapes[i] && play.transitionTrigger==TRANS_TRIGGER_START);
  }
  crossBoss=false; notified=-1; std::fill(std::begin(week),std::end(week),false); HandleGiantsCutsceneSkip();
  assert(notified==-1 && week[flags[i]] && "disabled remains native");
 }
}
'''
with tempfile.TemporaryDirectory(prefix='mm-cross-boss-') as tmp:
 source=pathlib.Path(tmp)/'test.cpp'; binary=pathlib.Path(tmp)/'test'
 source.write_text(prefix+functions+main)
 subprocess.run(['g++',*sanitize_flags,'-std=c++20','-I',str(root),'-I',str(root.parent/'build-deps/usr/include'),str(source),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
