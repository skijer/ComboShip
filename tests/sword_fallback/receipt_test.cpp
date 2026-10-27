/* RESOURCE_TYPES */
#include <unordered_map>
#include <stdexcept>
#include "combo/menu/ComboElementalArrowGi.h"
#include "soh/soh/Enhancements/randomizer/NeiElementalGiColors.h"
namespace NeiGi {struct TextureMaterial;}
using f32=float;using s16=int16_t;
constexpr int MTXMODE_APPLY=1;
using Kind=NeiGi::Kind;
void NeiGi_ReportBlackMarketBounds(const NeiGi::Point&,const NeiGi::Point&) {}
void NeiGi_ReportBlackMarketModel(const char*,const char*,const char* = nullptr) {}
struct PlayState {struct {void* gfxCtx;}state;uint32_t gameplayFrames=42;} play;
PlayState* gPlayState=&play;
struct Pose {float scale=1,lift=0,ry=0,rz=0;bool operator==(const Pose&)const=default;} pose;
std::vector<Pose> poses;
void Matrix_Get(MtxF* m){m->xx=pose.scale;m->yw=pose.lift;}
void Matrix_Put(MtxF* m){pose.scale=m->xx;pose.lift=m->yw;pose.ry=pose.rz=0;}
void Matrix_Push(){poses.push_back(pose);}
void Matrix_Pop(){assert(!poses.empty());pose=poses.back();poses.pop_back();}
void Matrix_Translate(float,float y,float,int){pose.lift+=y*pose.scale;}
void Matrix_Scale(float x,float y,float z,int){assert(x==y&&y==z);pose.scale*=x;}
void Matrix_RotateY(float a,int){pose.ry+=a;}
void Matrix_RotateZ(float a,int){pose.rz+=a;}
#include "combo/menu/ComboSwordGiFit.h"
// ResourceManager is the controlled boundary. The production query still
// parses each root's owner, follows live Alt resources and restores scopes.
std::vector<std::string> loadedOwners;
struct SelectedOwner;
struct ArchiveManager {
    SelectedOwner* owner;
    bool HasFile(const std::string& path);
};
bool failArchiveParse;
struct ResourceLoader {
    std::shared_ptr<Ship::IResource> LoadResource(const std::string&,std::shared_ptr<Ship::IResource> file){
        if(failArchiveParse)throw std::runtime_error("controlled malformed archive resource");return file;
    }
};
struct SelectedOwner {
    const char* game;bool alt=true;
    std::map<std::string,std::shared_ptr<Ship::IResource>> overrides;
    bool IsAltAssetsEnabled(){return alt;}
    std::shared_ptr<Ship::IResource> LoadResource(const char* path){
        loadedOwners.push_back(std::string(game)+":"+path);
        auto it=overrides.find(path);return alt&&it!=overrides.end()?it->second:loader(path);
    }
    std::shared_ptr<Ship::IResource> LoadResource(uint64_t hash){return loader(hash);}
    std::shared_ptr<ArchiveManager> GetArchiveManager(){return std::make_shared<ArchiveManager>(ArchiveManager{this});}
    std::shared_ptr<ResourceLoader> GetResourceLoader(){return std::make_shared<ResourceLoader>();}
};
bool ArchiveManager::HasFile(const std::string& path){return path.starts_with("alt/")?owner->overrides.contains(path.substr(4)):bool(loader(path.c_str()));}
namespace Ship {class ResourceManager:public SelectedOwner {public:explicit ResourceManager(const char* game):SelectedOwner{game}{}};}
auto ootOwner=std::make_shared<Ship::ResourceManager>("oot");
auto mmOwner=std::make_shared<Ship::ResourceManager>("mm");
std::shared_ptr<Ship::ResourceManager> currentOwner=ootOwner;
namespace Ship {
struct Archive {
    std::map<std::string,std::shared_ptr<IResource>> files;
    std::shared_ptr<IResource> LoadFile(const std::string& path){auto it=files.find(path);return it==files.end()?nullptr:it->second;}
};
struct CrossRMRegistry {
    static std::shared_ptr<ResourceManager> Get(const std::string& game){return game=="oot"?ootOwner:game=="mm"?mmOwner:nullptr;}
};
struct ResourceManagerScope {
    std::shared_ptr<ResourceManager> before;
    ResourceManagerScope(std::shared_ptr<ResourceManager> owner):before(currentOwner){currentOwner=owner;}
    ~ResourceManagerScope(){currentOwner=before;}
};
}
struct Context {std::shared_ptr<Ship::ResourceManager> GetResourceManager(){return mmOwner;}};
struct OTRGlobals {static inline OTRGlobals* Instance;std::shared_ptr<Context> context=std::make_shared<Context>();} globals;
std::vector<std::shared_ptr<Ship::Archive>> sOotArchives;
extern "C" unsigned char MmAssets_OotArchivesLoaded(){return !sOotArchives.empty();}
extern "C" const char* MmAssets_HashToPath(unsigned long long hash){auto it=loader.names.find(hash);return it==loader.names.end()?nullptr:it->second.c_str();}
/* COMPANION_FIT */
/* RESOURCE_FIT */
extern "C" int ResourceMgr_GetGiModelFitForGame(const char*,const char* path,float scale,float tilt,int context,float out[2]){
    NeiGi::ShopFit fit{};if(!NeiGi::SelectedModelFit(loader,path,scale,tilt,context,0,fit))return 0;
    out[0]=fit.scale;out[1]=fit.lift;return 1;
}
extern "C" int ResourceMgr_IsGiModelAvailableForGame(const char*,const char*){
    assert(false && "sword receipt must not query the legacy tunic fallback");return 0;
}
int CVarGetInteger(const char*,int);
bool ResourceMgr_IsAltAssetsEnabled(){return mmOwner->alt;}
/* RESOURCE_API */
#include "combo/menu/ComboSwordGiLegacyFit.h"
/* DECLARATIONS */
extern "C" int ResourceMgr_IsIkanaShieldGiAvailable() {
 assert(false && "sword receipt must not query the native shield pair");return 0;
}
struct Actor {int id;struct{struct{int x;}rot;}home;};
constexpr int ACTOR_EN_GIRLA=1,ACTOR_EN_ITEM00=2,PLAYER_FORM_HUMAN=0,PLAYER_FORM_GORON=1;
int form=PLAYER_FORM_HUMAN;
#define GET_PLAYER_FORM form
struct Save {int comboObtained[1]{};} save;
Save* Nei_Save(){return &save;}
constexpr int FC_OOT_SWORD_BIGGORON=0;
std::set<std::string> ootMods;
std::set<std::string> mmMods={"objects/object_gi_sword_2/gGiRazorSwordDL","objects/object_gi_sword_4/gGiGreatFairysSwordBladeDL"};
bool mmArchiveAvailable=true;
extern "C" uint8_t MmAssets_IsAvailable(){return mmArchiveAvailable;}
extern "C" int ResourceMgr_IsModAssetForGame(const char* owner,const char* path){
    return !strcmp(owner,"oot")?ootMods.contains(path):
           !strcmp(owner,"mm")&&mmMods.contains(path);
}
int CVarGetInteger(const char*,int){return 0;}
int ComboSongForMmItem(RandoItemId){return -1;}
const char* NeiResource_Route(const char* path){return path;}
int OwnerNeiDescribe(const char* slug,CwItemDrawInfo* out){
    out->neiShimmer=int(!strcmp(slug,"great_fairy_sword")?Kind::GreatFairySword:Kind::RazorSword)+1;return 0;
}
int OwnerItemDescribe(const char*,CwItemDrawInfo* out){
    out->drawKind=CW_DRAW_KIND_CUSTOM_GI;out->dlistCount=1;out->dlists[0]="__OTR__alt/objects/object_custom_equip/gCustomKokiriSwordDL";
    out->neiShimmer=int(Kind::RazorSword)+1;out->opCount=1;out->ops[0].op=CW_OP_ROTATE_Z;return 1;
}
void* Combo_ResolveSym(const char*,const char* name){
    if(!strcmp(name,"OOT_GetNeiGiDrawInfo"))return reinterpret_cast<void*>(OwnerNeiDescribe);
    if(!strcmp(name,"OOT_GetItemDrawInfo"))return reinterpret_cast<void*>(OwnerItemDescribe);
    return nullptr; // Missing optional exports must not alias an incompatible function.
}
void DrawSong(RandoItemId){assert(false);}
extern "C" int NeiGi_DrawElementalArrowShop(PlayState*,int){
    assert(false && "sword receipt must not dispatch elemental arrows");return 0;
}
extern "C" int NeiGi_DrawElementalSpell(PlayState*,int,int,int){
    assert(false && "sword receipt must not dispatch elemental spells");return 0;
}
void MM_DrawNeiGi(const CwItemDrawInfo&,bool,int){assert(false&&"legacy mod must retain its native callback");}
NeiGi::Basis NeiGi_CameraBasis(PlayState*){return {};}
std::vector<Pose> effects;
void NeiGi_DrawMesh(PlayState*,const NeiGi::Mesh& mesh){if(mesh.count)effects.push_back(pose);}
Pose nativePose;int nativeDraws;
void DrawNativeSword(){nativePose=pose;++nativeDraws;Matrix_Scale(3,3,3,MTXMODE_APPLY);}
/* PRODUCTION */
namespace NativeOot {
struct GetItemEntry;
using CustomDrawFunc=void(*)(PlayState*,GetItemEntry*);
struct GetItemEntry {CustomDrawFunc drawFunc;int gid=0;};
struct Vec3f {float x,y,z;};
struct Presentation {CustomDrawFunc draw;const char* opaque;const char* translucent;float scale;Kind effect;
                     Vec3f effectCenter{};bool alwaysShimmer=true;Kind shimmer=Kind::Neutral;};
Presentation selected{};
const Presentation* FindPresentation(GetItemEntry*){return &selected;}
int SongForEntry(GetItemEntry*){return -1;}
int SeasonForDraw(CustomDrawFunc){return 0;}
const char* SelectedSwordPath(const Presentation&,bool,bool(*)(const char*)){return nullptr;}
bool HasResource(const char* path){return bool(loader(path));}
int ResourceMgr_IsAltAssetsEnabled(){return ootOwner->alt;}
int ResourceMgr_IsModAsset(const char* path){return ootMods.contains(path);}
bool HasRedesignGiMod(const Presentation&){return false;}
struct NeiGi_ArenaScope {template<class... Args> NeiGi_ArenaScope(Args...){ }operator bool(){return true;}};
void NeiGi_DrawSong(PlayState*,int){assert(false);}
void NeiGi_DrawSeasonOverlay(PlayState*,int,const char*){assert(false);}
void Randomizer_DrawCaneSomariaUpgrade(PlayState*,GetItemEntry*){assert(false);}
void Randomizer_DrawCaneSomariaUpgradeFlame(PlayState*){assert(false);}
void Randomizer_DrawTrueMasterSwordFlame(PlayState*){assert(false);}
void Randomizer_DrawMarioMask(PlayState*,GetItemEntry*){assert(false);}
void Randomizer_DrawExtSpiritBreastplate(PlayState*,GetItemEntry*){assert(false);}
void Randomizer_DrawExtSagesTunic(PlayState*,GetItemEntry*){assert(false);}
void Randomizer_DrawExtShieldOfIkana(PlayState*,GetItemEntry*){assert(false && "sword receipt selected a shield");}
void Randomizer_DrawExtChampionsTunic(PlayState*,GetItemEntry*){assert(false);}
void NeiGi_DrawSelectedSword(PlayState*,const char*,bool,bool){assert(false);}
void GetItem_Draw(PlayState*,int){assert(false);}
void NeiGi_DrawMesh(PlayState* p,const NeiGi::Mesh& mesh,Kind){::NeiGi_DrawMesh(p,mesh);}
void LegacyRazor(PlayState*,GetItemEntry*){DrawNativeSword();}
void DrawCustomItemDiamond(PlayState*,Gfx*,float scale){Matrix_Scale(scale,scale,scale,1);DrawNativeSword();}
void DrawCustomItemDiamondTint(PlayState* p,Gfx* dl,Gfx*,float scale,int,int,int){DrawCustomItemDiamond(p,dl,scale);}
Gfx* ResourceMgr_LoadGfxByName(const char*){return nullptr;}
void DrawMmWeaponGi(PlayState*,Gfx*,Gfx*,float,bool){assert(false);}
const char dgNeiFourSwordBladeDL[]="__OTR__objects/object_nei_four_sword/gNeiFourSwordBladeDL";
const char dgNeiFourSwordHiltDL[]="__OTR__objects/object_nei_four_sword/gNeiFourSwordHiltDL";
const char gGiKokiriSwordDL[]="objects/object_gi_sword_1/gGiKokiriSwordDL";
const char gGiBiggoronSwordDL[]="objects/object_gi_longsword/gGiBiggoronSwordDL";
#define CVAR_NEI_GI_EFFECTS "test.ItemEffects"
#define OPEN_DISPS(...) ((void)0)
#define CLOSE_DISPS(...) ((void)0)
#define Gfx_SetupDL_25Opa(...) ((void)0)
#define Gfx_SetupDL_25Xlu(...) ((void)0)
#undef gSPGrayscale
#undef gDPSetPrimColor
#undef gDPSetEnvColor
#undef gSPMatrix
#undef gDma1p
#define gSPGrayscale(...) ((void)0)
#define gDPSetPrimColor(...) ((void)0)
#define gDPSetEnvColor(...) ((void)0)
#define gSPMatrix(...) ((void)0)
#define gDma1p(...) ((void)0)
const char* NeiGi_BaseSwordPath(const char*){return nullptr;}
int32_t OOT_NeiResourceExists(const char*){return 0;}
template<class...T>void NeiGi_DrawPresentation(T...){assert(false);}
/* OOT_PRODUCTION */
}
int main(){
    OTRGlobals::Instance=&globals;
    LegacyVertex("test/bodyVertices",1,{{-200,-600,0},{200,4200,0}});
    LegacyVertex("test/detailVertices",2,{{-300,-900,0},{300,8000,0}});
    List("objects/object_gi_sword_2/gGiRazorSwordDL",3,{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("test/bodyVertices")},{2,0},{uintptr_t(G_ENDDL)<<24,0}});
    const char* tridentRoot="__OTR__objects/object_gnd/gPhantomGanonSkelLimbsLimb_00C610DL_009298";
    List(tridentRoot+7,40,{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("test/bodyVertices")},{2,0},
                         {uintptr_t(G_DL)<<24,0x08000001},{uintptr_t(G_ENDDL)<<24,0}});
    float frameLow[3]{},frameHigh[3]{};
    assert(ResourceMgr_GetGiModelBoundsForGame("oot",&tridentRoot,1,frameLow,frameHigh)&&
           frameLow[1]==-600&&frameHigh[1]==4200);
    List("test/unknownSegment",41,{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("test/bodyVertices")},{2,0},
                                  {uintptr_t(G_DL)<<24,0x08000001},{uintptr_t(G_ENDDL)<<24,0}});
    const char* unknownSegment="__OTR__test/unknownSegment";
    assert(!ResourceMgr_GetGiModelBoundsForGame("oot",&unknownSegment,1,frameLow,frameHigh));
    const char* spellOrb="__OTR__objects/object_gi_goddess/gGiMagicSpellOrbDL";
    List(spellOrb+7,42,{{uintptr_t(G_DL)<<24,0x08000001},
                       {uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("test/bodyVertices")},{2,0},
                       {uintptr_t(G_ENDDL)<<24,0}});
    assert(ResourceMgr_GetGiModelBoundsForGame("mm",&spellOrb,1,frameLow,frameHigh)&&frameHigh[1]==4200);
    /* REAL_SPELL_BOUNDS */
    List("objects/object_gi_sword_2/gGiRazorSwordEmptyDL",4,{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("test/detailVertices")},{2,0},{uintptr_t(G_ENDDL)<<24,0}});
    Actor receipt{ACTOR_EN_ITEM00,{{CustomItem::CALLED_ACTION|CustomItem::GIVE_ITEM_CUTSCENE}}};
    for(int playerForm:{PLAYER_FORM_HUMAN,PLAYER_FORM_GORON}){
        form=playerForm;const float origin=form==PLAYER_FORM_GORON?98.3f:53.3f;
        pose={.21f,origin,0,0};const Pose caller=pose;nativeDraws=0;effects.clear();poses.clear();
        CwItemDrawInfo info{};assert(!MM_DescribeNeiGi(RI_SWORD_RAZOR,&info));
        DrawReceipt(RI_SWORD_RAZOR,&receipt);
        const float bottom=form==PLAYER_FORM_GORON?36.f:-48.5f,top=form==PLAYER_FORM_GORON?90.f:20.5f;
        assert(nativePose.lift+8000*nativePose.scale<=origin+.21f*top+.001f&&"native legacy receipt leaves second selected sword root above the frame");
        assert(nativePose.lift-900*nativePose.scale>=origin+.21f*bottom-.001f);
        assert(nativeDraws==1&&effects.size()==2&&"receipt correction changed legacy callback or sword identity effects");
        const auto frame=NeiGi::FrameFit(*NeiGi::FindSwordFrameBounds(Kind::RazorSword),1.f,false,
                                        form==PLAYER_FORM_GORON?2:1);
        const float center=form==PLAYER_FORM_GORON?63.f:-14.f;
        const float size=form==PLAYER_FORM_GORON?1.f:1.15f;
        const Pose expected{caller.scale*size*frame.scale,caller.lift+caller.scale*((1.f-size)*center+size*frame.lift),0,0};
        assert(std::abs(effects[0].scale-expected.scale)<.000001f&&std::abs(effects[0].lift-expected.lift)<.0001f&&
               effects[0]==effects[1]&&effects[0]!=nativePose&&
               "legacy binary receipt effects must retain the authored award footprint");
        assert(pose==caller&&poses.empty()&&"legacy callback transforms escaped its receipt scope");
    }
    LegacyVertex("test/bgsVertices",5,{{-200,-900,0},{200,8000,0}});
    LegacyVertex("test/fairyVertices",6,{{-20,-20,0},{20,40,0}});
    List("objects/object_gi_longsword/gGiBiggoronSwordDL",7,{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("test/bgsVertices")},{2,0},{uintptr_t(G_ENDDL)<<24,0}});
    List("objects/object_gi_sword_4/gGiGreatFairysSwordBladeDL",8,{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("test/fairyVertices")},{2,0},{uintptr_t(G_ENDDL)<<24,0}});
    List("objects/object_gi_sword_4/gGiGreatFairysSwordHiltEmblemDL",9,{{uintptr_t(G_ENDDL)<<24,0}});
    form=PLAYER_FORM_HUMAN;pose={.21f,51.3f,0,0};effects.clear();
    DrawReceipt(RI_GREAT_FAIRY_SWORD,&receipt);
    assert(nativePose.lift+8000*nativePose.scale<=55.606f&&"legacy Great Fairy receipt fitted the identity instead of the callback's selected Biggoron geometry");
    save.comboObtained[0]=1;pose={.21f,51.3f,0,0};effects.clear();
    DrawReceipt(RI_GREAT_FAIRY_SWORD,&receipt);
    assert(nativePose.lift+40*nativePose.scale<=55.606f&&nativePose.lift-20*nativePose.scale>=41.114f&&
           "native material-only secondary pass must still allow the geometric root's receipt fit");
    // The native callback chooses OoT's local replacement for an imported
    // root before MM's donor, independently for each pass.
    const char* razor="objects/object_gi_sword_2/gGiRazorSwordDL";
    const char* detail="objects/object_gi_sword_2/gGiRazorSwordEmptyDL";
    ootMods.insert(razor);loadedOwners.clear();pose={};
    ComboSwordGi_ApplyLegacyFit("oot",Kind::RazorSword);
    assert(loadedOwners==std::vector<std::string>({std::string("oot:")+razor,"oot:test/bodyVertices",
                                                 std::string("mm:")+detail,"mm:test/detailVertices"})&&
           currentOwner==ootOwner&&"legacy fit queried a different owner than the retained callback");
    // Live Alt roots can contain independent oversized geometry. Re-query
    // each draw rather than retaining base/archive catalog bounds.
    Vertex("test/altDetailVertices",10,{{-300,-900,0},{300,32000,0}});
    List("test/altDetail",11,{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("test/altDetailVertices")},{2,0},{uintptr_t(G_ENDDL)<<24,0}});
    mmOwner->overrides[detail]=loader("test/altDetail");
    const char* roots[]={"__OTR__@mm:objects/object_gi_sword_4/gGiGreatFairysSwordBladeDL",
                         "__OTR__@mm:objects/object_gi_sword_2/gGiRazorSwordEmptyDL"};
    float correction[2]{};assert(ResourceMgr_GetGiModelsFitForGame("oot",roots,2,1,0,2,correction));
    assert(ResourceMgr_GetGiModelBoundsForGame("oot",roots,2,frameLow,frameHigh)&&
           frameLow[0]==-300&&frameLow[1]==-900&&frameHigh[0]==300&&frameHigh[1]==32000&&currentOwner==ootOwner);
    assert(correction[1]+32000*correction[0]<=16.001f&&correction[1]-900*correction[0]>=-44.001f);
    const float altFit=correction[0];
    mmOwner->alt=false;assert(ResourceMgr_GetGiModelsFitForGame("oot",roots,2,1,0,2,correction));
    assert(ResourceMgr_GetGiModelBoundsForGame("oot",roots,2,frameLow,frameHigh)&&frameHigh[1]==8000);
    assert(correction[0]>altFit&&currentOwner==ootOwner&&"fit retained an Alt graph after selection changed");
    // Unsupported second geometry refuses the whole query. Keeping the
    // original draw cannot be described as a partial successful fit.
    List("test/unsupported",12,{{uintptr_t(G_MTX)<<24,0},{uintptr_t(G_ENDDL)<<24,0}});
    roots[1]="__OTR__@mm:test/unsupported";
    frameLow[0]=-777;frameHigh[0]=888;
    assert(!ResourceMgr_GetGiModelBoundsForGame("oot",roots,2,frameLow,frameHigh)&&
           frameLow[0]==-777&&frameHigh[0]==888&&currentOwner==ootOwner);
    assert(!ResourceMgr_GetGiModelsFitForGame("oot",roots,2,1,0,2,correction)&&currentOwner==ootOwner);
    auto scaleMatrix=std::make_shared<Fast::Matrix>();
    const float matrixValues[16]={3,0,0,0, 0,3,0,0, 0,0,3,0, 0,0,0,1};
    auto* matrixWords=reinterpret_cast<uint32_t*>(&scaleMatrix->Matrx);
    for(int i=0;i<8;++i){const uint32_t a=int32_t(matrixValues[i*2]*65536.f),b=int32_t(matrixValues[i*2+1]*65536.f);
        matrixWords[i]=(a&0xffff0000u)|(b>>16);matrixWords[i+8]=(a<<16)|(b&0xffffu);}
    loader.files["test/carryMatrix"]=scaleMatrix;
    List("test/carryPass",13,{{uintptr_t(G_MTX_OTR_FILEPATH)<<24|G_MTX_PUSH,uintptr_t("test/carryMatrix")},{uintptr_t(G_ENDDL)<<24,0}});
    roots[0]="__OTR__@mm:test/carryPass";roots[1]="__OTR__@mm:objects/object_gi_sword_2/gGiRazorSwordEmptyDL";
    assert(!ResourceMgr_GetGiModelBoundsForGame("oot",roots,2,frameLow,frameHigh));
    assert(!ResourceMgr_GetGiModelsFitForGame("oot",roots,2,1,0,2,correction)&&
           "stateful material pass underfits following geometry when independent root readers reset its MUL");
    // A companion root is independent of MM's same-name base remnant. Its
    // deep loader may select a live Alt child and then ordinary MM descendants.
    auto donor=std::make_shared<Ship::Archive>();sOotArchives={donor};
    List("test/donorChild",20,{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("test/descendantVertices")},{2,0},{uintptr_t(G_ENDDL)<<24,0}});
    Vertex("test/descendantVertices",21,{{-200,-900,0},{200,32000,0}});
    auto selectedChild=loader("test/donorChild");
    Vertex("test/archiveVertices",22,{{-200,-100,0},{200,100,0}});
    donor->files["test/descendantVertices"]=loader("test/archiveVertices");
    List("test/archiveChild",23,{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("test/descendantVertices")},{2,0},{uintptr_t(G_ENDDL)<<24,0}});
    donor->files["test/donorChild"]=loader("test/archiveChild");
    List("test/archiveRoot",24,{{uintptr_t(G_DL_OTR_HASH)<<24,0},{0,20},
                              {uintptr_t(G_DL)<<24,0x08000001},{uintptr_t(G_ENDDL)<<24,0}});
    const char* biggoron="objects/object_gi_longsword/gGiBiggoronSwordDL";
    donor->files[biggoron]=loader("test/archiveRoot");
    mmOwner->overrides["test/donorChild"]=selectedChild;mmOwner->alt=true;
    const char* companionRoots[]={biggoron};
    assert(ResourceMgr_GetGiModelBoundsForGame("oot-companion",companionRoots,1,frameLow,frameHigh)&&
           frameLow[1]==-900&&frameHigh[1]==32000&&currentOwner==ootOwner);
    assert(ResourceMgr_GetGiModelsFitForGame("oot-companion",companionRoots,1,1,0,2,correction)&&
           correction[1]+32000*correction[0]<=16.001f&&correction[1]-900*correction[0]>=-44.001f&&
           "companion fit omitted the winning Alt child or read its descendants from donor instead of MM");
    save.comboObtained[0]=0;pose={.21f,51.3f,0,0};effects.clear();
    DrawReceipt(RI_GREAT_FAIRY_SWORD,&receipt);
    assert(nativePose.lift+32000*nativePose.scale<=55.606f&&
           "native legacy Biggoron callback queried the MM twin instead of its companion's selected Alt child");
    const float modChildFit=correction[0];mmOwner->alt=false;
    assert(ResourceMgr_GetGiModelBoundsForGame("oot-companion",companionRoots,1,frameLow,frameHigh)&&frameHigh[1]==100);
    assert(ResourceMgr_GetGiModelsFitForGame("oot-companion",companionRoots,1,1,0,2,correction)&&
           correction[0]>modChildFit&&"companion fit used MM's same-name root or stale Alt descendant");
    assert(currentOwner==ootOwner);
    auto unsupportedDonor=std::make_shared<Ship::Archive>();
    unsupportedDonor->files[biggoron]=loader("test/unsupported");sOotArchives={unsupportedDonor};
    assert(!ResourceMgr_GetGiModelBoundsForGame("oot-companion",companionRoots,1,frameLow,frameHigh)&&currentOwner==ootOwner);
    assert(!ResourceMgr_GetGiModelsFitForGame("oot-companion",companionRoots,1,1,0,2,correction)&&
           "unsupported donor graph incorrectly fitted the native MM fallback");
    sOotArchives={donor};failArchiveParse=true;
    assert(!ResourceMgr_GetGiModelBoundsForGame("oot-companion",companionRoots,1,frameLow,frameHigh)&&currentOwner==ootOwner);
    assert(!ResourceMgr_GetGiModelsFitForGame("oot-companion",companionRoots,1,1,0,2,correction)&&currentOwner==ootOwner);
    failArchiveParse=false;
    sOotArchives.clear();
    assert(ResourceMgr_GetGiModelBoundsForGame("oot-companion",companionRoots,1,frameLow,frameHigh)&&frameHigh[1]==8000);
    assert(ResourceMgr_GetGiModelsFitForGame("oot-companion",companionRoots,1,1,0,2,correction)&&
           correction[1]+8000*correction[0]<=16.001f&&currentOwner==ootOwner);
    mmOwner->alt=true;mmOwner->overrides.clear();
    NativeOot::selected={NativeOot::LegacyRazor,"__OTR__objects/nei_gi_redesign/razor_sword/gi_dl",nullptr,.04f,Kind::RazorSword};
    NativeOot::GetItemEntry entry{NativeOot::LegacyRazor};
    pose={};const Pose ootCaller=pose;effects.clear();nativeDraws=0;
    assert(NativeOot::NeiGi_DrawImpl(&play,&entry,false));
    assert(nativePose.lift+8000*nativePose.scale<=55.501f&&nativePose.lift-900*nativePose.scale>=-59.501f&&
           "native OoT legacy mod receipt bypassed selected model fitting");
    const auto razorFrame=NeiGi::FrameFit(*NeiGi::FindSwordFrameBounds(Kind::RazorSword),1.f,false);
    assert(nativeDraws==1&&effects.size()>=2&&std::abs(effects[0].scale-1.15f*razorFrame.scale)<.000001f&&
           std::abs(effects[0].lift-(.3f+1.15f*razorFrame.lift))<.0001f&&
           "native OoT legacy effects inherited selected geometry units");
    assert(pose==ootCaller&&poses.empty());
    Vertex("test/kokiriVertices",13,{{-200,-900,0},{200,8000,0}});
    List("objects/object_gi_sword_1/gGiKokiriSwordDL",14,{{uintptr_t(G_VTX_OTR_FILEPATH)<<24,uintptr_t("test/kokiriVertices")},{2,0},{uintptr_t(G_ENDDL)<<24,0}});
    ootMods.insert("objects/object_gi_sword_1/gGiKokiriSwordDL");
    NativeOot::selected={NativeOot::Randomizer_DrawProgressiveMasterSword,"__OTR__objects/nei_gi_redesign/master_sword/gi_dl",nullptr,.04f,Kind::MasterSword};
    entry.drawFunc=NativeOot::Randomizer_DrawProgressiveMasterSword;pose={};effects.clear();nativeDraws=0;
    assert(NativeOot::NeiGi_DrawImpl(&play,&entry,false));
    assert(nativePose.lift+8000*nativePose.scale<=55.501f&&nativePose.lift-900*nativePose.scale>=-59.501f&&
           "native progressive Master callback fitted Temple/authored bounds instead of its selected Kokiri placeholder at .6");
    assert(nativeDraws==1&&effects.size()>=2&&pose==ootCaller&&poses.empty());
    NativeOot::selected={NativeOot::Randomizer_DrawExtFourSword,"__OTR__objects/nei_gi_redesign/four_sword/gi_dl",nullptr,.04f,Kind::FourSword};
    entry.drawFunc=NativeOot::Randomizer_DrawExtFourSword;pose={};effects.clear();nativeDraws=0;
    assert(NativeOot::NeiGi_DrawImpl(&play,&entry,false));
    assert(nativePose.lift+8000*nativePose.scale<=55.501f&&nativePose.lift-900*nativePose.scale>=-59.501f&&
           "missing Four Sword primary left the retained .55 selected Kokiri fallback unbounded");
    Vertex("test/kokiriVertices",25,{{-200,-900,0},{200,32000,0}});
    ootMods.erase(razor);mmMods.erase(razor);mmArchiveAvailable=false;pose={};loadedOwners.clear();
    ComboSwordGi_ApplyLegacyFit("oot",Kind::RazorSword);Matrix_Scale(.55f,.55f,.55f,1);
    assert(pose.lift+32000*pose.scale<=48.001f&&pose.lift-900*pose.scale>=-56.001f&&
           loadedOwners.front()=="oot:objects/object_gi_sword_1/gGiKokiriSwordDL"&&
           "absent MM companion fitted a donor sword instead of the original tinted Kokiri fallback");
    std::cout<<"PASS native receipt descriptor refusal, original callback, all selected roots, fitted sword particles/shimmer and scope restoration\n";
}
