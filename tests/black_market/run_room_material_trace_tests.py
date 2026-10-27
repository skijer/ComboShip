#!/usr/bin/env python3
"""Exercise actual MM material tracing and resource decisions with native host types."""
from pathlib import Path
import os, subprocess, sys
ROOT=Path(__file__).resolve().parents[2]
import tempfile
_temp=tempfile.TemporaryDirectory(prefix="material-trace-")
OUT=Path(_temp.name)
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
native=(ROOT/'tests/black_market/room_magic_native_test.cpp').read_text().split('int main() {')[0]
resources=(ROOT/'combo/BlackMarketRoomMagicResources.h').read_text()
actual=block_from(resources,resources.index('class Resources {'))+';'
native=native.replace(block_from(native,native.index('struct Resources {'))+';',actual)
source=(ROOT/'mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp').read_text()
trace=(block_from(source,source.index('void TraceRoomMagic(const char* result, RoomShape* source, bool prepared) {')) if 'void TraceRoomMagic(const char* result, RoomShape* source, bool prepared) {' in source else 'void TraceRoomMagic(const char*, RoomShape*, bool) {}')
start=source.index('BlackMarketRoomMagic::Resources sRoomMagicResources;')
end=source.index('void RestoreRoomMagic(',start)
production=source[start:end]+'\n'.join(block_from(source,source.index(sig)) for sig in ('void RestoreRoomMagic(','void ClearRoomMagic(','void BeginRoomMagic('))
stub=block_from(native,native.index('void TraceRoomMagic('))
native=native.replace(stub, '', 1)
native=native.replace('/* PALETTE */',block_from(resources,resources.index('inline Palette ReadPalette('))).replace('/* PRODUCTION */',production)
boundary=(ROOT/'tests/black_market/room_magic_resources_test.cpp').read_text().split('namespace BlackMarketRoomMagic {\n/* RESOURCES */')[0]
graph=(ROOT/'tests/black_market/room_magic_test.cpp').read_text().split('int main() {')[0]
boundary=boundary.replace('#define main RoomGraphFixtureMain\n#include \"room_magic_test.cpp\"\n#undef main',graph)
boundary=boundary.replace('bool alt = false;', 'bool alt = false, throwLoad = false;').replace('requested.push_back(path);', 'if (throwLoad) throw std::runtime_error("controlled RM failure"); requested.push_back(path);')
instrument=r'''
#include "mods/nei_save.h"
#include "combo/BlackMarketGranny.h"
static RoomShape sGrannyShape{};
static NeiSaveData nei;
extern "C" NeiSaveData* Nei_Save() { return &nei; }
SaveContext gSaveContext;
namespace Ship {
struct Context {
 static Context* GetRawInstance() { static Context c; return &c; }
 auto GetResourceManager() { return CrossRMRegistry::active; }
};
}
static std::vector<std::pair<std::string,std::string>> events;
template<class... Args> void CaptureLog(const char*, const char* result, const char* prepare, Args...) { events.emplace_back(result,prepare); }
#define SPDLOG_INFO(...) CaptureLog(__VA_ARGS__)
void TraceRoomMagic(const char*, RoomShape* = nullptr, bool = false);
'''
main=r'''

int main() {
 PlayState play{}; gPlayState=&play;
 const std::string path=std::string("__OTR__")+BlackMarketRoomMagic::SourceRoom(false);
 Entry entry{reinterpret_cast<Gfx*>(const_cast<char*>(path.c_str())),nullptr};
 Shape shape{}; SetShape(shape,entry); ShapePointer(play)=&shape;
 auto own=std::make_shared<Ship::ResourceManager>();
 Ship::CrossRMRegistry::active=own; Ship::CrossRMRegistry::owners["mm"]=own;
 integers[BlackMarketRoomMagic::kCvar]=1;
 const auto draw=[&] { BeginRoomMagic(); RestoreRoomMagic(); };
 const auto expected=[&](const char* result,const char* prepare) { assert(!events.empty()); if(events.back()!=std::make_pair(std::string(result),std::string(prepare))) std::fprintf(stderr,"expected %s/%s got %s/%s\n",result,prepare,events.back().first.c_str(),events.back().second.c_str()); assert(events.back()==std::make_pair(std::string(result),std::string(prepare))); };
 draw(); expected("attached","ready");
 // A failed optional Granny request must not gate the built-in magical room.
 integers[BlackMarketGranny::kCvar]=1; draw(); expected("attached","ready");
 integers.erase(BlackMarketGranny::kCvar);
 draw(); expected("attached","ready");
 auto count=events.size(); for(int i=0;i<500;++i) draw(); assert(events.size()==count);
 nei.timeGateAdultMode=1; draw(); assert(events.size()==++count); draw(); assert(events.size()==count);
 integers["gAdultLink.ForceOn"]=1; draw(); assert(events.size()==++count);
 own->alt=true; draw(); assert(events.size()==++count); expected("attached","ready");
 own->archive->paths.insert(std::string("alt/")+SourceRoom(false));
 draw(); expected("resource-unavailable","root-not-display-list");
 count=events.size(); for(int i=0;i<500;++i) draw(); assert(events.size()==count);
 own->alt=false; draw(); expected("attached","ready");
 // A scene may name its Alt root explicitly even when global Alt is off.
 // Its exact mesh still owns the draw; do not substitute the base-room mesh.
 auto explicitRoot=std::make_shared<Fast::DisplayList>();
 explicitRoot->Instructions=std::dynamic_pointer_cast<Fast::DisplayList>(own->resources.at(kRoom))->Instructions;
 Gfx explicitTriangle={gsSP1Triangle(0,1,2,0)};
 explicitRoot->Instructions.insert(explicitRoot->Instructions.end()-1,explicitTriangle);
 const std::string explicitName=std::string("alt/")+SourceRoom(false);
 own->resources[explicitName]=explicitRoot;
 const std::string explicitPath=std::string("__OTR__")+explicitName;
 entry.opa=reinterpret_cast<Gfx*>(const_cast<char*>(explicitPath.c_str()));
 BeginRoomMagic(); expected("attached","ready");
 assert(std::memcmp(&sRoomMagicResources.Opaque()[30],&explicitTriangle,sizeof(Gfx))==0);
 RestoreRoomMagic();
 assert(ShapePointer(play)==&shape && entry.opa==reinterpret_cast<const Gfx*>(explicitPath.c_str()));
 entry.opa=reinterpret_cast<Gfx*>(const_cast<char*>(path.c_str()));
 own->resources.erase(explicitName); own->archive->paths.erase(explicitName);
 draw(); expected("attached","ready");
 const auto requests=own->requested.size();
 integers[BlackMarketRoomMagic::kCvar]=0; draw(); expected("disabled","skipped");
 assert(own->requested.size()==requests);
 integers[BlackMarketRoomMagic::kCvar]=1; sGrannyActive=true; draw(); expected("granny","skipped");
 Gfx wrappers[2]{}; Entry grannyEntry{wrappers,nullptr}; SetShape(sGrannyShape,grannyEntry);
 ShapePointer(play)=&sGrannyShape; draw(); count=events.size();
 for(int i=0;i<500;++i) { grannyEntry.opa=wrappers+(i%2); draw(); } assert(events.size()==count);
 assert(own->requested.size()==requests);
 ShapePointer(play)=&shape; sGrannyActive=false; draw(); expected("attached","ready");
 play.roomCtx.curRoom.num=1; draw(); expected("room","skipped"); play.roomCtx.curRoom.num=0;
 draw(); expected("attached","ready");
 own->throwLoad=true; draw(); expected("resource-unavailable","exception"); own->throwLoad=false;
 draw(); expected("attached","ready");
 auto root=own->resources.at(kRoom); own->resources.erase(kRoom); draw(); expected("resource-unavailable","root-not-display-list");
 own->archive->paths.erase(kRoom); draw(); expected("resource-unavailable","graph-absent");
 own->archive->paths.insert(kRoom); own->resources[kRoom]=root; draw(); expected("attached","ready");
 auto foreign=std::make_shared<Ship::ResourceManager>(); Ship::CrossRMRegistry::active=foreign;
 count=events.size(); draw(); assert(events.size()==count+1); expected("attached","ready"); assert(foreign->requested.empty());
 Ship::CrossRMRegistry::owners.clear(); draw(); expected("resource-unavailable","no-owner");
 Ship::CrossRMRegistry::owners["mm"]=own; draw(); expected("attached","ready");
 auto replacement=std::make_shared<Fast::DisplayList>(); replacement->Instructions=std::dynamic_pointer_cast<Fast::DisplayList>(root)->Instructions;
 own->resources[kRoom]=replacement; count=events.size(); draw(); assert(events.size()==count+1); expected("attached","ready");
 auto malformed=std::make_shared<Fast::DisplayList>(); malformed->Instructions=replacement->Instructions;
 malformed->Instructions[0].words.w0=uint32_t(G_DL)<<24; own->resources[kRoom]=malformed;
 draw(); expected("resource-unavailable","graph-invalid"); own->resources[kRoom]=replacement;
 draw(); expected("attached","ready");
 Gfx raw[]={gsSPEndDisplayList()}; entry.opa=raw; draw(); expected("source-mismatch","skipped");
 entry.opa=reinterpret_cast<Gfx*>(const_cast<char*>(path.c_str())); shape.base.type=1; draw(); expected("shape-layout","skipped");
 shape.base.type=0; ShapePointer(play)=nullptr; draw(); expected("shape-null","skipped"); ShapePointer(play)=&shape;
 draw(); expected("attached","ready"); count=events.size(); ClearRoomMagic(); draw(); assert(events.size()==count+1);
 count=events.size(); interior=false; for(int i=0;i<500;++i) draw(); assert(events.size()==count);
 gPlayState=nullptr; ClearRoomMagic();
 puts("PASS: production MM attachment/Prepare reason boundaries; stable 500-draw dedupe; age/ForceOn/Alt/owner/root/source changes; scene reset; no exterior noise");
}
'''
cpp=OUT/'production_material_probe.cpp';cpp.write_text('#include \"global.h\"\n'+boundary+'\n'+instrument+'\n'+native+'\n'+trace+'\n'+main)
includes=['mm','mm/include','mm/include/PR','mm/src','mm/assets','mm/2s2h','libultraship/include','combo','tests/black_market']
cmd=[os.environ.get('CXX','c++'),'-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie','-I'+str(ROOT),*['-I'+str(ROOT/x) for x in includes],str(cpp),'-o',str(OUT/'production_material_probe')]

subprocess.run(cmd,check=True)
subprocess.run([str(OUT/'production_material_probe')],check=True,env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
