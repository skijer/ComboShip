// Execute the real MM native-fallback export and OoT OPS consumer.
// Catalog, resource query and graphics services are fixture boundaries.
#include "combo/menu/ComboItemDrawABI.h"
#include "combo/menu/ComboCollectibleShimmer.h"
#include "combo/menu/ComboElementalArrowGi.h"
#include "mm/2s2h/Rando/Types.h"
#include "mm/include/z64item.h"
#include "soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include "mm/assets/objects/object_gi_shield_3/object_gi_shield_3.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <string>
#include <vector>
using s16=int16_t; using s32=int32_t; using f32=float; using u8=uint8_t;
#define COMBO_EXPORT
namespace Ship {
inline bool ownerActive=false;
struct CrossRMRegistry {
 static int Get(const char* game) { assert(!std::strcmp(game,"mm")); return 1; }
};
struct ResourceManagerScope {
 bool before=ownerActive;
 explicit ResourceManagerScope(int owner) { assert(owner==1); ownerActive=true; }
 ~ResourceManagerScope() { ownerActive=before; }
};
}
namespace ItemGrantAudit { struct Scope { Scope(const char*,int,int,bool){} }; }
bool flat=true, sideways=false, readable=true, authored=false;
int queryCalls=0;
int ResourceMgr_GetIkanaShieldGiTiltXForGame(const char* owner,const char* path,float* tilt) {
    ++queryCalls;
    assert(Ship::ownerActive);
    assert(!std::strcmp(owner,"mm")&&!std::strcmp(path,"objects/object_gi_shield_3/gGiMirrorShieldDL"));
    if(!readable)return 0;
    *tilt=flat?1.5707963267948966f:0.f;return 1;
}
int ResourceMgr_GetIkanaShieldGiPoseForGame(const char* owner,const char* path,float* tilt,float* roll) {
    if(!ResourceMgr_GetIkanaShieldGiTiltXForGame(owner,path,tilt))return 0;
    *roll=sideways?-1.5707963267948966f:0.f;return 1;
}
namespace Rando {
RandoItemId ConvertItem(RandoItemId id){return id;}
RandoItemId CurrentJunkItem(){return RI_SHIELD_MIRROR;}
RandoItemId CurrentTrapItem(){return RI_SHIELD_MIRROR;}
namespace StaticData {
struct Item { const char* name="Shield of Ikana"; int drawId=1; };
std::map<RandoItemId,Item> Items{{RI_SHIELD_MIRROR,{}},{RI_SHIELD_HERO,{"Hero's Shield",2}}};
RandoItemId GetItemIdFromDisplayName(const char* name) {
    return !std::strcmp(name,"Shield of Ikana")?RI_SHIELD_MIRROR:RI_UNKNOWN;
}
RandoItemId GetItemIdFromName(const char* name){return GetItemIdFromDisplayName(name);}
} }
int DungeonItem_GetOwner(RandoItemId){return -1;}
bool MM_HasAnimDraw(RandoItemId){return false;}
bool MM_DescribeNeiGi(RandoItemId,CwItemDrawInfo* out) {
    out->neiShimmer=17;
    if(!authored)return false;
    out->drawKind=CW_DRAW_KIND_NEI_GI;out->dlistCount=1;
    out->dlists[0]="__OTR__objects/nei_gi_redesign/shield_of_ikana/gi_dl";
    return true;
}
int MM_FillImportedSwordFallback(RandoItemId,CwItemDrawInfo*){return 0;}
int MM_FillDungeonKeyModelInfo(RandoItemId,s16,CwItemDrawInfo*){return 0;}
int MM_FillDungeonTintInfo(RandoItemId,s16,CwItemDrawInfo*){return 0;}
int MM_FillSpinAttackGi(CwItemDrawInfo*){return 0;}
int MM_FillSongDrawInfo(RandoItemId,CwItemDrawInfo*){return 0;}
int MM_FillOpsDrawInfo(RandoItemId,CwItemDrawInfo*){return 0;}
int MM_FillSimpleDrawInfo(RandoItemId,CwItemDrawInfo*){return 0;}
int MM_FillEnemySoulDrawInfo(RandoItemId,CwItemDrawInfo*){return 0;}
int MM_FillGidAliasDrawInfo(RandoItemId,CwItemDrawInfo*){return 0;}
int MM_OotBottleShimmerColor(RandoItemId,u8*){return 0;}
const char* gGiMoonsTearItemDL="tear",*gGiMoonsTearTexAnim="tear material",*gGiFairyBottleTexAnim="fairy material";
float selectedScale=0;
int GetItem_GetDrawTableEntry(int,void** dls,int,int* xlu,float* scale,int* scroll,int* kind) {
    /* NATIVE_SHIELD_ROW */
    *xlu=1;*scale=selectedScale;*scroll=0;*kind=CW_DRAW_KIND_SIMPLE;return 2;
}
void GetItem_GetDrawSetupDLs(int,void**,void**){}
int GetItem_GetShimmerColor(s16,u8*){return 0;}
/* MM_PRODUCER */

