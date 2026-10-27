#!/usr/bin/env python3
"""Execute native companion follow/update and player C-Up candidate selection."""
from pathlib import Path
import os,re,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
with tempfile.TemporaryDirectory(prefix='market-fairy-quiet-') as directory:
 for host in ((os.environ['FAIRY_TEST_HOST'],) if 'FAIRY_TEST_HOST' in os.environ else ('mm','soh')):
  oot=host=='soh';elf=(ROOT/host/'src/overlays/actors/ovl_En_Elf/z_en_elf.c').read_text()
  player=(ROOT/host/'src/overlays/actors/ovl_player_actor/z_player.c').read_text()
  signatures=['void func_80A03CF8(EnElf* this, PlayState* play) {','void func_80A053F0(Actor* thisx, PlayState* play) {'] if oot else ['void func_8088E850(EnElf* this, PlayState* play) {','void func_8089010C(Actor* thisx, PlayState* play) {']
  functions='\n'.join(block_from(elf,elf.index(s)) for s in signatures)
  # SoH's unused debug-register mirror indexes sceneFlags[127] past its
  # 124-entry native type. It is outside companion behavior; isolate it here.
  if oot: functions=functions.replace('gSaveContext.sceneFlags[127].chest','0')
  helper=(block_from(elf,elf.index('static s32 EnElf_BlackMarketQuiet(')) if 'static s32 EnElf_BlackMarketQuiet(' in elf else '')
  begin=player.index('s32 Player_ActionHandler_Talk(Player* this, PlayState* play) {')
  marker='    if (GameInteractor_Should(VB_SPEAK,' if oot else '        if ((talkOfferActor != NULL) || (cUpTalkActor != NULL))'
  prefix=player[begin:player.index(marker,begin)]
  prefix=prefix.replace('s32 Player_ActionHandler_Talk','CUpCandidates NativeCUpCandidates',1)
  prefix+='return {talkOfferActor,cUpTalkActor};\n'+('}\n' if not oot else '')+'return {};\n}'
  fixture=(ROOT/'tests/black_market/fairy_quiet_native_test.cpp').read_text().replace('/* PRODUCTION */',helper+'\n'+functions+'\n'+prefix)
  fixture=re.sub(r'\bthis\b','self',fixture)
  cpp=Path(directory)/(host+'.cpp');cpp.write_text(fixture);binary=Path(directory)/host
  paths=('.',host,host+'/include',host+'/include/PR',host+'/src',host+'/assets',host+('/soh' if oot else '/2s2h'),'libultraship/include','combo')
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2','-g',
   '-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*(['-DFAIRY_QUIET_OOT'] if oot else []),
   '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"','-DCVAR_PREFIX_SETTING="gSettings"',
   *['-I'+str(ROOT/p) for p in paths],str(cpp),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
