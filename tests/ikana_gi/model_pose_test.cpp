// Real resource-graph reader and owner query; registry/archive loading are seams.
#include "combo/NeiGiModelBounds.h"
#include <fast/resource/type/Texture.h>
#include <cassert>
#include <cstring>
#include <iostream>
#include <map>
#include <string>

namespace Ship {
class ResourceManager;
class Context {
  public:
    static Context* GetRawInstance();
    std::shared_ptr<ResourceManager> GetResourceManager(){return active;}
    void SetResourceManager(std::shared_ptr<ResourceManager> value){active=value;}
    std::shared_ptr<ResourceManager> active;
};
static Context context;
Context* Context::GetRawInstance(){return &context;}
class ResourceManager {
  public:
    std::shared_ptr<IResource> LoadResource(std::string path){
        assert(context.active.get()==this);
        if(path.starts_with("__OTR__"))path.erase(0,7);
        if(alt&&files.contains("alt/"+path))path="alt/"+path;
        return files.contains(path)?files[path]:nullptr;
    }
    std::shared_ptr<IResource> LoadResource(uint64_t hash){return hashes.contains(hash)?LoadResource(hashes[hash]):nullptr;}
    std::map<std::string,std::shared_ptr<IResource>> files;
    std::map<uint64_t,std::string> hashes;
    bool alt=false;
};
struct CrossRMRegistry {
    static std::shared_ptr<ResourceManager> Get(const std::string& game){return owners.contains(game)?owners[game]:nullptr;}
    static inline std::map<std::string,std::shared_ptr<ResourceManager>> owners;
};
} // namespace Ship
/* RESOURCE_SCOPE */
namespace NeiAssetPriority {
/* POSE_QUERY */
}
/* OWNER_QUERY */

