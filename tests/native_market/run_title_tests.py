#!/usr/bin/env python3
"""Run both real market presenters with native title contexts, fades and hide policy."""
from pathlib import Path
import os
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/diagnostics'))
from run_time_pedestal_tests import block_from

def function(source, signature):
    return block_from(source, source.index(signature))

with tempfile.TemporaryDirectory(prefix='native-market-title-') as temporary:
    traces = []
    for host, path in [('mm', 'mm/2s2h/Enhancements/Gameplay/BlackMarket.cpp'),
                       ('soh', 'soh/soh/Enhancements/Gameplay/ComboBlackMarketNative.cpp')]:
        source = (ROOT/path).read_text()
        actor = (ROOT/host/'src/code/z_actor.c').read_text()
        library = (ROOT/host/'src/code/z_lib.c').read_text()
        native = function(library, 's32 Math_StepToS(')
        native += '\n' + function(actor, 'void TitleCard_Update(')
        helper = ''
        request = ''
        guard = ''
        if host == 'mm':
            native += '\n' + function(actor, 'void TitleCard_InitBossName(')
            implementation = (ROOT/'mm/2s2h/Enhancements/Graphics/ComboOpeningForestTitleCard.cpp').read_text()
            if 'int ComboPlaceTitleCard_ShowAt(' in implementation:
                helper += function(implementation, 'extern "C" int ComboPlaceTitleCard_ShowAt(') + '\n'
            helper += function(implementation, 'extern "C" int ComboPlaceTitleCard_Show(')
        else:
            boot = (ROOT/'soh/src/code/title_setup.c').read_text()
            request = re.search(r'static s32 sComboCrossGameTitleCardPending[^;]*;', boot)[0]
            for signature in ('int Combo_SOH_HasCrossGameTitleCard(', 'int Combo_SOH_ConsumeCrossGameTitleCard(',
                              'void Combo_SOH_RequestCrossGameTitleCard('):
                request += '\n' + function(boot, signature)
            if 'int Combo_SOH_SwapCustomTitleCardPresentation(' in boot:
                request += '\n' + re.search(r'static s32 sComboCustomTitleCardPresentation[^;]*;', boot)[0]
                request += '\n' + function(boot, 'int Combo_SOH_SwapCustomTitleCardPresentation(')
                request += '\n' + function(boot, 'int Combo_SOH_IsPresentingCustomTitleCard(')
            handler = (ROOT/'soh/soh/Enhancements/TimeSavers/timesaver_hook_handlers.cpp').read_text()
            guard = handler[handler.index('        case VB_SHOW_TITLE_CARD:'):handler.index('        case VB_WONDER_TALK:')]
        fixture = r'''
#include "global.h"
#include "gameplay/CrossGameTitleCard.h"
#ifdef OOT_TITLE_TEST
#include "soh/Enhancements/game-interactor/vanilla-behavior/GIVanillaBehavior.h"
#else
#include "2s2h/GameInteractor/GameInteractor_VanillaBehavior.h"
#endif
#include <cassert>
#include <cstdio>
#include <cstring>
#include <initializer_list>
SaveContext gSaveContext{};
PlayState* gPlayState;
GameState* gGameState;
static bool titleAvailable, altAvailable, altEnabled, stockActive, playerCs, veto, randomized;
static int hide = -1, presentations;
/* CARD_RESOURCE */
bool Active(){return stockActive;}
bool Interior(){return true;}
bool Safe(PlayState* p){return p && p->state.running && p->transitionTrigger==0 && p->transitionMode==0 && p->pauseCtx.state==0 && p->gameOverCtx.state==0 && p->csCtx.state==0;}
#ifdef OOT_TITLE_TEST
extern "C" s32 Player_InCsMode(PlayState*){return playerCs;}
#else
extern "C" bool Player_InCsMode(PlayState*){return playerCs;}
#endif
u8 ResourceMgr_FileExists(const char*){return titleAvailable;}
bool ResourceMgr_IsAltAssetsEnabled(){return altEnabled;}
u8 ResourceMgr_FileAltExists(const char*){return altAvailable;}
extern "C" int32_t CVarGetInteger(const char*,int32_t fallback){return hide < 0 ? fallback : hide;}
#ifdef OOT_TITLE_TEST
extern "C" Color_RGB8 CVarGetColor24(const char*,Color_RGB8 value){return value;}
extern "C" void GameInteractor_ExecuteOnPresentTitleCard(){++presentations;}
#define CVAR_COSMETIC(name) name
#define CVAR_ENHANCEMENT(name) name
#define IS_RANDO randomized
#else
extern "C" bool Player_InCsMode(PlayState*);
#endif
/* REQUEST */
extern "C" bool GameInteractor_Should(GIVanillaBehavior behavior,uint32_t value,...){
 bool result=value; bool* should=&result;
 switch(behavior){
/* GUARD */
 default:break;
 }
 return result && !veto && hide!=1;
}
/* NATIVE */
/* HELPER */
bool sTitlePending;
/* PRESENT */
static void Reset(PlayState& play){
#ifdef OOT_TITLE_TEST
 Combo_SOH_ConsumeCrossGameTitleCard();Combo_SOH_SwapCustomTitleCardPresentation(false);
#endif
 play={};gPlayState=&play;gGameState=&play.state;play.state.running=true;
 gSaveContext={};gSaveContext.fileNum=0;
 titleAvailable=true;altAvailable=altEnabled=stockActive=playerCs=veto=randomized=false;
 hide=-1;sTitlePending=true;
}
int main(){
 PlayState play;Reset(play);
 const auto saved=gSaveContext;
 PresentTitle();
 assert(play.actorCtx.titleCtx.texture==kCard && "entry title must not require active stock");
 assert(play.actorCtx.titleCtx.x==160 && play.actorCtx.titleCtx.y==120);
 assert(play.actorCtx.titleCtx.width==240 && play.actorCtx.titleCtx.height==24);
 assert(play.actorCtx.titleCtx.delayTimer==20 && play.actorCtx.titleCtx.durationTimer==80);
 assert(!sTitlePending && !std::memcmp(&saved,&gSaveContext,sizeof(saved)));
 for(int frame=0;frame<120;++frame){TitleCard_Update(
#ifdef OOT_TITLE_TEST
 &play,
#else
 &play.state,
#endif
 &play.actorCtx.titleCtx);
 const auto& card=play.actorCtx.titleCtx;
#ifdef OOT_TITLE_TEST
 std::printf("TRACE %d %d %d %d %d %d %d\n",frame,card.delayTimer,card.durationTimer,card.alpha,card.intensityR,card.intensityG,card.intensityB);
#else
 std::printf("TRACE %d %d %d %d %d %d %d\n",frame,card.delayTimer,card.durationTimer,card.alpha,card.intensity,card.intensity,card.intensity);
#endif
 }
 assert(play.actorCtx.titleCtx.alpha==0 && play.actorCtx.titleCtx.durationTimer==0);
 PresentTitle();assert(play.actorCtx.titleCtx.durationTimer==0);
 for(int reason=0;reason<8;++reason){
  Reset(play);
  if(reason==0)play.transitionTrigger=1;
  if(reason==1)play.transitionMode=1;
  if(reason==2)play.pauseCtx.state=1;
  if(reason==3)play.gameOverCtx.state=1;
  if(reason==4)play.csCtx.state=1;
  if(reason==5)playerCs=true;
  if(reason==6)play.actorCtx.titleCtx.durationTimer=4;
  if(reason==7)play.actorCtx.titleCtx.alpha=255;
  const auto before=play.actorCtx.titleCtx;
  PresentTitle();assert(sTitlePending && !std::memcmp(&before,&play.actorCtx.titleCtx,sizeof(before)));
  play.transitionTrigger=play.transitionMode=play.pauseCtx.state=play.gameOverCtx.state=play.csCtx.state=0;
  playerCs=false;play.actorCtx.titleCtx={};PresentTitle();
  assert(!sTitlePending && play.actorCtx.titleCtx.texture==kCard);
 }
 for(bool rando:{false,true})for(int setting:{-1,0,1})for(bool other:{false,true}){
  Reset(play);randomized=rando;hide=setting;veto=other;
#ifdef OOT_TITLE_TEST
  for(bool pending:{false,true})for(bool scoped:{false,true}){
   Reset(play);randomized=rando;hide=setting;veto=other;
   if(pending)Combo_SOH_RequestCrossGameTitleCard();
   Combo_SOH_SwapCustomTitleCardPresentation(scoped);
   const auto before=gSaveContext;
   PresentTitle();
   assert(bool(play.actorCtx.titleCtx.texture)==(setting!=1 && !other));
   assert(!sTitlePending && Combo_SOH_HasCrossGameTitleCard()==pending);
   assert(Combo_SOH_IsPresentingCustomTitleCard()==scoped);
   assert(!std::memcmp(&before,&gSaveContext,sizeof(before)));
  }
#else
  PresentTitle();assert(bool(play.actorCtx.titleCtx.texture)==(setting!=1 && !other));assert(!sTitlePending);
#endif
 }
 Reset(play);titleAvailable=false;PresentTitle();assert(!play.actorCtx.titleCtx.texture && !sTitlePending);
 Reset(play);titleAvailable=false;altAvailable=true;altEnabled=true;PresentTitle();assert(play.actorCtx.titleCtx.texture==kCard);
 Reset(play);titleAvailable=false;altAvailable=true;PresentTitle();assert(!play.actorCtx.titleCtx.texture);
 puts("PASS native market card: stock-independent entry, OoT position, native fade, defer/retry, hide/other veto, Alt fallback, one visit and save/request preservation");
}
'''
        # Baseline has no scope API: preserve a behavioral red phase for its
        # stock/layout problem before the explicit hide-policy seam is added.
        if host == 'soh' and 'SwapCustomTitleCardPresentation' not in request:
            request += '\nint Combo_SOH_SwapCustomTitleCardPresentation(int){return 0;}\nint Combo_SOH_IsPresentingCustomTitleCard(){return 0;}\n'
        fixture = fixture.replace('/* REQUEST */',request).replace('/* GUARD */',guard)
        fixture = fixture.replace('/* NATIVE */',native).replace('/* HELPER */',helper)
        fixture = fixture.replace('/* PRESENT */',function(source,'void PresentTitle('))
        fixture = fixture.replace('/* CARD_RESOURCE */', re.search(r'alignas\(2\) constexpr char kCard\[\][^;]*;', source)[0])
        target=Path(temporary)/(host+'.cpp');target.write_text(fixture)
        binary=Path(temporary)/host
        paths=[host,host+'/include',host+'/include/PR',host+'/src',host+'/assets',host+('/2s2h' if host=='mm' else '/soh'),'libultraship/include','combo']
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-w','-DCOMBO_BUILD','-DF3DEX_GBI_2',
                        '-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',
                        *(['-DOOT_TITLE_TEST'] if host=='soh' else []),
                        *['-I'+str(ROOT/p) for p in paths],str(target),'-o',str(binary)],check=True)
        result = subprocess.run([str(binary)],text=True,capture_output=True,
                                env={**os.environ,'ASAN_OPTIONS':'detect_leaks=0'})
        if result.returncode:
            print(result.stdout + result.stderr, file=sys.stderr)
            result.check_returncode()
        traces.append([line for line in result.stdout.splitlines() if line.startswith('TRACE ')])
        print('\n'.join(line for line in result.stdout.splitlines() if not line.startswith('TRACE ')))
    assert len(traces[0]) == 120 and traces[0] == traces[1], 'MM/OoT native title fade differs over120 frames'
    print('PASS exact120-frame native OoT/MM delay, duration, alpha and intensity parity')
