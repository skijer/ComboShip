#!/usr/bin/env python3
"""Exercise the native door policy and per-attempt logs with real engine types."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

MM_MAIN = r'''
int main() {
 TransitionActorEntry entries[2]{}; KnobDoorActor door{};
 auto prepare=[&] {
  Reset(); diagnostics.clear(); destination=true;
  play.sceneId=SCENE_LOST_WOODS;gSaveContext.save.entrance=ENTRANCE(LOST_WOODS,0);
  entries[0]={};entries[0].id=ACTOR_EN_HOLL;entries[0].sides[0].room=1;
  entries[1]={};entries[1].id=-ACTOR_EN_DOOR;entries[1].pos={-680,0,72};entries[1].rotY=(106<<7)|127;
  play.transitionActors.list=entries;play.transitionActors.count=2;
  door={};door.dyna.actor.id=ACTOR_EN_DOOR;door.dyna.actor.params=TRANSITION_ACTOR_PARAMS(1,0);
  door.dyna.actor.home.pos=door.dyna.actor.world.pos={-680,0,72};
  player.doorActor=&door.dyna.actor;player.actionFunc=Player_Action_36;
 };
 const char* failedCheck[]={nullptr,nullptr,"pause_closed","scene_cutscene_idle","cutscene_not_pending",
  "alive","player_action_expected","player_owns_door","game_over_inactive","game_state_current",
  "time_song_idle",nullptr,"running","normal_mode","playable_slot","player_present"};
 for(int state=0;state<16;++state) {
  prepare();
  if(state==0)sSession.Bind(nullptr,-1);
  if(state==1)pack=false;
  if(state==2)play.pauseCtx.state=PAUSE_STATE_MAIN;
  if(state==3)play.csCtx.state=1;
  if(state==4)gSaveContext.cutsceneTrigger=1;
  if(state==5)gSaveContext.save.saveInfo.playerData.health=0;
  if(state==6)player.actionFunc=NativeIdle;
  if(state==7)player.doorActor=nullptr;
  if(state==8)play.gameOverCtx.state=1;
  if(state==9)gGameState=nullptr;
  if(state==10)play.soaringCsOrSoTCsPlaying=true;
  if(state==11)sEntering=true;
  if(state==12)play.state.running=false;
  if(state==13)gSaveContext.gameMode=GAMEMODE_TITLE_SCREEN;
  if(state==14)gSaveContext.fileNum=0xFF;
  if(state==15)play.actorCtx.actorLists[ACTORCAT_PLAYER].first=nullptr;
  const bool safe=failedCheck[state]==nullptr;
  const bool recognized=state!=13 && state!=14;
  assert(ComboBlackMarket_TryDoorEntrance(&play,&door.dyna.actor)==
         (!recognized?COMBO_BLACK_MARKET_DOOR_UNHANDLED:
          safe?COMBO_BLACK_MARKET_DOOR_ENTERED:COMBO_BLACK_MARKET_DOOR_BLOCKED));
  assert(diagnostics.size()==1 && diagnostics.back()["result"]==
         (!recognized?"unhandled":safe?"entered":"blocked"));
  assert(diagnostics.back()["gates"].size()==3);
  assert(diagnostics.back()["failed_checks"].empty()==safe);
  if(!safe)assert(diagnostics.back()["checks"][failedCheck[state]]==false);
  assert(diagnostics.back()["gates"]["destination_ready"]==true);
  assert(diagnostics.back()["state"].contains("player_action_expected"));
  assert(diagnostics.back()["state"].contains("soaring_or_time_song"));
  if(state==0)assert(diagnostics.back()["market"]["active"]==false);
  if(state==2)assert(diagnostics.back()["state"]["pause"]==PAUSE_STATE_MAIN);
 }
 for(int failure=0;failure<3;++failure) {
  prepare();
  if(failure==0)destination=false;
  if(failure==1)play.transitionTrigger=TRANS_TRIGGER_START;
  if(failure==2)play.transitionMode=TRANS_MODE_SETUP;
  assert(ComboBlackMarket_TryDoorEntrance(&play,&door.dyna.actor)==COMBO_BLACK_MARKET_DOOR_BLOCKED);
  assert(diagnostics.size()==1 && diagnostics.back()["result"]=="blocked");
  if(failure==0)assert(!diagnostics.back()["resources"]["destination_missing"].empty());
  else assert(diagnostics.back()["gates"]["transition_idle"]==false);
 }
 prepare();entries[1].sides[1].room=1;
 assert(ComboBlackMarket_TryDoorEntrance(&play,&door.dyna.actor)==COMBO_BLACK_MARKET_DOOR_UNHANDLED);
 assert(diagnostics.size()==1 && diagnostics.back()["result"]=="unhandled");
 prepare();play.sceneId=SCENE_TOWN;
 assert(ComboBlackMarket_TryDoorEntrance(&play,&door.dyna.actor)==COMBO_BLACK_MARKET_DOOR_UNHANDLED);
 assert(diagnostics.empty());
 prepare();sSession.Bind(nullptr,-1);pack=false;
 assert(ComboBlackMarket_TryDoorEntrance(&play,&door.dyna.actor)==COMBO_BLACK_MARKET_DOOR_ENTERED);
 gSaveContext.save.entrance=COMBO_BLACK_MARKET_ENTRANCE;
 ComboBlackMarket_NormalizeEntry();assert(gSaveContext.save.entrance==COMBO_BLACK_MARKET_ENTRANCE);
 play.sceneId=COMBO_BLACK_MARKET_SCENE;play.transitionTrigger=TRANS_TRIGGER_OFF;
 player.actor.world.pos={0,0,0};diagnostics.clear();
 PlayerUpdate(&player.actor);assert(play.transitionTrigger==TRANS_TRIGGER_OFF && diagnostics.empty());
 u16 emptyText=BlackMarketEntry::kUnavailableText;bool fromTable=true;
 MarketText(&emptyText,&fromTable);
 assert(!fromTable && CustomMessage::last.msg.find("wares aren't ready")!=std::string::npos);
 assert(emptyText<0x4E20 && "empty-stock dialogue must not select the credits decoder");
 gSaveContext.respawnFlag=1;ComboBlackMarket_NormalizeEntry();
 assert(gSaveContext.save.entrance==ENTRANCE(LOST_WOODS,0));
 puts("PASS MM native safety preserved; stock/art do not lock entry; each failed check logged; empty stock stays inside; ordinary doors and respawn recovery preserved");
}
'''

OOT_MAIN = r'''
int main() {
 auto prepare=[&] {Reset();PlaceDoor();diagnostics.clear();destination=true;};
 const char* failedCheck[]={nullptr,nullptr,"pause_closed","scene_cutscene_idle","cutscene_not_pending",
  "alive","player_action_expected","player_owns_door","game_over_inactive","door_type_handle",
  nullptr,"running","normal_mode","playable_slot","player_present"};
 for(int state=0;state<15;++state) {
  prepare();
  if(state==0)sSession.Bind(nullptr,-1);
  if(state==1)pack=false;
  if(state==2)play.pauseCtx.state=1;
  if(state==3)play.csCtx.state=1;
  if(state==4)gSaveContext.cutsceneTrigger=1;
  if(state==5)gSaveContext.health=0;
  if(state==6)player.actionFunc=Player_Action_Idle;
  if(state==7)player.doorActor=nullptr;
  if(state==8)play.gameOverCtx.state=1;
  if(state==9)player.doorType=PLAYER_DOORTYPE_FAKE;
  if(state==10)sEntering=true;
  if(state==11)play.state.running=false;
  if(state==12)gSaveContext.gameMode=GAMEMODE_TITLE_SCREEN;
  if(state==13)gSaveContext.fileNum=0xFF;
  if(state==14)play.actorCtx.actorLists[ACTORCAT_PLAYER].head=nullptr;
  const bool safe=failedCheck[state]==nullptr;
  const bool recognized=state!=12 && state!=13;
  assert(ComboBlackMarketOot_TryDoorEntrance(&play,&nativeDoor.actor)==
         (!recognized?COMBO_BLACK_MARKET_OOT_DOOR_UNHANDLED:
          safe?COMBO_BLACK_MARKET_OOT_DOOR_ENTERED:COMBO_BLACK_MARKET_OOT_DOOR_BLOCKED));
  assert(diagnostics.size()==1 && diagnostics.back()["result"]==
         (!recognized?"unhandled":safe?"entered":"blocked"));
  assert(diagnostics.back()["gates"].size()==3);
  assert(diagnostics.back()["failed_checks"].empty()==safe);
  if(!safe)assert(diagnostics.back()["checks"][failedCheck[state]]==false);
  assert(diagnostics.back()["state"].contains("player_action_expected"));
  if(state==0)assert(diagnostics.back()["market"]["active"]==false);
  if(state==2)assert(diagnostics.back()["state"]["pause"]==1);
 }
 for(int failure=0;failure<3;++failure) {
  prepare();
  if(failure==0)destination=false;
  if(failure==1)play.transitionTrigger=TRANS_TRIGGER_START;
  if(failure==2)play.transitionMode=TRANS_MODE_SETUP;
  assert(ComboBlackMarketOot_TryDoorEntrance(&play,&nativeDoor.actor)==COMBO_BLACK_MARKET_OOT_DOOR_BLOCKED);
  assert(diagnostics.size()==1 && diagnostics.back()["result"]=="blocked");
  if(failure==0)assert(!diagnostics.back()["resources"]["destination_missing"].empty());
  else assert(diagnostics.back()["gates"]["transition_idle"]==false);
 }
 prepare();nativeDoor.actor.home.rot.y+=1;
 assert(ComboBlackMarketOot_TryDoorEntrance(&play,&nativeDoor.actor)==COMBO_BLACK_MARKET_OOT_DOOR_UNHANDLED);
 assert(diagnostics.size()==1 && diagnostics.back()["result"]=="unhandled");
 prepare();play.sceneNum=SCENE_KOKIRI_FOREST;
 assert(ComboBlackMarketOot_TryDoorEntrance(&play,&nativeDoor.actor)==COMBO_BLACK_MARKET_OOT_DOOR_UNHANDLED);
 assert(diagnostics.empty());
 prepare();sSession.Bind(nullptr,-1);pack=false;
 assert(ComboBlackMarketOot_TryDoorEntrance(&play,&nativeDoor.actor)==COMBO_BLACK_MARKET_OOT_DOOR_ENTERED);
 gSaveContext.entranceIndex=ENTR_SKIEJER_MARKET;
 ComboBlackMarketOot_NormalizeEntry();assert(gSaveContext.entranceIndex==ENTR_SKIEJER_MARKET);
 play.sceneNum=SCENE_SKIEJER_MARKET;play.transitionTrigger=TRANS_TRIGGER_OFF;
 player.actor.world.pos={0,0,0};diagnostics.clear();
 PlayerUpdate();assert(play.transitionTrigger==TRANS_TRIGGER_OFF && diagnostics.empty());
 gSaveContext.respawnFlag=1;ComboBlackMarketOot_NormalizeEntry();
 assert(gSaveContext.entranceIndex==ENTR_SKIEJER_FOREST);
 puts("PASS OoT native safety preserved; stock/art do not lock entry; each failed check logged; empty stock stays inside; ordinary doors and respawn recovery preserved");
}
'''

with tempfile.TemporaryDirectory(prefix="market-entry-policy-") as temporary:
    for host, name, main in (("mm", "native", MM_MAIN), ("oot", "oot_native", OOT_MAIN)):
        fixture = (ROOT / "tests/black_market" / (name + "_test.cpp")).read_text()
        marker = re.search(r'^int main\([^\n]*\)\s*\{', fixture, re.M)
        if marker is None:
            raise ValueError('native entry fixture main is missing: ' + host)
        fixture = fixture[:marker.start()] + main
        fixture_path = Path(temporary) / (host + "-fixture.cpp")
        fixture_path.write_text(fixture)
        driver = (ROOT / "tests/black_market" / ("run_" + name + "_tests.py")).read_text()
        driver = driver.replace("ROOT = Path(__file__).resolve().parents[2]", f"ROOT = Path({str(ROOT)!r})")
        driver = driver.replace('(ROOT / "tests/black_market/' + name + '_test.cpp").read_text()',
                                f'Path({str(fixture_path)!r}).read_text()')
        runner = Path(temporary) / (host + "-runner.py")
        runner.write_text(driver)
        subprocess.run([os.sys.executable, str(runner)], check=True, cwd=ROOT)
