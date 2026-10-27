"""Execute actual active normalizer/snapshot/restore/save-policy bodies with native MM option IDs."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/diagnostics'))
from run_time_pedestal_tests import block_from
extract=lambda s,q:block_from(s,s.index(q))
oot=(ROOT/'soh/soh/OTRGlobals.cpp').read_text()
mm=(ROOT/'mm/2s2h/BenPort.cpp').read_text()
settings=(ROOT/'soh/soh/Enhancements/randomizer/settings.cpp').read_text()
bodies='\n'.join(extract(s,q) for s,q in [
 (oot,'extern "C" COMBO_EXPORT void SOH_NormalizeComboGraceFromMM('),
 (oot,'extern "C" COMBO_EXPORT const char* SOH_DumpRandoSettings('),
 (oot,'extern "C" COMBO_EXPORT void SOH_RestoreRandoSettings('),
 (mm,'extern "C" COMBO_EXPORT const char* MM_DumpRandoSettings('),
 (mm,'extern "C" COMBO_EXPORT void MM_RestoreRandoSettings(')])
force_oot=extract(settings[settings.index('void Settings::SetAllToContext()'):], '    if (Combo_IsCombinedGame() && CVarGetInteger("gRando.Generation.ShuffleMagicMirror", 0))')
force_mm=extract(mm, '        if (Combo_IsCombinedGame() && CVarGetInteger("gRando.Generation.ShuffleMagicMirror", 0))')
# These are single-statement ifs: block_from would include the following unrelated braced section.
force_oot=force_oot.split(';',1)[0]+';'
force_mm=force_mm.split(';',1)[0]+';'
fixture=r'''
#include <nlohmann/json.hpp>
#include "global.h"
#include "2s2h/Rando/Rando.h"
#include "rando/MagicMirrorShuffle.h"
#include "NeiGracePolicy.h"
#include "NeiSeasonsPolicy.h"
#include "tests/test_require.h"
#include <map>
#include <cstring>
#define COMBO_EXPORT
#define CVAR_RANDOMIZER_SETTING(name) "gRandoSettings." name
#define RSK_SHUFFLE_MAGIC_MIRROR 1
extern "C" { int gComboGameMode=COMBO_GAME_BOTH; }
static std::map<std::string,int> ints;
static std::map<std::string,std::string> strings;
extern "C" int32_t CVarGetInteger(const char* key,int32_t fallback){auto it=ints.find(key);return it==ints.end()?fallback:it->second;}
extern "C" void CVarSetInteger(const char* key,int32_t value){ints[key]=value;}
extern "C" const char* CVarGetString(const char* key,const char* fallback){auto it=strings.find(key);return it==strings.end()?fallback:it->second.c_str();}
extern "C" void CVarSetString(const char* key,const char* value){strings[key]=value;}
extern "C" void CVarClear(const char* key){ints.erase(key);strings.erase(key);}
static bool Combo_ClearNumericExclusions(){return false;}
namespace Rando {
struct FakeOption {std::string cvar; const std::string& GetCVarName()const{return cvar;} int GetOptionIndex()const{return CVarGetInteger(cvar.c_str(),0);}};
struct Settings {static Settings* GetInstance(){static Settings s;return &s;} std::vector<FakeOption> GetAllOptions(){return {{"gRandoSettings.ShuffleMagicMirror"}};}};
namespace StaticData {
std::map<RandoOptionId,RandoStaticOption> Options={{RO_SHUFFLE_MAGIC_MIRROR,{RO_SHUFFLE_MAGIC_MIRROR,"RO_SHUFFLE_MAGIC_MIRROR","gRando.Options.RO_SHUFFLE_MAGIC_MIRROR",0}}};
std::map<RandoCheckId,RandoStaticCheck> Checks;
std::map<RandoItemId,RandoStaticItem> Items;
RandoCheckId GetCheckIdFromName(const char*){return RC_UNKNOWN;}
RandoItemId GetItemIdFromName(const char*){return RI_UNKNOWN;}
}
std::vector<RandoCheckId> GetExcludedChecksFromConfig(){return {};}
void SetExcludedChecksInConfig(std::vector<RandoCheckId>&){}
std::vector<RandoItemId> GetStartingItemsFromConfig(){return {};}
void SetStartingItemsInConfig(std::vector<RandoItemId>&){}
}
/* BODIES */
struct Value {int value=0;void Set(int v){value=v;}};
struct Ctx {Value option;Value& GetOption(int){return option;}} context;
static Ctx* mContext=&context;
int EffectiveOot(){context.option.value=CVarGetInteger("gRandoSettings.ShuffleMagicMirror",0);/* OOT */return context.option.value;}
int EffectiveMm(){nlohmann::json options={{"RO_SHUFFLE_MAGIC_MIRROR",CVarGetInteger("gRando.Options.RO_SHUFFLE_MAGIC_MIRROR",0)}};/* MM */return options["RO_SHUFFLE_MAGIC_MIRROR"].get<int>();}
int main(){
 for(bool oot:{false,true})for(bool mm:{false,true}) {
  ints.clear();strings.clear();
  CVarSetInteger("gRandoSettings.ShuffleMagicMirror",oot);CVarSetInteger("gRando.Options.RO_SHUFFLE_MAGIC_MIRROR",mm);
  CVarSetInteger("gMods.CustomItems.Enabled",0);CVarSetInteger("gRando.Options.RO_SHUFFLE_NEI_ITEMS",0);
  SOH_NormalizeComboGraceFromMM();
  REQUIRE(CVarGetInteger("gRando.Generation.ShuffleMagicMirror",0)==(oot||mm));
  REQUIRE(CVarGetInteger("gRandoSettings.ShuffleMagicMirror",0)==oot);
  REQUIRE(CVarGetInteger("gRando.Options.RO_SHUFFLE_MAGIC_MIRROR",0)==mm);
  REQUIRE(EffectiveOot()==(oot||mm)&&EffectiveMm()==(oot||mm));
  const auto os=std::string(SOH_DumpRandoSettings()),ms=std::string(MM_DumpRandoSettings());
  ints["gRando.Generation.ShuffleMagicMirror"]=0;
  SOH_RestoreRandoSettings(os.c_str());MM_RestoreRandoSettings(ms.c_str());
  REQUIRE(EffectiveOot()==(oot||mm)&&EffectiveMm()==(oot||mm));
  REQUIRE(CVarGetInteger("gRandoSettings.ShuffleMagicMirror",0)==oot);
  REQUIRE(CVarGetInteger("gRando.Options.RO_SHUFFLE_MAGIC_MIRROR",0)==mm);
  REQUIRE(!CVarGetInteger("gMods.CustomItems.Enabled",0)&&!CVarGetInteger("gRando.Options.RO_SHUFFLE_NEI_ITEMS",0));
 }
 SOH_RestoreRandoSettings("{}");MM_RestoreRandoSettings("{}");
 REQUIRE(!EffectiveOot()&&!EffectiveMm()&&!CVarGetInteger("gRando.Generation.ShuffleMagicMirror",0));
 CVarSetInteger("gRando.Generation.ShuffleMagicMirror",1);gComboGameMode=COMBO_GAME_MM;
 REQUIRE(!EffectiveMm()&&!EffectiveOot());
 puts("PASS active normalizer + native seed flag policies + real snapshot/restore: OR requests, unchanged UI/NEI, both serialized, new-to-old OFF, solo ignores combined override");
}
'''.replace('/* BODIES */',bodies).replace('/* OOT */',force_oot).replace('/* MM */',force_mm)
# Verify the real launcher calls the normalizer before each native dump sequence.
launcher=(ROOT/'combo/ComboShip.cpp').read_text()
assert launcher.index('SOH_NormalizeComboGraceFromMM();',launcher.index('const bool haveOracles')) < launcher.index('SOH_DumpRandoStaticData()',launcher.index('const bool haveOracles'))
assert 'RegionTable(RR_THE_LOST_WOODS)->HasAccess()' in extract(oot,'extern "C" COMBO_EXPORT const char* Combo_SOH_Rando_GetReachableChecks(')
assert 'mContext->GetOption(RSK_SHUFFLE_MAGIC_MIRROR).Set(RO_GENERIC_OFF)' in extract(settings,'void Settings::ParseJson(')
with tempfile.TemporaryDirectory(prefix='mirror-active-options-') as td:
 temp=Path(td);(temp/'test.cpp').write_text(fixture)
 flags=['-std=c++20','-w','-DF3DEX_GBI_2','-DCOMBO_BUILD','-DCONTROLLERBUTTONS_T=uint32_t']
 for p in ['', 'mm','mm/2s2h','mm/include','mm/include/PR','mm/src','mm/assets','libultraship/include','combo']:
  flags+=['-I'+str(ROOT/p)]
 result=subprocess.run([os.environ.get('CXX','c++'),*flags,str(temp/'test.cpp'),'-o',str(temp/'test')],capture_output=True,text=True)
 if result.returncode:raise RuntimeError(result.stdout+result.stderr)
 subprocess.run([str(temp/'test')],check=True)
