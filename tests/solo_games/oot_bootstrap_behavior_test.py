#!/usr/bin/env python3
"""Run the real OoT mode setter and finish-init exports around the ROM-free chooser."""
from pathlib import Path
import os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'soh/soh/OTRGlobals.cpp').read_text()
def function(marker):
    start=source.index(marker); b=source.index('{',start);n=1;i=b+1
    while n:n+=(source[i]=='{')-(source[i]=='}');i+=1
    return source[start:i]
cpp=r'''
#include <memory>
#include <cassert>
#include "ComboGameMode.h"
#define COMBO_EXPORT
extern "C" { int gComboGameMode=COMBO_GAME_BOTH; }
static bool sComboGameModeLocked=false;
struct Gui { int menu=0; void SetMenu(int m){ menu=m; } };
int contextDestroys=0,guiDestroys=0,unregisters=0;
struct Context { Gui gui; Context* GetWindow(){ return this; } Gui* GetGui(){ return &gui; } static void DestroyInstance(){++contextDestroys;} };
namespace Ship { using Context=::Context; namespace CrossRMRegistry { void Unregister(const char*){++unregisters;} } }
namespace ImGui { void* current=(void*)1;void SetCurrentContext(void* p){current=p;} }
std::shared_ptr<int> sohFast3dWindow=std::make_shared<int>(),sOOTResourceManager=std::make_shared<int>();
struct OTRGlobals { static OTRGlobals* Instance; std::shared_ptr<Context> context=std::make_shared<Context>(); };
OTRGlobals* OTRGlobals::Instance=nullptr;
namespace SohGui { int GetSohMenu(){ return 42; } void Destroy(){++guiDestroys;} }
int finishes=0;
static void Combo_FinishInit(){ ++finishes; }
'''+function('extern "C" COMBO_EXPORT void SOH_SetGameMode(')+'\n'+function('extern "C" COMBO_EXPORT void SOH_FinishInit(')+'\n'+function('extern "C" COMBO_EXPORT void SOH_DeinitWindowOnly(')+r'''
int main(){
 SOH_SetGameMode(COMBO_GAME_BOTH); auto* bootstrap=new OTRGlobals; OTRGlobals::Instance=bootstrap;
 SOH_SetGameMode(COMBO_GAME_OOT);
 assert(gComboGameMode==COMBO_GAME_OOT && "chooser may select solo after ROM-free window bootstrap");
 SOH_FinishInit(); assert(finishes==1 && bootstrap->context->gui.menu==42);
 SOH_SetGameMode(COMBO_GAME_BOTH);
 assert(gComboGameMode==COMBO_GAME_OOT && "session mode is immutable after game initialization");
 SOH_DeinitWindowOnly();
 assert(OTRGlobals::Instance==nullptr && !sohFast3dWindow && !sOOTResourceManager);
 assert(guiDestroys==1 && contextDestroys==1 && unregisters==2 && ImGui::current==nullptr);
 SOH_DeinitWindowOnly();assert(contextDestroys==1);
}
'''
with tempfile.TemporaryDirectory(prefix='oot-bootstrap-') as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(cpp)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DCOMBO_BUILD','-I'+str(ROOT/'combo'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS OoT chooser mode, native menu activation and session lock')
