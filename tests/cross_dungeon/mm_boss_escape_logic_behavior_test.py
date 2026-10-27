#!/usr/bin/env python3
"""Execute actual MM boss escape and native defeat predicates through the production exit oracle."""
import os
import pathlib
import re
import subprocess
import sys
import tempfile
root = pathlib.Path(__file__).resolve().parents[2]
sanitize = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-g'] if '--sanitize' in sys.argv else []
if sanitize:
    os.environ['ASAN_OPTIONS'] = 'detect_leaks=0'

def block(text, marker):
    start = text.index(marker)
    opening = text.index('{', start)
    depth, end = 1, opening + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]

def arguments(text, marker):
    start = text.index(marker) + len(marker)
    depth, args, last = 1, [], start
    for i in range(start, len(text)):
        if text[i] == '(':
            depth += 1
        elif text[i] == ')':
            depth -= 1
            if not depth:
                args.append(text[last:i].strip())
                return args
        elif text[i] == ',' and depth == 1:
            args.append(text[last:i].strip())
            last = i + 1
    raise ValueError('Unterminated macro')

# Read the production EXIT expressions; no parallel list of expected conditions.
regions = [('SnowheadTemple', 'RR_SNOWHEAD_TEMPLE_BOSS_ROOM', 'ACTOR_BOSS_HAKUGIN'),
           ('GreatBayTemple', 'RR_GREAT_BAY_TEMPLE_BOSS_ROOM', 'ACTOR_BOSS_03'),
           ('StoneTowerTemple', 'RR_STONE_TOWER_TEMPLE_BOSS_ROOM', 'ACTOR_BOSS_02')]
escapes = []
for file, region, actor in regions:
    text = block((root/f'mm/2s2h/Rando/Logic/Regions/{file}.cpp').read_text(), f'Regions[{region}] = RandoRegion')
    escape = arguments(text[text.index('.exits ='):], 'EXIT(')
    escapes.append((escape[0], escape[2], actor))

# Compile the selected cases from the real CanKillEnemy, including its enemy-soul guard.
logic = (root/'mm/2s2h/Rando/Logic/Logic.h').read_text()
native = block(logic, 'inline bool CanKillEnemy(ActorId EnemyId)')
guard = native[native.index('{') + 1:native.index('switch (EnemyId)')]
cases = []
for _, _, actor in regions:
    start = native.index('case '+actor+':')
    end = native.index('case ', start + len('case '))
    cases.append(native[start:end])
defeat = 'static bool CanKillEnemy(int EnemyId) {' + guard + 'switch (EnemyId) {' + ''.join(cases) + 'default:return false;}}\n'

