"""Execute both production native cancel filters plus included backend lifecycle code on native structs."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
extract=lambda s,q:block_from(s,s.index(q))
for host in ['soh','mm']:
    backend=(ROOT/host/('soh' if host=='soh' else '2s2h')/'Enhancements/Gameplay/MagicMirrorRando.inc.h').read_text()
    backend='\n'.join(line for line in backend.splitlines() if not line.startswith('#include'))
    prefix=r'''
#include <nlohmann/json.hpp>
#include "global.h"
#include "mods/extended_inventory.h"
#include "tests/test_require.h"
#include <queue>
#include <algorithm>
#include <variant>
#include <cstring>
#include <memory>
SaveContext gSaveContext{};
static bool contextValid=true;
static int expectedToken=77;
namespace MirrorRuntime {bool FinishPickup(int token){return contextValid&&token&&token==expectedToken;}}
'''
    if host=='soh':
        native=(ROOT/'soh/soh/Enhancements/randomizer/hook_handlers.cpp').read_text()
        bodies='\n'.join(extract(native,q) for q in ['extern "C" void Randomizer_CancelMagicMirrorStump(', 'extern "C" bool Randomizer_MagicMirrorStumpQueued(', 'extern "C" bool Randomizer_QueueMagicMirrorStump('])
        prefix+=r'''
#include "ComboGameMode.h"
#include "soh/Enhancements/randomizer/randomizerEnums.h"
#undef IS_RANDO
#define IS_RANDO true
extern "C" {int gComboGameMode=COMBO_GAME_OOT;}
static bool collected=false,forbidContext=false;
extern "C" s32 Flags_GetRandomizerInf(RandomizerInf){return collected;}
namespace Rando {
struct Value {bool Is(int)const{return true;}};
struct Context {static Context* GetInstance(){REQUIRE(!forbidContext);static Context c;return &c;}
 Value GetOption(int){return {};}
 GetItemEntry GetFinalGIEntry(RandomizerCheck,bool,GetItemID,bool){return GET_ITEM_NONE;}};
}
static std::queue<RandomizerCheck> randomizerQueuedChecks;
static RandomizerCheck randomizerQueuedCheck=RC_UNKNOWN_CHECK;
static GetItemEntry randomizerQueuedItemEntry=GET_ITEM_NONE;
static PlayState* sMirrorAcceptedPlay=nullptr;
static int sMirrorAcceptedSlot=-1,sMirrorAcceptedScene=-1,sMirrorAcceptedMode=-1;
static uint32_t sMirrorAcceptedSeed=0;
'''
        main=r'''
int main(){
 PlayState reused{};
 randomizerQueuedChecks.push(RC_LW_SKULL_KID);
 randomizerQueuedChecks.push(RC_LOST_WOODS_MAGIC_MIRROR_STUMP);
 randomizerQueuedChecks.push(RC_LW_TARGET_IN_WOODS);
 randomizerQueuedCheck=RC_LW_OCARINA_MEMORY_GAME;randomizerQueuedItemEntry.itemId=ITEM_BOW;
 sMirrorAcceptedPlay=&reused;sMirrorAcceptedSlot=1;sMirrorAcceptedScene=2;sMirrorAcceptedMode=3;sMirrorAcceptedSeed=99;
 forbidContext=true;MirrorShuffle::CancelStumpReward(); // boot/teardown must need no Rando context
 REQUIRE(randomizerQueuedChecks.size()==2&&randomizerQueuedChecks.front()==RC_LW_SKULL_KID);
 randomizerQueuedChecks.pop();REQUIRE(randomizerQueuedChecks.front()==RC_LW_TARGET_IN_WOODS);
 REQUIRE(randomizerQueuedCheck==RC_LW_OCARINA_MEMORY_GAME&&randomizerQueuedItemEntry.itemId==ITEM_BOW);
 REQUIRE(!sMirrorAcceptedPlay&&sMirrorAcceptedSlot==-1&&sMirrorAcceptedSeed==0);
 forbidContext=false;randomizerQueuedChecks={};randomizerQueuedCheck=RC_UNKNOWN_CHECK;
 REQUIRE(MirrorShuffle::QueueStumpReward(77)&&Randomizer_MagicMirrorStumpQueued());
 REQUIRE(MagicMirror_CommitStumpRewardToken());
 REQUIRE(!MagicMirror_CommitStumpRewardToken());
 randomizerQueuedChecks={};randomizerQueuedCheck=RC_LOST_WOODS_MAGIC_MIRROR_STUMP;
 randomizerQueuedItemEntry.itemId=ITEM_BOW;sMirrorAcceptedPlay=&reused;
 collected=true;forbidContext=true;MirrorShuffle::CancelStumpReward();
 REQUIRE(randomizerQueuedCheck==RC_UNKNOWN_CHECK&&!sMirrorAcceptedPlay&&collected);
 REQUIRE(!Randomizer_MagicMirrorStumpQueued());
 forbidContext=false;contextValid=false; // same PlayState address cannot resurrect the consumed token
 REQUIRE(!MagicMirror_CommitStumpRewardToken());
 puts("PASS native OoT cancel/backend: unrelated queued/active rewards retained, accepted state/token cleared, same-address stale commit rejected, collected flag preserved, no context/actor dependency");
}
'''
    else:
        native=(ROOT/'mm/2s2h/Rando/MiscBehavior/CheckQueue.cpp').read_text()
        bodies=extract(native,'extern "C" void Rando_CancelMagicMirrorStump(')
        prefix+=r'''
#include "2s2h/Rando/Rando.h"
extern "C" {int gComboGameMode=COMBO_GAME_MM;}
GameInteractor* GameInteractor::Instance=nullptr;
static bool queued=false;
static RandoCheckId queuedCheck=RC_UNKNOWN;
static ActorFunc sCheckQueueGive=nullptr;
void NativeGive(Actor*,PlayState*){}
void FixedGive(Actor*,PlayState*){}
'''
        main=r'''
int main(){
 MirrorShuffle::CancelStumpReward(); // before interactor init
 auto interactor=std::make_unique<GameInteractor>();GameInteractor::Instance=interactor.get();
 gSaveContext.fileNum=1;gSaveContext.save.shipSaveInfo.saveType=SAVETYPE_RANDO;
 RANDO_SAVE_OPTIONS[RO_SHUFFLE_MAGIC_MIRROR]=1;gSaveContext.save.shipSaveInfo.rando.finalSeed=101;
 auto& stump=RANDO_SAVE_CHECKS[RC_LOST_WOODS_MAGIC_MIRROR_STUMP];stump.randoItemId=RI_BOW;
 REQUIRE(MirrorShuffle::QueueStumpReward(77)&&stump.eligible);
 sCheckQueueGive=NativeGive;queued=true;queuedCheck=RC_LOST_WOODS_MAGIC_MIRROR_STUMP;
 interactor->events={GIEventGiveItem{true,RC_CLOCK_TOWN_SOUTH_CHEST_LOWER,NativeGive,nullptr},
  GIEventGiveItem{true,RC_LOST_WOODS_MAGIC_MIRROR_STUMP,NativeGive,nullptr},
  GIEventGiveItem{true,RC_LOST_WOODS_MAGIC_MIRROR_STUMP,FixedGive,nullptr}};
 interactor->currentEvent=GIEventGiveItem{true,RC_LOST_WOODS_MAGIC_MIRROR_STUMP,NativeGive,nullptr};
 stump.obtained=true;MirrorShuffle::CancelStumpReward();
 REQUIRE(!stump.eligible&&stump.obtained&&stump.randoItemId==RI_BOW);
 REQUIRE(!queued&&queuedCheck==RC_UNKNOWN&&std::holds_alternative<GIEventNone>(interactor->currentEvent));
 REQUIRE(interactor->events.size()==2);
 REQUIRE(std::get<GIEventGiveItem>(interactor->events[0]).param==RC_CLOCK_TOWN_SOUTH_CHEST_LOWER);
 REQUIRE(std::get<GIEventGiveItem>(interactor->events[1]).giveItem==FixedGive);
 REQUIRE(!MagicMirror_CommitStumpRewardToken());
 // Reused native context with a normal check stays intact, including active event and queue status.
 queued=true;queuedCheck=RC_CLOCK_TOWN_SOUTH_CHEST_LOWER;
 interactor->currentEvent=GIEventGiveItem{true,RC_CLOCK_TOWN_SOUTH_CHEST_LOWER,NativeGive,nullptr};
 MirrorShuffle::CancelStumpReward();REQUIRE(queued&&queuedCheck==RC_CLOCK_TOWN_SOUTH_CHEST_LOWER);
 REQUIRE(std::holds_alternative<GIEventGiveItem>(interactor->currentEvent));
 stump={RI_BOW,true,false,false,false,false,0};REQUIRE(MirrorShuffle::QueueStumpReward(77));
 REQUIRE(MagicMirror_CommitStumpRewardToken()&&!MagicMirror_CommitStumpRewardToken());
 stump.eligible=false;REQUIRE(MirrorShuffle::QueueStumpReward(77));
 gSaveContext.fileNum=2;gSaveContext.save.shipSaveInfo.rando.finalSeed=202;
 stump.eligible=true;MirrorShuffle::CancelStumpReward();
 REQUIRE(stump.eligible); // stale cancellation cannot erase a newly-bound save's eligibility
 contextValid=false;REQUIRE(!MagicMirror_CommitStumpRewardToken());
 GameInteractor::Instance=nullptr;MirrorShuffle::CancelStumpReward();
 puts("PASS native MM cancel/backend: only stump event/current native queue removed; fixed/unrelated events and obtained/assigned reward persist; new-slot eligibility protected; stale token rejected; boot safe");
}
'''
    fixture=prefix+'\n'+bodies+'\n'+backend+'\n'+main
    with tempfile.TemporaryDirectory(prefix='mirror-cancel-'+host) as td:
        temp=Path(td);(temp/'test.cpp').write_text(fixture)
        flags=['-std=c++20','-w','-DF3DEX_GBI_2','-DCOMBO_BUILD','-DCONTROLLERBUTTONS_T=uint32_t',
               '-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
        for p in ['',host,host+'/include',host+'/include/PR',host+'/src',host+'/assets',host+('/soh' if host=='soh' else '/2s2h'),'libultraship/include','combo','combo/menu']:
            flags+=['-I'+str(ROOT/p)]
        result=subprocess.run([os.environ.get('CXX','c++'),*flags,str(temp/'test.cpp'),'-o',str(temp/'test')],capture_output=True,text=True)
        if result.returncode:raise RuntimeError(host+'\n'+result.stdout+result.stderr)
        subprocess.run([str(temp/'test')],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
