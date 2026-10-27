#!/usr/bin/env python3
"""Regional source/native fixtures; does not claim configured game runtime proof."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import re
ROOT=Path(__file__).resolve().parents[2]
SANITIZE='--sanitize' in sys.argv
ENV={**os.environ}
if os.environ.get('COMBO_NEI_FIXTURE_CPATH'):
    ENV['CPATH']=os.pathsep.join((os.environ['COMBO_NEI_FIXTURE_CPATH'],ENV.get('CPATH','')))

def build(host,name,source):
    with tempfile.TemporaryDirectory(prefix='regional-'+name+'-') as temporary:
        cpp=Path(temporary)/'test.cpp'; binary=Path(temporary)/'test'; cpp.write_text(source)
        paths=[host,host+('/2s2h' if host=='mm' else '/soh'),host+'/include',host+'/include/PR',host+'/src',host+'/assets',
               'libultraship/include','combo','combo/menu',host+('/2s2h' if host=='mm' else '/soh')+'/Enhancements/Gameplay'] if host else ['combo']
        command=[os.environ.get('CXX','c++'),'-std=c++20','-w','-DF3DEX_GBI_2','-DCOMBO_BUILD','-DFMT_HEADER_ONLY',
                 '-ffunction-sections','-fdata-sections','-Wl,--gc-sections',*['-I'+str(ROOT/p) for p in paths]]
        if SANITIZE: command+=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer','-no-pie']
        result=subprocess.run([*command,str(cpp),'-o',str(binary)],capture_output=True,text=True,env=ENV)
        if result.returncode: raise SystemExit(result.stderr[:8000])
        subprocess.run([str(binary)],check=True,env={**ENV,'ASAN_OPTIONS':'detect_leaks=0'})

subprocess.run([sys.executable, str(ROOT / 'tests/regional_passages/run_header_portability_tests.py')], check=True, env=ENV)
build('', 'geometry', (ROOT/'tests/regional_passages/geometry_test.cpp').read_text())
# Fixed providers and endpoint owners exercise actual native actors and start
# modes in focused fixtures. Keep the shared handoff/boot priority probes below.
for runner, accepts_sanitize in (
    ("run_bridge_combiner_tests.py", True),
    ("run_mm_fixed_tests.py", True),
    ("run_ikana_tests.py", True),
    ("run_lake_bridge_tests.py", True),
    ("run_lake_sand_tests.py", False),
    ("run_snowhead_bridge_tests.py", False),
):
    command = [sys.executable, str(ROOT / "tests/regional_passages" / runner)]
    if SANITIZE and accepts_sanitize:
        command.append("--sanitize")
    subprocess.run(command, check=True, env=ENV)

sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from

def function(source, signature):
    return block_from(source,source.index(signature))

# Endpoint fixtures above own collision/actors. These fixtures run the actual
# registered fade/save/boot bodies, with endpoint/DLL/IO boundaries.
oot_port=(ROOT/'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8-sig')
mm_port=(ROOT/'mm/2s2h/BenPort.cpp').read_text(encoding='utf-8-sig')
def fixture(name):
    body = (ROOT / "tests/lost_woods_entrance" / name).read_text().split("int main()")[0]
    if name == "mm_arrival_test.cpp":
        # This fixture executes native boot bodies with the SDK/application
        # boundary replaced below; it uses the combo export ABI, not OTRGlobals.
        body = body.replace('#include "2s2h/BenPort.h"', '#include "ComboExport.h"')
    return '#include "ComboGameMode.h"\nextern "C" { int gComboGameMode=COMBO_GAME_BOTH; }\n' + body

source=fixture('oot_handoff_test.cpp')
source=source.replace('extern "C" s32 Player_InCsMode(PlayState*) { return 0; }',
    'static bool regionalCutscene;\nextern "C" s32 Player_InCsMode(PlayState*) { return regionalCutscene; }')
mirror=oot_port.index('static int sComboMirrorSwitchSlot = -1;')
source=source.replace('/* MIRROR_QUEUE */',oot_port[mirror:oot_port.index('\n',mirror)]+'\n'+function(oot_port,'extern "C" int SOH_QueueMagicMirrorHandoff('))
begin=oot_port.index('    // Flag set when we want to switch to MM.')
source=source.replace('/* HANDOFF_HOOKS */',oot_port[begin:oot_port.index('    // ComboShip release gate:',begin)])
source+=r'''
void EnterRegional(int route,int slot=0) {
    GameInteractor::Fire<GameInteractor::OnPlayDestroy>(); play={}; player={};
    gPlayState=&play; gGameState=&play.state; play.state.running=true;
    play.sceneNum=route==5?SCENE_GRAVEYARD:route==6?SCENE_LAKE_HYLIA:SCENE_ZORAS_FOUNTAIN;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].head=&player.actor;
    gSaveContext.gameMode=GAMEMODE_NORMAL; gSaveContext.fileNum=slot;
    regionalDeparture=route; regionalReady=true; regionalFall=route==5; regionalCutscene=route==5;
    GameInteractor::Fire<GameInteractor::OnPlayerUpdate>();
}
int main() {
    RegisterCandidate();
    for(int route=5;route<=7;++route) for(int slot=0;slot<3;++slot) {
        const int old=saves; EnterRegional(route,slot); Frames();
        require(saves==old+1&&switches==saves&&savedSlot==slot&&recordedRoute==route,
                "each regional route must save its captured slot once after a fully covered outgoing frame");
    }
    const int completed=saves;
    EnterRegional(5); regionalReady=false; Frames();
    EnterRegional(6); gSaveContext.fileNum=2; Frames();
    EnterRegional(7); GameInteractor::Fire<GameInteractor::OnPlayDestroy>(); Frames();
    require(saves==completed&&switches==completed,"seed/endpoint, slot and destroyed arena must cancel before IO");
    EnterRegional(7); ComboCrossOotRuntime::pending=true; Frames();
    require(recordedRoute==0,"shuffled dungeon departure must retain priority over every regional route");
    puts("PASS regional OoT production handoff: all routes/slots, owned grave cutscene, covered-frame save, cancellation and dungeon priority");
}
'''
build('soh','oot-handoff',source)
source=fixture('mm_handoff_test.cpp')
begin=mm_port.index('static bool sComboReturnPending = false;')
source=source.replace('/* RETURN_STATE */',mm_port[begin:mm_port.index("// MM's own ResourceManager",begin)])
begin=mm_port.index('    // Reverse MM->OOT trigger:')
source=source.replace('/* HANDOFF_HOOKS */',mm_port[begin:mm_port.index('    } // Combo-only handoff hooks:',begin)])
source+=r'''
void EnterRegional(int route,int slot=0) {
    Combo_ClearReturnRequests(); play={}; player={}; gPlayState=&play; gGameState=&play.state;
    play.state.running=true; play.sceneId=route==5?SCENE_IKANAMAE:route==6?SCENE_30GYOSON:SCENE_10YUKIYAMANOMURA;
    gSaveContext.gameMode=GAMEMODE_NORMAL; gSaveContext.fileNum=slot; regionalReturn=route; regionalReady=true;
    GameInteractor::Fire<GameInteractor::OnActorUpdate>(&player.actor);
}
int main() {
    RegisterCandidate();
    for(int route=5;route<=7;++route) for(int slot=0;slot<3;++slot) {
        const int old=saves; EnterRegional(route,slot); Frames();
        require(saves==old+1&&switches==saves&&recordedRoute==route&&kind==0,
                "every regional return must persist once and route the native OoT boot after a covered frame");
    }
    const int completed=saves;
    EnterRegional(5); regionalReady=false; Frames();
    EnterRegional(6); gSaveContext.fileNum=2; Frames();
    EnterRegional(7); GameInteractor::Fire<GameInteractor::OnPlayDestroy>(); Frames();
    require(saves==completed&&switches==completed,"MM regional cancellation must not leak into another file/seed/arena");
    EnterRegional(6); MM_QueueFwHandoff(); Frames(); require(recordedRoute==0&&kind==3,"Farore must supersede regional travel");
    EnterRegional(7); MM_RequestComboReturn(); Frames(); require(recordedRoute==0&&kind==1,"reset must discard regional travel");
    puts("PASS regional MM production handoff: all routes/slots, covered-frame return, cancellation, Farore/reset priority");
}
'''
build('mm','mm-handoff',source)
source=fixture('oot_arrival_test.cpp').replace('/* ARRIVAL_BODY */',function(oot_port,'extern "C" void SOH_ApplyComboPortalArrival('))
source=source.replace('/* BOOT_BODY */',function((ROOT/'soh/src/code/title_setup.c').read_text(),'void TitleSetup_InitImpl('))
source+=r'''
int main() {
    GameState state{};
    for(int route=5;route<=7;++route) for(int priority=-1;priority<3;++priority) {
        regionalArrival=0; gComboReturnFileNum=0; sComboReturnPortalRoute=route;
        if(priority==0) crossArrival=ENTR_DEKU_TREE_ENTRANCE;
        if(priority==1) bossRecovery=ENTR_DEKU_TREE_ENTRANCE;
        if(priority==2) fwArrival=ENTR_DEKU_TREE_ENTRANCE;
        TitleSetup_InitImpl(&state);
        require(sComboReturnPortalRoute==0,"regional boot request must be consumed exactly once");
        if(priority<0) require(regionalArrival==route,"native source endpoint must receive the requested regional return");
        else require(gSaveContext.entranceIndex==ENTR_DEKU_TREE_ENTRANCE,"composed arrival must supersede regional endpoint");
        if(priority==0||priority==1) require(regionalArrival==0,"dungeon/boss arrival must prevent regional respawn writes");
    }
    puts("PASS regional OoT native boot: one-shot dispatch and composed-arrival priority");
}
'''
build('soh','oot-boot',source)
source=fixture('mm_arrival_test.cpp')
source=source.replace('/* ARRIVAL_BODY */',function(mm_port,'extern "C" void MM_ApplyComboPortalArrival('))
source=source.replace('/* ROUTE_BODY */','\n'.join(function(mm_port,sig) for sig in (
    'static void Combo_CancelLostWoodsPassage(','static void Combo_ClearReturnRequests(',
    'extern "C" COMBO_EXPORT void MM_SetComboEntryIsResume(','extern "C" COMBO_EXPORT void MM_SetComboPortalRoute(')))
source=source.replace('/* BOOT_BODY */',re.sub(r'\bthis\b','self',function((ROOT/'mm/src/code/title_setup.c').read_text(),'void Setup_InitImpl(')))
source+=r'''
int main() {
    SetupState state{};
    for(int route=5;route<=7;++route) for(int slot=0;slot<3;++slot) {
        regionalArrival=0; MM_SetComboEntryIsResume(0); MM_SetComboPortalRoute(route); gComboStartFileNum=slot;
        Setup_InitImpl(&state); require(regionalArrival==route&&!sComboPortalArrivalPending,"regional route must apply after native slot load exactly once");
        regionalArrival=0; MM_ApplyComboPortalArrival(); require(!regionalArrival,"native regional boot dispatch must be one-shot");
    }
    for(int route=5;route<=7;++route) for(int priority=0;priority<3;++priority) {
        MM_SetComboEntryIsResume(0); MM_SetComboPortalRoute(route); gComboStartFileNum=0;
        if(priority==0) crossArrival=ENTRANCE(SNOWHEAD_TEMPLE,0);
        if(priority==1) bossRecovery=ENTRANCE(SNOWHEAD_TEMPLE,0);
        if(priority==2) fwArrival=ENTRANCE(SNOWHEAD_TEMPLE,0);
        Setup_InitImpl(&state);
        require(gSaveContext.save.entrance==ENTRANCE(SNOWHEAD_TEMPLE,0)&&!sComboPortalArrivalPending,
                "native explicit MM arrivals must supersede regional destination fields");
    }
    MM_SetComboPortalRoute(7); MM_SetComboEntryIsResume(1); regionalArrival=0; gComboStartFileNum=0;
    Setup_InitImpl(&state); require(!regionalArrival&&!sComboPortalArrivalPending,"ordinary resume must discard a stale regional request");
    puts("PASS regional MM native boot: all routes/slots, one-shot dispatch, ordinary resume and dungeon/boss/Farore priority");
}
'''
build('mm','mm-boot',source)
