#!/usr/bin/env python3
"""Exercise real boss death/reload adapters; an unbeaten arena cannot own recovery."""
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


def fixture(name, variable):
    path = ROOT / 'tests/cross_dungeon' / name
    env = {'__file__': str(path)}
    previous = sys.argv
    try:
        sys.argv = [str(path)]
        prefix = path.read_text().split('with tempfile.TemporaryDirectory(')[0]
        exec(compile(prefix, str(path), 'exec'), env)
    finally:
        sys.argv = previous
    source = env[variable]
    return source[:source.index('int main()')]


oot = fixture('oot_pool_runtime_behavior_test.py', 'source')
oot = oot.replace('void* Combo_ResolveSym(const char*,const char*) { return nullptr; }', r'''
static int resumedGame=-1, resumedEntrance=-1, resumes=0;
static int ResumePeer(int game,int entrance) {
    resumedGame=game;resumedEntrance=entrance;++resumes;return 1;
}
void* Combo_ResolveSym(const char*,const char* name) {
    return std::string(name)=="MM_QueueCrossBossRecovery" ? reinterpret_cast<void*>(ResumePeer) : nullptr;
}
''')
oot_title = (ROOT / 'soh/src/code/title_setup.c').read_text()
oot_host = (ROOT / 'soh/soh/OTRGlobals.cpp').read_text(encoding='utf-8-sig')
oot += r'''
#include "combo/entrances/RegionalPassages.h"
#include "soh/Enhancements/Gameplay/ComboRegionalPassages.h"
#include <cstdlib>
// A composed boss recovery must consume a stale passage without placing it.
extern "C" int ComboRegionalPassagesOot_ApplyArrival(int) { std::abort(); }
static int sComboReturnPortalRoute = COMBO_PORTAL_MARKET;
'''
oot += block_from(oot_host, oot_host.index('extern "C" void SOH_ApplyComboPortalArrival(')) + '\n'
oot_title = oot_title[oot_title.index('        s32 crossDungeonArrival ='):oot_title.index('        // A portal return')]
oot += '\nint16_t Entrance_OverrideNextIndex(int16_t index) { return index; }\nvoid NativeTitleArrival() {\n' + oot_title + '\n}\n'
oot += r'''
static Dungeon OotBoss(const char* name,int id,int entry,int reverse,int parent,int warp) {
    Dungeon d{name,{0,entry},{0,reverse},{0,reverse},{{0,warp}}};
    d.pool=Pool::Boss;d.boss=id;d.owner=name;d.warpReturn=Endpoint{0,warp};
    d.reverseCheckpoint=d.deathCheckpoint=Endpoint{0,parent};return d;
}
int main() {
    PlayState play{};gPlayState=&play;gSaveContext.fileNum=1;
    auto gohma=OotBoss("gohma",0,ENTR_DEKU_TREE_BOSS_ENTRANCE,ENTR_DEKU_TREE_BOSS_DOOR,
                       ENTR_DEKU_TREE_ENTRANCE,ENTR_KOKIRI_FOREST_DEKU_TREE_BLUE_WARP);
    auto volvagia=OotBoss("volvagia",4,ENTR_FIRE_TEMPLE_BOSS_ENTRANCE,ENTR_FIRE_TEMPLE_BOSS_DOOR,
                         ENTR_FIRE_TEMPLE_ENTRANCE,ENTR_DEATH_MOUNTAIN_CRATER_FIRE_TEMPLE_BLUE_WARP);
    auto seed=Encode({gohma,volvagia},{1,0});
    assert(SOH_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    play.sceneNum=SCENE_FIRE_TEMPLE_BOSS;
    assert(SOH_QueueCrossDungeonArrival(volvagia.entrance.index));
    assert(Combo_SOH_ConsumeCrossDungeonArrival()==volvagia.entrance.index);
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==volvagia.entrance.index); // Physical identity remains distinct.
    assert(Combo_SOH_CrossDungeonDeathCheckpoint()==gohma.reverseCheckpoint->index &&
           "death in an unbeaten destination boss must return to the source dungeon");
    Combo_SOH_LoadCrossDungeonCheckpoint(volvagia.entrance.index);
    assert(Combo_SOH_ConsumeCrossDungeonRecovery()==gohma.reverseCheckpoint->index &&
           "saved reload must not pin the inaccessible physical boss arena");
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==gohma.reverseCheckpoint->index);
    auto nativeMixed=gohma;
    nativeMixed.returnTo=Endpoint{0,ENTR_WATER_TEMPLE_BOSS_DOOR};
    nativeMixed.reverseCheckpoint=Endpoint{0,ENTR_WATER_TEMPLE_ENTRANCE};
    nativeMixed.deathCheckpoint=Endpoint{0,ENTR_FIRE_TEMPLE_ENTRANCE};
    seed=Encode({nativeMixed,volvagia},{1,0});
    assert(SOH_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    assert(SOH_QueueCrossDungeonArrival(volvagia.entrance.index));Combo_SOH_ConsumeCrossDungeonArrival();
    assert(Combo_SOH_CrossDungeonDeathCheckpoint()==ENTR_FIRE_TEMPLE_ENTRANCE &&
           "death follows the incoming source even when its reverse independently leads to Water");
    Combo_SOH_LoadCrossDungeonCheckpoint(volvagia.entrance.index);
    assert(Combo_SOH_ConsumeCrossDungeonRecovery()==ENTR_FIRE_TEMPLE_ENTRANCE);
    Combo_SOH_LoadCrossDungeonCheckpoint(ENTR_FIRE_TEMPLE_ENTRANCE);
    assert(Combo_SOH_ConsumeCrossDungeonRecovery()==ENTR_FIRE_TEMPLE_ENTRANCE);
    assert(!PrepareSwitch());
    assert(state.Lookup(Phase::BeforeNative,volvagia.exit.index)->to==nativeMixed.returnTo);
    seed=Encode({gohma,volvagia},{1,0});assert(SOH_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    // Reverse-door and physical completion warp routing retain their existing destinations.
    assert(state.Lookup(Phase::BeforeNative,volvagia.exit.index)->to==gohma.returnTo);
    assert(state.Lookup(Phase::BeforeNative,volvagia.escapeExits[0].index)->to==*gohma.warpReturn);
    // A foreign source resumes its frozen half. The request carries the arena identity, never a free entrance.
    Dungeon mm{"odolwa",{1,0x3200},{1,0x3010},{1,0x3000},{{1,0x3010}}};
    mm.pool=Pool::Boss;mm.boss=8;mm.owner="woodfall";mm.hasReverse=false;
    mm.warpReturn=Endpoint{1,0x3010};mm.reverseCheckpoint=Endpoint{1,0x3000};
    // Loading the frozen source state must still use death ownership for the actual spawn.
    seed=Encode({nativeMixed,mm},{1,0});
    assert(SOH_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    for (int route : {COMBO_PORTAL_LOST_WOODS, COMBO_PORTAL_OPENING_FOREST, COMBO_PORTAL_WOODFALL,
                      COMBO_PORTAL_IKANA, COMBO_PORTAL_GREAT_BAY, COMBO_PORTAL_SNOWHEAD}) {
        assert(SOH_QueueCrossBossRecovery(1,mm.entrance.index));
        Combo_SOH_LoadCrossDungeonCheckpoint(nativeMixed.returnTo.index);
        sComboReturnPortalRoute=route;
        gSaveContext.respawnFlag=0;
        NativeTitleArrival();
        assert(sComboReturnPortalRoute==COMBO_PORTAL_MARKET && gSaveContext.respawnFlag==0 &&
               "passage placement must not override source-owned boss recovery");
        assert(gSaveContext.entranceIndex==ENTR_FIRE_TEMPLE_ENTRANCE &&
               "foreign death must physically resume at the incoming Fire source, not the Water reverse");
    }
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==ENTR_FIRE_TEMPLE_ENTRANCE);
    Combo_SOH_LoadCrossDungeonCheckpoint(Combo_SOH_SaveCrossDungeonCheckpoint());
    assert(Combo_SOH_ConsumeCrossDungeonRecovery()==ENTR_FIRE_TEMPLE_ENTRANCE && !PrepareSwitch());
    seed=Encode({volvagia,mm},{1,0});
    assert(SOH_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    assert(SOH_QueueCrossDungeonArrival(volvagia.entrance.index));
    Combo_SOH_ConsumeCrossDungeonArrival();
    Combo_SOH_CrossDungeonDeathCheckpoint();
    assert(PrepareSwitch() && resumes==1 && resumedGame==0 && resumedEntrance==volvagia.entrance.index &&
           "foreign-source death must stage a frozen-source resume");
    Combo_SOH_LoadCrossDungeonCheckpoint(volvagia.entrance.index);
    Combo_SOH_ConsumeCrossDungeonRecovery();
    assert(PrepareSwitch() && resumes==2 && "foreign-source cold reload must also resume its source half");
    Combo_SOH_LoadCrossDungeonCheckpoint(volvagia.entrance.index);
    Combo_SOH_ConsumeCrossDungeonRecovery();
    Combo_SOH_RestoreCrossDungeonCheckpoint(-1); // Explicit Farore arrival wins over ordinary recovery.
    assert(!PrepareSwitch() && resumes==2);
    Combo_SOH_LoadCrossDungeonCheckpoint(volvagia.entrance.index);
    Combo_SOH_ConsumeCrossDungeonRecovery();
    assert(SOH_QueueCrossDungeonArrival(volvagia.entrance.index));
    Combo_SOH_ConsumeCrossDungeonArrival();
    assert(!PrepareSwitch() && "a new routed ingress must cancel stale save recovery");
    // The other direction: resume the real frozen source after its native save load.
    assert(!SOH_QueueCrossBossRecovery(1,0x1234));
    assert(!SOH_QueueCrossBossRecovery(0,volvagia.entrance.index));
    assert(SOH_QueueCrossBossRecovery(1,mm.entrance.index));
    assert(!SOH_QueueCrossDungeonArrival(volvagia.returnTo.index)); // No arbitrary endpoint queue.
    Combo_SOH_LoadCrossDungeonCheckpoint(volvagia.returnTo.index);
    assert(Combo_SOH_ConsumeCrossDungeonRecovery()==-1); // Dedicated resume follows native hooks.
    gSaveContext.entranceIndex=999;
    NativeTitleArrival();
    assert(gSaveContext.entranceIndex==volvagia.deathCheckpoint->index &&
           "MM->OoT recovery must bypass the fixed portal arrival");
    assert(Combo_SOH_SaveCrossDungeonCheckpoint()==volvagia.reverseCheckpoint->index);
    assert(Combo_SOH_ConsumeCrossBossRecoveryResume()==-1);
    assert(SOH_QueueCrossBossRecovery(1,mm.entrance.index));
    assert(SOH_SetCrossDungeonRuntimeSeed(seed.dump().c_str())); // Identical slot rebind preserves the request.
    assert(Combo_SOH_ConsumeCrossBossRecoveryResume()==volvagia.deathCheckpoint->index);
    assert(SOH_QueueCrossBossRecovery(1,mm.entrance.index));
    auto changed=seed;
    changed["dungeons"][0]["deathCheckpoint"]=EndpointJson(Endpoint{0,ENTR_WATER_TEMPLE_ENTRANCE});
    assert(Decode(changed)==Decode(seed)); // MM's absent reverse portal leaves routes identical.
    assert(SOH_SetCrossDungeonRuntimeSeed(changed.dump().c_str()));
    assert(Combo_SOH_ConsumeCrossBossRecoveryResume()==-1); // Changed recovery metadata cancels a stale request.
    assert(SOH_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    assert(SOH_QueueCrossBossRecovery(1,mm.entrance.index));
    Combo_SOH_RestoreCrossDungeonCheckpoint(-1);
    assert(Combo_SOH_ConsumeCrossBossRecoveryResume()==-1); // Explicit recall cancels source resume.
    auto invalid=seed;invalid["dungeons"][0]["deathCheckpoint"]["index"]=0xFFFF;
    assert(!SOH_SetCrossDungeonRuntimeSeed(invalid.dump().c_str()));
    assert(state.RecoveryForBoss(mm.entrance)->parent==*volvagia.deathCheckpoint); // Transactional rejection.
    // Dungeon-only v1 recovery keeps its original native endpoint.
    Dungeon legacy{"legacy",{0,123},{0,124},{0,124}};
    Dungeon legacyPeer{"legacy-peer",{1,123},{1,124},{1,124}};
    seed=Encode({legacy,legacyPeer},{0,1});assert(seed.at("version")==1);
    assert(SOH_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    Combo_SOH_LoadCrossDungeonCheckpoint(123);assert(Combo_SOH_ConsumeCrossDungeonRecovery()==123);
    std::cout<<"OoT boss recovery: source parent, foreign frozen resume, reload and v1 parity PASS\n";
}
'''

