#!/usr/bin/env python3
"""Reuse native settings fixture/extraction; replace its test main with solo-mode assertions.

The existing fixture supplies real Option/Context/FinalizeSettings and native
JSON save templates. Only engine services are fixtures. --source-rev selects a
recorded production finalizer for a repeatable red control.
"""
from pathlib import Path
import argparse,importlib.util,os,subprocess,sys
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--source-rev');args=p.parse_args()
runner=ROOT/'tests/item_receipts/run_seed_settings_tests.py'
spec=importlib.util.spec_from_file_location('native_seed_fixture',runner);mod=importlib.util.module_from_spec(spec);spec.loader.exec_module(mod)
original_run=subprocess.run
original_read=Path.read_text
baseline=(subprocess.check_output(['git','show',args.source_rev+':soh/soh/Enhancements/randomizer/settings.cpp'],cwd=ROOT,text=True) if args.source_rev else None)
def read(self,*a,**k):
 if baseline is not None and self==ROOT/'soh/soh/Enhancements/randomizer/settings.cpp':return baseline
 return original_read(self,*a,**k)
main=r'''
int main(){
 using namespace Rando;
 SaveManager storage; SaveManager::Instance=&storage;
 auto settings=Settings::GetInstance();
 auto context=Context::CreateInstance();settings->AssignContext(context);
 globals.gRandoContext=context;
 auto prepare=[&](){
  context->GetOption(RSK_WINCON).Set(RO_WINCON_TRIFORCE_PIECES);
  context->GetOption(RSK_WINCON_TRIFORCE_COUNT).Set(10);
  context->GetOption(RSK_TRIFORCE_HUNT_PIECES_TOTAL).Set(20);
  context->GetOption(RSK_TRIFORCE_HUNT_PIECES_LOCATION).Set(1);
  context->GetOption(RSK_MASK_SHOP_HINT).Set(RO_GENERIC_ON);
  context->GetOption(RSK_SHUFFLE_CHILD_WALLET).Set(RO_GENERIC_ON);
  context->GetOption(RSK_STARTING_AGE).Set(RO_AGE_ADULT);
  context->GetOption(RSK_FOREST).Set(RO_CLOSED_FOREST_OFF);
  context->GetOption(RSK_DOOR_OF_TIME).Set(RO_DOOROFTIME_OPEN);
 };
 gComboGameMode=COMBO_GAME_OOT;
 gComboGoalHunt=0;gComboGoalRequired=0;gComboGoalPieces=0;
 gComboStartingGameMM=1;gComboSharedMask=1<<ComboRando::SF_WALLET;
 prepare();context->FinalizeSettings({},{});
 Check(context->GetOption(RSK_WINCON).Is(RO_WINCON_TRIFORCE_PIECES),"solo retains native win condition");
 Check(context->GetOption(RSK_WINCON_TRIFORCE_COUNT).Is(10),"solo retains native required pieces");
 Check(context->GetOption(RSK_TRIFORCE_HUNT_PIECES_TOTAL).Is(20),"solo retains native pool size");
 Check(context->GetOption(RSK_TRIFORCE_HUNT_PIECES_LOCATION).Is(1),"solo retains piece location choice");
 Check(context->GetOption(RSK_MASK_SHOP_HINT).Is(RO_GENERIC_ON),"solo Mask Shop remains reachable");
 Check(context->GetOption(RSK_SHUFFLE_CHILD_WALLET).Is(RO_GENERIC_ON),"solo Child Wallet remains shuffleable");
 Check(context->GetOption(RSK_SELECTED_STARTING_AGE).Is(RO_AGE_ADULT),"stale MM start cannot force solo age");
 gComboGameMode=COMBO_GAME_BOTH;prepare();context->FinalizeSettings({},{});
 Check(context->GetOption(RSK_WINCON).Is(RO_WINCON_DEFEAT_GANON),"combined goal override preserved");
 Check(context->GetOption(RSK_TRIFORCE_HUNT_PIECES_TOTAL).Is(0),"combined pool override preserved");
 Check(context->GetOption(RSK_MASK_SHOP_HINT).Is(RO_GENERIC_OFF),"combined portal hint exclusion preserved");
 Check(context->GetOption(RSK_SHUFFLE_CHILD_WALLET).Is(RO_GENERIC_OFF),"combined shared wallet exclusion preserved");
 Check(context->GetOption(RSK_SELECTED_STARTING_AGE).Is(RO_AGE_CHILD),"combined MM start preserved");
 context->GetLogic()->SetContext(nullptr);
 std::puts("PASS actual native FinalizeSettings solo choices and combined overrides");
}
'''
def run(command,*a,**k):
 command=list(command)
 fixture=str(ROOT/'tests/item_receipts/seed_settings_test.cpp')
 if fixture in command:
  directory=Path(command[command.index('-o')+1]).parent
  prefix=original_read(Path(fixture)).split('int main(')[0]
  prefix='#include "ComboGameMode.h"\n'+prefix
  if 'int gComboGameMode' not in prefix:prefix+='extern "C" { int gComboGameMode=COMBO_GAME_BOTH; }\n'
  replacement=directory/'oot_solo_settings.cpp';replacement.write_text(prefix+main)
  command[command.index(fixture)]=str(replacement)
  k['capture_output']=True;k['text']=True
  want_check=k.pop('check',False)
  result=original_run(command,*a,**k)
  if result.returncode:sys.stderr.write(result.stdout+result.stderr)
  if want_check:result.check_returncode()
  return result
 return original_run(command,*a,**k)
# The compile uses the same documented dependency/header set as the project checks.
defaults=['/workspace/scratch/4288e889429f/build-deps/include','/workspace/scratch/4288e889429f/build-deps/sysroot/usr/include','/workspace/scratch/60110ae7df32/analysis/deps','/workspace/scratch/60110ae7df32/analysis/deps/SDL2']
os.environ['CPLUS_INCLUDE_PATH']=os.environ.get('CPLUS_INCLUDE_PATH',':'.join(defaults))
Path.read_text=read;subprocess.run=run;sys.argv=[str(runner)]
try:sys.exit(mod.main())
finally:Path.read_text=original_read;subprocess.run=original_run
