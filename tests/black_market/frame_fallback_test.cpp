// Actual winning legacy draw bodies execute; resource I/O and GPU are boundaries.
#include "combo/BlackMarketFrame.h"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
namespace NeiGi { struct Point { float x, y, z; }; }
struct MtxF { float xx=1,xy=0,xz=0,xw=0,yx=0,yy=1,yz=0,yw=0,zx=0,zy=0,zz=1,zw=0; } matrix;
void Matrix_Get(MtxF* out) { *out=matrix; }
using f32=float; using s16=int16_t; using u8=uint8_t; using u32=uint32_t; using Gfx=char;
struct GetItemEntry {};
struct PlayState { struct { void* gfxCtx; } state; uint32_t gameplayFrames=42; } play;
PlayState* gPlayState=&play;
constexpr int MTXMODE_APPLY=1;
#ifndef M_PIf
constexpr float M_PIf=3.14159265358979323846f;
#endif
void Matrix_Scale(float x,float y,float z,int) {
    matrix.xx*=x;matrix.yx*=x;matrix.zx*=x;
    matrix.xy*=y;matrix.yy*=y;matrix.zy*=y;
    matrix.xz*=z;matrix.yz*=z;matrix.zz*=z;
}
void Matrix_RotateY(float a,int) {
    const auto old=matrix;const float c=std::cos(a),s=std::sin(a);
    matrix.xx=old.xx*c-old.xz*s;matrix.xz=old.xx*s+old.xz*c;
    matrix.yx=old.yx*c-old.yz*s;matrix.yz=old.yx*s+old.yz*c;
    matrix.zx=old.zx*c-old.zz*s;matrix.zz=old.zx*s+old.zz*c;
}
void Matrix_RotateX(float a,int) {
    const auto old=matrix;const float c=std::cos(a),s=std::sin(a);
    matrix.xy=old.xy*c+old.xz*s;matrix.xz=-old.xy*s+old.xz*c;
    matrix.yy=old.yy*c+old.yz*s;matrix.yz=-old.yy*s+old.yz*c;
    matrix.zy=old.zy*c+old.zz*s;matrix.zz=-old.zy*s+old.zz*c;
}
void Matrix_RotateZ(float a,int) {
    const auto old=matrix;const float c=std::cos(a),s=std::sin(a);
    matrix.xx=old.xx*c+old.xy*s;matrix.xy=-old.xx*s+old.xy*c;
    matrix.yx=old.yx*c+old.yy*s;matrix.yy=-old.yx*s+old.yy*c;
    matrix.zx=old.zx*c+old.zy*s;matrix.zy=-old.zx*s+old.zy*c;
}
void Matrix_Translate(float x,float y,float z,int) {
    matrix.xw+=matrix.xx*x+matrix.xy*y+matrix.xz*z;
    matrix.yw+=matrix.yx*x+matrix.yy*y+matrix.yz*z;
    matrix.zw+=matrix.zx*x+matrix.zy*y+matrix.zz*z;
}
#define Matrix_RotateXF Matrix_RotateX
#define OPEN_DISPS(...) ((void)0)
#define CLOSE_DISPS(...) ((void)0)
#define Gfx_SetupDL_25Opa(...) ((void)0)
#define Gfx_SetupDL_25Xlu(...) ((void)0)
#define Gfx_SetupDL25_Opa(...) ((void)0)
#define Gfx_SetupDL25_Xlu(...) ((void)0)
#define gSPMatrix(...) ((void)0)
#define gSPDisplayList(pkt,dl) ((void)(dl))
#define gSPSegment(...) ((void)0)
#define gDPPipeSync(...) ((void)0)
#define gDPSetPrimColor(...) ((void)0)
#define gDPSetEnvColor(...) ((void)0)
#define gDPSetGrayscaleColor(...) ((void)0)
#define gSPGrayscale(...) ((void)0)
#define MATRIX_FINALIZE_AND_LOAD(...) ((void)0)
alignas(2) const char gGiHoverBootsDL[]="__OTR__objects/object_gi_hoverboots/gGiHoverBootsDL";
alignas(2) const char gGiKokiriSwordDL[]="__OTR__objects/object_gi_sword_1/gGiKokiriSwordDL";
alignas(2) const char gGiIronBootsDL[]="__OTR__objects/object_gi_boots_2/gGiIronBootsDL";
alignas(2) const char gGiIronBootsRivetsDL[]="__OTR__objects/object_gi_boots_2/gGiIronBootsRivetsDL";
alignas(2) const char gPhantomGanonSkelLimbsLimb_00C610DL_009298[]="__OTR__objects/object_gnd/gPhantomGanonSkelLimbsLimb_00C610DL_009298";
alignas(2) const char dgNeiFourSwordBladeDL[]="__OTR__objects/object_nei_four_sword/gNeiFourSwordBladeDL";
alignas(2) const char dgNeiFourSwordHiltDL[]="__OTR__objects/object_nei_four_sword/gNeiFourSwordHiltDL";
struct Query { std::string owner;std::vector<std::string> roots; };
std::vector<Query> queries;
bool localCloth=true,realSpellAvailable=true;
extern "C" int ResourceMgr_GetGiModelBoundsForGame(const char* owner,const char* const* roots,int count,float low[3],float high[3]) {
    assert(count>0&&count<=2);queries.push_back({owner,{}});
    low[0]=-10;low[1]=-20;low[2]=-30;high[0]=50;high[1]=80;high[2]=90;
    for(int i=0;i<count;++i) {
        queries.back().roots.push_back(roots[i]);
        if(std::strstr(roots[i],"Wave"))high[1]=160;
        if(std::strstr(roots[i],"Rivets"))high[1]=120;
    }
    return 1;
}
int ResourceMgr_IsModAsset(const char*) { return localCloth; }
int NeiResource_Available(const char*) { return true; }
const char* NeiResource_Route(const char* path) {
    static std::string collar="__OTR__@oot:objects/object_gi_clothes/gGiTunicCollarDL";
    static std::string tunic="__OTR__@oot:objects/object_gi_clothes/gGiTunicDL";
    return std::strstr(path,"Collar")?collar.c_str():tunic.c_str();
}
Gfx* LoadNeiRealGfx(const char* path,Gfx**,u8*) { return realSpellAvailable?const_cast<Gfx*>(path):nullptr; }
Gfx* LoadNeiLegacyGfx(const char* path,bool) { return const_cast<Gfx*>(path); }
Gfx* ResourceMgr_LoadGfxByName(const char*) { static Gfx raw[16]{1};return raw; }
float ComboSwordGi_SelectedTilt(float tilt,bool) { return tilt; }
Gfx* Pegasus_GetRecoloredBootsDL() { return const_cast<Gfx*>(gGiHoverBootsDL); }
u32 ClimbBoots_YellowIronRamp(u32 color) { return color; }
u32 RocBoots_GoldRamp(u32 color) { return color; }
Gfx* BuildRecoloredGiDL(const char* path,u32(*)(u32),std::vector<Gfx>&) { return const_cast<Gfx*>(path); }
Gfx* BuildRecoloredOotGiDL(const char* path,u32(*)(u32),Gfx*,bool*) { return const_cast<Gfx*>(path); }
void DrawCustomItemDiamondTint(PlayState*,Gfx*,Gfx*,f32,u8,u8,u8) { assert(false); }
void DrawOotGetItemOpaTint(const char*,Gfx**,u8,u8,u8) { assert(false); }
void DrawOotMagicSpellSetup() {}
bool LoadOotMagicSpellDiamondOrb(Gfx** diamond,Gfx** orb) {
    *diamond=const_cast<Gfx*>("__OTR__objects/object_gi_goddess/gGiMagicSpellDiamondDL");
    *orb=const_cast<Gfx*>("__OTR__objects/object_gi_goddess/gGiMagicSpellOrbDL");return true;
}
void NeiGi_ReportBlackMarketModel(const char*,const char*,const char* = nullptr);
/* PRODUCTION_REPORTERS */
namespace Oot {
/* PRODUCTION_OOT */
}
namespace Mm {
/* PRODUCTION_MM */
}
static void Reset(float shelf=0.25f) {
    matrix={};matrix.xw=10;matrix.yw=20;matrix.zw=30;
    Matrix_Scale(shelf,shelf,shelf,MTXMODE_APPLY);queries.clear();
}
static void Near(float a,float b) { assert(std::abs(a-b)<.0002f); }
int main(int argc,char** argv) {
    assert(argc==2);const std::string test=argv[1];
    for(float shelf:{.25f,.32f}) {
        Reset(shelf);BlackMarketVisual::Capture capture;
        if(test=="oot-cape") {
            for(uint32_t frame:{0u,8u}) {
                Reset(shelf);play.gameplayFrames=frame;BlackMarketVisual::Capture pose;
                Oot::Randomizer_DrawExtMagicCape(&play,nullptr);
                assert(pose.frame.valid&&queries.size()==1&&queries[0].owner=="oot");
                assert((queries[0].roots[0].find("Wave")!=std::string::npos)==(frame==8));
                const float midY=frame==8?70.f:30.f;
                Near((pose.frame.minimum.y+pose.frame.maximum.y)*.5f,20+shelf*(22+.55f*midY));
            }
            continue;
        }
        if(test=="mm-cloth") {
            for(bool local:{true,false}) {
                Reset(shelf);localCloth=local;BlackMarketVisual::Capture pose;
                Mm::DrawOotTunicTint(240,244,250,true);
                assert(pose.frame.valid&&queries.size()==1&&queries[0].roots.size()==2);
                assert((queries[0].roots[0].find("@oot:")!=std::string::npos)==!local);
                Near((pose.frame.minimum.y+pose.frame.maximum.y)*.5f,20+shelf*30);
            }
            continue;
        }
        if(test=="mm-spell") {
            for(const char* path:{"__OTR__objects/object_nei_magic_spell/gHyliaGraceGiveDL",
                                  "__OTR__objects/object_nei_magic_spell/gZonaiPermafrostGiveDL",
                                  "__OTR__objects/object_nei_magic_spell/gDemiseDestructionGiveDL"}) {
                Reset(shelf);BlackMarketVisual::Capture pose;Gfx* cache=nullptr;u8 tried=0;
                Mm::DrawOotNeiSpellReal(path,&cache,&tried,10,20,30);
                assert(pose.frame.valid&&queries.size()==1&&queries[0].owner=="mm"&&queries[0].roots[0]==path);
                Near((pose.frame.minimum.y+pose.frame.maximum.y)*.5f,20+shelf*30);
            }
            continue;
        }
        if(test=="mm-spell-fallback") {
            realSpellAvailable=false;Gfx* cache=nullptr;u8 tried=0;
            Mm::DrawOotNeiSpellReal("__OTR__missing/real-spell",&cache,&tried,10,20,30);
            assert(capture.frame.valid&&queries.size()==1&&queries[0].owner=="mm"&&queries[0].roots.size()==2);
            assert(queries[0].roots[0].find("MagicSpellDiamond")!=std::string::npos&&
                   queries[0].roots[1].find("MagicSpellOrb")!=std::string::npos);
            Near((capture.frame.minimum.y+capture.frame.maximum.y)*.5f,20+shelf*30);continue;
        }
        play.gameplayFrames=42;localCloth=true;
        if(test=="oot-pegasus")Oot::Randomizer_DrawExtPegasusAnklet(&play,nullptr);
        else if(test=="oot-climb")Oot::Randomizer_DrawExtClimbBoots(&play,nullptr);
        else if(test=="oot-roc")Oot::Randomizer_DrawExtRocBoots(&play,nullptr);
        else if(test=="oot-trident")Oot::Randomizer_DrawExtTrident(&play,nullptr);
        else if(test=="mm-pegasus")Mm::DrawOotExtPegasusAnklet();
        else if(test=="mm-climb")Mm::DrawOotExtClimbBoots();
        else if(test=="mm-roc")Mm::DrawOotExtRocBoots();
        else if(test=="mm-trident")Mm::DrawOotExtTrident();
        else if(test=="oot-four")Oot::Randomizer_DrawExtFourSwordPresentation(&play,nullptr,true);
        else assert(false);
        assert(capture.frame.valid&&queries.size()==1);
        if(test=="oot-four") {
            assert(queries[0].roots.size()==2&&queries[0].roots[0]==dgNeiFourSwordBladeDL&&queries[0].roots[1]==dgNeiFourSwordHiltDL);
            Near((capture.frame.minimum.y+capture.frame.maximum.y)*.5f,20+shelf*.04f*20);
        } else if(test.find("trident")!=std::string::npos) {
            assert(queries[0].roots[0].find("PhantomGanon")!=std::string::npos);
            Near((capture.frame.minimum.y+capture.frame.maximum.y)*.5f,20+shelf*.00625f*(30-1485));
            Near(capture.frame.maximum.y-capture.frame.minimum.y,shelf*.00625f*120);
        } else {
            const bool climb=test.find("climb")!=std::string::npos;
            assert(queries[0].roots.size()==(climb?2u:1u));
            assert(queries[0].roots[0].find(climb?"IronBoots":"HoverBoots")!=std::string::npos);
            Near((capture.frame.minimum.y+capture.frame.maximum.y)*.5f,20+shelf*(climb?50:30));
        }
    }
    std::cout<<"PASS actual fallback capture "<<test<<": selected roots, final pose and .25/.32 scales\n";
}
