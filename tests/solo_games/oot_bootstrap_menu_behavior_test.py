#!/usr/bin/env python3
"""Execute native SetupMenu before ROM init; only the modal may attach to the shared GUI.

--install-early reintroduces the rejected remembered-solo bootstrap behavior as a
negative control. The ordinary path compiles the unmodified production body.
"""
from pathlib import Path
import argparse,os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--install-early',action='store_true');args=p.parse_args()
s=(ROOT/'soh/soh/SohGui/SohGui.cpp').read_text();a=s.index('void SetupMenu()');b=s.index('{',a);n=1;i=b+1
while n:n+=(s[i]=='{')-(s[i]=='}');i+=1
body=s[a:i]
if args.install_early:body=body.replace('#ifndef COMBO_BUILD\n    gui->SetMenu(mSohMenu);\n#endif','if (!Combo_IsCombinedGame()) { gui->SetMenu(mSohMenu); }',1)
cpp=r'''
#include <cassert>
#include <memory>
#include "ComboGameMode.h"
#define CVAR_WINDOW(x) x
extern "C"{int gComboGameMode=COMBO_GAME_OOT;}
struct SohMenu{SohMenu(const char*,const char*){}};
struct SohModalWindow{bool shown=false;SohModalWindow(const char*,const char*){}void Show(){shown=true;}};
struct Gui{int menus=0,windows=0;void SetMenu(std::shared_ptr<SohMenu>){++menus;}void AddGuiWindow(std::shared_ptr<SohModalWindow>){++windows;}};
namespace Ship{struct Context{Gui gui;static Context* GetRawInstance(){static Context c;return &c;}Context* GetWindow(){return this;}Gui* GetGui(){return &gui;}};}
namespace SohGui{std::shared_ptr<SohMenu>mSohMenu;std::shared_ptr<SohModalWindow>mModalWindow;
'''+body+r'''
}
int main(){for(int mode:{COMBO_GAME_BOTH,COMBO_GAME_OOT,COMBO_GAME_MM}){
 gComboGameMode=mode;auto gui=Ship::Context::GetRawInstance()->GetGui();gui->menus=0;
 SohGui::SetupMenu();assert(gui->menus==0 && "window-only bootstrap cannot render the uninitialized native menu");
 assert(SohGui::mSohMenu && SohGui::mModalWindow->shown);
}}
'''
with tempfile.TemporaryDirectory(prefix='oot-bootstrap-menu-') as tmp:
 path=Path(tmp);(path/'test.cpp').write_text(cpp)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DCOMBO_BUILD','-I'+str(ROOT/'combo'),str(path/'test.cpp'),'-o',str(path/'test')],check=True)
 subprocess.run([str(path/'test')],check=True)
print('PASS combined/OoT/MM window-only bootstrap leaves the native menu detached')
