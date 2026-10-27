#!/usr/bin/env python3
"""Probe the actual linked game modules, without a window or ROM archives."""
import argparse
import ctypes
import json
from pathlib import Path

p=argparse.ArgumentParser()
p.add_argument('game_dir',type=Path)
p.add_argument('--output',type=Path,default=Path('cross-dungeon-native-evidence.json'))
a=p.parse_args()
modules=[ctypes.CDLL(str(a.game_dir.resolve()/name),mode=ctypes.RTLD_GLOBAL)
         for name in ['libsoh.so','lib2ship.so']]

def call(game,name,arg=None,kind='void'):
    f=getattr(modules[game],name)
    f.argtypes=[] if arg is None else [ctypes.c_char_p]
    f.restype={'void':None,'json':ctypes.c_char_p,'int':ctypes.c_int}[kind]
    result=f() if arg is None else f(json.dumps(arg).encode())
    if kind=='json':
        if result is None: raise AssertionError(name+' returned null')
        return json.loads(result)
    return result

prefix=['SOH','MM']
for g in range(2): call(g,prefix[g]+'_InitRandoHeadless')
# A first reload must preflight before the lazy native graph initialization. Exercise that order.
early={'oot':{'settings':call(0,'SOH_DumpRandoSettings',kind='json')},'entrances':{'oot':[]}}
catalogues=[call(0,'SOH_DumpCrossDungeonCatalogueForSeed',early,'json'),
            call(1,'MM_DumpCrossDungeonCatalogue',kind='json')]
assert len(catalogues[0])==11 and len(catalogues[1])==5, list(map(len,catalogues))
all_dungeons=sum(catalogues,[])
permutation=list(range(len(all_dungeons)))
oot=next(i for i,d in enumerate(all_dungeons) if d['id']=='oot:0')
mm=next(i for i,d in enumerate(all_dungeons) if d['id']=='mm:woodfall')
permutation[oot],permutation[mm]=mm,oot
seed={'version':1,'enabled':True,'dungeons':all_dungeons,'permutation':permutation}
for g in range(2):
    assert call(g,prefix[g]+'_SetCrossDungeonSeed',seed,'int')==1
    assert call(g,prefix[g]+'_SetCrossDungeonRuntimeSeed',seed,'int')==1
dumps=[call(g,prefix[g]+'_DumpRandoStaticData',kind='json') for g in range(2)]
assert call(0,'SOH_DumpCrossDungeonCatalogue',kind='json')==catalogues[0]
consolidated={'oot':{'settings':call(0,'SOH_DumpRandoSettings',kind='json')},
              'entrances':{'oot':call(0,'SOH_DumpEntranceOverrides',kind='json')}}
assert call(0,'SOH_DumpCrossDungeonCatalogueForSeed',consolidated,'json')==catalogues[0]
bad=json.loads(json.dumps(consolidated))
bad['entrances']['oot']=[{'type':5,'index':catalogues[0][0]['entrance']['index'],
                         'override':catalogues[0][0]['entrance']['index'],
                         'destination':2**64-1,'overrideDestination':-1}]
assert modules[0].SOH_DumpCrossDungeonCatalogueForSeed(json.dumps(bad).encode()) is None

counts=[]
for g in range(2):
    # Use the real pool multiplicities and fixed placements, including progressive levels.
    owned=[item['name'] for item in dumps[g]['pool']]+[item['item'] for item in dumps[g]['fixed']]
    dungeon=all_dungeons[[oot,mm][g]]
    def query(world,arrivals):
        call(g,'Combo_'+prefix[g]+'_Rando_Reset')
        call(g,'Combo_'+prefix[g]+'_Rando_SetOwnedItems',owned)
        call(g,prefix[g]+'_SetCrossDungeonAccess',{'overworld':world,'arrivals':arrivals})
        return set(call(g,'Combo_'+prefix[g]+'_Rando_GetReachableChecks',kind='json'))
    full=query(True,[])
    empty=query(False,[])
    restricted=query(False,[dungeon['entrance']['index']])
    assert restricted, (g,'arrival has no dungeon checks')
    assert len(restricted)<len(full), (g,'foreign arrival opened full overworld')
    assert len(empty)<len(restricted), (g,'empty roots already reach destination')
    marker='Deku Tree' if g==0 else 'Woodfall Temple'
    assert any(marker.lower() in name.lower() for name in restricted), (g,marker,sorted(restricted)[:5])
    outside=[name for name in restricted if ('Kokiri Sword Chest' if g==0 else 'Clock Town') in name]
    assert not outside, (g,'foreign arrival leaked overworld',outside[:5])
    counts.append({'game':prefix[g],'world':len(full),'empty':len(empty),'dungeon':len(restricted),
                   'dungeonCheckSample':sorted(restricted)[:12]})
