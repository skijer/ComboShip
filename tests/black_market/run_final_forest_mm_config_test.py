#!/usr/bin/env python3
"""Execute the real private MM scene registration up to hook installation."""
from pathlib import Path
import os,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'mm/2s2h/Enhancements/Gameplay/ComboSoloForest.cpp').read_text()
start=source.index('void RegisterSoloForest() {')
end=source.index('    GameInteractor::',start)
body=source[start:end]+'}\n'
fixture='''#include "global.h"
#include "ComboGameMode.h"
#include "entrances/SoloForest.h"
#include <cassert>
#include <cstdio>
extern "C" { int gComboGameMode=COMBO_GAME_MM; }
SceneTableEntry gSceneTable[SCENE_MAX]{};
SceneEntranceTableEntry sSceneEntranceTable[ENTR_SCENE_MAX]{};
'''+body+'''
int main(){
 gSceneTable[COMBO_SOLO_FOREST_MM_SCENE].drawConfig=0xFF;
 gComboGameMode=COMBO_GAME_BOTH;RegisterSoloForest();
 assert(gSceneTable[COMBO_SOLO_FOREST_MM_SCENE].drawConfig==0xFF);
 gComboGameMode=COMBO_GAME_MM;RegisterSoloForest();
 assert(gSceneTable[COMBO_SOLO_FOREST_MM_SCENE].drawConfig==SCENE_DRAW_CFG_MAT_ANIM);
 puts("PASS MM final forest registration enables native animation draw config and excludes combined mode");
}
'''
with tempfile.TemporaryDirectory(prefix='final-mm-scene-config-') as temp:
 path=Path(temp)/'test.cpp';path.write_text(fixture);binary=Path(temp)/'test'
 includes=['mm','mm/include','mm/include/PR','mm/src','mm/assets','mm/2s2h','combo','libultraship/include']
 subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-w','-DF3DEX_GBI_2','-DCOMBO_BUILD',
  '-fsanitize=undefined','-fno-sanitize-recover=all',*['-I'+str(ROOT/p) for p in includes],str(path),'-o',str(binary)],check=True)
 subprocess.run([str(binary)],check=True)
