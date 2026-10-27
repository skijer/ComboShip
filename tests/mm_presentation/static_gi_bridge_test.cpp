#include <algorithm>
// Black Market reporting is an observer outside this rendering fixture.
void NeiGi_ReportBlackMarketModel(const char*, const char*, const char*) {}
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>
#include "combo/menu/ComboItemDrawABI.h"
#include "combo/menu/ComboCollectibleShimmer.h"
#include "z64item.h"
#include "combo/menu/ComboIkanaShieldResources.h"
extern "C" int ResourceMgr_IsIkanaShieldGiAvailable() {
 assert(false && "static item fixture must not select the native shield pair");return 0;
}
#include "combo/menu/ComboRewardGi.h"
#include "combo/menu/ComboBottleContents.h"
#include "combo/menu/ComboItemEffectColors.h"
#include "soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#define RANDO_ENUM_BEGIN(x) enum x {
#define RANDO_ENUM_ITEM(x) x,
#define RANDO_ENUM_END(x) };
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
using s16=int16_t;using s32=int32_t;using f32=float;
using u8=uint8_t;
struct Color_RGB8 {uint8_t r,g,b;};
#define CVAR_COSMETIC(x) x
#define CVAR_ENHANCEMENT(x) x
bool changedMagic=false, dinSelected=false;
int CVarGetInteger(const char* name,int value) {return !strcmp(name,"Consumable.Magic.Changed") ? changedMagic : !strcmp(name,"DinFireSword") ? dinSelected : value;}
Color_RGB8 CVarGetColor24(const char*,Color_RGB8 c) {return c;}
Color_RGB8 liveMagic{12,210,250};
Color_RGB8 CwLiveCosmeticColor(const char*,Color_RGB8) {return liveMagic;}
#define PATH(name) const char* name="__OTR__" #name;
PATH(object_toki_objects_DL_001BD0) PATH(gGiHeartBorderDL) PATH(gGiHeartContainerDL)
PATH(gGiScaleDL) PATH(gGiScaleWaterDL) PATH(gSkeletonKeyDL) PATH(gTriforcePiece1DL)
PATH(gTriforcePiece2DL) PATH(gTriforcePiece0DL) PATH(gTriforcePieceCompletedDL)
PATH(gGiRocsFeatherDL) PATH(gGiGrabDL) PATH(gGiClimbDL) PATH(gGiCrawlDL)
PATH(gGiOpenChestsDL) PATH(gGiFishingPoleDL)
PATH(gStatDefenseDL) PATH(gStatSpeedDL) PATH(gStatPowerDL) PATH(gStatCrawlSpeedDL) PATH(gStatClimbSpeedDL) PATH(gStatPushSpeedDL)
struct {struct {struct {struct {struct {int triforcePiecesCollected=0;} randomizer;} data;} quest;} ship;} gSaveContext;
constexpr int TABLE_RANDOMIZER=1;
void Randomizer_DrawCaneOfSomaria() {} void Randomizer_DrawCanePacci() {}
void Randomizer_DrawCaneSomariaUpgrade() {} void Randomizer_DrawCanePacciUpgrade() {}
void Randomizer_DrawCanePacciUltrahand() {} void Custom() {}
struct GetItemEntry {int tableId,drawItemId,gid,itemId;void (*drawFunc)();};
RandomizerGet concrete=RG_NONE;
namespace Rando::StaticData {
std::unordered_map<std::string,RandomizerGet> itemNameToEnum;
struct Name {std::string english="concrete award";};
struct Item {RandomizerGet rg;std::shared_ptr<GetItemEntry> GetGIEntry(RandomizerGet* actual) {
 *actual=concrete;return std::make_shared<GetItemEntry>(GetItemEntry{TABLE_RANDOMIZER,rg,rg==RG_MAGIC_STAT_UPGRADE?GID_MAGIC_LARGE:0,0,(rg==RG_MAGIC_STAT_UPGRADE || concrete==RG_KOKIRI_SWORD || concrete==RG_BIGGORON_SWORD)?nullptr:Custom});
 } const Name& GetName() {static Name name;return name;}};
Item RetrieveItem(RandomizerGet rg) {return {rg};}
}
int NeiGi_DescribeEntry(const GetItemEntry*,CwItemDrawInfo*) {return 0;}
int ResourceMgr_IsModAssetForGame(const char*,const char*);
int ResourceMgr_GetIkanaShieldGiTiltXForGame(const char*,const char*,float*);
int ResourceMgr_GetIkanaShieldGiPoseForGame(const char*,const char*,float*,float*);
bool DrawOwnerActive();
int GetItem_GetDrawTableEntry(int,void** dls,int,int*,float*,int*,uint8_t*) {
 assert(DrawOwnerActive());
 if(concrete!=RG_MAGIC_STAT_UPGRADE && concrete!=RG_KOKIRI_SWORD && concrete!=RG_BIGGORON_SWORD)return 0;
 dls[0]=(void*)"__OTR__native_sword_row";return 1;
}
void GetItem_GetDrawSetupDLs(int,void**,void**) {} int GetItem_GetShimmerColor(s16,uint8_t* color) {memset(color,1,4);return concrete==RG_MAGIC_STAT_UPGRADE;}
std::set<std::string> resources;
bool ownerAlt=false;
bool ownerPresent=true, hostAlt=false, magicResourcePresent=true, magicLoaded=false;
bool magicAltPresent=true, magicAltLoads=true, magicAltMetaPresent=false;
std::vector<std::string> loadedPaths;
namespace Ship {
 struct ResourceManager;
 extern std::shared_ptr<ResourceManager> activeRm;
 struct InitData {bool IsCustom;};
 struct Resource {std::shared_ptr<InitData> init;std::shared_ptr<InitData> GetInitData(){return init;}};
 struct Archive {bool HasFile(const std::string& path) {if(path=="alt/magic")return magicAltPresent;if(path=="alt/magic.meta")return magicAltMetaPresent;return resources.count("__OTR__"+path);}};
 struct ResourceManager {
  bool owner;bool IsAltAssetsEnabled() {return owner?ownerAlt:hostAlt;}
  std::shared_ptr<Archive> GetArchiveManager() {assert(owner);return std::make_shared<Archive>();}
  std::shared_ptr<Resource> LoadResource(const std::string& path,bool exact) {
   // Skeleton factories resolve nested limbs through the active context RM,
   // even when the root is loaded directly from the registered owner.
   assert(activeRm.get()==this && "nested factory loads must use the root resource owner");
   assert(owner && exact);magicLoaded=true;loadedPaths.push_back(path);
   if(!magicResourcePresent || (path=="alt/magic" && !magicAltLoads))return nullptr;
   return std::make_shared<Resource>(Resource{std::make_shared<InitData>(InitData{path=="alt/magic"})});
  }
 };
 std::shared_ptr<ResourceManager> ownerRm=std::make_shared<ResourceManager>(ResourceManager{true});
 std::shared_ptr<ResourceManager> hostRm=std::make_shared<ResourceManager>(ResourceManager{false});
 std::shared_ptr<ResourceManager> activeRm=hostRm;
 struct CrossRMRegistry {static std::shared_ptr<ResourceManager> Get(const char* game) {assert(!strcmp(game,"oot"));return ownerPresent?ownerRm:nullptr;}};
 struct ResourceManagerScope {std::shared_ptr<ResourceManager> old;ResourceManagerScope(std::shared_ptr<ResourceManager> rm):old(activeRm) {activeRm=rm;}~ResourceManagerScope(){activeRm=old;}};
}
bool DrawOwnerActive() { return Ship::activeRm==Ship::ownerRm; }
struct Gfx;
Gfx* ResourceMgr_LoadGfxByName(const char*) {assert(Ship::activeRm==Ship::ownerRm);magicLoaded=true;return magicResourcePresent?reinterpret_cast<Gfx*>(&magicLoaded):nullptr;}
uint8_t ResourceGetIsCustomByName(const char*) {assert(magicLoaded && Ship::activeRm==Ship::ownerRm);return Ship::activeRm->IsAltAssetsEnabled();}
#define COMBO_EXPORT
/* OWNER_ALT_QUERY */
/* OWNER_MAGIC_QUERY */
// MM starts first: OoT's legacy ExtensionCache has never been populated.
int ColdOotFileExists(const char*) {return 0;}
int ColdOotFileAltExists(const char*) {return 0;}
#define ResourceMgr_FileExists ColdOotFileExists
#define ResourceMgr_FileAltExists ColdOotFileAltExists
/* OWNER_RESOURCE_QUERY */
#undef ResourceMgr_FileExists
#undef ResourceMgr_FileAltExists
void ComboMaskShimmerColor(int,uint8_t* color) {color[0]=255;color[1]=255;color[2]=255;color[3]=255;}
void ComboOotMaskShimmerColor(int i,uint8_t* color) {ComboMaskShimmerColor(i,color);}
void Seasons_SeasonColor(uint8_t season,uint8_t* r,uint8_t* g,uint8_t* b) {
 static const uint8_t colors[][3]={{6,235,64},{235,5,7},{235,166,6},{4,105,235}};
 *r=colors[season][0];*g=colors[season][1];*b=colors[season][2];
}
/* OWNER_HELPERS */
void OOT_DescribeHeartCosmetics(s16,CwItemDrawInfo*) {}
/* OWNER_MAGIC_DESCRIPTOR */
int32_t OOT_FillSongDrawInfo(RandomizerGet,CwItemDrawInfo*) {return 0;} // Songs have their own production fixture.
/* OWNER_DESCRIPTOR */
namespace ItemGrantAudit {struct Scope {Scope(const char*,int,int,bool) {}};}
struct OTRGlobals {static inline OTRGlobals* Instance=nullptr;void* gRandomizer;void* gRandoContext;};
static bool OOT_BossSoulUsesSkeleton(RandomizerGet) {return false;}
/* OWNER_EXPORT */
/* HOST_INFO */
RandomizerGet selected=RG_SHEIKAH_SLATE;
bool useExport=false;
namespace ComboRando {constexpr int GAME_OOT=0;struct ForeignItem {int itemGame=GAME_OOT;std::string itemName="item",fakeItemName;bool HasDisguise()const{return false;}};}
using RandoCheckId=int;
namespace Rando::MiscBehavior {const ComboRando::ForeignItem* MM_LookupForeign(int) {static ComboRando::ForeignItem item;return &item;}}
int malformed=0;
int32_t Describe(const char*,CwItemDrawInfo* out) {
 if(useExport)return OOT_GetItemDrawInfo("Pendant of Memories",out);
 int result=OOT_FillItemDrawInfo(selected,out);
 out->stateDependent=OOT_DrawDependency(selected,*out);
 if(malformed==1)out->opCount=CW_DRAW_MAX_OPS+1;
 if(malformed==2)out->xluStartIndex=out->dlistCount+1;
 if(malformed==3) {out->opCount=1;out->ops[0].op=CW_OP_DLIST;}
 if(malformed==4)out->dlists[0]="__OTR__@invalid:model";
 if(malformed==5)out->ops[0].a=-1;
 if(malformed==6)out->ops[0].a=1.5f;
 if(malformed==7)out->opCount=0;
 if(malformed==8)out->ops[0].op=CW_OP_DLIST;
 if(malformed==9)out->neiEffect=0;
 if(malformed==10)out->neiEffect=7;
 if(malformed==11) {out->dlistCount=1;out->dlists[0]="__OTR__objects/object_nei_rod_of_seasons/gNeiRodOfSeasonsDL";}
 if(malformed==12)out->dlistCount=0;
 if(malformed==13) {out->opCount=1;out->ops[0].op=CW_OP_SCALE;}
 return result;
}
void* Combo_ResolveSym(const char*,const char* name) {return !strcmp(name,"OOT_GetItemDrawInfo")?(void*)Describe:nullptr;}
const char* ComboInternRoutedPathOOT(const std::string& path) {static std::set<std::string> pool;return pool.insert(path).first->c_str();}
enum class ComboForeignResolveOOT {Ok,Unknown,NotReady};
struct Gfx {int stream;};Gfx opa[64],xlu[64];
struct GraphicsContext {Gfx* o=opa;Gfx* x=xlu;} gfx;
struct PlayState {struct {GraphicsContext* gfxCtx=&gfx;int frames=0;} state;int gameplayFrames=9;} play;
PlayState* gPlayState=&play;
/* HOST_RESOLVER */
std::vector<std::pair<int,std::string>> submitted;
std::vector<std::string> transforms;
std::vector<std::vector<int>> flames;
int matrixDepth=0, cullDisable=0, cullRestore=0;
#define OPEN_DISPS(g) ((void)(g))
#define CLOSE_DISPS(g) ((void)(g))
#define POLY_OPA_DISP gfx.o
#define POLY_XLU_DISP gfx.x
#define MM_FOREIGN_PIN_OPA() ((void)0)
#define MM_FOREIGN_PIN_XLU() ((void)0)
#define MATRIX_FINALIZE_AND_LOAD(p,g) ((void)(p))
#define MTXMODE_APPLY 1
#define gSPDisplayList(p,dl) submitted.emplace_back((p)->stream,(const char*)(dl))
#define gDPSetGrayscaleColor(p,...) ((void)(p))
#define gSPGrayscale(p,...) ((void)(p))
#define gDPPipeSync(p) ((void)(p))
#define gDPSetPrimColor(p,...) ((void)(p))
#define gDPSetEnvColor(p,...) ((void)(p))
#define gSPClearGeometryMode(p,...) ((void)(p), ++cullDisable)
#define gSPSetGeometryMode(p,...) ((void)(p), ++cullRestore)
void Matrix_Push() {++matrixDepth;} void Matrix_Pop() {assert(matrixDepth>0);--matrixDepth;}
void Matrix_Scale(float,float,float,int) {transforms.push_back("scale");}
void Matrix_RotateYF(float,int) {transforms.push_back("spin");}
void Matrix_RotateXF(float,int) {transforms.push_back("x");}
void Matrix_RotateZF(float,int) {transforms.push_back("z");}
void Matrix_Translate(float,float,float,int) {transforms.push_back("translate");}
void Gfx_SetupDL25_Opa(GraphicsContext*) {} void Gfx_SetupDL25_Xlu(GraphicsContext*) {}
void Gfx_SetupDL26_Opa(GraphicsContext*) {}
void DrawOotSlateRuneFlame(uint8_t r,uint8_t g,uint8_t b) {assert(matrixDepth==0);flames.push_back({r,g,b});}
#ifndef M_PIf
constexpr float M_PIf=3.14159265358979323846f;
#endif
const char* gIKAxeInlineDL="__native_inline_axe";
/* HOST_AXE_DRAW */
int nativeCalled=0;
std::set<std::string> hostResources,hostMods;
int ResourceMgr_IsModAsset(const char* path) {return hostMods.contains(path);}
int ResourceMgr_IsModAssetForGame(const char*,const char* path) {return ResourceMgr_IsModAsset(path);}
int ResourceMgr_GetIkanaShieldGiTiltXForGame(const char*,const char*,float*) {return 0;}
int ResourceMgr_GetIkanaShieldGiPoseForGame(const char*,const char*,float*,float*) {return 0;}
uint8_t ResourceMgr_FileExists(const char* path) {return hostResources.contains(path);}
void* OotAssets_LoadGfx(const char* path) {return hostResources.contains(path)?(void*)path:nullptr;}
void* OotAssets_LoadGfxDirect(const char* path) {return OotAssets_LoadGfx(path);}
const char* NeiResource_Route(const char* path) {return ComboInternRoutedPathOOT(std::string("__OTR__@oot:")+(path+7));}
int NeiResource_Available(const char* path) {return OOT_NeiResourceExists(path);}
void NeiGi_DrawRewardMaterial(PlayState*, int, const char*, const char*, const char*) {
 assert(false && "Sage's Tunic miniature medallions must keep the native material");
}
/* HOST_TUNIC_DRAW */
void DrawOotExtPegasusAnklet() {nativeCalled=CW_OOT_EQUIP_PEGASUS_BOOTS;}
void DrawOotExtTrident() {nativeCalled=CW_OOT_EQUIP_TRIDENT;}
void DrawOotExtClimbBoots() {nativeCalled=CW_OOT_EQUIP_CLIMB_BOOTS;}
void DrawOotExtRocBoots() {nativeCalled=CW_OOT_EQUIP_ROC_BOOTS;}
/* HOST_NATIVE_DRAW */
// The production custom dispatcher also handles Din's separate sword layers.
// Their real material and host submission paths run in the sword fixture;
// this static recipe fixture must never dispatch the sword-layer path.
void ComboDinSwordGi_DrawLayers(PlayState*, const char*, const char*) {
 assert(false && "static GI fixture unexpectedly selected Din sword layers");
}
/* HOST_CUSTOM_DRAW */
int main() {
 resources.insert("__OTR__objects/object_gi_clothes/gGiTunicCollarDL");
 assert(OOT_NeiResourceExists("__OTR__objects/object_gi_clothes/gGiTunicCollarDL") == 1 &&
        "MM-first tunic lookup must use the resident owner archive, not a cold OoT ExtensionCache");
 resources.clear();
 resources.insert("__OTR__objects/object_gi_clothes/alias-only.meta");
 assert(OOT_NeiResourceExists("__OTR__objects/object_gi_clothes/alias-only") == 1 &&
        "a cold owner must accept a resource supplied only through alias metadata");
 resources.clear();
 resources.insert("__OTR__alt/objects/object_gi_clothes/alias-only.meta");
 assert(OOT_NeiResourceExists("objects/object_gi_clothes/alias-only") == 0);
 ownerAlt=true;
 assert(OOT_NeiResourceExists("objects/object_gi_clothes/alias-only") == 1 &&
        "alias-only Alt resources must follow the registered owner's Alt selection");
 ownerAlt=false;
 resources.clear();
 OTRGlobals globals{&globals,&globals};OTRGlobals::Instance=&globals;
 /* NAME_MAP_INIT */
 CwItemDrawInfo dependency{};
 for(auto sword:{RG_KOKIRI_SWORD,RG_RAZOR_SWORD,RG_GILDED_SWORD,RG_MASTER_SWORD,
                 RG_TRUE_MASTER_SWORD,RG_BIGGORON_SWORD,RG_GREAT_FAIRY_SWORD}) {
  dependency.drawKind=CW_DRAW_KIND_NEI_GI;
  assert(OOT_DrawDependency(sword,dependency)==2);
  dependency.drawKind=CW_DRAW_KIND_CUSTOM_GI;
  assert(OOT_DrawDependency(sword,dependency)==2);
 }
 for(auto progressive:{RG_PROGRESSIVE_KOKIRI_SWORD,RG_PROGRESSIVE_MASTER_SWORD,
                       RG_PROGRESSIVE_BGS,RG_PROGRESSIVE_HAMMER}) {
  dependency.drawKind=CW_DRAW_KIND_NEI_GI;
  assert(OOT_DrawDependency(progressive,dependency)==1);
 }
 dependency={};dependency.drawKind=CW_DRAW_KIND_NEI_GI;
 assert(OOT_DrawDependency(RG_SHEIKAH_SLATE,dependency)==2);
 for(auto& cmd:opa)cmd.stream=0;
 for(auto& cmd:xlu)cmd.stream=1;
 ownerAlt=true;hostAlt=false;magicLoaded=false;
 assert(OOT_MagicJarUsesCustomAsset("__OTR__magic")==1 && magicLoaded && Ship::activeRm==Ship::hostRm);
 assert(loadedPaths==std::vector<std::string>({"alt/magic"}));
 loadedPaths.clear();
 ownerAlt=false;hostAlt=true;magicLoaded=false;
 assert(OOT_MagicJarUsesCustomAsset("magic")==0 && magicLoaded && Ship::activeRm==Ship::hostRm);
 assert(loadedPaths==std::vector<std::string>({"magic"}));
 loadedPaths.clear();ownerAlt=true;magicAltPresent=false;
 assert(OOT_MagicJarUsesCustomAsset("magic")==0 && loadedPaths==std::vector<std::string>({"magic"}));
 loadedPaths.clear();magicAltMetaPresent=true;
 assert(OOT_MagicJarUsesCustomAsset("magic")==1 && loadedPaths==std::vector<std::string>({"alt/magic"}));
 loadedPaths.clear();magicAltMetaPresent=false;magicAltPresent=true;magicAltLoads=false;
 assert(OOT_MagicJarUsesCustomAsset("magic")==0 && loadedPaths==std::vector<std::string>({"alt/magic","magic"}));
 loadedPaths.clear();magicAltLoads=true;
 magicResourcePresent=false;magicLoaded=false;
 assert(OOT_MagicJarUsesCustomAsset("magic")==0 && magicLoaded);
 ownerPresent=false;magicLoaded=false;
 assert(OOT_MagicJarUsesCustomAsset("magic")==0 && !magicLoaded);
 assert(OOT_NeiAltAssetsEnabled()==0);
 ownerPresent=true;magicResourcePresent=true;ownerAlt=false;hostAlt=false;
 assert(OOT_MagicJarUsesCustomAsset(nullptr)==0);
 resources.insert("__OTR__objects/object_nei_sheikah_slate/gNeiSheikahSlateDL");
 CwItemDrawInfo out{};
 assert(OOT_FillItemDrawInfo(RG_SHEIKAH_SLATE,&out)==1);
 assert(!strcmp(out.dlists[0],"__OTR__objects/object_nei_sheikah_slate/gNeiSheikahSlateDL"));
 assert(out.scale==.35f);
 selected=RG_SLATE_RUNE_STASIS;
 ComboForeignDrawInfoOOT info;
 assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Ok);
 MM_DrawForeignCustomGi(&info);
 assert(submitted.size()==1 && submitted[0].second=="__OTR__@oot:objects/object_nei_sheikah_slate/gNeiSheikahSlateDL");
 assert(flames==std::vector<std::vector<int>>({{250,200,70}}) && matrixDepth==0);
 out={}; assert(OOT_FillItemDrawInfo(RG_DEFENSE_UPGRADE,&out)==1);
 assert(out.dlists[0]==gStatDefenseDL);
 assert(out.itemShimmer && out.itemShimmerColor[2]==255 && out.itemShimmerColor[0]==64);
 out={}; assert(OOT_FillItemDrawInfo(RG_POWER_UPGRADE,&out)==1);
 assert(out.dlists[0]==gStatPowerDL);
 assert(out.itemShimmer && out.itemShimmerColor[0]==255 && out.itemShimmerColor[2]==64);

 // Individual seasons resolve without a rod resource, and carry no model DL.
 for(auto season:{RG_SEASON_SPRING,RG_SEASON_SUMMER,RG_SEASON_AUTUMN,RG_SEASON_WINTER}) {
  selected=season;info={};
  assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Ok);
  assert(info.drawKind==36 && info.neiEffect==1+season-RG_SEASON_SPRING);
  assert(info.count==0 && info.dls[0]==nullptr && info.opCount==0);
  assert(info.primColorXlu[3]==0);
 }
 resources.insert("__OTR__objects/object_nei_rod_of_seasons/gNeiRodOfSeasonsDL");
 selected=RG_ROD_OF_SEASONS;info={};
 assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Ok);
 assert(info.neiEffect==5 && info.count==1 && info.dls[0] && info.scale==.35f);
 for(malformed=9;malformed<=13;++malformed) {
  selected=malformed==12 ? RG_ROD_OF_SEASONS : RG_SEASON_SPRING;info={};
  assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Unknown);
 }
 malformed=0;
 const char* wandPaths[][2]={
  {"sand_rod","SandRod"},{"tornado_rod","TornadoRod"},{"water_rod","WaterRod"},
  {"meteor_rod","MeteorRod"},{"storm_rod","StormRod"},{"shadow_scepter","ShadowScepter"}};
 for(int i=0;i<6;++i) {
  const std::string stem=std::string("__OTR__objects/object_nei_wand_")+wandPaths[i][0]+"/gNei"+wandPaths[i][1];
  resources.insert(stem+"DL");resources.insert(stem+"XluDL");
  selected=static_cast<RandomizerGet>(RG_WAND_SAND_ROD+i);info={};
  assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Ok);
  assert(info.count==2 && info.xluStart==1);
  gfx.o=opa;gfx.x=xlu;submitted.clear();transforms.clear();flames.clear();
  MM_DrawForeignCustomGi(&info);
  assert(submitted.size()==2 && submitted[0].first==0 && submitted[1].first==1);
  assert(matrixDepth==0);
  resources.erase(stem+"XluDL");out={};
  assert(OOT_FillItemDrawInfo(selected,&out)==0);
 }
 const RandomizerGet tiers[]={RG_RAZOR_SWORD,RG_GILDED_SWORD,RG_GREAT_FAIRY_SWORD};
 for(auto tier:tiers) {
  concrete=tier;selected=RG_PROGRESSIVE_KOKIRI_SWORD;info={};
  assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Ok);
  assert(std::string(info.dls[0]).starts_with("__OTR__@mm:objects/object_gi_sword_"));
  assert(info.count==2 && info.xluStart==(tier==RG_GREAT_FAIRY_SWORD?1:-1));
  assert(info.resolvedName=="concrete award");
 }
 ownerAlt=false;
 for(auto base:{RG_KOKIRI_SWORD,RG_BIGGORON_SWORD}) {
  concrete=base;selected=RG_PROGRESSIVE_KOKIRI_SWORD;out={};
  assert(OOT_FillItemDrawInfo(selected,&out)==1 && out.itemShimmer==1);
  assert(out.itemShimmerColor[3]==255);
 }
 resources.insert("__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL");
 concrete=RG_TRUE_MASTER_SWORD;selected=RG_PROGRESSIVE_MASTER_SWORD;
 ownerAlt=true;info={};
 assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Ok);
 assert(std::string(info.dls[0])=="__OTR__@oot:alt/objects/object_custom_equip/gCustomMasterSwordDL");
 assert(info.opCount==1 && info.ops[0].op==CW_OP_ROTATE_Z);
 ownerAlt=false;info={};
 assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Ok);
 assert(info.drawKind==CW_DRAW_KIND_MASTER_SWORD && info.primColorXlu[0]==120);
 concrete=RG_NONE;ownerAlt=false;
 struct SelectedSwordTheme {RandomizerGet rg;NeiGi::Kind kind;const char* equipment;const char* fire;};
 const SelectedSwordTheme swordThemes[]={
  {RG_KOKIRI_SWORD,NeiGi::Kind::KokiriSword,"KokiriSword","child"},
  {RG_RAZOR_SWORD,NeiGi::Kind::RazorSword,"KokiriSword","child"},
  {RG_GILDED_SWORD,NeiGi::Kind::GildedSword,"KokiriSword","child"},
  {RG_MASTER_SWORD,NeiGi::Kind::MasterSword,"MasterSword","adult"},
  {RG_TRUE_MASTER_SWORD,NeiGi::Kind::SwordAura,"MasterSword","adult"},
  {RG_BIGGORON_SWORD,NeiGi::Kind::BiggoronSword,"Longsword","bgs"},
  {RG_GREAT_FAIRY_SWORD,NeiGi::Kind::GreatFairySword,"Longsword","bgs"}};
 for(const auto& theme:swordThemes)for(bool fire:{false,true}) {
  ownerAlt=true;dinSelected=fire;concrete=theme.rg;selected=RG_PROGRESSIVE_MASTER_SWORD;
  resources.insert(std::string("__OTR__alt/objects/object_custom_equip/gCustom")+theme.equipment+"DL");
  const auto firePath=std::string("__OTR__objects/din_fire_sword/progressive/")+theme.fire+"/SwordDL";
  if(fire)resources.insert(firePath);else resources.erase(firePath);
  info={};assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Ok);
  assert(info.drawKind==CW_DRAW_KIND_CUSTOM_GI && info.itemShimmer);
  assert(info.neiShimmer==int(theme.kind)+1 && "selected standalone/Din full producer lost its item-specific identity");
  assert(info.neiEffect==0 && !info.primColorOpa[3]);
  assert(info.primColorXlu[3]==(theme.rg==RG_TRUE_MASTER_SWORD?255:0));
  assert(bool(std::strstr(info.dls[0],"din_fire_sword"))==fire);
  assert(info.opCount==1 && info.ops[0].op==CW_OP_ROTATE_Z && info.scale==.04f);
 }
 ownerAlt=false;dinSelected=false;concrete=RG_NONE;
 std::cout<<"PASS selected standalone/Din full producer+resolver: seven exact themes, independent shimmer and unchanged geometry/flame fields\n";
 const std::pair<RandomizerGet,const char*> direct[] = {
  {RG_LANTERN,"__OTR__objects/object_poh/gPoeLanternDL"},
  {RG_POKEBALL,"__OTR__objects/object_nei_pokeball/ItmPokeBall_opaque_dl"},
  {RG_MARIO_MASK,"__OTR__objects/object_nei_mario_mask/g_mario_mask_dl"},
  {RG_EXT_CANE_OF_BYRNA,"__OTR__objects/object_somaria/g_byrna_cane_give_dl"},
  {RG_EXT_MAGIC_CAPE,"__OTR__objects/object_nei_magic_cape/gNeiMagicCapeDL"},
  {RG_ULTRASHOT,"__OTR__objects/object_gi_hookshot/gGiLongshotDL"},
  {RG_NET,"__OTR__objects/object_nei_net/g_net_dl"},
 };
 resources.insert("__OTR__objects/object_nei_net/g_net_xlu_dl");
 resources.insert("__OTR__objects/object_nei_magic_cape/gNeiMagicCapeWaveDL");
 for(auto [rg,path]:direct) {
  resources.insert(path);selected=rg;info={};
  assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Ok);
  if(rg==RG_LANTERN || rg==RG_POKEBALL || rg==RG_MARIO_MASK) {
   assert(info.appearanceDependent);
   gfx.o=opa;gfx.x=xlu;submitted.clear();transforms.clear();cullDisable=cullRestore=0;
   MM_DrawForeignCustomGi(&info);
   assert(submitted.size()==1 && submitted[0].second==std::string("__OTR__@oot:")+(path+7));
   assert(matrixDepth==0);
   if(rg==RG_MARIO_MASK) {
    assert(info.scale==.038f && info.opCount==2 && info.itemShimmer);
    assert(cullDisable==1 && cullRestore==1);
    assert(std::find(transforms.begin(),transforms.end(),"x")!=transforms.end());
   } else {
    assert(info.scale==(rg==RG_LANTERN?.025f:.18f) && cullDisable==0 && cullRestore==0);
   }
  }
 }
 selected=RG_EXT_MAGIC_CAPE;info={};
 assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Ok);
 for(int frame:{0,8}) {
  play.gameplayFrames=frame;gfx.o=opa;gfx.x=xlu;submitted.clear();
  MM_DrawForeignCustomGi(&info);
  assert(submitted.size()==1);
  assert(submitted[0].second==std::string("__OTR__@oot:objects/object_nei_magic_cape/")+
   (frame?"gNeiMagicCapeWaveDL":"gNeiMagicCapeDL"));
 }
 concrete=RG_NONE;
 for(auto rg:{RG_IRON_KNUCKLE_AXE,RG_EXT_TRIDENT,RG_EXT_SPIRIT_BREASTPLATE,RG_EXT_CHAMPIONS_TUNIC,
              RG_EXT_WATER_DRAGON_SCALE,RG_EXT_PEGASUS_ANKLET,RG_EXT_CLIMB_BOOTS,RG_EXT_ROC_BOOTS}) {
  selected=rg;info={};assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Ok);
  nativeCalled=0;gfx.o=opa;gfx.x=xlu;submitted.clear();transforms.clear();
  MM_DrawForeignNativeEquipment(&info);
  if(rg==RG_IRON_KNUCKLE_AXE) {
   assert(submitted.size()==1 && submitted[0].second=="__native_inline_axe");
   assert(transforms==std::vector<std::string>({"x","scale","translate"}));
  } else if(rg!=RG_EXT_SPIRIT_BREASTPLATE && rg!=RG_EXT_CHAMPIONS_TUNIC && rg!=RG_EXT_WATER_DRAGON_SCALE)
   assert(nativeCalled==static_cast<int>(info.ops[0].a));
  assert(matrixDepth==0);
 }
 selected=RG_IRON_KNUCKLE_AXE;concrete=RG_NONE;
 for(malformed=5;malformed<=8;++malformed) {
  info={};assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Unknown);
 }
 malformed=0;
 ownerAlt=true;concrete=RG_TRUE_MASTER_SWORD;selected=RG_PROGRESSIVE_MASTER_SWORD;
 for(malformed=1;malformed<=4;++malformed) {
  info={};assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Unknown);
 }
 malformed=0;ownerAlt=false;
 concrete=RG_NONE;resources.clear();out={};
 assert(OOT_FillItemDrawInfo(RG_SHEIKAH_SLATE,&out)==0);
 for(auto rg:{RG_OPEN_CHEST,RG_CRAWL,RG_CLIMB,RG_POWER_BRACELET}) {out={};assert(OOT_FillItemDrawInfo(rg,&out)==1);}
 concrete=RG_MAGIC_STAT_UPGRADE; changedMagic=true; out={};
 assert(OOT_FillItemDrawInfo(RG_MAGIC_STAT_UPGRADE,&out)==1 && out.itemShimmer && out.stateDependent==2);
 assert(out.itemShimmerColor[0]==12 && out.itemShimmerColor[1]==210 && out.itemShimmerColor[2]==250);
 liveMagic={240,40,190}; out={}; assert(OOT_FillItemDrawInfo(RG_MAGIC_STAT_UPGRADE,&out)==1);
 assert(out.itemShimmerColor[0]==240 && out.itemShimmerColor[1]==40 && out.itemShimmerColor[2]==190);
 concrete=RG_NONE;selected=RG_BOMB_ARROWS;info={};
 resources.insert("__OTR__objects/object_nei_bombarrows/gBombarrowsGiveDL");
 assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Ok && "Bomb Arrows must reach a portable GI recipe");
 gfx.o=opa;gfx.x=xlu;submitted.clear();transforms.clear();
 MM_DrawForeignCustomGi(&info);
 assert((submitted==std::vector<std::pair<int,std::string>>({{0,"__OTR__@oot:objects/object_nei_bombarrows/gBombarrowsGiveDL"}})));
 assert(info.scale==.5f && std::find(transforms.begin(),transforms.end(),"z")!=transforms.end() && matrixDepth==0);
 assert(Rando::StaticData::itemNameToEnum.at("Pendant of Memories")==RG_MM_PENDANT_OF_MEMORIES);
 useExport=true;info={};
 assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Ok && "the exported Pendant English name must resolve the actual imported MM row");
 assert(info.count==2 && info.xluStart==1);
 assert(info.itemShimmer && info.neiShimmer==int(NeiGi::Kind::PendantOfMemories)+1 && info.stateDependent && info.appearanceDependent);
 assert(std::string(info.dls[0])=="__OTR__@mm:objects/object_gi_reserve_c_01/gGiPendantOfMemoriesEmptyDL");
 assert(std::string(info.dls[1])=="__OTR__@mm:objects/object_gi_reserve_c_01/gGiPendantOfMemoriesDL");
 useExport=false;
 const char* collar="__OTR__objects/object_gi_clothes/gGiTunicCollarDL";
 const char* tunic="__OTR__objects/object_gi_clothes/gGiTunicDL";
 resources.insert(collar);resources.insert(tunic);
 const char* medallionFaces[] = {
  "__OTR__objects/object_gi_medal/gGiForestMedallionFaceDL",
  "__OTR__objects/object_gi_medal/gGiFireMedallionFaceDL",
  "__OTR__objects/object_gi_medal/gGiWaterMedallionFaceDL",
  "__OTR__objects/object_gi_medal/gGiSpiritMedallionFaceDL",
  "__OTR__objects/object_gi_medal/gGiShadowMedallionFaceDL",
  "__OTR__objects/object_gi_medal/gGiLightMedallionFaceDL",
 };
 const char* medallionRing="__OTR__objects/object_gi_medal/gGiMedallionDL";
 for(const char* path:medallionFaces)hostResources.insert(path);
 hostResources.insert(medallionRing);
 for(auto rg:{RG_EXT_CHAMPIONS_TUNIC,RG_EXT_WATER_DRAGON_SCALE,RG_EXT_SPIRIT_BREASTPLATE}) {
  selected=rg;info={};assert(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Ok);
  gfx.o=opa;gfx.x=xlu;submitted.clear();
  MM_DrawForeignNativeEquipment(&info);
  assert(submitted.size()>=2 && "donor-only tunic geometry must be submitted while MM is active");
  const size_t medallionCommands=submitted.size()-2;
  assert(submitted[medallionCommands].second=="__OTR__@oot:objects/object_gi_clothes/gGiTunicCollarDL");
  assert(submitted[medallionCommands+1].second=="__OTR__@oot:objects/object_gi_clothes/gGiTunicDL");
  if(rg==RG_EXT_WATER_DRAGON_SCALE) {
   assert(medallionCommands>0 && medallionCommands%2==0);
   for(size_t i=0;i<medallionCommands;i+=2) {
    assert(std::find(std::begin(medallionFaces),std::end(medallionFaces),submitted[i].second)!=std::end(medallionFaces));
    assert(submitted[i+1].second==medallionRing);
   }
  } else assert(medallionCommands==0);
  assert(matrixDepth==0);
 }
 hostResources.insert(collar);hostResources.insert(tunic);hostMods.insert(tunic);
 submitted.clear();gfx.o=opa;DrawOotExtChampionsTunic();
 assert(submitted.size()==2 && submitted[1].second==tunic && "a winning MM tunic replacement keeps its host owner");
 hostResources.clear();hostMods.clear();resources.erase(tunic);
 submitted.clear();gfx.o=opa;DrawOotExtChampionsTunic();assert(submitted.empty());
 resources.insert(tunic);DrawOotExtChampionsTunic();assert(submitted.size()==2 && matrixDepth==0);
 std::cout<<"PASS active MM donor-only tunics, live host overrides/missing recovery, exported duplicate-name Pendant and Bomb Arrows GI routing\n";
 std::cout<<"PASS production static custom GI recipes, rune flame, native MM tiers and resolved awards\n";
}
