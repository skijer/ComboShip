"""Execute native empty/potion drawers and their real foreign exports."""
from pathlib import Path
import argparse
import os
import re
import shlex
import subprocess
import tempfile
from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]

def native(namespace, game, member, resource_type, revision=None):
    path=str(Path(game)/'src/code/z_draw.c')
    src=subprocess.check_output(['git','show',revision+':'+path],cwd=ROOT,text=True) if revision else (ROOT/path).read_text()
    export=function(src,'GetItem_GetDrawTableEntry')
    names=sorted(set(re.findall(r'GetItem_Draw\w+',export)))
    constants='const char* gGiBottleStopperDL="cork"; const char* gGiBottleDL="glass";\n'
    constants+='const char* gGiEmptyBottleCorkDL="cork"; const char* gGiEmptyBottleGlassDL="glass";\n'
    constants+='const char* gGiBlueFireChamberstickDL="bluefire";\n'
    parts=['namespace '+namespace+' {',constants,'constexpr int GID_BOTTLE=3;']
    draws=('GetItem_DrawPotion','GetItem_DrawOpa0Xlu1')
    parts += ['void '+n+'(PlayState*,s16)'+(';' if n in draws else ' {}') for n in names]
    rows=[]
    for p in range(3):
        paths=['pot','palette'+str(p),'old-liquid-palette','liquid','old-pattern-palette','shell']
        rows.append('{GetItem_DrawPotion,{'+','.join('('+resource_type+')"'+v+'"' for v in paths)+'}}')
    # Import the actual first native row (GID_BOTTLE), including its symbols.
    table=src[src.index('sDrawItemTable['):]
    row=re.search(r'\{\s*(GetItem_Draw\w+),\s*\{([^}]+)\}',table)
    assert row and row[1]=='GetItem_DrawOpa0Xlu1'
    rows.append('{'+row[1]+',{'+','.join('('+resource_type+')'+s.strip() for s in row[2].split(','))+'}}')
    helpers=[function(src,'GetItem_FairyBottleShell')]
    if 'static ComboFairyBottleShell GetItem_EmptyBottleShell(' in src:
        helpers.insert(0,function(src,'GetItem_EmptyBottleShell'))
    parts += ['struct Entry {void(*drawFunc)(PlayState*,s16);'+resource_type+' '+member+'[8];};',
              'Entry sDrawItemTable[]={'+','.join(rows)+'};',
              *helpers,export,*[function(src,n) for n in draws],'}']
    return '\n'.join(parts)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--baseline-state',help='negative control: owner dependency policy from this revision')
    parser.add_argument('--baseline-rendering',help='negative control: native rendering/export bodies from this revision')
    args=parser.parse_args()
    bodies=native('oot','soh','dlists','Gfx*',args.baseline_rendering)+'\n'+native('mm','mm','drawResources','void*',args.baseline_rendering)
    fixture=(ROOT/'tests/bottle_gi/render_test.cpp').read_text().replace('/* PRODUCTION_NATIVE */',bodies)
    with tempfile.TemporaryDirectory(prefix='bottle-gi-test-') as tmp:
        out=Path(tmp); source=out/'test.cpp'; source.write_text(fixture)
        binary=out/'test'
        subprocess.run([*shlex.split(os.environ.get('CXX','c++')),'-std=c++20','-Wall','-Wextra',
                        '-Wno-unused-variable','-Wno-unused-parameter','-Wno-sign-compare',
                        '-I'+str(ROOT),str(source),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)
        owners={}
        for game in ('OOT','MM'):
            path='combo/menu/ComboItemDraw'+game+'.h'
            owners[game]=subprocess.check_output(['git','show',args.baseline_state+':'+path],cwd=ROOT,text=True) if args.baseline_state else (ROOT/path).read_text()
        enum=re.search(r'typedef enum \{\s*RI_UNKNOWN,.*?\} RandoItemId;',
                       (ROOT/'mm/2s2h/Rando/Types.h').read_text(),re.S)[0]
        dependencies='\n'.join(function(owners['OOT'],name) for name in ('OOT_IsStateDependentDraw','OOT_DrawDependency'))
        dependencies+='\n'+'\n'.join(function(owners['MM'],name) for name in ('MM_IsProgressiveItem','MM_IsStateDependentDraw','MM_IsSwordAppearanceDependent','MM_GetItemDrawInfo'))
        source.write_text((ROOT/'tests/bottle_gi/state_test.cpp').read_text().replace('/* MM_ITEM_ENUM */',enum).replace('/* PRODUCTION_DEPENDENCIES */',dependencies))
        subprocess.run([*shlex.split(os.environ.get('CXX','c++')),'-std=c++20','-Wall','-Wextra','-Werror',
                        '-I'+str(ROOT),str(source),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

if __name__=='__main__': main()
