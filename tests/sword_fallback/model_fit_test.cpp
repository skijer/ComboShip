#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include "combo/NeiGiModelBounds.h"
#include <fast/lus_gbi.h>
#ifdef MM_BUILD_DLL
#include "mm/2s2h/resource/type/Array.h"
#else
#include "soh/soh/resource/type/Array.h"
#endif
#include <fast/resource/type/Matrix.h>
#include "combo/menu/ComboItemDrawABI.h"
// Resource loading is the test boundary. Actual Fast/SOH resource payloads and the
// production graph traversal/FrameFit execute below, on supplied mod archives.
namespace Ship {
IResource::IResource(std::shared_ptr<ResourceInitData> init) : mInitData(init) {}
IResource::~IResource() = default;
std::shared_ptr<ResourceInitData> IResource::GetInitData() { return mInitData; }
}
namespace Fast {
DisplayList::DisplayList() : Resource(nullptr) {}
DisplayList::~DisplayList() = default;
Gfx* DisplayList::GetPointer() { return Instructions.data(); }
size_t DisplayList::GetPointerSize() { return Instructions.size()*sizeof(Gfx); }
Vertex::Vertex() : Resource(nullptr) {}
Vtx* Vertex::GetPointer() { return VertexList.data(); }
size_t Vertex::GetPointerSize() { return VertexList.size()*sizeof(Vtx); }
Matrix::Matrix() : Resource(nullptr) {}
Mtx* Matrix::GetPointer() { return &Matrx; }
size_t Matrix::GetPointerSize() { return sizeof(Mtx); }
}
struct Loader {
    std::map<std::string,std::shared_ptr<Ship::IResource>> files;
    std::map<uint64_t,std::string> names;
    std::shared_ptr<Ship::IResource> operator()(const char* name) const {
        if (!name) return nullptr;
        if (!std::strncmp(name,"__OTR__",7)) name+=7;
        auto it=files.find(name);return it==files.end()?nullptr:it->second;
    }
    std::shared_ptr<Ship::IResource> operator()(uint64_t hash) const {
        auto it=names.find(hash);return it==names.end()?nullptr:(*this)(it->second.c_str());
    }
} loader;
std::set<std::string> available;
bool dinEnabled;
void List(const char* name,uint64_t hash,std::initializer_list<std::pair<uintptr_t,uintptr_t>> words) {
    auto dl=std::make_shared<Fast::DisplayList>();dl->UCode=ucode_f3dex2;
    for(auto [w0,w1]:words){Gfx command{};command.words.w0=w0;command.words.w1=w1;dl->Instructions.push_back(command);}
    loader.files[name]=dl;loader.names[hash]=name;
}
void Vertex(const char* name,uint64_t hash,std::initializer_list<std::array<int16_t,3>> points) {
    auto vertices=std::make_shared<Fast::Vertex>();
    for(auto p:points){Vtx vertex{};std::copy(p.begin(),p.end(),vertex.v.ob);vertices->VertexList.push_back(vertex);}
    loader.files[name]=vertices;loader.names[hash]=name;
}
void LegacyVertex(const char* name,uint64_t hash,std::initializer_list<std::array<int16_t,3>> points) {
    auto vertices=std::make_shared<SOH::Array>();
    vertices->ArrayType=SOH::ArrayResourceType::Vertex;
    vertices->ArrayScalarType=SOH::ScalarType::ZSCALAR_NONE;
    vertices->ArrayCount=points.size();
    for(auto p:points){Fast::F3DVtx vertex{};std::copy(p.begin(),p.end(),vertex.v.ob);vertices->Vertices.push_back(vertex);}
    loader.files[name]=vertices;loader.names[hash]=name;
}
using f32=float;using s16=int16_t;
constexpr int MTXMODE_APPLY=1;
struct PlayState {struct{void* gfxCtx;} state;uint32_t gameplayFrames=42;} play;
struct Pose {float scale=1,lift=0,ry=0,rz=0;} pose;
void Matrix_Translate(float,float y,float,int) {pose.lift+=y*pose.scale;}
void Matrix_Scale(float x,float y,float z,int) {assert(x==y&&y==z);pose.scale*=x;}
void Matrix_RotateY(float a,int) {pose.ry+=a;}
void Matrix_RotateZ(float a,int) {pose.rz+=a;}
#include "combo/menu/ComboSwordGiFit.h"
extern "C" int ResourceMgr_GetGiModelFitForGame(const char*,const char* path,float scale,float tilt,int shop,float out[2]) {
    const char* resource=!std::strncmp(path,"__OTR__",7)?path+7:path;
    const int profile=DinSwordGi::SelectedProfile(resource,dinEnabled,true,[](const char* key){
        if(!std::strncmp(key,"__OTR__",7))key+=7;
        return available.contains(key);
    });
    NeiGi::ShopFit fit{};
    if(!NeiGi::SelectedModelFit(loader,path,scale,tilt,shop,profile,fit))return 0;
    out[0]=fit.scale;out[1]=fit.lift;return 1;
}
#define OPEN_DISPS(...) ((void)0)
#define CLOSE_DISPS(...) ((void)0)
#define Gfx_SetupDL_25Opa(...) ((void)0)
#undef gDPSetPrimColor
#undef gDPSetEnvColor
#undef gSPGrayscale
#undef gSPMatrix
#undef gDma1p
#define gDPSetPrimColor(...) ((void)0)
#define gDPSetEnvColor(...) ((void)0)
#define gSPGrayscale(...) ((void)0)
#define gSPMatrix(...) ((void)0)
#define gDma1p(...) ((void)0)
#define CVAR_ENHANCEMENT(x) x
int CVarGetInteger(const char*,int){return 0;}
void ComboDinSwordGi_DrawLayers(PlayState*,const char*,const char*) {}
enum RandomizerGet{RG_KOKIRI_SWORD,RG_RAZOR_SWORD,RG_GILDED_SWORD,RG_TRUE_MASTER_SWORD,RG_MASTER_SWORD,RG_BIGGORON_SWORD,RG_GREAT_FAIRY_SWORD,RG_GIANTS_KNIFE};
int32_t OOT_NeiAltAssetsEnabled(){return 1;}
int32_t OOT_NeiResourceExists(const char* path){return bool(loader(path));}
void NeiGi_ReportBlackMarketModel(const char*,const char*,const char* = nullptr) {}
/* PRODUCTION */
struct Expected {
    const char* path;
    float low,high,width,layerLow,layerHigh,layerWidth;
    int dinProfile;
    float tilt=1.8f,scale=.04f;
    // Archive.mesh independently applies serialized resource matrices before
    // these coordinates receive the production GI pose and fitted scale.
    // The no-archive synthetic fixture leaves them empty.
    std::vector<std::array<double,3>> points,layerPoints;
};
std::vector<Expected> expected;
/* ARCHIVES */
int main(){
    InitArchive();
    // The interpreter caches vertex pointers in binary hash packets. Read
    // offsets against the current resource, and reject stale/misaligned data.
    Vertex("guard/vertices",99,{{10,-10,0},{20,20,2},{30,30,3}});
    auto v=std::dynamic_pointer_cast<Fast::Vertex>(loader("guard/vertices"));
    List("guard/cached",100,{{uintptr_t(G_VTX_OTR_HASH)<<24 | 3u<<12,uintptr_t(v->GetPointer())},
                              {0,99},{uintptr_t(G_ENDDL)<<24,0}});
    NeiGi::FrameBounds guard{};
    NeiGi::ModelBoundsReader<Loader> cached(loader,0);
    assert(cached.Read("guard/cached",guard)&&guard.minimum.y==-10&&guard.maximum.y==30);
    NeiGi::Point exactLow{},exactHigh{};
    NeiGi::ModelBoundsReader<Loader,true> exact(loader,0);
    assert(!exact.ExactBounds(exactLow,exactHigh));
    assert(exact.Read("guard/cached",guard)&&exact.ExactBounds(exactLow,exactHigh));
    assert(exactLow.x==10&&exactHigh.x==30&&exactLow.y==-10&&exactHigh.y==30&&
           exactLow.z==0&&exactHigh.z==3&&guard.minimum.x==0&&guard.maximum.z==0);
    // Binary O2R sword vertices use the registered SOH_Array factory, not
    // Fast::Vertex. Preserve that type before and after interpreter caching.
    LegacyVertex("guard/legacyVertices",105,{{10,-10,0},{20,20,2},{30,30,3}});
    auto legacy=std::dynamic_pointer_cast<SOH::Array>(loader("guard/legacyVertices"));
    List("guard/legacy",106,{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("guard/legacyVertices")},{3,0},
                            {uintptr_t(G_ENDDL)<<24,0}});
    NeiGi::ModelBoundsReader<Loader> legacyReader(loader,0);
    assert(legacyReader.Read("guard/legacy",guard)&&guard.minimum.y==-10&&guard.maximum.y==30 &&
           "binary SOH_Array vertices must receive the same fit as XML Fast::Vertex");
    for(auto offset:{uintptr_t(sizeof(Vtx)),uintptr_t(legacy->GetPointer())+sizeof(Vtx)}) {
        List("guard/legacyHash",107,{{uintptr_t(G_VTX_OTR_HASH)<<24 | 2u<<12,offset},{0,105},
                                    {uintptr_t(G_ENDDL)<<24,0}});
        NeiGi::ModelBoundsReader<Loader> legacyHash(loader,0);
        assert(legacyHash.Read("guard/legacyHash",guard)&&guard.minimum.y==20&&guard.maximum.y==30);
    }
    auto legacyDl=std::dynamic_pointer_cast<Fast::DisplayList>(loader("guard/legacyHash"));
    ++legacyDl->Instructions[0].words.w1;
    NeiGi::ModelBoundsReader<Loader> misalignedLegacy(loader,0);
    assert(!misalignedLegacy.Read("guard/legacyHash",guard));
    legacyDl->Instructions[0].words.w1=3*sizeof(Vtx);
    NeiGi::ModelBoundsReader<Loader> pastLegacyEnd(loader,0);
    assert(!pastLegacyEnd.Read("guard/legacyHash",guard));
    legacy->ArrayType=SOH::ArrayResourceType::Scalar;
    NeiGi::ModelBoundsReader<Loader> scalarArray(loader,0);
    assert(!scalarArray.Read("guard/legacy",guard));
    legacy->ArrayType=SOH::ArrayResourceType::Vertex;
    ++legacy->ArrayCount;
    NeiGi::ModelBoundsReader<Loader> malformedArray(loader,0);
    assert(!malformedArray.Read("guard/legacy",guard));
    --legacy->ArrayCount;
    auto cachedDl=std::dynamic_pointer_cast<Fast::DisplayList>(loader("guard/cached"));
    ++cachedDl->Instructions[0].words.w1;
    NeiGi::ModelBoundsReader<Loader> stale(loader,0);
    assert(!stale.Read("guard/cached",guard));
    List("guard/branch",101,{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("guard/vertices")},{3,0},
                             {uintptr_t(G_BRANCH_Z_OTR)<<24,0},{uintptr_t(G_ENDDL)<<24,0},
                             {uintptr_t(G_ENDDL)<<24,0}});
    NeiGi::ModelBoundsReader<Loader> branch(loader,0);
    assert(!branch.Read("guard/branch",guard)&&"branch payload must not become a premature geometry end");
    List("guard/recursive",102,{{uintptr_t(G_DL_OTR_FILEPATH)<<24,uintptr_t("guard/recursive")},
                               {uintptr_t(G_ENDDL)<<24,0}});
    NeiGi::ModelBoundsReader<Loader> recursive(loader,0);
    assert(!recursive.Read("guard/recursive",guard));
    // Actual XML replacements can bake a scale/translation in their DL. Fit
    // transformed vertices, with modelview state shared across child calls.
    auto matrix=std::make_shared<Fast::Matrix>();
    const float values[16]={.5f,0,0,0, 0,.5f,0,0, 0,0,.5f,0, 0,20,0,1};
    auto* words=reinterpret_cast<uint32_t*>(&matrix->Matrx);
    for(int i=0;i<8;++i) {
        const uint32_t a=int32_t(values[i*2]*65536.f),b=int32_t(values[i*2+1]*65536.f);
        words[i]=(a&0xffff0000u)|(b>>16);words[i+8]=(a<<16)|(b&0xffffu);
    }
    loader.files["guard/matrix"]=matrix;loader.names[103]="guard/matrix";
    List("guard/transformed",104,{{uintptr_t(G_MTX_OTR_FILEPATH)<<24,uintptr_t("guard/matrix")},
      {uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("guard/vertices")},{3,0},
      {uintptr_t(G_POPMTX)<<24,64},{uintptr_t(G_ENDDL)<<24,0}});
    NeiGi::ModelBoundsReader<Loader> transformed(loader,0);
    assert(transformed.Read("guard/transformed",guard) && guard.minimum.y==15 && guard.maximum.y==35 &&
           "serialized resource matrices must establish selected model bounds");
    NeiGi::ModelBoundsReader<Loader,true> exactTransformed(loader,0);
    assert(exactTransformed.Read("guard/transformed",guard)&&exactTransformed.ExactBounds(exactLow,exactHigh));
    assert(exactLow.x==5&&exactHigh.x==15&&exactLow.y==15&&exactHigh.y==35&&
           exactLow.z==0&&exactHigh.z==1.5f&&exactTransformed.RestoresModelView());
    size_t cameraMeshes=0;
    for(const auto& e:expected){
        const int profile=DinSwordGi::SelectedProfile(e.path+7,true,true,[](const char* key){
            if(!std::strncmp(key,"__OTR__",7))key+=7;return available.contains(key);});
        assert(profile==e.dinProfile && "Din eligibility disagrees with the actual supplied archive");
        NeiGi::FrameBounds bounds{};NeiGi::ModelBoundsReader<Loader> reader(loader,e.tilt);
        assert(reader.Read(e.path,bounds));
        assert(reader.RestoresModelView());
        assert(std::abs(bounds.minimum.y-e.low)<.01f&&std::abs(bounds.maximum.y-e.high)<.01f);
        assert(std::abs(bounds.spinningWidth-e.width)<.01f);
        for(bool din:{false,true})for(bool shop:{false,true})for(uint32_t frame=0;frame<360;++frame){
            dinEnabled=din;
            pose={};play.gameplayFrames=frame;
            if(e.tilt!=0.f)NeiGi_DrawSelectedSword(&play,e.path,shop);
            else {ComboSwordGi_ApplyPresentationSize(shop);ComboSwordGi_ApplyFit("mm",e.path,e.scale,e.tilt,shop);Matrix_Scale(e.scale,e.scale,e.scale,1);}
            const float actualTilt=e.tilt;
            assert(std::abs(pose.rz-actualTilt)<.0001f);
            NeiGi::FrameBounds selected{};
            assert(NeiGi::SelectedModelBounds(loader,e.path,actualTilt,din?profile:0,selected));
            const float low=selected.minimum.y, high=selected.maximum.y, width=selected.spinningWidth;
            {
                NeiGi::FrameBounds original{"control",{0,low,0},{0,high,0},width,NeiGi::Kind::Neutral,{}};
                const auto fitted=NeiGi::FrameFit(original,e.scale,shop);
                assert(std::abs(pose.scale/(e.scale*fitted.scale)-(shop?1.f:1.15f))<.0001f &&
                       "sword pickups must enlarge after fitting while the accepted shelf size stays unchanged");
            }
            assert(low*pose.scale+pose.lift >= (shop?-22.f:-59.5f)-.001f);
            assert(high*pose.scale+pose.lift <= (shop?52.f:55.5f)+.001f);
            assert(width*pose.scale <= (shop?76.f:1.15f*104.f)+.001f);
        }
        if(e.points.empty())
            continue; // Synthetic resource fitting supplies no archive camera evidence.
        ++cameraMeshes;
        // Camera_KeepOn4's upright Item0 rows, with CustomItem's visible Y
        // including its accepted +2 lift and .007 * 900 shape offset.
        struct Receipt {int context;float origin,pitch,at,distance,fov,forward;};
        const Receipt receipts[]={{1,53.3f,25.f,46.8f,39.2f,45.f,0.f},
          {2,98.3f,55.f,94.8f,33.3f,55.f,12.f},
          {1,83.3f,30.f,74.8f,47.6f,42.f,4.f},
          {1,43.3f,-8.f,28.2f,46.8f,60.f,0.f},
          {1,108.3f,40.f,95.2f,33.6f,80.f,6.f}};
        double peakProjection=0;
        for(bool din:{false,true})for(const auto& receipt:receipts) {
            dinEnabled=din;
            pose={.21f,receipt.origin,0,0};
            ComboSwordGi_ApplyPresentationSize(false,receipt.context);
            ComboSwordGi_ApplyFit("oot",e.path,e.scale,e.tilt,false,receipt.context);
            if(e.tilt!=0.f)NeiGi_DrawSelectedSword(&play,e.path,false,false);
            else Matrix_Scale(e.scale,e.scale,e.scale,1);
            const double pitch=receipt.pitch*std::acos(-1.)/180.;
            const double sp=std::sin(pitch),cp=std::cos(pitch);
            const double tanFov=std::tan(receipt.fov*std::acos(-1.)/360.);
            const double sinTilt=std::sin(double(pose.rz)),cosTilt=std::cos(double(pose.rz));
            const auto project=[&](const auto& points,const char* part) {
                for(size_t vertex=0;vertex<points.size();++vertex) {
                    const auto& p=points[vertex];
                    const double x=p[0]*cosTilt-p[1]*sinTilt;
                    const double y=p[0]*sinTilt+p[1]*cosTilt;
                    const double radius=std::hypot(x,p[2]);
                    const double dy=pose.lift+pose.scale*y-receipt.at;
                    const double a=receipt.distance-dy*sp-receipt.forward*cp;
                    const double b=pose.scale*radius*cp;
                    assert(a>std::abs(b) && "selected archive vertex crosses the receipt camera");
                    // For a fixed vertex Y, projected Y is monotonic in Z.
                    // Its exact full-spin extremes occur at this vertex's
                    // own +/-radius, not the largest radius elsewhere in the mesh.
                    for(double side:{-1.,1.}) {
                        const double z=receipt.forward+side*pose.scale*radius;
                        const double depth=receipt.distance-dy*sp-z*cp;
                        const double projected=(dy*cp-z*sp)/(depth*tanFov);
                        peakProjection=std::max(peakProjection,std::abs(projected));
                        if (!(projected<.99 && projected>-.99))
                            std::cerr<<e.path<<" part="<<part<<" vertex="<<vertex<<" context="<<receipt.context
                                     <<" origin="<<receipt.origin<<" y="<<y<<" radius="<<radius
                                     <<" ndcY="<<projected<<" depth="<<depth<<'\n';
                        assert(projected<.99 && projected>-.99 && "selected archive geometry clips MM receipt camera");
                    }
                    // Maximize x/(a-cp*z) around x*x+z*z=(scale*radius)^2.
                    // This covers continuous yaw and the 4:3 horizontal FOV.
                    const double projectedX=pose.scale*radius/(tanFov*(4./3.)*std::sqrt(a*a-b*b));
                    peakProjection=std::max(peakProjection,projectedX);
                    if (!(projectedX<.99))
                        std::cerr<<e.path<<" part="<<part<<" vertex="<<vertex<<" context="<<receipt.context
                                 <<" ndcX="<<projectedX<<'\n';
                    // Keep at least one percent of the viewport in both axes.
                    assert(projectedX<.99 && "selected archive geometry clips MM receipt camera horizontally");
                }
            };
            project(e.points,"model");
            if(din)
                project(e.layerPoints,"Din layers");
        }
        std::cout<<e.path<<" peak receipt projection="<<peakProjection<<'\n';
    }
    for(auto id:{RG_KOKIRI_SWORD,RG_RAZOR_SWORD,RG_GILDED_SWORD,RG_MASTER_SWORD,RG_TRUE_MASTER_SWORD,RG_BIGGORON_SWORD,RG_GREAT_FAIRY_SWORD,RG_GIANTS_KNIFE}){
        CwItemDrawInfo info{};
        if(CwAltSwordGi(id,&info)){
            assert(loader(info.dlists[0])&&info.itemShimmer&&info.neiShimmer>0);
            pose={};NeiGi_DrawSelectedSword(&play,info.dlists[0]);
            NeiGi::FrameBounds bounds{};NeiGi::ModelBoundsReader<Loader> reader(loader,1.5707963267948966f);assert(reader.Read(info.dlists[0],bounds));
            assert(bounds.maximum.y*pose.scale+pose.lift<=55.501f);
        }
    }
    std::cout<<"PASS selected resource graph + native/producer GI route: "<<expected.size()
             <<" meshes, upright full-spin pickup/shop bounds including Din layers";
    if(cameraMeshes) {
        assert(cameraMeshes==expected.size());
        std::cout<<"; actual archive vertices in all five MM receipt cameras\n";
    } else {
        std::cout<<"; synthetic fit fixture (archive camera gate skipped)\n";
    }
}
