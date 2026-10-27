#include <cassert>
#include <atomic>
#define SPDLOG_WARN(...) ((void)0)
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
#include "combo/menu/ComboBottleContents.h"
#include "combo/menu/ComboElementalArrowGi.h"
#include "combo/menu/ComboRewardGi.h"
#include "combo/menu/ComboItemEffectColors.h"
#include "soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include "soh/mods/mm_sources/objects/object_gi_masks_all.h"
#include "soh/mods/mm_sources/objects/object_mm_rando_items.h"
#include "z64item.h"
// The actual owner export scopes its resource manager even for native mask rows.
namespace Ship {
inline bool ownerActive = false;
struct CrossRMRegistry {
 static int Get(const char* game) { assert(!std::strcmp(game,"oot")); return 1; }
};
struct ResourceManagerScope {
 bool before = ownerActive;
 explicit ResourceManagerScope(int owner) { assert(owner==1); ownerActive=true; }
 ~ResourceManagerScope() { ownerActive=before; }
};
}
/* NATIVE_ITEM_IDS */
/* HOST_NATIVE_IMPORT */
#define RANDO_ENUM_BEGIN(x) enum x {
#define RANDO_ENUM_ITEM(x) x,
#define RANDO_ENUM_END(x) };
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
using s16=int16_t;using s32=int32_t;using f32=float;
#define ARRAY_COUNT(a) (sizeof(a)/sizeof((a)[0]))
void* Combo_ResolveSym(const char*,const char*);
int CVarGetInteger(const char*, int fallback) { return fallback; }
/* MASK_TABLE */
constexpr int TABLE_RANDOMIZER=1;
void Randomizer_DrawMmMask() {}
void Randomizer_DrawMmRemains() {}
void Randomizer_DrawCaneOfSomaria() {}
void Randomizer_DrawCanePacci() {}
void Randomizer_DrawCaneSomariaUpgrade() {}
void Randomizer_DrawCanePacciUpgrade() {}
void Randomizer_DrawCanePacciUltrahand() {}
struct GetItemEntry {int tableId,drawItemId,gid,itemId;void (*drawFunc)();};
namespace Rando::StaticData {
struct Name {std::string english="mask";};
struct Item {
 RandomizerGet rg;
 std::shared_ptr<GetItemEntry> GetGIEntry(RandomizerGet*) {
  if(rg>=RG_MM_REMAINS_ODOLWA && rg<=RG_MM_REMAINS_TWINMOLD)
   return std::make_shared<GetItemEntry>(GetItemEntry{TABLE_RANDOMIZER,rg,0,rg,Randomizer_DrawMmRemains});
  bool native=rg==RG_MM_MASK_GORON || rg==RG_MM_MASK_ZORA || rg==RG_MM_MASK_KEATON || rg==RG_MM_MASK_BUNNY || rg==RG_MM_MASK_TRUTH;
  return std::make_shared<GetItemEntry>(GetItemEntry{TABLE_RANDOMIZER,rg,native?40:0,ITEM_MM_MASK_POSTMAN+(int)rg-(int)RG_MM_MASK_POSTMAN,native?nullptr:Randomizer_DrawMmMask});
 }
 const Name& GetName() {static Name name;return name;}
};
Item RetrieveItem(RandomizerGet rg) {return {rg};}
}
int nativeRows=0;
int NeiGi_DescribeEntry(const GetItemEntry*,CwItemDrawInfo*) {return 0;}
int GetItem_GetDrawTableEntry(int,void**,int,int*,float*,int*,uint8_t*) {++nativeRows;return 0;}
void GetItem_GetDrawSetupDLs(int,void**,void**) {}
int GetItem_GetShimmerColor(s16,uint8_t*) {return 0;}
void OOT_DescribeHeartCosmetics(s16,CwItemDrawInfo*) {}
void OOT_DescribeMagicJar(s16,CwItemDrawInfo*) {}
int CwAltSwordGi(RandomizerGet,CwItemDrawInfo*) {return 0;} // Static weapon policy is exercised separately.
int32_t OOT_FillSongDrawInfo(RandomizerGet,CwItemDrawInfo*) {return 0;} // Song producer tested separately.
/* OWNER_DESCRIPTOR */
/* HOST_INFO */
RandomizerGet selected=RG_MM_MASK_POSTMAN;
int producerResult=1;
bool magicSelected=false; uint8_t magicRed=53;
int32_t Describe(const char*,CwItemDrawInfo* out) {
 if(producerResult!=1) return producerResult;
 if(magicSelected) {out->drawKind=CW_DRAW_KIND_MAGIC_JAR;out->dlistCount=1;out->xluStartIndex=-1;out->dlists[0]="__OTR__magic";out->stateDependent=2;out->primColorOpa[0]=magicRed;out->primColorOpa[3]=255;return 1;}
 return OOT_FillItemDrawInfo(selected,out);
}
namespace ComboRando {
constexpr int GAME_OOT=0;
struct ForeignItem {int itemGame=GAME_OOT;std::string itemName="mask",fakeItemName;bool HasDisguise()const{return !fakeItemName.empty();}};
}
using RandoCheckId=int;
ComboRando::ForeignItem foreign;
namespace Rando::MiscBehavior {const ComboRando::ForeignItem* MM_LookupForeign(int) {return &foreign;}}
int spinResult=1, spinRed=17;
int32_t SpinDonor(const char* name,CwItemDrawInfo* out) {
 assert(!strcmp(name,"Great Spin Attack"));
 if(spinResult!=1)return spinResult;
 out->drawKind=CW_DRAW_KIND_MM_SPIN_ATTACK;out->dlistCount=2;
 out->dlists[0]="__OTR__objects/gameplay_keep/gGreatSpinAttackDiskDL";
 out->dlists[1]="__OTR__objects/gameplay_keep/gGreatSpinAttackCylinderDL";
 out->scale=0.012f;out->stateDependent=2;out->primColorXlu[0]=spinRed;
 return 1;
}
void* Combo_ResolveSym(const char*,const char* name) {
 if(!strcmp(name,"OOT_GetItemDrawInfo"))return (void*)Describe;
 return !strcmp(name,"MM_GetItemDrawInfo")?(void*)SpinDonor:nullptr;
}
const char* ComboInternRoutedPathOOT(const std::string& path) {static std::set<std::string> paths;return paths.insert(path).first->c_str();}
enum class ComboForeignResolveOOT {Ok,Unknown,NotReady};
constexpr int RC_UNKNOWN=-1;
struct {int fileNum=0;} gSaveContext;
namespace Rando::MiscBehavior {uint64_t ComboRandoGen() {return 1;}}
struct Gfx {int stream;};
Gfx opa[64],xlu[64];
struct GraphicsContext {Gfx* o=opa;Gfx* x=xlu;};
GraphicsContext gfx;
struct PlayState {struct {GraphicsContext* gfxCtx=&gfx;} state; int gameplayFrames=0;};
#include "combo/menu/ComboCollectibleShimmerDraw.h"
extern "C" void NeiGi_DrawTintedShimmerOverlay(PlayState*, const uint8_t[4], const char*) {
 assert(false && "mask bridge fixture unexpectedly selected collectible shimmer");
}
PlayState play;PlayState* gPlayState=&play;
NeiGi::Basis NeiGi_CameraBasis(PlayState*) { return {}; }
void NeiGi_DrawMesh(PlayState*, const NeiGi::Mesh&) { assert(0 && "mask dispatch must use its own palette overlay"); }
/* HOST_RESOLVER */
/* HOST_CACHE */
std::vector<std::pair<int,std::string>> submitted;
std::vector<int> luts;
#define OPEN_DISPS(g) ((void)(g))
#define CLOSE_DISPS(g) ((void)(g))
#define POLY_OPA_DISP gfx.o
#define POLY_XLU_DISP gfx.x
#define gSPDisplayList(p,dl) submitted.emplace_back((p)->stream,(const char*)(dl))
#define gDPSetTextureLUT(p,lut) luts.push_back((p)->stream)
#define G_TT_NONE 0
#define MM_FOREIGN_PIN_OPA() ((void)0)
#define MM_FOREIGN_PIN_XLU() ((void)0)
#define MATRIX_FINALIZE_AND_LOAD(p,g) ((void)(p))
#define MTXMODE_APPLY 1
float scale=0;
int matrixDepth=0,shimmerDraws=0;
uint8_t shimmerColor[4]{};
void Matrix_Push() {++matrixDepth;}
void Matrix_Pop() {assert(matrixDepth>0);--matrixDepth;}
void Matrix_RotateZF(float, int) {assert(false && "mask/remains must not consume a shield roll");}
void Matrix_RotateXF(float, int) {assert(false && "mask/remains must not consume a shield tilt");}
void ComboDrawMaskShimmer(PlayState*,const char* root,const uint8_t color[4],const char* owner) {
 assert(root == nullptr && !strcmp(owner,"mm"));
 assert(matrixDepth==0);memcpy(shimmerColor,color,4);++shimmerDraws;
}
void Matrix_Scale(float x,float y,float z,int) {assert(x==y && y==z);scale=x;}
void Gfx_SetupDL25_Opa(GraphicsContext*) {}
void Gfx_SetupDL25_Xlu(GraphicsContext*) {}
#define gDPSetEnvColor(p,...) ((void)(p))
std::vector<int> grayEvents;
uint8_t submittedMagicRed=0;
#define gDPSetGrayscaleColor(p,r,g,b,a) ((void)(p), submittedMagicRed=(r))
#define gSPGrayscale(p,on) ((void)(p), grayEvents.push_back(on))
/* HOST_MAGIC_DRAW */
/* HOST_SIMPLE_DRAW */
int spinDraws=0;
void ComboDrawSpinAttackGi(PlayState*,const char* disk,const char* cylinder,float size,const uint8_t*,const char* owner) {
 assert(strstr(disk,"__OTR__@mm:objects/gameplay_keep/gGreatSpinAttackDiskDL"));
 assert(strstr(cylinder,"__OTR__@mm:objects/gameplay_keep/gGreatSpinAttackCylinderDL"));
 assert(size==0.012f && !strcmp(owner,"mm"));spinDraws++;
}
/* HOST_SPIN_DRAW */
int sentinels=0;
ComboForeignDrawInfoOOT current;
const ComboForeignDrawInfoOOT* ComboResolveForeignDrawInfoOOT(int rc) {return ComboFillForeignDrawInfoOOT(rc,current)==ComboForeignResolveOOT::Ok?&current:nullptr;}
void GetItem_Draw(PlayState*,int gid) {assert(gid==GID_RUPEE_BLUE);++sentinels;}
bool ComboForeignAnim_Draw(const CwItemAnimDrawInfo*,const char*,PlayState*) {assert(0);return false;}
struct Actor {};
std::vector<RandoItemId> nativeDraws;
namespace Rando {
void DrawResolvedItem(RandoItemId item, RandoCheckId check, Actor*) {
 assert(check == RC_UNKNOWN);
 nativeDraws.push_back(item);
}
}
void DrawOotNeiUltrahand() {assert(0);}
void DrawOotNeiCaneOfSomaria(int) {assert(0);}
void MM_DrawNeiGi(const CwItemDrawInfo&,bool shop=false,int mmPickup=0) {assert(0);}
void NeiGi_DrawSeasonOverlay(PlayState*,int,const char*) {assert(0 && "mask fixture must not select weather");}
void NeiGi_DrawSongOverlay(PlayState*,int,const char*) {assert(false && "song dispatch has its own production fixture");}
extern "C" void NeiGi_DrawElementalArrow(PlayState*,int) {assert(false && "mask fixture must not dispatch elemental arrows");}
extern "C" void NeiGi_DrawElementalArrowForOwner(PlayState*,int,int,int) {assert(false && "mask fixture must not dispatch elemental arrows");}
extern "C" void NeiGi_DrawRewardMaterial(PlayState*,int,const char*,const char*,const char*) {assert(false && "mask fixture must not dispatch reward materials");}
extern "C" int NeiGi_DrawElementalSpell(PlayState*,int,int,int) {assert(false && "mask fixture must not dispatch elemental spells");return 0;}
extern "C" int ComboBottleContents_Draw(PlayState*,int) {assert(false && "mask fixture must not dispatch bottle contents");return 0;}
// Sword drawing/fitting is tested by the selected-model and foreign sword
// fixtures. Masks must never enter either branch in this dispatcher.
void ComboSwordGi_ApplyPresentationSize(bool=false,int=0) {assert(0 && "mask sword presentation size");}
float ComboSwordGi_SelectedTilt(float,bool=false) {assert(0 && "mask selected sword tilt");return 0;}
void ComboSwordGi_ApplyModelsFit(const char*,const char* const*,int,float,float,bool=false,int=0) {assert(0 && "mask selected sword fit");}
void MM_DrawForeignCustomGi(const ComboForeignDrawInfoOOT*,bool=false,bool=true) {assert(0 && "mask selected custom sword draw");}
void ComboSwordGi_ApplyEffectFit(NeiGi::Kind,bool=false,int=0) {assert(0 && "mask selected sword effects fit");}
float ComboSwordGi_ParticleScale(NeiGi::Kind,bool=false,int=0,bool=true) {assert(0 && "mask selected sword particle size");return 1.f;}
void DrawOotSlateRuneFlame(uint8_t,uint8_t,uint8_t) {assert(0 && "mask selected sword flame");}
/* OTHER_DRAW_HANDLERS */
/* HOST_DISPATCH */
int main() {
 // These are OoT-owned aliases at MM checks, not native placements. The
 // resolver/dispatcher must select MM's existing concrete draw handler.
 const std::pair<const char*,RandoItemId> imports[] = {
  {"Woodfall Map",RI_WOODFALL_MAP},{"Snowhead Map",RI_SNOWHEAD_MAP},
  {"Great Bay Map",RI_GREAT_BAY_MAP},{"Stone Tower Map",RI_STONE_TOWER_MAP},
  {"Woodfall Compass",RI_WOODFALL_COMPASS},{"Snowhead Compass",RI_SNOWHEAD_COMPASS},
  {"Great Bay Compass",RI_GREAT_BAY_COMPASS},{"Stone Tower Compass",RI_STONE_TOWER_COMPASS},
  {"Woodfall Small Key",RI_WOODFALL_SMALL_KEY},{"Snowhead Small Key",RI_SNOWHEAD_SMALL_KEY},
  {"Great Bay Small Key",RI_GREAT_BAY_SMALL_KEY},{"Stone Tower Small Key",RI_STONE_TOWER_SMALL_KEY},
  {"Woodfall Boss Key",RI_WOODFALL_BOSS_KEY},{"Snowhead Boss Key",RI_SNOWHEAD_BOSS_KEY},
  {"Great Bay Boss Key",RI_GREAT_BAY_BOSS_KEY},{"Stone Tower Boss Key",RI_STONE_TOWER_BOSS_KEY},
  {"Soul of Odolwa",RI_SOUL_BOSS_ODOLWA},{"Soul of Goht",RI_SOUL_BOSS_GOHT},
  {"Soul of Gyorg",RI_SOUL_BOSS_GYORG},{"Soul of Twinmold",RI_SOUL_BOSS_TWINMOLD},
  {"Soul of Majora",RI_SOUL_BOSS_MAJORA},{"Bottle With Gold Dust",RI_BOTTLE_GOLD_DUST},
  {"Bottle with Magic Mushroom",RI_OOT_BOTTLE_MAGIC_MUSHROOM},
  {"Room Key",RI_ROOM_KEY},
  {"Tingle's Clock Town Map",RI_TINGLE_MAP_CLOCK_TOWN},
  {"Tingle's Woodfall Map",RI_TINGLE_MAP_WOODFALL},
  {"Tingle's Snowhead Map",RI_TINGLE_MAP_SNOWHEAD},
  {"Tingle's Romani Ranch Map",RI_TINGLE_MAP_ROMANI_RANCH},
  {"Tingle's Great Bay Map",RI_TINGLE_MAP_GREAT_BAY},
  {"Tingle's Stone Tower Map",RI_TINGLE_MAP_STONE_TOWER}
 };
 for(auto [name,native]:imports) {
  foreign.itemName=name;current={};nativeDraws.clear();
  MM_DrawComboForeign(static_cast<RandoCheckId>(1));
  assert(nativeDraws == std::vector<RandoItemId>{native} &&
         "OoT imports of MM items must dispatch their native MM models");
  assert(foreign.itemGame==ComboRando::GAME_OOT && foreign.itemName==name);
 }
 std::cout<<"PASS 30 OoT-owned MM aliases through production foreign resolver and native dispatch; grant owner and names preserved\n";
 foreign.itemName="mask";current={};
 for(auto& cmd:opa)cmd.stream=0;
 for(auto& cmd:xlu)cmd.stream=1;
 assert(CwMinDlistsForKind(CW_DRAW_KIND_MM_MASK)==2);
 CwItemDrawInfo bad{};
 assert(!OOT_DescribeMmMaskDraw(ITEM_MM_MASK_POSTMAN-1,&bad));
 assert(!OOT_DescribeMmMaskDraw(ITEM_MM_MASK_FIERCE_DEITY+1,&bad));
 assert(!OOT_DescribeMmMaskDraw(ITEM_MM_MASK_ROMANI,nullptr));
 for(int i=0;i<24;++i) {
  selected=(RandomizerGet)(RG_MM_MASK_POSTMAN+i);CwItemDrawInfo out{};
  assert(OOT_FillItemDrawInfo(selected,&out)==1 && out.drawKind==CW_DRAW_KIND_MM_MASK);
  assert(nativeRows==0 && out.dlistCount==2);
  const auto& expected=sMmMaskDrawTable[i];
  assert(!strcmp(out.dlists[0],expected.dl1) && !strcmp(out.dlists[1],expected.dl2));
  assert(out.xluStartIndex==(expected.mode==MM_MASK_DRAW_OPA0_XLU1?1:-1));
  uint8_t expectedColor[4];ComboMmMaskShimmerColor(i,expectedColor);
  assert(out.itemShimmer && !memcmp(out.itemShimmerColor,expectedColor,4));
  shimmerDraws=0;
  gfx.o=opa;gfx.x=xlu;submitted.clear();luts.clear();current={};
  MM_DrawComboForeign(1);
  assert(sentinels==0 && current.ok && submitted.size()==2);
  assert(shimmerDraws==1 && matrixDepth==0 && !memcmp(shimmerColor,expectedColor,4));
  for(int layer=0;layer<2;++layer) {
   assert(submitted[layer].second==std::string("__OTR__@mm:")+(out.dlists[layer]+7));
   assert(submitted[layer].first==(layer==1 && out.xluStartIndex==1?1:0));
  }
  assert(luts==(out.xluStartIndex==1?std::vector<int>{0,1}:std::vector<int>{0}));
 }
 const char* remains[]={gMmRemainsOdolwaDL,gMmRemainsGohtDL,gMmRemainsGyorgDL,gMmRemainsTwinmoldDL};
 assert(!OOT_DescribeMmRemainsDraw((int)RG_MM_REMAINS_ODOLWA-1,&bad));
 assert(!OOT_DescribeMmRemainsDraw(RG_MM_REMAINS_GOHT,nullptr));
 for(int i=0;i<4;++i) {
  selected=(RandomizerGet)(RG_MM_REMAINS_ODOLWA+i);CwItemDrawInfo out{};
  assert(OOT_FillItemDrawInfo(selected,&out)==1 && out.drawKind==CW_DRAW_KIND_MM_REMAINS);
  assert(out.dlistCount==1 && out.xluStartIndex==-1 && out.scale==0.02f && nativeRows==0);
  assert(!strcmp(out.dlists[0],remains[i]));
  uint8_t expectedColor[4];ComboMmRemainsShimmerColor(i,expectedColor);
  assert(out.itemShimmer && !memcmp(out.itemShimmerColor,expectedColor,4));
  shimmerDraws=0;
  gfx.o=opa;gfx.x=xlu;submitted.clear();luts.clear();current={};scale=0;
  MM_DrawComboForeign(1);
  assert(sentinels==0 && current.ok && submitted.size()==1 && submitted[0].first==0);
  assert(submitted[0].second==std::string("__OTR__@mm:")+(remains[i]+7));
  assert(luts==std::vector<int>{0} && scale==0.02f);
  assert(shimmerDraws==1 && matrixDepth==0 && !memcmp(shimmerColor,expectedColor,4));
 }
 selected=RG_MM_MASK_ROMANI;CwItemDrawInfo romani{};assert(OOT_FillItemDrawInfo(selected,&romani));
 assert(!strcmp(romani.dlists[0],gGiRomaniMaskCapDL) && !strcmp(romani.dlists[1],gGiRomaniMaskNoseEyeDL));
 const char* retained=current.dls[0];for(int i=0;i<1000;++i)ComboInternRoutedPathOOT(std::to_string(i));
 assert(!strncmp(retained,"__OTR__@mm:",11));
 producerResult=CW_DRAW_NOT_READY;current={};assert(ComboFillForeignDrawInfoOOT(1,current)==ComboForeignResolveOOT::NotReady);
 producerResult=1;selected=RG_MM_GREAT_SPIN_ATTACK;
 assert(OOT_DescribeMmSpinAttackDraw(nullptr)==0);
 CwItemDrawInfo spin{};assert(OOT_FillItemDrawInfo(selected,&spin)==1 && spin.stateDependent==2);
 auto* live=TestResolveForeignDrawInfoOOT(2);
 assert(live && live->appearanceDependent && live->primColorXlu[0]==17);
 ComboLatchForeignDrawOOT(2);spinRed=211;
 assert(TestResolveForeignDrawInfoOOT(2)->primColorXlu[0]==211);
 MM_DrawComboForeign(2);assert(spinDraws==1);
 spinResult=CW_DRAW_NOT_READY;
 assert(OOT_FillItemDrawInfo(selected,&spin)==CW_DRAW_NOT_READY);
 assert(TestResolveForeignDrawInfoOOT(2)==nullptr);
 spinResult=1;assert(TestResolveForeignDrawInfoOOT(2)!=nullptr);
 MM_DrawComboForeign(RC_UNKNOWN);assert(sentinels==1);
 magicSelected=true;assert(CwMinDlistsForKind(CW_DRAW_KIND_MAGIC_JAR)==1);
 auto* magic=TestResolveForeignDrawInfoOOT(4);assert(magic && magic->appearanceDependent);
 ComboLatchForeignDrawOOT(4);magicRed=211;
 magic=TestResolveForeignDrawInfoOOT(4);assert(magic && magic->primColorOpa[0]==211);
 gfx.o=opa;gfx.x=xlu;submitted.clear();grayEvents.clear();
 MM_DrawComboForeign(4);
 assert(submitted.size()==1 && submitted[0].second=="__OTR__@oot:magic");
 assert(submittedMagicRed==211 && grayEvents==std::vector<int>({1,0}));
 std::cout<<"PASS all 24 mask and four boss-remains production recipes and MM draw dispatch: no blue sentinel, correct native MM routing, opaque/translucent passes, LUT reset, Romani layers and retained paths\n";
}
