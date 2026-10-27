#define MM_REAL_RENDERER
#define main ExistingMmRodMain
#include "../mm_nei/rod_runtime_test.cpp"
#undef main
#include "2s2h/Rando/NeiHeldPresentation.h"
#include "mods/extended_inventory.h"
extern "C" int NeiResource_IsMod(const char*);
#define QUEST_HELD_IS_MOD NeiResource_IsMod
#include "mods/items/objects/quest_held_resources.inc"
#include "mods/items/logic/adult_link_render.h"
#include <set>
namespace {std::set<std::string> available;std::vector<std::string> drawn;
bool mod=false;std::set<std::string> modPaths;u8 wandMode=0;bool wandOut=true,slateOut=true;int shadow=0,storm=0,wind=0;}
extern "C" {
void CustomItems_DrawElementalWand(Player*,PlayState*);
void CustomItems_DrawSheikahSlate(Player*,PlayState*);
void Wand_Draw(Player*,PlayState*);
u8 Wand_GetMode(){return wandMode;}
u8 Wand_IsDrawn(){return wandOut;}
u8 Slate_IsDrawn(){return slateOut;}
void WandShadow_Draw(PlayState*){++shadow;}
void WandStorm_Draw(PlayState*){++storm;}
void WandWind_Draw(Player*,PlayState*){++wind;}
const char* NeiResource_Route(const char* p){static std::set<std::string> paths;return paths.insert(std::string("__OTR__@oot:")+(p+7)).first->c_str();}
int NeiResource_IsMod(const char* path){return modPaths.contains(path)||(mod&&std::string(path).find("Legacy")==std::string::npos);}
bool NeiHeld_HasResources(const char* opa,const char* xlu){return opa&&available.contains(opa)&&(!xlu||available.contains(xlu));}
bool NeiHeld_DrawModel(PlayState*,const char* opa,const char* xlu){if(!NeiHeld_HasResources(opa,xlu))return false;drawn.emplace_back(opa);if(xlu)drawn.emplace_back(xlu);poses.push_back(current);return true;}
}
int main(int argc,char**argv){
  Player p{};p.transformation=PLAYER_FORM_HUMAN;PlayState play{};play.actorCtx.actorLists[ACTORCAT_PLAYER].first=&p.actor;gPlayState=&play;
  GraphicsContext graphics{};Gfx opa[8192]{},xlu[8192]{},overlay[8192]{};
  graphics.polyOpa.p=opa;graphics.polyOpa.d=opa+8192;
  graphics.polyXlu.p=xlu;graphics.polyXlu.d=xlu+8192;
  graphics.overlay.p=overlay;graphics.overlay.d=overlay+8192;play.state.gfxCtx=&graphics;
  const QuestHeldModel* models[]={&sQuest_sand_rod,&sQuest_tornado_rod,&sQuest_water_rod,&sQuest_meteor_rod,&sQuest_storm_rod,&sQuest_shadow_scepter,&sQuest_sheikah_slate};
  mod=argc>1&&std::string(argv[1])=="--mod";
  bool fallback=argc>1&&std::string(argv[1])=="--fallback";bool empty=argc>1&&std::string(argv[1])=="--missing";
  for(auto* m:models){for(auto r=m->required;*r;++r)available.insert(*r);for(auto r=m->fallbackRequired;*r;++r)available.insert(*r);}
  if(fallback){for(auto* m:models)available.erase(*m->required);}if(empty)available.clear();
  if(mod){for(auto it=available.begin();it!=available.end();){if(it->find("nei_held_redesign/")!=std::string::npos)it=available.erase(it);else ++it;}}
  if(argc>1&&std::string(argv[1])=="--partial-mod"){
    RigFit_Set(1,1,0,0);p.actor.scale={.01,.01,.01};
    Matrix_Translate(0,0,0,MTXMODE_NEW);Matrix_Scale(.01,.01,.01,MTXMODE_APPLY);ItemEquip_CaptureHandMatrix();
    for(int i=0;i<6;i++)for(bool xluOnly:{false,true}){
      const auto* m=models[i];wandMode=i;modPaths={xluOnly?m->xlu:m->opa};
      available.clear();for(auto r=m->fallbackRequired;*r;++r)available.insert(*r);
      available.insert(m->opa);available.insert(m->xlu);
      drawn.clear();poses.clear();Wand_Draw(&p,&play);
      assert((drawn==std::vector<std::string>{xluOnly?m->fallbackOpa:m->opa,xluOnly?m->xlu:m->fallbackXlu}));
      const char* const* required=xluOnly?m->fallbackOpaRequired:m->fallbackXluRequired;
      for(auto missing=required;*missing;++missing){
        available.erase(*missing);drawn.clear();poses.clear();graphics.polyXlu.p=xlu;Wand_Draw(&p,&play);
        if(xluOnly){
          assert(drawn.empty());const Gfx* command=xlu;
          while(command<graphics.polyXlu.p&&(command->words.w0>>24)!=G_DL_OTR_FILEPATH)++command;
          assert(command<graphics.polyXlu.p);
          assert(std::string((const char*)command->words.w1)==std::string("__OTR__@oot:")+(m->xlu+7));
        }else assert((drawn==std::vector<std::string>{m->opa}));
        available.insert(*missing);
      }
    }
    std::cout<<"PASS MM OPA-only/XLU-only mods retain custom pass with missing new/default counterpart resources\n";return 0;
  }
  for(bool adult:{false,true})for(float scale:{.01f,.001f})for(int sample=0;sample<8;sample++){
    RigFit_Set(1,!adult,adult,0);
    p.actor.scale.x=p.actor.scale.y=p.actor.scale.z=scale;
    Matrix_Translate(42+sample,60,90,MTXMODE_NEW);Matrix_RotateXF(sample*.4,MTXMODE_APPLY);Matrix_RotateYF(sample*.7,MTXMODE_APPLY);Matrix_RotateZF(sample*-.2,MTXMODE_APPLY);Matrix_Scale(scale,scale,scale,MTXMODE_APPLY);ItemEquip_CaptureHandMatrix();
    const MtxF wrist=current;
    for(int i=0;i<6;i++){
      wandMode=i;drawn.clear();poses.clear();MtxF caller=current;Player before=p;
      Wand_Draw(&p,&play);assert(memcmp(&p,&before,sizeof(p))==0&&matrices.empty());
      if(empty){assert(drawn.empty()&&poses.empty());continue;}
      const auto* m=models[i];assert(drawn.front()==(fallback?m->fallbackOpa:m->opa));assert(drawn.back()==(fallback?m->fallbackXlu:m->xlu));
      // Grip must be the measured palm independently of local blade length,
      // model origin, body scale, adult presentation, or wrist animation.
      const Vec3f expected=transform(wrist,{0,adult?328.0f:216.22f,adult?77.0f:-4.5f});
      const auto actualGrip=transform(poses.front(),{0,-45.5f,0});
      if(std::abs(actualGrip.x-expected.x)>.001f||std::abs(actualGrip.y-expected.y)>.001f||std::abs(actualGrip.z-expected.z)>.001f)std::cerr<<"Grip "<<adult<<" "<<scale<<" "<<sample<<" actual "<<actualGrip.x<<","<<actualGrip.y<<","<<actualGrip.z<<" expected "<<expected.x<<","<<expected.y<<","<<expected.z<<"\n";
      near(actualGrip,expected);
      const Vec3f a=transform(poses.front(),{0,-45.5f,0}),b=transform(poses.front(),{0,175,0});
      assert(std::abs(std::sqrt((b.x-a.x)*(b.x-a.x)+(b.y-a.y)*(b.y-a.y)+(b.z-a.z)*(b.z-a.z))-220.5f*.12f)<.001f);
      assert(memcmp(&current,&caller,sizeof(current))==0);
    }
  }
  // Slate retains its separate forearm-derived native MM orientation and pose.
  p.bodyPartsPos[PLAYER_BODYPART_RIGHT_FOREARM]={10,20,30};p.bodyPartsPos[PLAYER_BODYPART_RIGHT_HAND]={20,30,60};
  drawn.clear();poses.clear();CustomItems_DrawSheikahSlate(&p,&play);
  if(empty)assert(drawn.empty());else{
    assert(drawn==std::vector<std::string>{fallback?sQuest_sheikah_slate.fallbackOpa:sQuest_sheikah_slate.opa});
    MtxF actual=poses.front();float dx=10,dy=10,dz=30;
    Matrix_Translate(20,30,60,MTXMODE_NEW);Matrix_RotateYF(atan2f(dx,dz),MTXMODE_APPLY);Matrix_RotateXF(-atan2f(dy,sqrtf(dx*dx+dz*dz)),MTXMODE_APPLY);
    Matrix_RotateYF(DEG_TO_RAD(180),MTXMODE_APPLY);Matrix_RotateXF(DEG_TO_RAD(78.416f),MTXMODE_APPLY);Matrix_RotateZF(DEG_TO_RAD(13.664f),MTXMODE_APPLY);Matrix_Translate(-3.036f,-12.327f,-.264f,MTXMODE_APPLY);Matrix_Scale(.146f,.146f,.146f,MTXMODE_APPLY);
    near(transform(actual,{0,45,0}),transform(current,{0,45,0}));near(transform(actual,{1,0,0}),transform(current,{1,0,0}));
  }
  drawn.clear();poses.clear();wandOut=false;wandMode=0;Wand_Draw(&p,&play);assert(drawn.empty());wandOut=true;wandMode=WAND_MODE_COUNT;Wand_Draw(&p,&play);assert(drawn.empty());
  ItemEquip_ReleaseHandMatrix();wandMode=0;Wand_Draw(&p,&play);assert(drawn.empty());slateOut=false;CustomItems_DrawSheikahSlate(&p,&play);assert(drawn.empty());
  assert(shadow==195&&storm==195&&wind==195);
  std::cout<<"PASS MM actual Wand_Draw callback, six modes, 32 native wrist frames, exact palm/Slate poses, world effects, "<<(mod?"pack override despite absent defaults":empty?"missing resources":fallback?"full-resource fallback":"legacy override paths")<<"\n";
}