prefix = r'''
#define COMBO_BUILD 1
#include "combo/ComboGameMode.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#define COMBO_EXPORT
#include "combo/rando/CrossDungeonState.h"
#include <cassert>
#include <functional>
#include <map>
#include <unordered_map>
using namespace ComboRando::CrossDungeon;
enum { ACTOR_BOSS_HAKUGIN, ACTOR_BOSS_03, ACTOR_BOSS_02 };
enum { RO_GENERIC_NO=0, RO_SHUFFLE_ENEMY_SOULS=1, RO_SHUFFLE_BOSS_SOULS=2 };
enum { RANDO_INF_OBTAINED_SOUL_OF_BOSS_GOHT, RANDO_INF_OBTAINED_SOUL_OF_BOSS_GYORG, RANDO_INF_OBTAINED_SOUL_OF_BOSS_TWINMOLD };
enum { ITEM_BOW, ITEM_MASK_GIANT };
enum { MOUNTAIN_VILLAGE_SPRING=30, ZORA_CAPE=31, IKANA_CANYON=32 };
#define ENTRANCE(scene,spawn) (((scene)<<9)|((spawn)<<4))
static int options[3]{};
#define RANDO_SAVE_OPTIONS options
static bool items[2]{}, magic=false, zora=false, deity=false, humanSword=false, fireArrows=false, soul=false, enemySoul=true;
#define HAS_ITEM(item) items[item]
#define HAS_MAGIC magic
#define CAN_BE_ZORA zora
#define CAN_BE_DEITY deity
#define CAN_USE_HUMAN_SWORD humanSword
#define CAN_USE_MAGIC_ARROW(type) fireArrows
static bool Flags_GetRandoInf(int) { return soul; }
static bool HaveEnemySoul(int) { return enemySoul; }
'''
conditions = ''.join(f'static bool Escape{i}() {{ return {condition}; }}\n' for i, (_, condition, _) in enumerate(escapes))
prefix += 'extern \"C\" int Combo_MM_CrossBossLogicEnabled(void);\n' + defeat + conditions + r'''
namespace Rando::Logic {
struct Exit { std::function<bool()> condition; };
struct Region { std::map<int,Exit> exits; };
static std::map<int,Region> Regions;
static void SetCurrentRegionTime(const std::unordered_map<int,int>&,int) {}
}
namespace ComboCrossMm {
static HostState state(1);
static bool searching=false;
static std::set<int> reachable;
static std::unordered_map<int,int> timeStates;
}
namespace ComboCrossMmCatalogue { static int GrottoExitForRegion(int) { return -1; } }
'''
production_helper = block((root/'combo/rando/CrossDungeonMm.inc.h').read_text(), 'extern \"C\" int Combo_MM_CrossBossLogicEnabled(void)')
production_oracle = block((root/'combo/rando/CrossDungeonMm.inc.h').read_text(), 'extern "C" COMBO_EXPORT const char* MM_GetCrossDungeonExits(void)')
main = r'''
int main() {
 const int exits[]={''' + ','.join(escape for escape, _, _ in escapes) + r'''};
 bool (*conditions[])()={Escape0,Escape1,Escape2};
 for(int boss=0;boss<3;++boss) for(Pool pool : {Pool::Dungeon,Pool::Boss}) for(int returnGame : {0,1}) {
  Dungeon source{"source",{returnGame,30000},{returnGame,30001},{returnGame,30001},{{returnGame,30002}}};
  Dungeon target{"target",{1,40000},{1,40001},{1,40001},{{1,exits[boss]}}};
  source.pool=target.pool=pool;
  if(pool==Pool::Boss) {
   source.boss=returnGame==0?0:8;source.owner="source:dungeon";source.warpReturn=Endpoint{returnGame,30003};
   target.boss=9+boss;target.owner="target:dungeon";target.warpReturn=Endpoint{1,40002};
  }
  ComboCrossMm::state.routes=FromPermutation({source,target},{1,0});
  ComboCrossMm::state.dungeons={source,target};ComboCrossMm::reachable={boss};
  Rando::Logic::Regions.clear();Rando::Logic::Regions[boss].exits[exits[boss]]={conditions[boss]};
  items[0]=items[1]=magic=zora=deity=humanSword=fireArrows=false;
  options[RO_SHUFFLE_BOSS_SOULS]=RO_GENERIC_NO;options[RO_SHUFFLE_ENEMY_SOULS]=0;
  const bool bossShuffle=pool==Pool::Boss;
  assert(conditions[boss]()==!bossShuffle && "boss-only defeat gating must preserve native/legacy exits");
  auto beforeFight=nlohmann::json::parse(MM_GetCrossDungeonExits());
  assert(beforeFight.size()==(returnGame==0&&!bossShuffle?1:0));
  // Each native predicate receives exactly the capability its actual case requires.
  if(boss==0)fireArrows=true;
  if(boss==1)zora=magic=true;
  if(boss==2)items[ITEM_BOW]=true;
  assert(conditions[boss]());
  auto foreign=nlohmann::json::parse(MM_GetCrossDungeonExits());
  assert(foreign.size()==(returnGame==0?1:0));
  if(returnGame==0)assert(ParseEndpoint(foreign[0])==Resolve(ComboCrossMm::state.routes,Phase::BeforeNative,{1,exits[boss]}));
  options[RO_SHUFFLE_BOSS_SOULS]=1;soul=false;
  assert(conditions[boss]()==!bossShuffle);
  assert(nlohmann::json::parse(MM_GetCrossDungeonExits()).size()==(returnGame==0&&!bossShuffle?1:0));
  soul=true;assert(conditions[boss]());
  options[RO_SHUFFLE_ENEMY_SOULS]=1;enemySoul=false;assert(conditions[boss]()==!bossShuffle);enemySoul=true;
 }
}
'''
with tempfile.TemporaryDirectory(prefix='mm-boss-escape-') as tmp:
    source = pathlib.Path(tmp)/'test.cpp'
    binary = pathlib.Path(tmp)/'test'
    source.write_text(prefix+production_helper+production_oracle+main)
    subprocess.run(['g++',*sanitize,'-std=c++20','-Wall','-Wextra','-I',str(root),'-I',str(root.parent/'build-deps/usr/include'),str(source),'-o',str(binary)],check=True)
    subprocess.run([str(binary)],check=True)
