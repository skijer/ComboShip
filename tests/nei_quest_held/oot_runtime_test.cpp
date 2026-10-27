// Production wrapper and captured wrist helper; graphics/resources are boundaries.
#define main ExistingArticulatedMain
#include "../nei_held/articulated_runtime_test.cpp"
#undef main
#include "mods/extended_inventory.h"
extern "C" int ResourceMgr_IsModAsset(const char*);
#define QUEST_HELD_IS_MOD ResourceMgr_IsModAsset
#include "mods/items/objects/quest_held_resources.inc"
#include <set>
namespace {u8 wandMode=0;bool wandOut=true,slateOut=true,bombOut=false;int shadow=0,wind=0,ghost=0;
std::vector<std::string> loaded;bool mod=false;std::set<std::string> modPaths;}
extern "C" {
void CustomItems_DrawElementalWand(Player*,PlayState*);
void CustomItems_DrawSheikahSlate(Player*,PlayState*);
u8 Wand_GetMode(){return wandMode;}
u8 Wand_IsDrawn(){return wandOut;}
u8 Slate_IsDrawn(){return slateOut;}
u8 RemoteBomb_IsHeld(){return bombOut;}
void WandShadow_Draw(PlayState*){++shadow;}
void WandWind_Draw(Player*,PlayState*){++wind;}
void Cryonis_DrawGhost(PlayState*){++ghost;}
void Gfx_SetupDL_25Xlu(GraphicsContext*){}
int ResourceMgr_IsModAsset(const char* p){return modPaths.contains(p)||(mod&&std::string(p).find("Legacy")==std::string::npos);}
u8 ResourceMgr_FileExists(const char* p){return available.contains(p);}
Gfx* ResourceMgr_LoadGfxByName(const char* p){assert(available.contains(p));loaded.emplace_back(p);static Gfx dl[1];return dl;}
}
int main(int argc,char**argv){
  Player p{};PlayState play{};GraphicsContext graphics{};Gfx opa[2048],xlu[2048];
  graphics.polyOpa.p=opa;graphics.polyXlu.p=xlu;play.state.gfxCtx=&graphics;
  p.actor.scale.x=p.actor.scale.y=p.actor.scale.z=.01;
  if(argc>1&&std::string(argv[1])=="--cache"){
    Matrix_Translate(1,2,3,MTXMODE_NEW);Matrix_Scale(.01,.01,.01,MTXMODE_APPLY);ItemEquip_CaptureHandMatrix();
    const ItemHandPose pose={0,0,0,0,0,0,1};
    static const char* later="__OTR__objects/test/missing_then_present";
    assert(!ItemEquip_DrawHeldModel(&p,&play,later,nullptr,&pose));
    available.insert(later);
    assert(ItemEquip_DrawHeldModel(&p,&play,later,nullptr,&pose));
    std::vector<std::string> names;names.reserve(32);
    for(int i=0;i<32;i++){
      names.push_back("__OTR__objects/test/session_"+std::to_string(i));available.insert(names.back());
      assert(ItemEquip_DrawHeldModel(&p,&play,names.back().c_str(),nullptr,&pose));
    }
    loaded.clear();assert(ItemEquip_DrawHeldModel(&p,&play,later,nullptr,&pose));
    assert(loaded==std::vector<std::string>{later});
    std::cout<<"PASS held resource loader exceeds 24 paths and retries missing/changed resources\n";return 0;
  }
  const QuestHeldModel* models[]={&sQuest_sand_rod,&sQuest_tornado_rod,&sQuest_water_rod,&sQuest_meteor_rod,&sQuest_storm_rod,&sQuest_shadow_scepter,&sQuest_sheikah_slate};
  mod=argc>1&&std::string(argv[1])=="--mod";
  bool fallback=argc>1&&std::string(argv[1])=="--fallback";
  bool empty=argc>1&&std::string(argv[1])=="--missing";
  for(auto* m:models){for(auto r=m->required;*r;++r)available.insert(*r);for(auto r=m->fallbackRequired;*r;++r)available.insert(*r);}
  if(fallback){for(auto* m:models)available.erase(*m->required);}
  if(empty)available.clear();
  if(mod){for(auto it=available.begin();it!=available.end();){if(it->find("nei_held_redesign/")!=std::string::npos)it=available.erase(it);else ++it;}}

  Matrix_Translate(42,60,90,MTXMODE_NEW);Matrix_RotateX(.4,MTXMODE_APPLY);Matrix_RotateY(.7,MTXMODE_APPLY);Matrix_RotateZ(-.2,MTXMODE_APPLY);Matrix_Scale(.01,.01,.01,MTXMODE_APPLY);ItemEquip_CaptureHandMatrix();
  if(argc>1&&std::string(argv[1])=="--partial-mod"){
    for(int i=0;i<6;i++)for(bool xluOnly:{false,true}){
      const auto* m=models[i];wandMode=i;modPaths={xluOnly?m->xlu:m->opa};
      available.clear();for(auto r=m->fallbackRequired;*r;++r)available.insert(*r);
      available.insert(m->opa);available.insert(m->xlu);
      loaded.clear();nativeDraws=0;CustomItems_DrawElementalWand(&p,&play);
      assert((loaded==std::vector<std::string>{xluOnly?m->fallbackOpa:m->opa,xluOnly?m->xlu:m->fallbackXlu}));
      // Every selected pristine-pass dependency is gated independently.
      const char* const* required=xluOnly?m->fallbackOpaRequired:m->fallbackXluRequired;
      for(auto missing=required;*missing;++missing){
        available.erase(*missing);loaded.clear();nativeDraws=0;
        CustomItems_DrawElementalWand(&p,&play);
        assert((loaded==std::vector<std::string>{xluOnly?m->xlu:m->opa}));assert(nativeDraws==1);
        available.insert(*missing);
      }
    }
    std::cout<<"PASS OoT OPA-only/XLU-only mods retain custom pass with missing new/default counterpart resources\n";return 0;
  }
  if(argc>1&&std::string(argv[1])=="--partial"){
    int cases=0;
    for(int i=0;i<7;i++){
      const auto* m=models[i];wandMode=i;
      for(auto missing=m->required;*missing;++missing){
        available.erase(*missing);loaded.clear();nativeDraws=0;
        if(i<6)CustomItems_DrawElementalWand(&p,&play);else CustomItems_DrawSheikahSlate(&p,&play);
        assert(nativeDraws==(i<6?2:1)&&!loaded.empty()&&loaded.front()==m->fallbackOpa);
        available.insert(*missing);++cases;
      }
      available.erase(*m->required);
      for(auto missing=m->fallbackRequired;*missing;++missing){
        available.erase(*missing);loaded.clear();nativeDraws=0;
        if(i<6)CustomItems_DrawElementalWand(&p,&play);else CustomItems_DrawSheikahSlate(&p,&play);
        assert(nativeDraws==0&&loaded.empty());available.insert(*missing);++cases;
      }
      available.insert(*m->required);
    }
    std::cout<<"PASS "<<cases<<" individual new/fallback display-list, texture, vertex, matrix omissions\n";return 0;
  }
  for(int i=0;i<7;i++){
    nativeDraws=0;loaded.clear();wandMode=i;
    if(i<6)CustomItems_DrawElementalWand(&p,&play);else CustomItems_DrawSheikahSlate(&p,&play);
    if(empty){assert(nativeDraws==0&&loaded.empty());continue;}
    const auto* m=models[i];
    // Incomplete matching geometry must use an actual retained native model,
    // never the compatibility alias which would call a missing dependency.
    const std::string expected=fallback?m->fallbackOpa:m->opa;
    assert(!loaded.empty()&&loaded.front()==expected);
    assert(nativeDraws==(i<6?2:1));
    const ItemHandPose pose=i<6?ItemHandPose{0,7.356f,-3.218f,91.034f,180,111.724f,.12f}:ItemHandPose{0,-8.276f,0,180,180,0,.1f};
    MtxF actual=current;assert(ItemEquip_ApplyHandPose(&p,&pose));
    near(transform(actual,{0,-45.5f,0}),transform(current,{0,-45.5f,0}));
    near(transform(actual,{0,175,0}),transform(current,{0,175,0}));
    near(transform(actual,{1,0,0}),transform(current,{1,0,0}));
  }
  int draws=nativeDraws;wandOut=false;wandMode=0;CustomItems_DrawElementalWand(&p,&play);assert(nativeDraws==draws);
  wandOut=true;wandMode=WAND_MODE_COUNT;CustomItems_DrawElementalWand(&p,&play);assert(nativeDraws==draws);
  slateOut=false;CustomItems_DrawSheikahSlate(&p,&play);assert(nativeDraws==draws);
  slateOut=true;bombOut=true;CustomItems_DrawSheikahSlate(&p,&play);assert(nativeDraws==draws);
  ItemEquip_ReleaseHandMatrix();wandMode=0;CustomItems_DrawElementalWand(&p,&play);assert(nativeDraws==draws);
  assert(shadow==9&&wind==9&&ghost==3&&matrices.empty());
  std::cout<<"PASS OoT quest held modes, calibrated wrist poses, effects, gates, "<<(mod?"pack override despite absent defaults":empty?"missing resources":fallback?"full-resource fallback":"legacy override paths")<<"\n";
}
