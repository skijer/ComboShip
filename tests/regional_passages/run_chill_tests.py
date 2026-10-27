#!/usr/bin/env python3
from pathlib import Path
import os,subprocess,sys,tempfile,argparse,struct,zipfile
ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--native-archive',type=Path);args=parser.parse_args()
if args.native_archive:
 with zipfile.ZipFile(args.native_archive)as archive:
  data=archive.read('objects/gameplay_keep/gDust5Tex')
  assert struct.unpack_from('<III',data,64)==(6,32,32)
  for name,ids in (
   ('spot08_room_0',[0x74,0x40,0x12f,0x12c,0xa4,0x170,0x7c,0x188,0x163,0x24]),
   ('spot08_room_0Set_000330',[0x74,0x12f,7,0x16,0x11f,0x188,0x24,0x154,0x12c,0x163]),
   ('spot08_room_0Set_000700',[0x74,0x40,0x13,0x12f,0x12c,0xa4,0x170,0x7c,0x188,0x24,0x163])):
   data=archive.read('scenes/shared/spot08_scene/'+name)
   command=struct.pack('<II',11,len(ids))+struct.pack('<'+'H'*len(ids),*ids)
   assert data.find(command,64)>=64 and 0x114 not in ids
 print('PASS original archive: native Fountain object-list command identities, absent FZ and gameplay dust5 I8 32x32')
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
ENV=dict(os.environ)
if ENV.get('COMBO_NEI_FIXTURE_CPATH'):ENV['CPATH']=ENV['COMBO_NEI_FIXTURE_CPATH']+os.pathsep+ENV.get('CPATH','')
source=(ROOT/'soh/soh/Enhancements/Gameplay/ComboFountainSnowheadBridge.cpp').read_text()
production=block_from(source,source.index('bool Fountain('))
production+='\n'+block_from(source,source.index('float ParticleViewOpacity('))
production+='\nvoid DrawLocalChill(Actor*,PlayState*);\n'+block_from(source,source.index('void Draw('))
production+='\n'+block_from(source,source.rindex('void DrawLocalChill('))
rcp=(ROOT/'soh/src/code/z_rcp.c').read_text()
setup=block_from(rcp,rcp.index('Gfx sSetupDL['))+';\n'
setup+='\n'.join(block_from(rcp,rcp.index(name)) for name in ('void Gfx_SetupDL_25Opa(', 'void Gfx_SetupDL_25Xlu('))
wrappers=(ROOT/'soh/soh/GbiWrap.cpp').read_text()
setup+='\n'+block_from(wrappers,wrappers.index('extern "C" void gSPDisplayList('))
interpreter=(ROOT/'libultraship/src/fast/interpreter.cpp').read_text()
header=(ROOT/'libultraship/include/fast/interpreter.h').read_text()
combiner='\n'.join(block_from(header,header.index(name))+';' for name in (
 'enum {\n    SHADER_0', 'enum class ShaderOpts', 'struct ColorCombinerKey', 'struct ColorCombiner {', 'struct RGBA {'))
combiner+='\n#define SHADER_OPT(opt) ((uint64_t)(1 << static_cast<int>(ShaderOpts::opt)))\n'
live=interpreter[interpreter.index('void Interpreter::GfxDpSetCombineMode('):]
shader='\n'.join(block_from(live,live.index(name)) for name in (
 'static inline uint32_t color_comb(', 'static inline uint32_t alpha_comb('))
shader+='\n'+'\n'.join(block_from(interpreter,interpreter.index(name)) for name in (
 'void Interpreter::GenerateCC(', 'void Interpreter::GfxDpSetCombineMode(', 'bool gfx_set_combine_handler_rdp('))
shader=shader.replace('Interpreter','MaskShader')
packed_start=interpreter.index('for (int j = 0; j < numInputs; j++)')
packed=block_from(interpreter,packed_start)
ice=(ROOT/'soh/src/overlays/effects/ovl_Effect_Ss_Ice_Smoke/z_eff_ss_ice_smoke.c').read_text()
scene=(ROOT/'soh/src/code/z_scene.c').read_text();lib=(ROOT/'soh/src/code/z_lib.c').read_text()
def definition(src,head):
 start=src.index(head)
 while ';'in src[start:src.index('{',start)]:start=src.index(head,start+len(head))
 return block_from(src,start)
native='''#include "global.h"
#include "overlays/effects/ovl_Effect_Ss_Ice_Smoke/z_eff_ss_ice_smoke.h"
#undef osSyncPrintf
#define osSyncPrintf(...) ((void)0)
#define rObjBankIdx regs[0]
#define rAlpha regs[1]
#define rScale regs[2]
void EffectSsIceSmoke_Draw(PlayState*p,u32 i,EffectSs*s){}
void EffectSsIceSmoke_Update(PlayState*,u32,EffectSs*);
'''
for src,head in [(scene,'s32 Object_GetIndex('),(scene,'s32 Object_IsLoaded('),(lib,'void Math_Vec3f_Copy('),(ice,'u32 EffectSsIceSmoke_Init('),(ice,'void EffectSsIceSmoke_Update(')]:native+='\n'+definition(src,head)
paths=['combo','soh','soh/soh','soh/include','soh/include/PR','soh/src','soh/assets','libultraship/include']
flags=['-w','-DCOMBO_BUILD','-DF3DEX_GBI_2','-DFMT_HEADER_ONLY','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',*['-I'+str(ROOT/p)for p in paths]]
with tempfile.TemporaryDirectory(prefix='snowhead-chill-')as td:
 td=Path(td);c=td/'native.c';c.write_text(native);o=td/'native.o'
 subprocess.run([os.environ.get('CC','cc'),'-std=gnu11',*flags,'-c',str(c),'-o',str(o)],check=True,env=ENV)
 fixture=(ROOT/'tests/regional_passages/chill_native_test.cpp').read_text()
 for marker,value in [('NATIVE_SETUP',setup),('PRODUCTION',production),('NATIVE_COMBINER',combiner),('NATIVE_SHADER',shader),('NATIVE_PACKED_INPUTS',packed)]:fixture=fixture.replace('/* '+marker+' */',value)
 cpp=td/'test.cpp';cpp.write_text(fixture);binary=td/'test'
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20',*flags,str(cpp),str(o),'-o',str(binary)],check=True,env=ENV)
 subprocess.run([str(binary)],check=True,env={**ENV,'ASAN_OPTIONS':'detect_leaks=0'})