for g in range(2): assert call(g,prefix[g]+'_SetCrossDungeonSeed',{'enabled':False},'int')==1
# Native dungeon shuffle OFF must still allow both ages through shuffled mouths.
# Adult temples behind Deku/Well mouths previously inherited vanilla child-only
# conditions. Jabu's physical mouth remains child-only even in native ER, so this
# age-rule regression keeps Fire and Jabu at their own mouths.
settings_before=call(0,'SOH_DumpRandoSettings',kind='json')
assert next(v for k,v in settings_before.items() if k.endswith('.ShuffleDungeonsEntrances'))==0
owned_by_game=[[item['name'] for item in data['pool']]+[item['item'] for item in data['fixed']]
               for data in dumps]
def age_query(g,roots):
    call(g,'Combo_'+prefix[g]+'_Rando_Reset')
    call(g,'Combo_'+prefix[g]+'_Rando_SetOwnedItems',owned_by_game[g])
    call(g,prefix[g]+'_SetCrossDungeonAccess',{'overworld':True,'arrivals':sorted(roots)})
    return set(call(g,'Combo_'+prefix[g]+'_Rando_GetReachableChecks',kind='json'))
vanilla_checks=age_query(0,[])
targets=['oot:361','oot:136','oot:40','mm:greatbay','oot:357','oot:0',
         'mm:woodfall','oot:130','oot:16','mm:inverted','oot:8',
         'oot:152','oot:4','mm:snowhead','oot:55','mm:stonetower']
by_id={d['id']:i for i,d in enumerate(all_dungeons)}
age_seed={'version':1,'enabled':True,'dungeons':all_dungeons,
          'permutation':[by_id[target] for target in targets]}
for g in range(2): assert call(g,prefix[g]+'_SetCrossDungeonSeed',age_seed,'int')==1
roots=[set(),set()]
for iteration in range(514):
    before=[set(r) for r in roots]
    reached=[age_query(g,roots[g]) for g in range(2)]
    for g in range(2):
        for endpoint in call(g,prefix[g]+'_GetCrossDungeonExits',kind='json'):
            roots[endpoint['game']].add(endpoint['index'])
    if before==roots: break
else: raise AssertionError('joint age-access search did not converge')
markers={'Forest Temple Bow Chest','Fire Temple Megaton Hammer Chest',
         'Water Temple Longshot Chest','Jabu Jabus Belly Map Chest'}
assert markers<=vanilla_checks, ('age regression setup',markers-vanilla_checks)
assert not vanilla_checks-reached[0], ('shuffled mouths lost checks',sorted(vanilla_checks-reached[0]))
assert call(0,'SOH_DumpRandoSettings',kind='json')==settings_before, 'oracle changed saved native settings'
for g in range(2): assert call(g,prefix[g]+'_SetCrossDungeonSeed',{'enabled':False},'int')==1
# Replay a genuinely changed native graph, then query its catalogue twice. This exercises the
# native graph adapter after ParseJson (which does not populate replacement pointers).
x,y=catalogues[0][:2]
overrides=[]
for left,right in [(x,y),(y,x)]:
    for source,target,kind in [(left['entrance']['index'],right['entrance']['index'],5),
                               (left['exit']['index'],right['exit']['index'],5),
                               (left['escapeExits'][0]['index'],right['escapeExits'][0]['index'],4)]:
        overrides.append({'type':kind,'index':source,'override':target,
                          'destination':-1,'overrideDestination':-1})
assert call(0,'SOH_ApplyEntranceOverridesForCombo',overrides,'int')==1
replayed=call(0,'SOH_DumpCrossDungeonCatalogue',kind='json')
assert call(0,'SOH_DumpCrossDungeonCatalogue',kind='json')==replayed
consolidated['entrances']['oot']=overrides
assert call(0,'SOH_DumpCrossDungeonCatalogueForSeed',consolidated,'json')==replayed
by_id={d['id']:d for d in replayed}
assert by_id[x['id']]['returnTo']==y['exit']
assert by_id[y['id']]['returnTo']==x['exit']
assert call(0,'SOH_ApplyEntranceOverridesForCombo',[],'int')==1
result={'catalogueSizes':list(map(len,catalogues)),'installedAllNativeEndpoints':True,'firstReloadBeforeGraphInit':True,'overflowRejected':True,
        'pureSeedCatalogueMatches':True,'recordedNativeLayoutReplay':True,'restrictedRoots':counts,
        'nativeOffBothAges':True,'nativeSettingsPreserved':True}
a.output.write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
