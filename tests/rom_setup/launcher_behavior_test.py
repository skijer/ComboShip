#!/usr/bin/env python3
"""Run the launcher's real archive/setup boundary in controlled install directories."""
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--sanitize',action='store_true');args=parser.parse_args()
launcher=(ROOT/'combo/ComboShip.cpp').read_text()
checks=launcher[launcher.index('static bool OOTArchivesExist()'):launcher.index('// ComboShip (issue 24):')]
a=launcher.index('    bool windowInitialized = selectionWindowInitialized;',launcher.index('// --- 2/3.'))
b=launcher.index('    // --- 3b.',a)
block=launcher[a:b]
args_start=launcher.index('    bool romSetupRequested = false;',launcher.index('int main(int argc, char** argv)'))
args_block=launcher[args_start:launcher.index('\n#ifdef _WIN32',args_start)]
source=r'''
#include <cassert>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "combo/ComboExtract.h"
#include "combo/ComboLaunchOptions.h"
using DllHandle=void*;
static int screenCalls=0;
static int deinitCalls=0;
static bool cancelExtraction=false;
static ComboExtractCallbacks seen{};
static bool romSetupRequested=true;
static bool selectionWindowInitialized=false;
static ComboGameMode gameMode=COMBO_GAME_BOTH;
void Init(){}
void DeinitWindow(){++deinitCalls;}
static auto deinitWindowOnly=DeinitWindow;
int Validate(const char*){return 1;}
int Kind(const char*){return COMBO_OOT_ROM_MQ;}
void Progress(unsigned long long*,unsigned long long*,int*,int*){}
static auto SOH_InitWindowOnly=Init,SOH_FinishInit=Init;
static auto SOH_ValidateRom=Validate,SOH_ClassifyRom=Validate,SOH_ClassifyRomKind=Kind,SOH_StartExtraction=Validate;
static auto MM_ValidateRom=Validate,MM_ClassifyRom=Validate,MM_StartExtraction=Validate;
static auto SOH_GetExtractionProgress=Progress,MM_GetExtractionProgress=Progress;
static DllHandle sohModule=(void*)1,mmModule=(void*)2,comboUIModule=(void*)3;
static const char* comboUiDll="comboui.so";
void FreeDll(DllHandle){}
DllHandle LoadDll(const char*){return (void*)3;}
bool ComboConfigExists(){return true;}
int RunExtraction(const ComboExtractCallbacks* cb){
    ++screenCalls;seen=*cb;
    if(cancelExtraction)return 0;
    if(cb->sohNeeded) std::ofstream("oot.o2r").put('v');
    if(cb->mmNeeded) std::ofstream("mm.o2r").put('m');
    return 1;
}
void* GetSym(DllHandle,const char*){return reinterpret_cast<void*>(&RunExtraction);}
static ComboFnRunExtraction ComboUI_RunExtraction=nullptr;
'''+checks+'\nbool ParseArgs(int& argc,char** argv){\n'+args_block+r'''
    return romSetupRequested;
}
int Run(){
'''+block+r'''
    (void)windowInitialized;
    (void)freshInstall;
    return 0;
}
int main(int argc,char** argv){
    assert(argc==2);const int scenario=std::stoi(argv[1]);
    char name[]="ComboShip",setup[]="--rom-setup",other[]="--other",value[]="value";
    char* forwarded[]={name,setup,other,value,setup,nullptr};
    int forwardedCount=5;
    assert(ParseArgs(forwardedCount,forwarded) && forwardedCount==3);
    assert(forwarded[0]==name && forwarded[1]==other && forwarded[2]==value && forwarded[3]==nullptr);
    assert(!ParseArgs(forwardedCount,forwarded) && forwardedCount==3);
    if(scenario==4){
        cancelExtraction=true;
        assert(Run()==1);
        assert(deinitCalls==1 && "cancelled setup must release the live bootstrap before unloading modules");
        return 0;
    }
    // Existing vanilla-only and MQ-only installs can explicitly reopen setup.
    // Existing complete installs take the original fast path without the flag.
    if(scenario!=2)std::ofstream(scenario==1?"oot-mq.o2r":"oot.o2r").put('o');
    std::ofstream("mm.o2r").put('m');
    if(scenario==3)romSetupRequested=false;
    assert(Run()==0);
    assert(screenCalls==(scenario==3?0:1) && "--rom-setup must reopen setup for an existing install");
    if(scenario!=3){
        assert(seen.sohVanillaPresent==(scenario==0));
        assert(seen.sohMqPresent==(scenario==1));
        assert(seen.sohNeeded==(scenario==2));
        assert(!seen.mmNeeded);
        assert(seen.sohClassifyKind==Kind && "launcher must supply the quest-aware classifier");
    }
}
'''
with tempfile.TemporaryDirectory(prefix='rom-setup-launcher-') as tmp:
    cpp=Path(tmp)/'launcher.cpp';binary=Path(tmp)/'launcher';cpp.write_text(source)
    flags=['-std=c++20','-Wall','-Wextra','-Wno-unused-variable','-Wno-unused-function','-I',str(ROOT)]
    if args.sanitize: flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(['g++',*flags,str(cpp),'-o',str(binary)],check=True)
    for scenario in range(5):
        directory=Path(tmp)/str(scenario);directory.mkdir()
        subprocess.run([str(binary),str(scenario)],cwd=directory,check=True)
print('ROM setup launcher: argument preservation, reopen, archive availability, normal fast path and cancel teardown PASS')