mm = fixture('mm_adapter_behavior_test.py', 'program')
mm = mm.replace('int queued=-1;', 'int queued=-1,resumedGame=-1,resumedEntrance=-1,resumes=0;')
mm = mm.replace('void* Combo_ResolveSym(const char*,const char*){return reinterpret_cast<void*>(QueuePeer);}', r'''
int ResumePeer(int game,int entrance){resumedGame=game;resumedEntrance=entrance;++resumes;return 1;}
void* Combo_ResolveSym(const char*,const char* name){
    return std::string(name)=="SOH_QueueCrossBossRecovery" ? reinterpret_cast<void*>(ResumePeer) : reinterpret_cast<void*>(QueuePeer);
}
''')
mm += '\nvoid func_80169F78(Play*) { gSaveContext.respawnFlag=-1; }\nvoid func_80169FDC(Play* p) { func_80169F78(p); }\n'
for function, path, call in [
        ('NativeAutomaticGameOver', 'mm/src/code/z_game_over.c', 'func_80169F78(play);'),
        ('NativeContinueGameOver', 'mm/src/overlays/kaleido_scope/ovl_kaleido_scope/z_kaleido_scope_NES.c',
         'func_80169FDC(play);')]:
    native = (ROOT / path).read_text()
    start = native.index(call)
    snippet = native[start:native.index('gSaveContext.nextTransitionType', start)]
    mm += '\nvoid ' + function + '() { Play* play=gPlayState;\n' + snippet + '\n}\n'
