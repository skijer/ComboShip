#!/usr/bin/env python3
"""Exercise real safe writes, global settings, native slot copy/erase and foreign guards."""
from pathlib import Path
import argparse, os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--source-rev');args=parser.parse_args()
source=(subprocess.check_output(['git','show',args.source_rev+':soh/soh/SaveManager.cpp'],cwd=ROOT,text=True)
        if args.source_rev else (ROOT/'soh/soh/SaveManager.cpp').read_text())
def extract(marker):
 start=source.index(marker); b=source.index('{',start);n=1;i=b+1
 while n:n+=(source[i]=='{')-(source[i]=='}');i+=1
 return source[start:i]+'\n'
cpp=r'''
#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include "ComboGameMode.h"
#define SPDLOG_ERROR(...) ((void)0)
#define COMBO_EXPORT
extern "C" { int gComboGameMode=COMBO_GAME_OOT; }
namespace Ship { struct Context { static std::string GetPathRelativeToAppDirectory(const std::string& s){return s;} }; }
struct { int audioSetting=2,zTargetSetting=1,language=0; } gSaveContext;
constexpr int QUEST_NORMAL=0;
struct Meta { bool valid=false;int quest=0;bool requiresMasterQuest=false,requiresOriginal=false; };
struct SaveManager {
 static SaveManager* Instance;
 Meta fileMetaInfo[3];
 std::filesystem::path GetFileName(int);
 bool WriteFileSafely(const std::filesystem::path&,const std::string&);
 void SaveGlobal();void CopyZeldaFile(int,int);void DeleteZeldaFile(int);
};
SaveManager* SaveManager::Instance=nullptr;
struct GameInteractor {
 struct OnDeleteFile{}; static GameInteractor* Instance;
 template<class T>void ExecuteHooks(int){}
}; GameInteractor hooks;GameInteractor* GameInteractor::Instance=&hooks;
int foreignCopies=0,foreignDeletes=0;
void CountCopy(int,int){++foreignCopies;}
void (*gComboCopyContainer)(int,int)=CountCopy;
void CountDelete(int){++foreignDeletes;}
void (*gComboDeleteForeignSave)(int)=CountDelete;
'''
if 'static std::filesystem::path GetNativeSaveDirectory(' in source:
 cpp+=extract('static std::filesystem::path GetNativeSaveDirectory(')
for marker in ['bool SaveManager::WriteFileSafely(', 'std::filesystem::path SaveManager::GetFileName(',
 'void SaveManager::SaveGlobal(', 'void SaveManager::CopyZeldaFile(', 'void SaveManager::DeleteZeldaFile(',
 'extern "C" void Save_DeleteFile(']:cpp+=extract(marker)
cpp+=r'''
int main(){
 SaveManager manager;SaveManager::Instance=&manager;
 std::filesystem::create_directories("saves/oot");std::filesystem::create_directories("Save");
 std::ofstream("Save/file1.sav")<<"old-combo-fallback";
 std::ofstream("saves/oot/file1.sav")<<"native";
 std::ofstream("Save/global.sav")<<"old-global";
 manager.SaveGlobal();
 assert(std::filesystem::exists("saves/oot/global.sav") && "native global belongs under saves/oot");
 nlohmann::json global;std::ifstream("saves/oot/global.sav")>>global;assert(global["audioSetting"]==2);
 manager.fileMetaInfo[0].valid=true;manager.fileMetaInfo[0].quest=1;
 manager.CopyZeldaFile(0,1);
 assert(foreignCopies==0 && "solo copy never dispatches a stale container callback");
 std::string copied;std::ifstream("saves/oot/file2.sav")>>copied;assert(copied=="native");
 assert(manager.fileMetaInfo[1].valid && manager.fileMetaInfo[1].quest==1);
 Save_DeleteFile(0);assert(foreignDeletes==0);
 assert(!std::filesystem::exists("saves/oot/file1.sav") && std::filesystem::exists("Save/file1.sav"));
 std::string untouched;std::ifstream("Save/global.sav")>>untouched;assert(untouched=="old-global");
 gComboGameMode=COMBO_GAME_BOTH;
 manager.CopyZeldaFile(0,2);assert(foreignCopies==1);
 Save_DeleteFile(2);assert(foreignDeletes==1);
 assert(!std::filesystem::exists("saves/oot/file2.sav.temp"));
}
'''
with tempfile.TemporaryDirectory(prefix='oot-save-') as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(cpp);deps=Path(os.environ.get('COMBO_TEST_DEPS','/workspace/scratch/4288e889429f/build-deps'))
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DCOMBO_BUILD','-I'+str(ROOT/'combo'),'-I'+str(deps/'include'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],cwd=p,check=True)
print('PASS OoT native safe/global writes, slot copy/erase and combined callback preservation')
