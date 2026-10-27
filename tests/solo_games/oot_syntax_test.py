#!/usr/bin/env python3
"""Compile edited OoT production units with real project and dependency headers."""
from pathlib import Path
import argparse,os,re,subprocess
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--standalone',action='store_true');p.add_argument('--source-rev');p.add_argument('--source',action='append');args=p.parse_args()
prefixes=dict(re.findall(r'set\((CVAR_PREFIX_\w+)\s+"([^"]+)"\)',(ROOT/'CMake/soh-cvars.cmake').read_text()))
defines=[f'-D{k}="{v}"' for k,v in prefixes.items()]
for k,v in re.findall(r'set\((CVAR_\w+)\s+"([^"]+)"\)',(ROOT/'CMake/lus-cvars.cmake').read_text()):
 for key,value in prefixes.items():v=v.replace('${'+key+'}',value)
 defines.append(f'-D{k}="{v}"')
flags=['-DF3DEX_GBI_2','-DIMGUI_DEFINE_MATH_OPERATORS=','-DLOG_LEVEL_GAME_PRINTS=0','-DCONTROLLERBUTTONS_T=uint32_t','-DNON_EQUIVALENT','-DNON_MATCHING',*defines]
if not args.standalone:flags+=['-DCOMBO_BUILD','-DSOH_BUILD_DLL']
flags+=['-I'+str(ROOT/p) for p in ['soh/include','soh/include/PR','soh/src','soh','soh/soh','soh/assets','soh/mods','soh/soh/Extractor','libultraship/include','libultraship/src','combo','combo/menu','combo/title']]
sources = ['soh/soh/ComboFaroresWind.cpp', 'soh/soh/Enhancements/FileSelectEnhancements.cpp', 'soh/soh/Enhancements/randomizer/SeedContext.cpp', 'soh/soh/Enhancements/randomizer/ShuffleSigns.cpp', 'soh/soh/Enhancements/randomizer/entrance.cpp', 'soh/soh/Enhancements/randomizer/draw.cpp', 'soh/soh/Enhancements/randomizer/Messages/ItemMessages.cpp', 'soh/src/overlays/actors/ovl_player_actor/z_player.c', 'soh/soh/Enhancements/randomizer/hook_handlers.cpp', 'soh/soh/Enhancements/randomizer/randomizer.cpp', 'soh/soh/Enhancements/randomizer/randomizer_entrance.c', 'soh/soh/Enhancements/randomizer/randomizer_grotto.c', 'soh/soh/Enhancements/randomizer/randomizer_item_tracker.cpp', 'soh/soh/Enhancements/randomizer/settings.cpp', 'soh/soh/FleetShipCombo/FleetSharedItems.cpp', 'soh/soh/FleetShipCombo/FleetShipCombo.cpp', 'soh/soh/Network/Anchor/Anchor.cpp', 'soh/soh/Network/Anchor/Menu.cpp', 'soh/soh/Network/Network.cpp', 'soh/soh/OTRGlobals.cpp', 'soh/soh/SaveManager.cpp', 'soh/soh/SohGui/SohGui.cpp', 'soh/soh/SohGui/SohMenuDevTools.cpp', 'soh/soh/SohGui/SohMenuEnhancements.cpp', 'soh/soh/SohGui/SohMenuNEI.cpp', 'soh/soh/SohGui/SohMenuRandomizer.cpp', 'soh/soh/SohGui/SohMenuSettings.cpp', 'soh/src/code/z_parameter.c', 'soh/src/code/z_sram.c', 'soh/src/overlays/actors/ovl_Boss_Ganon2/z_boss_ganon2.c', 'soh/src/overlays/actors/ovl_En_Mag/z_en_mag.c', 'soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c']
if args.source:sources=args.source
defaults=['/workspace/scratch/4288e889429f/build-deps/include','/workspace/scratch/4288e889429f/build-deps/sysroot/usr/include','/workspace/scratch/60110ae7df32/analysis/deps','/workspace/scratch/60110ae7df32/analysis/deps/SDL2']
env={**os.environ,'CPLUS_INCLUDE_PATH':os.environ.get('CPLUS_INCLUDE_PATH',':'.join(defaults)),'C_INCLUDE_PATH':os.environ.get('C_INCLUDE_PATH',':'.join(defaults))}
failed=[]
for source in sources:
 c=source.endswith('.c');language='c' if c else 'c++';compiler=os.environ.get('CC','cc') if c else os.environ.get('CXX','c++')
 command=[compiler,'-std=gnu11' if c else '-std=c++20',*flags,'-fsyntax-only']
 content=None
 if args.source_rev:
  content=subprocess.check_output(['git','show',args.source_rev+':'+source],cwd=ROOT,text=True)
  command+=['-iquote',str((ROOT/source).parent),'-x',language,'-']
 else:command+=[str(ROOT/source)]
 result=subprocess.run(command,input=content,cwd=ROOT,capture_output=True,text=True,env=env)
 if result.returncode:
  failed.append(source);print('FAIL '+source,flush=True);print(result.stdout+result.stderr,flush=True)
 else:print('PASS '+source+f' ({result.stderr.count("warning:")} warnings)',flush=True)
if failed:raise SystemExit(f'{len(failed)}/{len(sources)} failed: '+', '.join(failed))
print(f'PASS {len(sources)} OoT production translation units'+(' standalone' if args.standalone else ' COMBO_BUILD'))