struct Gfx{};struct GraphicsContext{};struct MtxF{};
struct PlayState {struct {GraphicsContext* gfxCtx;} state;MtxF billboardMtxF;};
GraphicsContext gfx;PlayState play{{&gfx},{}};
/* CONSUMER_INFO */
Gfx commands[128];Gfx* outputOpa=commands;Gfx* outputXlu=commands+64;
constexpr int MTXMODE_APPLY=1,kMaxMatEntries=8;
float rotationX=0,rotationZ=0,scale=1;
struct Pose {float x,z,scale;};
std::vector<Pose> stack;
std::vector<float> heights,chins;
std::vector<int> rotations;
std::vector<int> streams;
std::vector<std::string> paths;
void Matrix_Push(){stack.push_back({rotationX,rotationZ,scale});}
void Matrix_Pop(){assert(!stack.empty());rotationX=stack.back().x;rotationZ=stack.back().z;scale=stack.back().scale;stack.pop_back();}
void Matrix_RotateZYX(s16 x,s16 y,s16 z,int){
    assert(!y&&(!x||!z));
    rotationX+=x*(3.14159265358979323846f/32768.f);
    rotationZ+=z*(3.14159265358979323846f/32768.f);
    rotations.push_back(z?CW_OP_ROTATE_Z:CW_OP_ROTATE_X);
}
void Matrix_Scale(float x,float y,float z,int){assert(x==y&&y==z);scale*=x;}
void Matrix_Translate(float,float,float,int){}
void Matrix_ReplaceRotation(MtxF*){}
void Gfx_SetupDL_25Opa(GraphicsContext*){}
void Gfx_SetupDL_25Xlu(GraphicsContext*){}
void Submit(Gfx* command,const char* path) {
    paths.emplace_back(path);streams.push_back(command>=commands+64);
    const float x=sideways?99.f:68.f,vertical=sideways?68.f:99.f;
    const float y=flat?2.f:vertical,z=flat?vertical:2.f;
    const float tiltedY=std::abs(std::cos(rotationX))*y+std::abs(std::sin(rotationX))*z;
    heights.push_back(scale*(std::abs(std::sin(rotationZ))*x+std::abs(std::cos(rotationZ))*tiltedY));
    chins.push_back(sideways?std::sin(rotationZ):std::cos(rotationZ)*(flat?-std::sin(rotationX):-std::cos(rotationX)));
}
void ComboForeignTexAnim_Run(PlayState*,const char*,const char*,bool,int*,int*){assert(false);}
void ComboForeignTexAnim_Restore(PlayState*,int*,int count,bool){assert(count==0);}
void OOT_RestoreForeignSegs(PlayState*,int*,int){assert(false);}
#define OPEN_DISPS(x) ((void)(x))
#define CLOSE_DISPS(x) ((void)(x))
#define POLY_OPA_DISP outputOpa
#define POLY_XLU_DISP outputXlu
#define OOT_FOREIGN_PIN_OPA() ((void)0)
#define OOT_FOREIGN_PIN_XLU() ((void)0)
#define COMBO_FOREIGN_MTX(p) ((void)(p))
#define gSPDisplayList(p,path) Submit(p,(const char*)(path))
#define gSPGrayscale(p,on) ((void)(p),(void)(on))
#define gSPSegment(p,...) ((void)(p))
#define gDPSetPrimColor(p,...) ((void)(p))
#define gDPSetEnvColor(p,...) ((void)(p))
#define gDPSetGrayscaleColor(p,...) ((void)(p))
/* OOT_CONSUMER */
void Render(const CwItemDrawInfo& out) {
    // The cross-DLL transport copies these fields and prefixes native paths @mm.
    ComboForeignDrawInfo info{};info.count=out.dlistCount;info.xluStart=out.xluStartIndex;
    info.drawKind=out.drawKind;info.scale=out.scale;info.opCount=out.opCount;
    info.setupDlOpa=out.setupDlOpa;info.setupDlXlu=out.setupDlXlu;
    std::copy(std::begin(out.ops),std::end(out.ops),std::begin(info.ops));
    std::vector<std::string> routed;
    for(int i=0;i<out.dlistCount;++i)routed.emplace_back(std::string("__OTR__@mm:")+(out.dlists[i]+7));
    for(int i=0;i<out.dlistCount;++i)info.dls[i]=routed[i].c_str();
    outputOpa=commands;outputXlu=commands+64;rotationX=rotationZ=0;scale=1;heights.clear();chins.clear();rotations.clear();streams.clear();paths.clear();
    if(info.drawKind==CW_DRAW_KIND_OPS)OOT_DrawForeignOps(&play,&info);
    else OOT_DrawForeignSimple(&play,&info);
}
int main() {
    for(bool selectedSideways:{false,true})for(bool selectedFlat:{false,true})for(float modelScale:{0.f,.5f}) {
        sideways=selectedSideways;flat=selectedFlat;selectedScale=modelScale;
        CwItemDrawInfo out{};assert(MM_GetItemDrawInfo("Shield of Ikana",&out)==1);
        assert(out.dlistCount==2&&out.xluStartIndex==1&&out.neiShimmer==17);
        Render(out);
        assert(heights.size()==2&&streams==std::vector<int>({0,1}));
        for(float height:heights)assert(std::abs(height-99.f*(modelScale>0?modelScale:1.f))<.0001f);
        for(float chin:chins)assert(chin<-.99f&&"MM shield exported to OoT must face upright, chin down");
        std::vector<int> expectedRotations;
        if(sideways)expectedRotations.push_back(CW_OP_ROTATE_Z);
        if(flat)expectedRotations.push_back(CW_OP_ROTATE_X);
        assert(rotations==expectedRotations&&"roll must precede X tilt without adding caller spin");
        assert(paths[0].starts_with("__OTR__@mm:")&&paths[1].ends_with("gGiMirrorShieldDL"));
        assert(stack.empty()&&rotationX==0&&rotationZ==0);
        assert(out.stateDependent==2&&"selected shield pose must refresh after an owner Alt toggle");
    }
    readable=false;flat=true;sideways=false;selectedScale=0;
    CwItemDrawInfo out{};assert(MM_GetItemDrawInfo("Shield of Ikana",&out)==1);
    assert(out.drawKind==CW_DRAW_KIND_SIMPLE&&out.opCount==0);
    Render(out);assert(heights[0]<10&&stack.empty());
    readable=true;authored=true;const int calls=queryCalls;
    out={};assert(MM_GetItemDrawInfo("Shield of Ikana",&out)==1);
    assert(out.drawKind==CW_DRAW_KIND_NEI_GI&&out.opCount==0&&queryCalls==calls);
    authored=false;out={};assert(MM_FillItemDrawInfo(RI_SHIELD_HERO,&out)==1);
    assert(out.drawKind==CW_DRAW_KIND_SIMPLE&&out.opCount==0&&queryCalls==calls);
    std::cout<<"PASS actual MM shield export and OoT consumer: upright/flat/sideways, chin-down sign, rotation order, OPA/XLU, original scale, live pose, authored bypass and unreadable defaults\n";
}
