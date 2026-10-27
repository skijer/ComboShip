#!/usr/bin/env python3
from pathlib import Path
import os,subprocess,sys,tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
source=(ROOT/'combo/BlackMarketGrannyResources.h').read_text()
if 'inline Vertices NativeVaporVertices(' in source:
 adapter='namespace BlackMarketGranny {\n'+block_from(source,source.index('inline Vertices NativeVaporVertices('))+'\n}'
else:
 # Exact old preparation boundary: native resources were cast only to Vertex.
 adapter='namespace BlackMarketGranny { inline Vertices NativeVaporVertices(const std::shared_ptr<Ship::IResource>& resource) { auto v=std::dynamic_pointer_cast<Fast::Vertex>(resource); return v ? Vertices{v->VertexList.data(),v->VertexList.size(),v} : Vertices{}; } }'
with tempfile.TemporaryDirectory(prefix='granny-vapor-resource-') as directory:
 for host in ('mm','soh'):
  native='2s2h' if host=='mm' else 'soh';cpp=Path(directory)/(host+'.cpp');binary=Path(directory)/host
  cpp.write_text((ROOT/'tests/black_market/granny_vapor_resource_test.cpp').read_text().replace('/* PRODUCTION ADAPTER */',adapter))
  subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DFMT_HEADER_ONLY','-DF3DEX_GBI_2','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',
   '-I'+str(ROOT),'-I'+str(ROOT/host/native),'-I'+str(ROOT/'libultraship/include'),
   str(cpp),str(ROOT/host/native/'resource/type/Array.cpp'),str(ROOT/'libultraship/src/fast/resource/type/Vertex.cpp'),
   str(ROOT/'libultraship/src/ship/resource/Resource.cpp'),'-o',str(binary)],check=True)
  subprocess.run([str(binary)],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
