#!/usr/bin/env python3
"""Run the complete production launcher against scripted game-module boundaries."""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--source-rev')
args = parser.parse_args()

MODULE = r'''
#include <fstream>
#include <cstdlib>
#include <string>
#include "ComboExtract.h"
static void Log(const std::string& s) { std::ofstream("trace",std::ios::app)<<s<<'\n'; }
extern "C" {
#ifdef TEST_SOH
void SOH_Init(){Log("OOT_INIT");}
void SOH_InitWindowOnly(){Log("WINDOW_INIT");}
void SOH_FinishInit(){Log("OOT_FINISH");}
void SOH_Deinit(){Log("OOT_DEINIT");}
void SOH_DeinitWindowOnly(){Log("WINDOW_DEINIT");}
void SOH_RunMain(int argc,char** argv){
    Log("OOT_RUN");for(int i=1;i<argc;++i)Log(std::string("ARG:")+argv[i]);
}
void SOH_SetGameMode(int m){Log("OOT_MODE:"+std::to_string(m));}
void SOH_SetOnNewSaveCallback(void(*)(int)){Log("COMBO_SAVE_CALLBACK");}
int SOH_ValidateRom(const char*){return 1;}
int SOH_ClassifyRomKind(const char*){return 1;}
int SOH_StartExtraction(const char*){return 1;}
void SOH_GetExtractionProgress(unsigned long long*,unsigned long long*,int*,int*){}
#endif
#ifdef TEST_MM
void MM_InitArchives(){Log("MM_ARCHIVES");}
void MM_SetGameMode(int m){Log("MM_MODE:"+std::to_string(m));}
void MM_RunSolo(){Log("MM_SOLO");}
void MM_Deinit(){Log("MM_DEINIT");}
int MM_InitRandoSaveFile(int,const char*,const unsigned char*){return 0;}
int MM_ValidateRom(const char*){return 1;}
int MM_StartExtraction(const char*){return 1;}
void MM_GetExtractionProgress(unsigned long long*,unsigned long long*,int*,int*){}
#endif
#ifdef TEST_UI
int ComboUI_SelectGame(int current){
    Log("PICK_CURRENT:"+std::to_string(current));
    const char* value=std::getenv("TEST_CHOOSE_GAME");return value ? std::atoi(value) : 0;
}
int ComboUI_RunExtraction(const ComboExtractCallbacks* cb){
    if(cb->sohNeeded){Log("EXTRACT_OOT");std::ofstream("oot.o2r")<<"fixture";}
    if(cb->mmNeeded){Log("EXTRACT_MM");std::ofstream("mm.o2r")<<"fixture";}
    return 1;
}
#endif
}
'''

NETWORK = r'''
#define SDL_MAIN_HANDLED
#include <SDL2/SDL_net.h>
extern "C" {
int SDLNet_Init(void){return 0;}
int SDLNet_ResolveHost(IPaddress*,const char*,Uint16){return -1;}
TCPsocket SDLNet_TCP_Open(IPaddress*){return nullptr;}
void SDLNet_TCP_Close(TCPsocket){}
int SDLNet_TCP_Send(TCPsocket,const void*,int){return -1;}
int SDLNet_TCP_Recv(TCPsocket,void*,int){return -1;}
SDLNet_SocketSet SDLNet_AllocSocketSet(int){return nullptr;}
void SDLNet_FreeSocketSet(SDLNet_SocketSet){}
int SDLNet_AddSocket(SDLNet_SocketSet,SDLNet_GenericSocket){return -1;}
int SDLNet_CheckSockets(SDLNet_SocketSet,Uint32){return 0;}
const char* SDL_GetError(void){return "scripted network boundary";}
const char* SDLNet_GetError(void){return "scripted network boundary";}
}
'''