mm += r'''
int main() {
    using namespace ComboCrossMmRuntime;
    InitRegions();
    CheckNativeBossCatalogue(); // Populate native declarations before GetRegionIdFromEntrance caches them.
    Rando::Logic::Regions[RR_WOODFALL_TEMPLE_BOSS_ROOM].oneWayEntrances.insert(ENTRANCE(ODOLWAS_LAIR,0));
    Rando::Logic::Regions[RR_SNOWHEAD_TEMPLE_BOSS_ROOM].oneWayEntrances.insert(ENTRANCE(GOHTS_LAIR,0));
    Rando::Logic::Regions[RR_WOODFALL_TEMPLE_ENTRANCE].oneWayEntrances.insert(ENTRANCE(WOODFALL_TEMPLE,0));
    Rando::Logic::Regions[RR_SNOWHEAD_TEMPLE_ENTRANCE_BEFORE_BLOCK].oneWayEntrances.insert(ENTRANCE(SNOWHEAD_TEMPLE,0));
    Rando::Logic::Regions[RR_WOODFALL_TEMPLE_DEKU_PRINCESS_ROOM].oneWayEntrances.insert(ENTRANCE(WOODFALL_TEMPLE,1));
    std::vector<Dungeon> bosses;ComboCrossMm::AppendBossCatalogue(bosses);
    const auto odolwa=bosses[0],goht=bosses[1];
    auto seed=Encode({odolwa,goht},{1,0});
    assert(MM_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    play.sceneId=SCENE_HAKUGIN_BS;
    assert(MM_QueueCrossDungeonArrival(goht.entrance.index));
    Combo_MM_ConsumeCrossDungeonArrival();
    assert(Combo_MM_SaveCrossDungeonCheckpoint()==goht.entrance.index);
    int32_t index=goht.escapeExits[0].index;gSaveContext.respawnFlag=-2;
    assert(!Combo_MM_CrossDungeonTransition(&index) && index==goht.escapeExits[0].index &&
           !PrepareSwitch()); // A negative native warp flag alone must never request death recovery.
    index=goht.entrance.index;gSaveContext.respawnFlag=1;
    assert(!Combo_MM_CrossDungeonTransition(&index) && index==goht.entrance.index); // Native void stays local.
    NativeAutomaticGameOver();assert(gSaveContext.respawnFlag==-6);
    assert(!Combo_MM_CrossDungeonTransition(&index) && index==odolwa.reverseCheckpoint->index &&
           "MM death must recover at the source temple, not the unbeaten boss");
    assert(gSaveContext.respawnFlag==0);
    assert(MM_QueueCrossDungeonArrival(goht.entrance.index));Combo_MM_ConsumeCrossDungeonArrival();
    index=goht.entrance.index;NativeContinueGameOver();assert(gSaveContext.respawnFlag==-2);
    assert(!Combo_MM_CrossDungeonTransition(&index) && index==odolwa.reverseCheckpoint->index);
    Combo_MM_LoadCrossDungeonCheckpoint(goht.entrance.index);
    assert(Combo_MM_ConsumeCrossDungeonRecovery()==odolwa.reverseCheckpoint->index);
    assert(Combo_MM_SaveCrossDungeonCheckpoint()==odolwa.reverseCheckpoint->index);
    assert(MM_QueueCrossDungeonArrival(goht.entrance.index));Combo_MM_ConsumeCrossDungeonArrival();
    index=goht.escapeExits[0].index;gSaveContext.respawnFlag=0;
    assert(!Combo_MM_CrossDungeonTransition(&index) && index==odolwa.warpReturn->index &&
           "a physical completion warp retains the logical source's completion route");
    Dungeon oot{"volvagia",{0,100},{0,101},{0,102},{{0,103}}};
    oot.pool=Pool::Boss;oot.boss=4;oot.owner="fire";oot.warpReturn=Endpoint{0,104};oot.reverseCheckpoint=Endpoint{0,105};
    seed=Encode({oot,goht},{1,0});assert(MM_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    assert(MM_QueueCrossDungeonArrival(goht.entrance.index));Combo_MM_ConsumeCrossDungeonArrival();
    index=goht.entrance.index;NativeContinueGameOver();
    assert(Combo_MM_CrossDungeonTransition(&index)==1 && PrepareSwitch() && resumes==1 &&
           resumedGame==1 && resumedEntrance==goht.entrance.index);
    Combo_MM_LoadCrossDungeonCheckpoint(goht.entrance.index);Combo_MM_ConsumeCrossDungeonRecovery();
    assert(PrepareSwitch() && resumes==2);
    Combo_MM_LoadCrossDungeonCheckpoint(goht.entrance.index);Combo_MM_ConsumeCrossDungeonRecovery();
    Combo_MM_RestoreCrossDungeonCheckpoint(-1);assert(!PrepareSwitch());
    assert(!MM_QueueCrossBossRecovery(0,999));
    assert(!MM_QueueCrossBossRecovery(1,goht.entrance.index));
    assert(MM_QueueCrossBossRecovery(0,oot.entrance.index));
    assert(!MM_QueueCrossDungeonArrival(0xFFFF)); // Resume does not introduce an arbitrary endpoint queue.
    Combo_MM_LoadCrossDungeonCheckpoint(goht.returnTo.index);
    gComboEntryIsResume=false;gSaveContext.save.entrance=999;
    NativeTitleArrival();
    assert(gSaveContext.save.entrance==goht.returnTo.index &&
           "OoT->MM recovery must resume the frozen source even for an ordinary launcher handoff");
    assert(Combo_MM_SaveCrossDungeonCheckpoint()==goht.reverseCheckpoint->index);
    assert(Combo_MM_ConsumeCrossBossRecoveryResume()==-1);
    assert(MM_QueueCrossBossRecovery(0,oot.entrance.index));
    Combo_MM_RestoreCrossDungeonCheckpoint(-1);
    assert(Combo_MM_ConsumeCrossBossRecoveryResume()==-1);
    auto invalid=seed;invalid["dungeons"][1]["reverseCheckpoint"]["index"]=0xFFFF;
    assert(!MM_SetCrossDungeonRuntimeSeed(invalid.dump().c_str()));
    assert(state.RecoveryForBoss(oot.entrance)->parent==*goht.deathCheckpoint);
    // Repeat against every real native MM boss/warp alias, including Gyorg's first exit.
    for (const auto& target : bosses) {
        seed=Encode({oot,target},{1,0});assert(MM_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
        assert(MM_QueueCrossDungeonArrival(target.entrance.index));Combo_MM_ConsumeCrossDungeonArrival();
        NativeAutomaticGameOver();index=target.entrance.index;
        assert(Combo_MM_CrossDungeonTransition(&index)==1 && PrepareSwitch());
        assert(resumedGame==1 && resumedEntrance==target.entrance.index);
        assert(MM_QueueCrossBossRecovery(0,oot.entrance.index));
        Combo_MM_LoadCrossDungeonCheckpoint(target.returnTo.index);
        NativeTitleArrival();
        assert(gSaveContext.save.entrance==target.returnTo.index);
        assert(Combo_MM_SaveCrossDungeonCheckpoint()==target.reverseCheckpoint->index);
    }
    std::cout<<"MM boss recovery: source temple, foreign frozen resume and cold reload PASS\n";
}
'''

for host, source in [('oot', oot), ('mm', mm)]:
    with tempfile.TemporaryDirectory(prefix=f'{host}-boss-recovery-') as directory:
        path = Path(directory)
        (path / 'test.cpp').write_text(source)
        cmd = ['g++', '-std=c++20', '-O1', '-DF3DEX_GBI_2',
               '-DCONTROLLERBUTTONS_T=uint32_t', '-DLOG_LEVEL_GAME_PRINTS=0', '-Wno-narrowing']
        if host == 'oot':
            cmd += ['-DCOMBO_BUILD']
        for include in ('', 'combo', 'soh', 'soh/include', 'soh/include/PR', 'soh/src',
                        'soh/assets', 'soh/mods', 'soh/soh', 'libultraship/include'):
            cmd += ['-I' + str(ROOT / include)]
        cmd += ['-I' + str(ROOT.parent / 'build-deps/usr/include')]
        if args.sanitize:
            cmd += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g']
        subprocess.run(cmd + [str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
        env = os.environ.copy()
        if args.sanitize:
            env['ASAN_OPTIONS'] = 'detect_leaks=0' # LeakSanitizer cannot inspect this container's process namespace.
        subprocess.run([str(path / 'test')], check=True, env=env)