void List(Ship::ResourceManager& owner,const char* path,std::initializer_list<std::pair<uintptr_t,uintptr_t>> words){
    auto list=std::make_shared<Fast::DisplayList>();list->UCode=ucode_f3dex2;
    for(auto [w0,w1]:words){Gfx cmd{};cmd.words.w0=w0;cmd.words.w1=w1;list->Instructions.push_back(cmd);}
    owner.files[path]=list;
}
void VertexPoints(Ship::ResourceManager& owner,const char* path,const std::vector<std::array<int16_t,3>>& points,bool binary){
    if(binary){
        auto vertices=std::make_shared<SOH::Array>();vertices->ArrayType=SOH::ArrayResourceType::Vertex;
        vertices->ArrayCount=points.size();
        for(auto point:points){Fast::F3DVtx v{};std::copy(point.begin(),point.end(),v.v.ob);vertices->Vertices.push_back(v);}
        owner.files[path]=vertices;
    }else{
        auto vertices=std::make_shared<Fast::Vertex>();
        for(auto point:points){Vtx v{};std::copy(point.begin(),point.end(),v.v.ob);vertices->VertexList.push_back(v);}
        owner.files[path]=vertices;
    }
}
void MatrixWords(Ship::ResourceManager& owner,const char* path,const std::array<uint32_t,16>& words){
    auto matrix=std::make_shared<Fast::Matrix>();std::copy(words.begin(),words.end(),reinterpret_cast<uint32_t*>(&matrix->Matrx));
    owner.files[path]=matrix;
}
struct ActualGeometry {
    const char* name;
    std::vector<std::array<int16_t,3>> points;
    std::array<uint32_t,16> matrix;
};
/* ACTUAL_GEOMETRY */
/* FADOS_GEOMETRY */
std::array<float,3> ApplyPose(std::array<float,3> p,float tilt,float roll){
    const float y=p[1]*std::cos(tilt)-p[2]*std::sin(tilt),z=p[1]*std::sin(tilt)+p[2]*std::cos(tilt);
    return {p[0]*std::cos(roll)-y*std::sin(roll),p[0]*std::sin(roll)+y*std::cos(roll),z};
}
void Vertices(Ship::ResourceManager& owner,const char* path,bool flat,bool binary){
    const int16_t y=flat?0:1000,z=flat?1000:20;
    std::array<std::array<int16_t,3>,4> points={{{-600,int16_t(-y),int16_t(-z)},{600,y,z},{-600,y,z},{600,int16_t(-y),int16_t(-z)}}};
    if(binary){
        auto vertices=std::make_shared<SOH::Array>();vertices->ArrayType=SOH::ArrayResourceType::Vertex;
        vertices->ArrayCount=points.size();
        for(auto point:points){Fast::F3DVtx v{};std::copy(point.begin(),point.end(),v.v.ob);vertices->Vertices.push_back(v);}
        owner.files[path]=vertices;
    }else{
        auto vertices=std::make_shared<Fast::Vertex>();
        for(auto point:points){Vtx v{};std::copy(point.begin(),point.end(),v.v.ob);vertices->VertexList.push_back(v);}
        owner.files[path]=vertices;
    }
}
int main(){
    auto oot=std::make_shared<Ship::ResourceManager>(),mm=std::make_shared<Ship::ResourceManager>();
    Ship::CrossRMRegistry::owners={{"oot",oot},{"mm",mm}};Ship::context.active=mm;
    constexpr char worn[]="objects/object_link_child/gLinkHumanMirrorShieldDL";
    constexpr char gi[]="objects/object_gi_shield_3/gGiMirrorShieldDL";
    for(auto owner:{oot,mm}){
        List(*owner,worn,{{uintptr_t(G_DL_OTR_FILEPATH)<<24,uintptr_t("body")},{uintptr_t(G_ENDDL)<<24,0}});
        List(*owner,gi,{{uintptr_t(G_DL_OTR_FILEPATH)<<24,uintptr_t("body")},{uintptr_t(G_ENDDL)<<24,0}});
        List(*owner,"body",{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("vertices")},{4,0},{uintptr_t(G_ENDDL)<<24,0}});
        owner->hashes[7]="vertices";
        List(*owner,"alt/body",{{(uintptr_t(G_VTX_OTR_HASH)<<24)|(4u<<12),0},{0,7},{uintptr_t(G_ENDDL)<<24,0}});
    }
    // Donor uses planar XZ geometry; MM is upright. Each Alt reverses its pose.
    Vertices(*oot,"vertices",true,false);Vertices(*oot,"alt/vertices",false,true);
    Vertices(*mm,"vertices",false,false);Vertices(*mm,"alt/vertices",true,true);
    for(bool hostAlt:{false,true})for(bool donorAlt:{false,true}){
        mm->alt=hostAlt;oot->alt=donorAlt;
        float tilt=-99,roll=-99;
        assert(ResourceMgr_GetIkanaShieldGiPoseForGame("oot",worn,&tilt,&roll)&&roll==0);
        assert(ResourceMgr_GetIkanaShieldGiTiltXForGame("oot",worn,&tilt));
        assert(std::abs(tilt-(donorAlt?0.f:float(M_PI/2)))<.0001f);
        assert(ResourceMgr_GetIkanaShieldGiPoseForGame("mm",gi,&tilt,&roll)&&roll==0);
        assert(ResourceMgr_GetIkanaShieldGiTiltXForGame("mm",gi,&tilt));
        assert(std::abs(tilt-(hostAlt?float(M_PI/2):0.f))<.0001f);
        assert(Ship::context.active==mm&&mm->alt==hostAlt&&oot->alt==donorAlt);
    }
    // Actual repository/O2R vertices and binary scale matrix, with quarter-turn
    // coordinate conversions. No mesh or matrix is rewritten in the source pack.
    for(const auto& geometry:actualGeometry)for(bool binary:{false,true}) {
        auto owner=oot;owner->alt=false;
        MatrixWords(*owner,"actual_scale",geometry.matrix);
        List(*owner,"actual_body",{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("actual_vertices")},
                                  {geometry.points.size(),0},{uintptr_t(G_ENDDL)<<24,0}});
        List(*owner,"actual_root",{{uintptr_t(G_MTX_OTR_FILEPATH)<<24,uintptr_t("actual_scale")},
                                  {uintptr_t(G_DL_OTR_FILEPATH)<<24,uintptr_t("actual_body")},
                                  {uintptr_t(G_POPMTX)<<24,64},{uintptr_t(G_ENDDL)<<24,0}});
        for(int frame:{0,1,2})for(int originOffset:{0,6000}) {
            auto points=geometry.points;
            // Rz(-90) matches the native worn decal: its chin points toward -X. The third frame
            // also puts the front normal in -Y, like an arm-local XZ shield.
            if(frame)for(auto& p:points){auto q=p;p={q[1],frame==1?int16_t(-q[0]):int16_t(-q[2]),frame==1?q[2]:int16_t(-q[0])};}
            for(auto& p:points){p[0]+=originOffset;p[1]-=originOffset/2;p[2]+=originOffset/3;}
            VertexPoints(*owner,"actual_vertices",points,binary);
            float tilt=-99,roll=-99,legacyTilt=-99;
            assert(ResourceMgr_GetIkanaShieldGiPoseForGame("oot","actual_root",&tilt,&roll));
            assert(std::abs(tilt-(frame==2?float(M_PI/2):0.f))<.00001f);
            assert(std::abs(roll-(frame?float(M_PI/2):0.f))<.00001f);
            assert(ResourceMgr_GetIkanaShieldGiTiltXForGame("oot","actual_root",&legacyTilt)&&legacyTilt==tilt);
            const auto chin=ApplyPose(frame?std::array<float,3>{-1,0,0}:std::array<float,3>{0,-1,0},tilt,roll);
            assert(chin[1]<-.999f&&std::abs(chin[0])<.00001f&&"face chin must point down, not up");
        }
        std::cout<<"PASS selected serialized Ikana vertices/matrix: "<<geometry.name<<(binary?" binary":" XML")<<"; upright, sidewaysXY/XZ, off-center origins and chin-down sign\n";
    }
    // The supplied Fados worn mesh has a -X chin inside a nearly round rim.
    // Its 2545/2460 span ratio is below the generic 5% sideways threshold.
    // Use the archive's exact 2290 vertices and all 76 load boundaries.
    if(!fadosPoints.empty())for(auto owner:{oot,mm})for(bool binary:{false,true}) {
        owner->alt=false;
        const char* game=owner==oot?"oot":"mm";
        VertexPoints(*owner,"fados_vertices",fadosPoints,binary);
        auto body=std::make_shared<Fast::DisplayList>();body->UCode=ucode_f3dex2;
        for(auto [offset,count]:fadosLoads){
            Gfx load{},args{};load.words.w0=uintptr_t(G_VTX_OTR_FILEPATH)<<24;
            load.words.w1=uintptr_t("fados_vertices");args.words.w0=count;args.words.w1=offset;
            body->Instructions.push_back(load);body->Instructions.push_back(args);
        }
        Gfx end{};end.words.w0=uintptr_t(G_ENDDL)<<24;body->Instructions.push_back(end);
        owner->files["fados_body"]=body;
        List(*owner,"fados_root",{{uintptr_t(G_DL_OTR_FILEPATH)<<24,uintptr_t("fados_body")},{uintptr_t(G_ENDDL)<<24,0}});
        float tilt=-99,roll=-99;
        assert(ResourceMgr_GetIkanaShieldGiPoseForGame(game,"fados_root",&tilt,&roll));
        assert(tilt==0 && ApplyPose({-1,0,0},tilt,roll)[1]<-.999f && "Fados pickup face must point down");
        std::cout<<"PASS supplied Fados selected mesh chin-down: "<<game<<(binary?" binary":" XML")<<"\n";

        // Live Alt selection must follow the chosen graph, not its filename.
        auto upright=fadosPoints;
        for(auto& p:upright){const auto q=p;p={int16_t(-q[1]),q[0],q[2]};}
        VertexPoints(*owner,"alt/fados_vertices",upright,binary);
        owner->alt=true;
        assert(ResourceMgr_GetIkanaShieldGiPoseForGame(game,"fados_root",&tilt,&roll)&&tilt==0&&roll==0);
        owner->alt=false;
        assert(ResourceMgr_GetIkanaShieldGiPoseForGame(game,"fados_root",&tilt,&roll));
        assert(ApplyPose({-1,0,0},tilt,roll)[1]<-.999f);

        // Identical bounds alone cannot authorize this specific mesh's pose.
        auto unknown=fadosPoints;unknown[0][0]+=1;
        VertexPoints(*owner,"fados_vertices",unknown,binary);
        assert(ResourceMgr_GetIkanaShieldGiPoseForGame(game,"fados_root",&tilt,&roll)&&tilt==0&&roll==0);
        VertexPoints(*owner,"fados_vertices",fadosPoints,binary);

        // A resource matrix can already stand the same raw mesh upright.
        MatrixWords(*owner,"fados_upright_matrix",{1,0,0xffff0000,0,0,0x00010000,0,1,0,0,0,0,0,0,0,0});
        // F3DEX2's encoded push bit is inverted by the resource interpreter.
        List(*owner,"fados_transformed",{{uintptr_t(G_MTX_OTR_FILEPATH)<<24,uintptr_t("fados_upright_matrix")},
            {uintptr_t(G_DL_OTR_FILEPATH)<<24,uintptr_t("fados_body")},{uintptr_t(G_POPMTX)<<24,64},{uintptr_t(G_ENDDL)<<24,0}});
        assert(ResourceMgr_GetIkanaShieldGiPoseForGame(game,"fados_transformed",&tilt,&roll)&&tilt==0&&roll==0);
        assert(Ship::context.active==mm);
    }
    // MM native worn shield decal, object_link_childVtx_015EE8. Texture T
    // increases toward negative X (2251 at -1195, -144 at 851): chin is -X.
    VertexPoints(*oot,"native_decal",{{-172,50,-403},{88,-804,-153},{-433,-804,-153},
        {640,-507,-248},{-985,-507,-248},{-1195,50,-403},{851,50,-403},
        {640,608,-248},{88,905,-153},{-433,905,-153},{-985,608,-248}},true);
    List(*oot,"native_worn",{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("native_decal")},
        {11,0},{uintptr_t(G_ENDDL)<<24,0}});
    float decalTilt=-99,decalRoll=-99;
    assert(ResourceMgr_GetIkanaShieldGiPoseForGame("oot","native_worn",&decalTilt,&decalRoll));
    assert(ApplyPose({-1,0,0},decalTilt,decalRoll)[1]<-.999f && "native Ikana decal is upside down");
    // The tolerance intentionally leaves ambiguous near-square geometry alone.
    for(int halfWidth:{1000,1049,1050,1060}) {
        VertexPoints(*oot,"square_vertices",{{int16_t(-halfWidth),-1000,-20},{int16_t(halfWidth),1000,20}},false);
        List(*oot,"square_root",{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("square_vertices")},{2,0},{uintptr_t(G_ENDDL)<<24,0}});
        float tilt=-99,roll=-99;
        assert(ResourceMgr_GetIkanaShieldGiPoseForGame("oot","square_root",&tilt,&roll)&&tilt==0);
        assert(roll==(halfWidth>1050?float(M_PI/2):0.f));
    }
    // Resource matrices contribute before the caller chooses an outer pose.
    auto matrix=std::make_shared<Fast::Matrix>();
    const float m[4][4]={{1,0,0,0},{0,0,1,0},{0,-1,0,0},{0,0,0,1}};
    auto* words=reinterpret_cast<uint32_t*>(&matrix->Matrx);
    for(int row=0;row<4;++row)for(int col=0;col<4;col+=2){
        const uint32_t a=int32_t(m[row][col]*65536),b=int32_t(m[row][col+1]*65536);
        words[row*2+col/2]=(a&0xffff0000u)|(b>>16);words[8+row*2+col/2]=(a<<16)|(b&0xffffu);
    }
    oot->alt=false;oot->files["pose"]=matrix;
    List(*oot,"transformed",{{(uintptr_t(G_MTX_OTR_FILEPATH)<<24)|G_MTX_PUSH,uintptr_t("pose")},
                            {uintptr_t(G_DL_OTR_FILEPATH)<<24,uintptr_t("body")},{uintptr_t(G_ENDDL)<<24,0}});
    float tilt=-99,roll=-99;
    assert(ResourceMgr_GetIkanaShieldGiPoseForGame("oot","transformed",&tilt,&roll)&&tilt==0&&roll==0);
    assert(ResourceMgr_GetIkanaShieldGiTiltXForGame("oot","transformed",&tilt)&&tilt==0);
    // A descendant matrix can rotate the same planar geometry in its plane.
    const float sidewaysMatrix[4][4]={{0,-1,0,0},{1,0,0,0},{0,0,1,0},{0,0,0,1}};
    for(int row=0;row<4;++row)for(int col=0;col<4;col+=2){
        const uint32_t a=int32_t(sidewaysMatrix[row][col]*65536),b=int32_t(sidewaysMatrix[row][col+1]*65536);
        words[row*2+col/2]=(a&0xffff0000u)|(b>>16);words[8+row*2+col/2]=(a<<16)|(b&0xffffu);
    }
    Vertices(*oot,"vertices",false,false);
    assert(ResourceMgr_GetIkanaShieldGiPoseForGame("oot","transformed",&tilt,&roll)&&tilt==0&&roll==float(M_PI/2));
    assert(ApplyPose({-1,0,0},tilt,roll)[1]<-.999f);
    // Missing/invalid descendants decline the correction and restore Context.
    oot->files["vertices"]=std::make_shared<Fast::Texture>();tilt=77;roll=88;
    assert(!ResourceMgr_GetIkanaShieldGiPoseForGame("oot",worn,&tilt,&roll)&&tilt==77&&roll==88);
    assert(!ResourceMgr_GetIkanaShieldGiPoseForGame("mm",gi,&tilt,nullptr));
    assert(!ResourceMgr_GetIkanaShieldGiPoseForGame("mm",gi,nullptr,&roll));
    assert(!ResourceMgr_GetIkanaShieldGiTiltXForGame("oot",worn,&tilt)&&tilt==77);
    assert(!ResourceMgr_GetIkanaShieldGiTiltXForGame("unknown",gi,&tilt));
    assert(!ResourceMgr_GetIkanaShieldGiTiltXForGame(nullptr,gi,&tilt));
    assert(!ResourceMgr_GetIkanaShieldGiTiltXForGame("mm",nullptr,&tilt));
    assert(!ResourceMgr_GetIkanaShieldGiTiltXForGame("mm",gi,nullptr));
    assert(Ship::context.active==mm);
    auto native=
#ifdef TEST_HOST_MM
        mm;constexpr char nativeGame[]="mm";
#else
        oot;constexpr char nativeGame[]="oot";
#endif
    Ship::CrossRMRegistry::owners.erase(nativeGame);
#ifdef COMBO_BUILD
    assert(!ResourceMgr_GetIkanaShieldGiTiltXForGame(nativeGame,gi,&tilt));
#else
    native->alt=false;Vertices(*native,"vertices",false,false);Ship::context.active=native;
    assert(ResourceMgr_GetIkanaShieldGiTiltXForGame(nativeGame,gi,&tilt)&&tilt==0);
#endif
    std::cout<<"PASS actual Ikana geometry reader/query: XML/hash/native vertices, planar/vertical meshes, "
                 "resource matrix, independent Alt owners, missing resources and scope restoration\n";
}
