#!/usr/bin/env python3
"""Portable ascent/snow/material behavior and actual texture factory/SETTIMG."""
from pathlib import Path
import os,re,subprocess,sys,tempfile,zipfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tests/mm_presentation'))
from run_foreign_scale_tests import function
ENV=dict(os.environ)
if ENV.get('COMBO_NEI_FIXTURE_CPATH'):ENV['CPATH']=ENV['COMBO_NEI_FIXTURE_CPATH']+os.pathsep+ENV.get('CPATH','')
with tempfile.TemporaryDirectory(prefix='regional-materials-')as td:
 td=Path(td)
 flags=['-std=c++20','-g','-DFMT_HEADER_ONLY','-I'+str(ROOT/'combo'),'-I'+str(ROOT/'libultraship/include'),'-I'+str(td),'-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie']
 for name in ('materials','ascent','snowfall'):
  binary=td/name
  subprocess.run([os.environ.get('CXX','c++'),*flags,'-Wall','-Wextra','-Werror',str(ROOT/'tests/regional_passages'/f'{name}_test.cpp'),'-o',str(binary)],check=True,env=ENV)
  subprocess.run([str(binary)],check=True,env={**ENV,'ASAN_OPTIONS':'detect_leaks=0'})
 subprocess.run([sys.executable,str(ROOT/'tests/regional_passages/material_pack_test.py')],check=True,env=ENV)
 pack=td/'pack.o2r';subprocess.run([sys.executable,str(ROOT/'tools/regional_materials/build_pack.py'),'--debug-swatches','--output',str(pack)],check=True)
 with zipfile.ZipFile(pack)as z:z.extractall(td/'unpacked')
 renderer=(ROOT/'libultraship/src/fast/interpreter.cpp').read_text();manager=(ROOT/'libultraship/src/ship/resource/ResourceManager.cpp').read_text();header=(ROOT/'libultraship/include/fast/interpreter.h').read_text()
 (td/'texture_signature.inc').write_text(function(manager,'ResourceManager::OtrSignatureCheck'))
 (td/'texture_metadata.inc').write_text(re.search(r'struct RawTexMetadata \{.*?\n\};',header,re.S).group(0))
 names=['Interpreter::SegAddr','IsValidResolvedAddress','gfx_check_image_signature','ComboLoadTextureResource','ReportTextureLoadFailure','gfx_set_timg_handler_rdp','gfx_set_timg_otr_filepath_handler_custom']
 (td/'foreign_texture_production.inc').write_text(''.join(function(renderer,n)for n in names))
 sources=['tests/regional_passages/material_native_test.cpp','libultraship/src/fast/resource/factory/TextureFactory.cpp','libultraship/src/fast/resource/type/Texture.cpp','libultraship/src/ship/resource/ResourceFactoryBinary.cpp','libultraship/src/ship/resource/Resource.cpp','libultraship/src/ship/utils/binarytools/BinaryReader.cpp','libultraship/src/ship/utils/binarytools/MemoryStream.cpp','libultraship/src/ship/utils/binarytools/Stream.cpp']
 binary=td/'native'
 subprocess.run([os.environ.get('CXX','c++'),*flags,*[str(ROOT/p)for p in sources],'-ldl','-o',str(binary)],check=True,env=ENV)
 subprocess.run([str(binary),str(td/'unpacked')],check=True,env={**ENV,'ASAN_OPTIONS':'detect_leaks=0'})
print('PASS regional material, right ascent, grounded trigger/return and actual snowfall suites')
