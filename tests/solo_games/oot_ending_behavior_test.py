#!/usr/bin/env python3
"""Execute the real Ganon ending state; an absent peer must not block solo credits."""
from pathlib import Path
import argparse,os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--source-rev');args=p.parse_args()
file='soh/src/overlays/actors/ovl_Boss_Ganon2/z_boss_ganon2.c'
s=subprocess.check_output(['git','show',args.source_rev+':'+file],cwd=ROOT,text=True) if args.source_rev else (ROOT/file).read_text()
a=s.index('// ComboShip: gate the ending on both bosses.');a=s.rindex('        case 20:',0,a)
b=s.index('\n    }',a)
cpp=r'''
#include <cassert>
#include <cstddef>
#include "ComboGameMode.h"
extern "C"{int gComboGameMode=COMBO_GAME_OOT;}
constexpr int ENTR_TEMPLE_OF_TIME_WARP_PAD=1,ENTR_CHAMBER_OF_THE_SAGES_0=2,TRANS_TRIGGER_START=1,TRANS_TYPE_FADE_WHITE=2,LINK_AGE_ADULT=0;
struct {int nextCutsceneIndex=0,fileNum=0;}gSaveContext;
struct PlayState {int nextEntranceIndex=0,transitionTrigger=0,transitionType=0,linkAgeOnLoad=0;};
int (*gComboFinalBossDefeated)(int,int)=nullptr;
int PeerDead(int,int){return 1;}
void Ending(PlayState* play){switch(20){
'''+s[a:b]+r'''
}}
int main(){
 PlayState p;Ending(&p);assert(p.nextEntranceIndex==ENTR_CHAMBER_OF_THE_SAGES_0 && gSaveContext.nextCutsceneIndex==0xFFF2);
 gComboGameMode=COMBO_GAME_BOTH;Ending(&p);assert(p.nextEntranceIndex==ENTR_TEMPLE_OF_TIME_WARP_PAD);
 gComboFinalBossDefeated=PeerDead;Ending(&p);assert(p.nextEntranceIndex==ENTR_CHAMBER_OF_THE_SAGES_0);
}
'''
with tempfile.TemporaryDirectory(prefix='oot-ending-') as tmp:
 path=Path(tmp);(path/'test.cpp').write_text(cpp)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DCOMBO_BUILD','-I'+str(ROOT/'combo'),str(path/'test.cpp'),'-o',str(path/'test')],check=True)
 subprocess.run([str(path/'test')],check=True)
print('PASS native Ganon ending and combined boss gate')
