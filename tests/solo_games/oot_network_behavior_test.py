#!/usr/bin/env python3
"""Run actual transport dispatch around an instrumented socket boundary (no network traffic)."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
s=(ROOT/'soh/soh/Network/Network.cpp').read_text()
def body(marker):
 a=s.index(marker);b=s.index('{',a);n=1;i=b+1
 while n:n+=(s[i]=='{')-(s[i]=='}');i+=1
 return s[a:i]+'\n'
cpp=r'''
#include <cassert>
#include <thread>
#include <atomic>
#include <cstring>
#include <string>
#include "ComboGameMode.h"
#define SPDLOG_ERROR(...) ((void)0)
#define SPDLOG_DEBUG(...) ((void)0)
extern "C" {int gComboGameMode=COMBO_GAME_OOT;}
int nativeConnects=0,nativeSends=0,comboConnects=0,comboSends=0,comboDisconnects=0;
int SDLNet_ResolveHost(int*,const char*,uint16_t){++nativeConnects;return 0;}
const char* SDLNet_GetError(){return "fixture";}
int SDLNet_TCP_Send(int,const char*,int n){++nativeSends;return n;}
void CountConnect(const char*,uint16_t){++comboConnects;}void CountSend(const char*){++comboSends;}void CountDisconnect(){++comboDisconnects;}
void (*gComboAnchorConnect)(const char*,uint16_t)=CountConnect;
void (*gComboAnchorSend)(const char*)=CountSend;
void (*gComboAnchorDisconnect)()=CountDisconnect;
struct Network {
 bool isEnabled=false,isConnected=false;int networkAddress=0,networkSocket=1;std::thread receiveThread;
 void Enable(const char*,uint16_t);void Disable();void SendDataToRemote(const char*);
 void ReceiveFromServer(){}
};
'''+body('void Network::Enable(')+body('void Network::Disable(')+body('void Network::SendDataToRemote(')+r'''
int main(){
 Network n;n.Enable("fixture",1);n.SendDataToRemote("x");n.Disable();
 assert(nativeConnects==1 && nativeSends==1 && comboConnects==0 && comboSends==0 && comboDisconnects==0);
 assert(!n.receiveThread.joinable());
 gComboGameMode=COMBO_GAME_BOTH;n.Enable("fixture",1);n.SendDataToRemote("x");n.Disable();
 assert(nativeConnects==1 && nativeSends==1 && comboConnects==1 && comboSends==1 && comboDisconnects==1);
}
'''
with tempfile.TemporaryDirectory(prefix='oot-network-') as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(cpp)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DCOMBO_BUILD','-pthread','-I'+str(ROOT/'combo'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS OoT native transport vs combined transport dispatch')
