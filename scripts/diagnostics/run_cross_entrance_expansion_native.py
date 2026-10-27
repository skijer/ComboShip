#!/usr/bin/env python3
"""Verify expanded pools using the linked native generator, replay and playthrough.

Runs in a disposable game directory containing the candidate engine modules. No ROM,
window or player saves are needed. The baseline generator uses the same native modules
with every cross pool disabled, isolating launcher/default-path compatibility.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import time

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('game_dir',type=Path)
p.add_argument('--baseline-comborando',required=True,type=Path)
p.add_argument('--comborando',type=Path)
p.add_argument('--base-settings',type=Path,help='Optional common settings fixture for generation controls/profiles')
p.add_argument('--legacy-reference',type=Path,help='Recovered v1 spoiler whose exact entrance map must be retained')
p.add_argument('--native-decoupled-stress',action='store_true',
               help='Also run all pools with native mixed decoupled entrances including Ganon; this control can exhaust fill retries')
p.add_argument('--output',type=Path,default=Path('cross-entrance-native-evidence'))
p.add_argument('--timeout',type=int,default=1200)
a=p.parse_args()
game=a.game_dir.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
exe=(a.comborando or game/('comborando.exe' if os.name=='nt' else 'comborando')).resolve()
baseline=a.baseline_comborando.resolve()
flags=['--cross-dungeon-entrances','--cross-boss-entrances','--expanded-dungeon-pool',
       '--cross-interior-entrances','--cross-grotto-entrances']
results=[]

def run(name,argv,binary=exe,seed=True):
    started=time.monotonic()
    if a.base_settings and '--playthrough' not in argv:
        argv=['--settings',str(a.base_settings.resolve()),*argv]
    with (out/(name+'.log')).open('w') as log:
        process=subprocess.run([str(binary),*argv],cwd=game,stdout=log,stderr=subprocess.STDOUT,timeout=a.timeout)
    result={'name':name,'exit':process.returncode,'seconds':round(time.monotonic()-started,2),'args':argv}
    results.append(result)
    (out/'RESULT.json').write_text(json.dumps(results,indent=2)+'\n')
    print(name,'exit',process.returncode,'seconds',result['seconds'],flush=True)
    if process.returncode:raise SystemExit('FAILED '+name+'; see '+str(out/(name+'.log')))
    if not seed:return None
    data=json.loads((game/'saves/combo/comborando.spoiler.json').read_text())
    (out/(name+'.json')).write_text(json.dumps(data,indent=2)+'\n')
    return data

def options(values):return [flag+'='+str(int(value)) for flag,value in zip(flags,values)]

def check_pools(seed,expected):
    data=seed['crossDungeonEntrances'];entries=data['dungeons'];permutation=data['permutation']
    assert data['enabled'] and len(entries)==len(permutation)
    assert sorted(permutation)==list(range(len(entries)))
    for i,target in enumerate(permutation):
        assert entries[i].get('pool',0)==entries[target].get('pool',0)
    actual={entry.get('pool',0) for entry in entries}
    assert actual==set(expected),(actual,expected)
    return entries

common=['--starting-game','oot','--shared-items','none']
control=['--seed','cross-entrance-disabled-control',*common]
original=run('disabled-baseline',control,baseline)
disabled=run('disabled-candidate',[*control,*options([0,0,0,0,0])])
assert original==disabled and 'crossDungeonEntrances' not in disabled

profiles=[('legacy-dungeons',[1,0,0,0,0],{0}),('boss-only',[0,1,0,0,0],{1}),
          ('expanded-dungeons',[1,0,1,0,0],{0}),('interior-only',[0,0,0,1,0],{2}),
          ('grotto-only',[0,0,0,0,1],{3}),('all-pools-oot',[1,1,1,1,1],{0,1,2,3})]
seeds={}
for name,selected,pools in profiles:
    label='cross-dungeon-native-off' if name=='legacy-dungeons' else 'cross-entrance-'+name
    data=run(name,['--seed',label,*common,*options(selected)])
    entries=check_pools(data,pools);seeds[name]=data
    mm=[entry for entry in entries if entry['entrance']['game']==1]
    if name=='legacy-dungeons':
        assert data['crossDungeonEntrances']['version']==1 and len(entries)==16
        if a.legacy_reference:
            recorded=json.loads(a.legacy_reference.read_text())
            assert data['crossDungeonEntrances']==recorded['crossDungeonEntrances'],'recovered v1 map changed'
    if name=='boss-only':assert len(entries)==12 and len(mm)==4
    if name=='expanded-dungeons':assert len(entries)==23 and sum(e.get('expanded',False) for e in mm)==7
    if name=='interior-only':assert len(mm)==42
    if name=='grotto-only':assert len(mm)==26
    if name=='all-pools-oot':assert len(mm)==84

all_seed=seeds['all-pools-oot']
again=run('all-pools-determinism',['--seed','cross-entrance-all-pools-oot',*common,*options([1,1,1,1,1])])
assert again==all_seed
replayed=run('all-pools-recorded-replay',['--settings',str(out/'all-pools-oot.json'),
    '--master-seed',str(all_seed['masterSeed']),'--seed','cross-entrance-all-pools-oot',*common])
assert replayed==all_seed
mm_seed=run('all-pools-mm',['--seed','cross-entrance-all-pools-mm','--starting-game','mm',
    '--shared-items','none',*options([1,1,1,1,1])])
check_pools(mm_seed,{0,1,2,3})
for name in ['boss-only','expanded-dungeons','interior-only','grotto-only','all-pools-oot','all-pools-mm']:
    run(name+'-playthrough',['--playthrough',str(out/(name+'.json'))],seed=False)

# Keep native RNG and its forward/reverse overrides in the comparison as well.
# Record the open Door of Time explicitly for the verified decoupled control.
native_profiles=[('coupled',False,False),('decoupled-open-time',True,False)]
if a.native_decoupled_stress:
    native_profiles.append(('decoupled-ganon-stress',True,True))
for suffix,decoupled,ganon_stress in native_profiles:
    name='all-pools-native-'+suffix
    settings=json.loads(json.dumps(all_seed));settings.pop('crossDungeonEntrances',None)
    settings.pop('entrances',None)
    updates={'ShuffleDungeonsEntrances':2 if ganon_stress else 1,'ShuffleBossEntrances':2,
             'ShuffleGrottosEntrances':1,'MixedEntrances':1,'MixDungeons':1,'MixBosses':1,
             'MixGrottos':1,'DecoupleEntrances':int(decoupled)}
    if decoupled and not ganon_stress:
        updates['DoorOfTime']=2
    for suffix,value in updates.items():
        keys=[key for key in settings['oot']['settings'] if key.endswith('.'+suffix)]
        assert len(keys)==1,(suffix,keys)
        settings['oot']['settings'][keys[0]]=value
    path=out/(name+'-settings.json');path.write_text(json.dumps(settings,indent=2)+'\n')
    # Coupled/open-time decoupled are verified controls. Decoupled plus Ganon is an
    # explicit stress control: rejected fills must remain failures in RESULT.json.
    label='cross-dungeon-native-mixed-decoupled' if decoupled else 'cross-dungeon-native-mixed-coupled'
    data=run(name,['--settings',str(path),'--seed',label,*common,*options([1,1,1,1,1])])
    check_pools(data,{0,1,2,3})
    run(name+'-playthrough',['--playthrough',str(out/(name+'.json'))],seed=False)
print('expanded native generation/replay/playthrough/disabled parity PASS',flush=True)
