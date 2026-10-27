#!/usr/bin/env python3
"""Execute the real ROM setup screen with deterministic picker/window boundaries."""
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser()
parser.add_argument('--sanitize',action='store_true')
args=parser.parse_args()
screen=(ROOT/'combo/gui/ComboExtractScreen.cpp').read_text()
# Leave standard-library includes intact; replace only headers requiring the
# renderer, native dialogs and live libultraship singleton.
screen='\n'.join(line for line in screen.splitlines() if not (
    line.startswith('#include "') or
    line.startswith(('#include <libultraship/','#include <ship/','#include <fast/','#include <imgui.h>'))))
source='#include "tests/rom_setup/screen_stubs.h"\n'+screen+r'''
int OotValidate(const char* p){return Test::kind(p)!=0;}
int MmValidate(const char* p){return std::filesystem::path(p).filename()=="mm.z64";}
int Start(const char* p){Test::jobs.emplace_back(p);return 1;}
void Progress(unsigned long long*,unsigned long long*,int* done,int* ok){*done=1;*ok=!Test::failExtraction;}
int main(int argc,char** argv){
    assert(argc==2);
    const int scenario=std::stoi(argv[1]);
    ComboExtractCallbacks cb{};
    cb.sohNeeded=cb.mmNeeded=1;
    cb.sohValidate=cb.sohClassify=OotValidate;
    cb.mmValidate=cb.mmClassify=MmValidate;
    cb.sohStart=cb.mmStart=Start;
    cb.sohProgress=cb.mmProgress=Progress;
    cb.sohClassifyKind=Test::kind;
    Test::pickPaths={"vanilla.z64","mq.z64","mm.z64"};
    std::vector<std::string> want=Test::pickPaths;
    if(scenario==1){Test::pickPaths=want={"mq.z64","vanilla.z64","mm.z64"};}
    if(scenario==2){Test::skipOther=true;Test::pickPaths=want={"vanilla.z64","mm.z64"};}
    if(scenario==3){cb.sohNeeded=cb.mmNeeded=0;cb.sohVanillaPresent=1;Test::pickPaths=want={"mq.z64"};}
    if(scenario==4){cb.sohNeeded=cb.mmNeeded=0;cb.sohMqPresent=1;Test::pickPaths=want={"vanilla.z64"};}
    if(scenario==5){cb.sohNeeded=0;cb.sohVanillaPresent=cb.sohMqPresent=1;Test::pickPaths=want={"mm.z64"};}
    if(scenario==6){cb.sohNeeded=cb.mmNeeded=0;cb.sohVanillaPresent=cb.sohMqPresent=1;Test::pickPaths=want={};}
    if(scenario==7){Test::pickPaths={"vanilla.z64","vanilla.z64","mq.z64","mm.z64"};}
    if(scenario==8){Test::pickPaths={"mq.z64","mm.z64","vanilla.z64","mm.z64"};want={"mq.z64","vanilla.z64","mm.z64"};}
    if(scenario==9 || scenario==10){
        for(const char* name:{"vanilla.z64","mq.z64","mm.z64"}) {FILE* f=fopen(name,"w");assert(f);fclose(f);}
        Test::pickPaths.clear();
        want={"mq.z64","vanilla.z64","mm.z64"}; // deterministic filename sort, either quest may be first
        if(scenario==10){Test::pickPaths={"vanilla.z64","mq.z64"};Test::changeOptional=false;want={"vanilla.z64","mq.z64","mm.z64"};}
    }
    if(scenario==11){
        cb.gameMode=COMBO_GAME_MM;cb.sohNeeded=0;cb.sohVanillaPresent=1;
        Test::pickPaths={"mq.z64","mm.z64"};want={"mm.z64"};
    }
    if(scenario==12){
        cb.gameMode=COMBO_GAME_OOT;want={"vanilla.z64","mq.z64"};
    }
    const int result=ComboUI_RunExtraction(&cb);
    for(auto& path:Test::jobs)path=std::filesystem::path(path).filename().string();
    assert(Test::jobs==want && "setup must select complementary OoT before MM without re-extracting installed archives");
    assert(result==1);
    assert(Test::disabled.empty());
    int mqFrame=-1,mmFrame=-1;
    for(size_t i=0;i<Test::prompts.size();++i){
        if(Test::prompts[i]=="Ocarina of Time: Master Quest" && mqFrame<0) mqFrame=Test::promptFrames[i];
        if(Test::prompts[i]=="Majora's Mask" && mmFrame<0) mmFrame=Test::promptFrames[i];
    }
    if(scenario==0)assert(mqFrame>0 && mmFrame>mqFrame && "MM selection must follow the complementary OoT step");
    if(scenario==7 || scenario==8){
        assert(std::any_of(Test::prompts.begin(),Test::prompts.end(),[](const auto& p){return p.starts_with("Not a valid Ocarina of Time:");}) &&
               "duplicate quest and wrong-game choices must be rejected at the optional step");
    }
}
'''
with tempfile.TemporaryDirectory(prefix='rom-setup-screen-') as tmp:
    cpp=Path(tmp)/'screen.cpp';binary=Path(tmp)/'screen';cpp.write_text(source)
    flags=['-std=c++20','-Wall','-Wextra','-Werror','-I',str(ROOT)]
    if args.sanitize: flags+=['-fsanitize=address,undefined','-fno-omit-frame-pointer','-g']
    subprocess.run(['g++',*flags,str(cpp),'-o',str(binary)],check=True)
    for scenario in range(13):
        directory=Path(tmp)/str(scenario);directory.mkdir()
        subprocess.run([str(binary),str(scenario)],cwd=directory,check=True)
print('ROM setup screen: 13 cases PASS (ordering, either quest first, skip, installed archives, rejection, auto-scan/reselection, solo ROM isolation)')
