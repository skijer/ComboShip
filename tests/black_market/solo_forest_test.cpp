#include <cassert>
#include <cmath>
#define COMBO_BUILD
#include "ComboGameMode.h"
int gComboGameMode = COMBO_GAME_MM;
#if __has_include("entrances/SoloForest.h")
#include "entrances/SoloForest.h"
#else
static int ComboSoloForest_IsMmVisit(int,int,int,int,int,int) { return 0; }
static int ComboSoloForest_IsOotVisit(int,int,int,int,int,int) { return 0; }
static int ComboSoloForest_AtStump(float,float,float) { return 0; }
#endif
int main() {
    assert(ComboSoloForest_IsMmVisit(3,0,0,0,0,0));
    assert(ComboSoloForest_IsMmVisit(3,1,0,2,0,0));
    assert(!ComboSoloForest_IsMmVisit(3,2,0,0,0,0));
    assert(!ComboSoloForest_IsMmVisit(0x65,0,0,0,0,0));
    assert(!ComboSoloForest_IsMmVisit(3,0,1,0,0,0));
    assert(!ComboSoloForest_IsMmVisit(3,0,0,255,0,0));
    assert(!ComboSoloForest_IsMmVisit(3,0,0,0,1,0));
    assert(!ComboSoloForest_IsMmVisit(3,0,0,0,0,0xFFF0));
    gComboGameMode = COMBO_GAME_BOTH;
    assert(!ComboSoloForest_IsMmVisit(3,0,0,0,0,0));
    assert(!ComboSoloForest_IsOotVisit(0x6F,0,0,0,0,0));
    gComboGameMode = COMBO_GAME_OOT;
    assert(ComboSoloForest_IsOotVisit(0x6F,0,0,0,0,0));
    assert(ComboSoloForest_IsOotVisit(0x6F,1,0,2,0,0));
    assert(!ComboSoloForest_IsOotVisit(0x5B,0,0,0,0,0));
    assert(ComboSoloForest_IsOotVisit(0x6F,0,0,0,1,0));
    assert(ComboSoloForest_IsOotVisit(0x6F,0,0,0,3,0));
    assert(!ComboSoloForest_IsOotVisit(0x6F,0,0,0,4,0));
    assert(ComboSoloForest_AtStump(-695,0,-313));
    assert(!ComboSoloForest_AtStump(-486,0,-227));
    assert(!ComboSoloForest_AtStump(-695,200,-313));
}