with tempfile.TemporaryDirectory(prefix='solo-launcher-') as temp:
    temp = Path(temp)
    module = temp/'module.cpp';module.write_text(MODULE)
    network = temp/'network.cpp';network.write_text(NETWORK)
    flags=['-std=c++20','-O0','-g0','-I'+str(ROOT/'combo')]
    for define, name in [('TEST_SOH','libsoh.so'),('TEST_MM','lib2ship.so'),('TEST_UI','comboui.so')]:
        subprocess.run(['c++',*flags,'-shared','-fPIC','-D'+define,str(module),'-o',str(temp/name)],check=True)
    launcher = ROOT/'combo/ComboShip.cpp'
    if args.source_rev:
        launcher = temp/'ComboShip.cpp'
        launcher.write_bytes(subprocess.check_output(['git','show',args.source_rev+':combo/ComboShip.cpp'],cwd=ROOT))
    binary=temp/'Fleet'
    subprocess.run(['c++',*flags,'-DCOMBO_RELEASE_VERSION="test"',str(launcher),str(network),
                    '-ldl','-pthread','-o',str(binary)],check=True)

    def run(name, arguments, archives=(), chosen=None, last='combo', expected=0):
        directory=temp/name;directory.mkdir()
        for module_name in ['libsoh.so','lib2ship.so','comboui.so']:
            (directory/module_name).symlink_to(temp/module_name)
        (directory/'comboship.json').write_text('{}')
        (directory/'combo-launcher.json').write_text('{"game":"'+last+'"}')
        for archive in archives:(directory/archive).write_text('fixture')
        env=os.environ.copy()
        if chosen is not None:env['TEST_CHOOSE_GAME']=str(chosen)
        result=subprocess.run([str(binary),*arguments],cwd=directory,env=env,capture_output=True,text=True)
        assert result.returncode==expected,(name,result.stdout,result.stderr)
        trace=(directory/'trace').read_text().splitlines() if (directory/'trace').exists() else []
        return directory,trace

    def assert_solo(trace, mode):
        assert 'COMBO_SAVE_CALLBACK' not in trace,trace
        if mode==1:
            assert 'OOT_RUN' in trace and 'MM_SOLO' not in trace,trace
            assert 'OOT_FINISH' in trace and 'OOT_INIT' not in trace,trace
            assert 'EXTRACT_MM' not in trace and 'MM_ARCHIVES' not in trace,trace
            assert 'OOT_DEINIT' in trace,trace
        else:
            assert 'MM_SOLO' in trace,trace
            assert not set(trace)&{'OOT_INIT','OOT_FINISH','OOT_RUN','EXTRACT_OOT','COMBO_SAVE_CALLBACK'},trace
            assert trace.index('MM_DEINIT')<trace.index('WINDOW_DEINIT'),trace

    _, trace=run('oot',['--game=oot','--native','hello'],['oot.o2r'])
    assert_solo(trace,1)
    assert [x for x in trace if x.startswith('ARG:')]==['ARG:--native','ARG:hello'],trace
    _, trace=run('mq',['--game','oot'],['oot-mq.o2r']);assert_solo(trace,1)
    _, trace=run('mm',['--game=mm'],['mm.o2r']);assert_solo(trace,2)
    _, trace=run('oot-setup',['--game=oot','--rom-setup']);assert_solo(trace,1)
    assert 'EXTRACT_OOT' in trace,trace
    _, trace=run('mm-setup',['--game=mm','--rom-setup']);assert_solo(trace,2)
    assert 'EXTRACT_MM' in trace,trace
    directory,trace=run('picker-switch',[],['oot.o2r'],chosen=1,last='mm')
    assert 'PICK_CURRENT:2' in trace and 'OOT_MODE:1' in trace,trace
    assert_solo(trace,1)
    assert 'oot' in (directory/'combo-launcher.json').read_text()
    directory,trace=run('cancel',[],chosen=-1,last='oot')
    assert not set(trace)&{'OOT_INIT','OOT_FINISH','OOT_RUN','MM_SOLO','EXTRACT_OOT','EXTRACT_MM'},trace
    assert (directory/'combo-launcher.json').read_text()=='{"game":"oot"}'
    _,trace=run('combined',['--game=combo'],['oot.o2r','mm.o2r'])
    assert 'OOT_INIT' in trace and 'OOT_RUN' in trace and 'COMBO_SAVE_CALLBACK' in trace,trace
    _,trace=run('invalid',['--game=nonsense'],expected=2)
    assert not trace,trace
print('Full production launcher: 9 scenarios PASS (solo boot/ROM/teardown isolation, picker/cancel, native arguments, Combo regression)')
