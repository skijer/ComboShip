#!/usr/bin/env python3
"""Execute the real native graph accessor with a deliberately foreign oracle response."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
s=(ROOT/'soh/soh/Enhancements/randomizer/entrance.cpp').read_text();a=s.index('RandomizerRegion Entrance::GetConnectedRegionKey() const');b=s.index('{',a);n=1;i=b+1
while n:n+=(s[i]=='{')-(s[i]=='}');i+=1
cpp=r'''
#include <cassert>
#include "ComboGameMode.h"
extern "C"{int gComboGameMode=COMBO_GAME_OOT;}
using RandomizerRegion=int;int calls=0;
int Combo_SOH_CrossDungeonRegion(int,int,int){++calls;return 999;}
struct Entrance {int index=1;Entrance* replacement=nullptr;int connectedRegion=42;int GetIndex()const{return index;}RandomizerRegion GetConnectedRegionKey()const;};
'''+s[a:i]+r'''
int main(){Entrance e;assert(e.GetConnectedRegionKey()==42 && calls==0);gComboGameMode=COMBO_GAME_BOTH;assert(e.GetConnectedRegionKey()==999 && calls==1);}
'''
with tempfile.TemporaryDirectory(prefix='oot-logic-') as tmp:
 p=Path(tmp);(p/'test.cpp').write_text(cpp)
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-DCOMBO_BUILD','-I'+str(ROOT/'combo'),str(p/'test.cpp'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
print('PASS native graph accessor bypasses combined oracle')
