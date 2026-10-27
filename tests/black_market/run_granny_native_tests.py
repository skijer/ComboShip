#!/usr/bin/env python3
from pathlib import Path
import os, subprocess, sys, tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
with tempfile.TemporaryDirectory(prefix='granny-native-') as temp:
    for host in ('mm','soh'):
        source=(ROOT/host/('2s2h' if host=='mm' else 'soh')/'Enhancements/Gameplay'/
                ('BlackMarket.cpp' if host=='mm' else 'ComboBlackMarketNative.cpp')).read_text()
        assert 'void ApplyGrannyInterior(' in source, f'{host}: missing optional Granny room attachment'
        bodies='\n'.join(block_from(source,source.index(s)) for s in (
            'void GrannySceneBegin(', 'void ApplyGrannyInterior(', 'void DrawGrannyRoom('))
        declarations=source[source.index('BlackMarketGranny::Resources sGrannyResources;'):source.index('void GrannySceneBegin(')]
        fixture=(ROOT/'tests/black_market/granny_native_test.cpp').read_text().replace('/* PRODUCTION */',declarations+bodies)
        cpp=Path(temp)/(host+'.cpp');cpp.write_text(fixture);binary=Path(temp)/host
        includes=[host,host+'/include',host+'/include/PR',host+'/src',host+'/assets',host+('/2s2h' if host=='mm' else '/soh'),'libultraship/include','combo']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2',
            *(['-DOOT_GRANNY_TEST'] if host=='soh' else []),'-g','-fsanitize=address,undefined',
            '-fno-sanitize-recover=all','-no-pie','-I'+str(ROOT),
            *['-I'+str(ROOT/p) for p in includes],str(cpp),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
