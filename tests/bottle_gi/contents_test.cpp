// Execute the production contents composer and arena reservation at draw boundaries.
#include <cassert>
#include <cmath>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "combo/menu/ComboBottleContents.h"
#include "combo/menu/ComboItemDrawABI.h"
#include "soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
using s16 = int16_t;
using NeiGi::Kind;
struct Gfx { uintptr_t words[2]; };
struct Mtx { unsigned char bytes[64]; };
struct Arena { Gfx* p; Gfx* d; };
struct GraphicsContext { Arena polyOpa, polyXlu, overlay; };
struct PlayState { struct { GraphicsContext* gfxCtx; } state; uint32_t gameplayFrames; };
struct Pose { float x=0,y=0,z=0,s=1; };
struct Draw { std::string path,owner; Pose pose; size_t vertices=0;int stream=-1; };
Gfx opa[8192],xlu[2048],overlay[64];
GraphicsContext gfx;
PlayState play{{&gfx},42};
PlayState* gPlayState=&play;
Pose pose,initial;
std::vector<Pose> matrices;
std::vector<Draw> draws;
std::vector<std::string> owners;
std::string gpu;
bool shellPresent=true,meshPresent=true,markerPresent=true,replacement=false,resourceThrows=false,dependenciesPresent=true,dependencyAlt=false,altEnabled=false;
int cpuOwner=0,loads=0;
#ifdef COMBO_BOTTLE_HOST_MM
const char* host="mm";
#else
const char* host="oot";
#endif
void Matrix_Push(){matrices.push_back(pose);}
void Matrix_Pop(){assert(!matrices.empty());pose=matrices.back();matrices.pop_back();}
constexpr int MTXMODE_APPLY=1;
void Matrix_Translate(float x,float y,float z,int){pose.x+=x;pose.y+=y;pose.z+=z;}
void Matrix_Scale(float x,float y,float z,int){assert(x==y&&y==z);pose.s*=x;}
Gfx resource;
bool ResourceMgr_FileExists(const char* path){assert(strstr(path,"polish/"));return dependenciesPresent;}
bool ResourceMgr_FileAltExists(const char* path){assert(strstr(path,"polish/"));return dependencyAlt;}
bool ResourceMgr_IsAltAssetsEnabled(){return altEnabled;}
int ResourceMgr_IsModAsset(const char* path){assert(strstr(path,"Bottle"));return replacement;}
Gfx* ResourceMgr_LoadGfxByName(const char* path){
    assert(cpuOwner==0);
    if(resourceThrows)throw std::runtime_error("bad bottle resource");
    if(strstr(path,"BottleShell"))return shellPresent?&resource:nullptr;
    if(strstr(path,"polish/"))return meshPresent?&resource:nullptr;
    return markerPresent?&resource:nullptr;
}
void Push(const char* p){owners.push_back(gpu);gpu=p;}
void Pop(){assert(!owners.empty());gpu=owners.back();owners.pop_back();}
void Display(Gfx* cmd,const char* path){const uintptr_t address=reinterpret_cast<uintptr_t>(cmd);const int stream=address>=reinterpret_cast<uintptr_t>(xlu)&&address<reinterpret_cast<uintptr_t>(xlu+2048);draws.push_back({path,gpu,pose,0,stream});}
void LoadMatrix(){assert(reinterpret_cast<uintptr_t>(gfx.polyOpa.d)-reinterpret_cast<uintptr_t>(gfx.polyOpa.p)>=sizeof(Mtx));gfx.polyOpa.d-=sizeof(Mtx)/sizeof(Gfx);}
#define POLY_OPA_DISP gfx.polyOpa.p
#define POLY_XLU_DISP gfx.polyXlu.p
#define OPEN_DISPS(...) ((void)0)
#define CLOSE_DISPS(...) ((void)0)
#define CFA_SETUP_OPA(...) ((void)0)
#define gDPSetPrimColor(...) ((void)0)
#define gDPSetEnvColor(...) ((void)0)
#define CFA_SETUP_XLU(...) ((void)0)
#define CFA_LOAD_MTX(p,ctx) ((void)(p),LoadMatrix())
#define gSPGrayscale(p,flag) ((void)(p),assert(!(flag)))
#define gSPComboRMPush(p,owner) ((void)(p),Push(owner))
#define gSPComboRMPop(p) ((void)(p),Pop())
#define gSPDisplayList(p,path) Display(p,reinterpret_cast<const char*>(path))
namespace NeiGi { struct TextureMaterial {}; }
/* PRODUCTION_ARENA */
static bool NeiGi_DrawMeshMaterial(PlayState* p,const NeiGi::Mesh& mesh,NeiGi::Kind,
                                    const NeiGi::TextureMaterial*,bool,const char* owner){
    // Instrument the child boundary: production reservation must protect the
    // later shell from this otherwise valid child arena request.
    const size_t bytes=mesh.count*16;
    if(!NeiGi_ArenaHasRoom(p,bytes,1,2,32+mesh.count/3))return false;
    assert(NeiGi_ReservedArena.matrixBytes>=sizeof(Mtx));
    p->state.gfxCtx->polyOpa.d-=(bytes+sizeof(Mtx))/sizeof(Gfx);
    p->state.gfxCtx->polyXlu.p+=32+mesh.count/3;
    draws.push_back({"gold",owner,pose,mesh.count});return true;
}
/* PRODUCTION_MESH_TAIL */
NeiGi::Basis NeiGi_CameraBasis(PlayState*){return {};}
/* PRODUCTION_CONTENTS */
int shimmers=0;
bool GetItem_GetShimmerColor(s16 id,uint8_t*){return id==1;}
void GetItem_Draw(PlayState*,s16 id){if(id==1)++shimmers;}
void ComboDrawMaskShimmer(PlayState*,void*,const uint8_t*,const char*){++shimmers;}
/* PRODUCTION_IMPORTED_BOTTLE */
bool same(const Pose& a,const Pose& b){return a.x==b.x&&a.y==b.y&&a.z==b.z&&a.s==b.s;}
void reset(size_t capacity=8192){
    gfx={{opa,opa+capacity},{xlu,xlu+2048},{overlay,overlay+64}};
    pose=initial={10,20,30,2};draws.clear();matrices.clear();owners.clear();gpu="foreign";loads=0;
    shellPresent=meshPresent=markerPresent=true;replacement=resourceThrows=false;dependenciesPresent=true;dependencyAlt=altEnabled=false;
}
int main(){
    for(int content:{CW_BOTTLE_MUSHROOM,CW_BOTTLE_PRINCESS,CW_BOTTLE_GOLD_DUST,CW_BOTTLE_SEAHORSE}){
        reset();assert(ComboBottleContents_Draw(&play,content)==1 && draws.size()==2);
        assert(draws.back().path=="__OTR__objects/combo_bottle_gi/BottleShell" && draws.back().owner==host);
        assert(same(draws.back().pose,initial) && same(pose,initial) && matrices.empty() && owners.empty() && gpu=="foreign");
        if(content==CW_BOTTLE_MUSHROOM){assert(loads==0 && draws[0].path=="__OTR__objects/combo_bottle_gi/polish/mushroom/gi_dl");assert(same(draws[0].pose,initial) && draws[0].owner==host);}
        if(content==CW_BOTTLE_PRINCESS)assert(draws[0].path=="__OTR__objects/combo_bottle_gi/polish/princess/gi_dl" && draws[0].owner==host && same(draws[0].pose,initial) && draws[0].stream==1);
        if(content==CW_BOTTLE_SEAHORSE)assert(draws[0].path=="__OTR__objects/combo_bottle_gi/polish/seahorse/gi_dl" && draws[0].owner==host && draws[0].stream==0);
        if(content==CW_BOTTLE_GOLD_DUST)assert(draws[0].vertices==672 && draws[0].owner==host);
    }
    for(int content:{CW_BOTTLE_MUSHROOM,CW_BOTTLE_PRINCESS,CW_BOTTLE_SEAHORSE}){
        for(int missing=0;missing<3;++missing){
            reset();shellPresent=missing!=0;meshPresent=missing!=1;resourceThrows=missing==2;
            assert(!ComboBottleContents_Draw(&play,content));
            assert(draws.empty() && same(pose,initial) && matrices.empty() && cpuOwner==0);
        }
        reset();dependenciesPresent=false;
        assert(!ComboBottleContents_Draw(&play,content) && draws.empty());
        reset();dependenciesPresent=false;dependencyAlt=true;
        assert(!ComboBottleContents_Draw(&play,content) && draws.empty());
        altEnabled=true;
        assert(ComboBottleContents_Draw(&play,content) && draws.size()==2);
        reset();replacement=true;shellPresent=meshPresent=false;
        assert(ComboBottleContents_Draw(&play,content) && draws.size()==1);
        assert(draws[0].path==ComboBottleContents_Marker(content) && draws[0].owner==host);
        assert(gpu=="foreign" && owners.empty() && same(pose,initial));
        reset();replacement=true;markerPresent=false;
        assert(!ComboBottleContents_Draw(&play,content) && draws.empty());
    }
    for(int content:{CW_BOTTLE_MUSHROOM,CW_BOTTLE_PRINCESS,CW_BOTTLE_GOLD_DUST,CW_BOTTLE_SEAHORSE}){
        for(size_t capacity=0;capacity<512;++capacity){
            reset(capacity);assert(ComboBottleContents_Draw(&play,content));
            assert(gfx.polyOpa.p<=gfx.polyOpa.d && gfx.polyXlu.p<=gfx.polyXlu.d && matrices.empty());
        }
    }
    // At just enough room for a child without its shell, reject that child and
    // draw the casing. The composition must never underflow the native allocator.
    reset(680);assert(ComboBottleContents_Draw(&play,CW_BOTTLE_GOLD_DUST));
    assert(draws.size()==1 && draws.back().path.find("BottleShell")!=std::string::npos);
    assert(gfx.polyOpa.p<=gfx.polyOpa.d && gfx.polyXlu.p<=gfx.polyXlu.d);
    for(int id:{0,1}){shimmers=0;const uint8_t color[]={255,255,255,255};DrawOotBottleWithShimmer(id,color);assert(shimmers==1);}
    assert(!ComboBottleContents_Draw(nullptr,CW_BOTTLE_GOLD_DUST));
    assert(!ComboBottleContents_Draw(&play,0));
    const auto gold=ComboBottleContents_Gold(42,{});
    for(size_t i=0;i<gold.count;++i){const auto& p=gold.vertices[i].p;assert(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z));assert(std::abs(p.x)<=12&&std::abs(p.z)<=12&&p.y>=-23.01f&&p.y<5);}
}
