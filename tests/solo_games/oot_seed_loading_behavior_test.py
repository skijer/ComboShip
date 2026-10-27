#!/usr/bin/env python3
"""Exercise actual native spoiler parsing and file-select generation dispatch."""
from pathlib import Path
import argparse,os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--source-rev');args=p.parse_args()
def read(path):return subprocess.check_output(['git','show',args.source_rev+':'+path],cwd=ROOT,text=True) if args.source_rev else (ROOT/path).read_text()
def extract(s,marker):
 start=s.index(marker);b=s.index('{',start);n=1;i=b+1
 while n:n+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[start:i]+'\n'
loader=extract(read('soh/soh/Enhancements/randomizer/SeedContext.cpp'),'void Context::ParseSpoiler(')
fc=read('soh/src/overlays/gamestates/ovl_file_choose/z_file_choose.c')
marker='else if (this->randomizerIndex == RSM_GENERATE_RANDOMIZER)'
dispatch=extract(fc,marker);dispatch=dispatch[dispatch.index('{'):]
cpp=r'''
#include <cassert>
#include <fstream>
#include <memory>
#include <nlohmann/json.hpp>
#include "ComboGameMode.h"
#define SPDLOG_ERROR(...) ((void)0)
extern "C" { int gComboGameMode=COMBO_GAME_OOT; }
namespace SohUtils{const char* Sanitize(const char* p){return p;}}
struct ParseTarget {void ParseJson(const nlohmann::json&){};};
namespace Rando {struct Settings: ParseTarget {static Settings* GetInstance(){static Settings s;return &s;}};}
struct Context {
 bool mSeedGenerated=true,mSpoilerLoaded=false;
 std::shared_ptr<ParseTarget> mEntranceShuffler=std::make_shared<ParseTarget>(),mDungeons=mEntranceShuffler,mTrials=mEntranceShuffler;
 int parsed=0;
 void ParseHashIconIndexesJson(const nlohmann::json&){++parsed;}
 void ParseItemLocationsJson(const nlohmann::json&){} void ParseTricksJson(const nlohmann::json&){} void ParseHintJson(const nlohmann::json&){}
 void ParseSpoiler(const char*);
};
int nativeGenerations=0,comboGenerations=0;
void Randomizer_GenerateRandomizer(){++nativeGenerations;}
void SOH_TriggerComboGenerate(){++comboGenerations;}
'''+loader+'void GeneratePressed()'+dispatch+r'''
int main(){
 std::ofstream("native.json")<<R"({"version":"x","finalSeed":123})";
 std::ofstream("combo.json")<<R"({"fileType":"ComboShipRandomizer","version":"x","finalSeed":123})";
 Context ctx;ctx.ParseSpoiler("combo.json");
 assert(!ctx.mSpoilerLoaded && ctx.parsed==0 && "a remembered Combo file must never enter native parsing");
 ctx.ParseSpoiler("native.json");assert(ctx.mSpoilerLoaded && !ctx.mSeedGenerated && ctx.parsed==1);
 GeneratePressed();assert(nativeGenerations==1 && comboGenerations==0);
 gComboGameMode=COMBO_GAME_BOTH;GeneratePressed();assert(nativeGenerations==1 && comboGenerations==1);
}
'''
with tempfile.TemporaryDirectory(prefix='oot-seed-loading-') as tmp:
 path=Path(tmp);(path/'test.cpp').write_text(cpp);deps=Path(os.environ.get('COMBO_TEST_DEPS','/workspace/scratch/4288e889429f/build-deps'))
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DCOMBO_BUILD','-I'+str(ROOT/'combo'),'-I'+str(deps/'include'),str(path/'test.cpp'),'-o',str(path/'test')],check=True)
 subprocess.run([str(path/'test')],cwd=path,check=True)
print('PASS native seed loading isolation and generation dispatch')
