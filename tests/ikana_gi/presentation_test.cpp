// Execute production shield pose selectors and icon routes at graphics/loader seams.
#include "combo/ComboGameMode.h"
#include "combo/menu/ComboIkanaShieldResources.h"
#include "combo/menu/ComboItemDrawABI.h"
#include "combo/menu/ComboCollectibleShimmer.h"
#include "combo/menu/ComboItemIconOwnership.h"
extern "C" { int gComboGameMode = COMBO_GAME_BOTH; }
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
using s16 = int16_t; using u8 = uint8_t;
constexpr int MTXMODE_APPLY=1, G_MTX_NOPUSH=0, G_MTX_LOAD=0, G_MTX_MODELVIEW=0;
constexpr int RI_SHIELD_MIRROR=1, RG_EXT_SHIELD_OF_IKANA=1;
struct Gfx {const char* route=nullptr;}; struct GraphicsContext {}; struct State { GraphicsContext* gfxCtx; };
struct PlayState { State state; int gameplayFrames=0; }; struct GetItemEntry {};
GraphicsContext ctx; PlayState play{{&ctx}}; PlayState* gPlayState=&play;
Gfx commands[64], model, wrongHostModel;
Gfx *opa = commands;
Gfx *xlu = commands + 32;
float poseX=0, poseZ=0, spin=0, scale=1, height=0, depth=0, width=0, chinY=0;
bool flat=false, sideways=false, ootMod=false, canMeasure=true, useMod=true, donorAvailable=true;
const char* drawnRoute=nullptr;
std::vector<std::string> drawnPaths;
int strictBindings = 0;
std::string poseOwner;
int failures=0;
struct Pose {float x,z,scale;};
std::vector<Pose> stack;
std::vector<int> rotations;
void check(bool ok,const char* label) { if(!ok){std::cerr<<"FAIL "<<label<<'\n';++failures;} }
void Matrix_Push(){stack.push_back({poseX,poseZ,scale});}
void Matrix_Pop(){poseX=stack.back().x;poseZ=stack.back().z;scale=stack.back().scale;stack.pop_back();}
void Matrix_RotateY(float angle,u8){spin=angle;}
void Matrix_RotateZ(float angle,u8){poseZ+=angle;rotations.push_back(CW_OP_ROTATE_Z);}
void Matrix_RotateZF(float angle,u8 mode){Matrix_RotateZ(angle,mode);}
void Matrix_RotateX(float angle,u8){poseX+=angle;rotations.push_back(CW_OP_ROTATE_X);}
void Matrix_RotateXF(float angle,u8 mode){Matrix_RotateX(angle,mode);}
void Matrix_Scale(float x,float,float,u8){scale*=x;}
void* Matrix_NewMtx(GraphicsContext*,char*,int){return nullptr;}
void Gfx_SetupDL_25Opa(GraphicsContext*){}
void NeiGi_ReportBlackMarketModel(const char *, const char *, const char *) {}
void record(const Gfx* selected=nullptr) {
  const char *literal = reinterpret_cast<const char *>(selected);
  drawnRoute = selected && !std::strncmp(literal, "__OTR__", 7) ? literal
               : selected ? selected->route
                          : nullptr;
  if (drawnRoute)
    drawnPaths.emplace_back(drawnRoute);
  const bool selectedFlat = selected == &wrongHostModel ? !flat : flat;
  const float x = sideways ? 99.f : 68.f;
  const float vertical = sideways ? 68.f : 99.f;
  const float y = selectedFlat ? 2.f : vertical,
              z = selectedFlat ? vertical : 2.f;
  const float tiltedY =
      std::abs(std::cos(poseX)) * y + std::abs(std::sin(poseX)) * z;
  height = scale * (std::abs(std::sin(poseZ)) * x +
                    std::abs(std::cos(poseZ)) * tiltedY);
  width = scale *
          (std::abs(std::cos(poseZ)) * x + std::abs(std::sin(poseZ)) * tiltedY);
  depth =
      scale * (std::abs(std::sin(poseX)) * y + std::abs(std::cos(poseX)) * z);
  const float chinX = sideways ? 1.f : 0.f;
  const float chinAfterX =
      sideways ? 0.f : (selectedFlat ? -std::sin(poseX) : -std::cos(poseX));
  chinY = std::sin(poseZ) * chinX + std::cos(poseZ) * chinAfterX;
}
#define OPEN_DISPS(x) ((void)(x))
#define CLOSE_DISPS(x) ((void)(x))
#define POLY_OPA_DISP opa
#define POLY_XLU_DISP xlu
#define gSPMatrix(p,...) ((void)(p))
#define gSPDisplayList(p,dl) ((void)(p),record(dl))
constexpr int G_DL_OTR_FILEPATH=1,G_DL_PUSH=0;
void gDma1p(Gfx* out,int,const char* path,int,int){out->route=path;}
void gSPEndDisplayList(Gfx*){}
Gfx* NeiGi_ModOverrideDL(const char*,bool){return useMod?&model:nullptr;}
void* TransformMasks_LoadMmDL(const char*){return &wrongHostModel;}
int ResourceMgr_IsGiModelAvailableForGame(const char* owner,const char*){
    check(!std::strcmp(owner,"mm"),"donor availability must follow measured MM owner");return donorAvailable;
}
int ResourceMgr_IsModAssetForGame(const char* game,const char*){return !std::strcmp(game,"oot")?ootMod:!ootMod;}
int ResourceMgr_GetIkanaShieldGiTiltXForGame(const char* owner,const char*,float* tilt){
    poseOwner=owner;
    if(!canMeasure)return 0;
    *tilt=flat?float(M_PI/2):0.f;return 1;
}
int ResourceMgr_GetIkanaShieldGiPoseForGame(const char* owner,const char* path,float* tilt,float* roll){
    if(!ResourceMgr_GetIkanaShieldGiTiltXForGame(owner,path,tilt))return 0;
    *roll=sideways?-float(M_PI/2):0.f;return 1;
}
void DescriptorPose(const CwItemDrawInfo& info){
    poseX=poseZ=0;scale=info.scale;rotations.clear();
    for(int i=0;i<info.opCount;++i){
        if(info.ops[i].op==CW_OP_ROTATE_X)Matrix_RotateX(info.ops[i].a*float(M_PI/32768),MTXMODE_APPLY);
        if(info.ops[i].op==CW_OP_ROTATE_Z)Matrix_RotateZ(info.ops[i].a*float(M_PI/32768),MTXMODE_APPLY);
    }
    record();
}
void GetItem_Draw(PlayState*,int){record();}
namespace Rando::StaticData {
struct Item {
  int drawId = 1;
};
std::map<int, Item> Items;
} // namespace Rando::StaticData

namespace Ship {
struct Resource {std::string name;};
class ResourceManager {
  public:
    std::shared_ptr<Resource> LoadResourceProcess(const char* raw){
        std::string path=raw;if(path.starts_with("__OTR__"))path.erase(0,7);
        return files.contains(path)?files[path]:nullptr;
    }
    std::map<std::string,std::shared_ptr<Resource>> files;
};
struct CrossRMRegistry {
    static std::shared_ptr<ResourceManager> Get(const std::string& game){return owners.contains(game)?owners[game]:nullptr;}
    static inline std::map<std::string,std::shared_ptr<ResourceManager>> owners;
};
} // namespace Ship
namespace Fast {using Texture=Ship::Resource;}
std::shared_ptr<Ship::ResourceManager> active;
std::shared_ptr<Ship::ResourceManager> ActiveResMgr(){return active;}
/* TEXTURE_RESOLVER */
bool nativeIconPresent=true, donorQueried=false;
u8 ResourceMgr_FileExists(const char*){return nativeIconPresent;}
u8 ResourceMgr_FileAltExists(const char*){return false;}
bool ResourceMgr_IsAltAssetsEnabled(){return false;}
int NeiResource_Available(const char*){donorQueried=true;return 1;}
const char* NeiResource_Route(const char*){return "__OTR__@oot:icon_item_static_yar/gItemIconMirrorShieldTex";}
/* EQUIPMENT_SELECTOR */
/* EQUIPMENT_TABLES */
/* OOT_CATALOG_ICON */
extern "C" int ResourceMgr_IsIkanaShieldGiAvailable() {
  return donorAvailable &&
         (Ship::CrossRMRegistry::Get("mm") || gComboGameMode == COMBO_GAME_OOT);
}
Gfx *MmAssets_LoadDisplayListGraphStrict(const char *path) {
  static Gfx optional[] = {{COMBO_IKANA_GI_OPA}, {COMBO_IKANA_GI_XLU}};
  return !donorAvailable                          ? nullptr
         : !std::strcmp(path, COMBO_IKANA_GI_OPA) ? &optional[0]
                                                  : &optional[1];
}
void MmAssets_EnsureStrictTextureBindings() { ++strictBindings; }
void Gfx_SetupDL_25Xlu(GraphicsContext *) {}
void Gfx_SetupDL25_Opa(GraphicsContext *) {}
void Gfx_SetupDL25_Xlu(GraphicsContext *) {}
#define MATRIX_FINALIZE_AND_LOAD(p, ctx) ((void)(p), (void)(ctx))
#define MM_FOREIGN_PIN_OPA() ((void)0)
#define MM_FOREIGN_PIN_XLU() ((void)0)
#define gDPSetTextureLUT(p, x) ((void)(p), (void)(x))
#define gDPSetEnvColor(p, ...) ((void)(p))
#define G_TT_NONE 0
Gfx *MM_DrawForeignMagicJarDList(Gfx *output, const char *, const uint8_t *) {
  return output;
}
/* MM_SIMPLE_INFO */
/* MM_SIMPLE_CONSUMER */
/* OOT_NATIVE_DRAW */
/* OOT_DESCRIPTOR */
/* MM_NATIVE_DRAW */

int main(){
    play.gameplayFrames=17;
    auto oot = std::make_shared<Ship::ResourceManager>(),
         mm = std::make_shared<Ship::ResourceManager>();
    Ship::CrossRMRegistry::owners = {{"oot", oot}, {"mm", mm}};
    constexpr char iconPath[] = "textures/icon_item_custom/gItemIconShieldOfIkanaTex";
    auto own = std::make_shared<Ship::Resource>();
    own->name = "MM Mirror Shield";
    auto wrong = std::make_shared<Ship::Resource>();
    wrong->name = "wrong local shield";
    mm->files[iconPath] = own;
    oot->files[iconPath] = wrong;
    active = oot;
    for(bool selectedSideways:{false,true})for(bool selectedFlat:{false,true}) {
        sideways = selectedSideways;
        flat = selectedFlat;
        poseX = poseZ = 0;
        scale = 1;
        opa = commands;
        xlu = commands + 32;
        rotations.clear();
        drawnPaths.clear();
        Randomizer_DrawExtShieldOfIkana(&play,nullptr);
        check(drawnPaths == std::vector<std::string>(
                                {COMBO_IKANA_GI_OPA_MM, COMBO_IKANA_GI_XLU_MM}),
              "OoT callback must draw native 2Ship GI OPA then XLU, never the "
              "worn shield");
        check(poseOwner == "mm" && scale == 1 &&
                  std::abs(spin - .34f) < .00001f,
              "native GI owner, scale and spin");
        check(stack.empty(), "OoT native GI matrix stack balanced");
        CwItemDrawInfo info{};DescribeIkana(&info);
        check(info.drawKind == CW_DRAW_KIND_SIMPLE && info.dlistCount == 2 &&
                  info.xluStartIndex == 1 && info.scale == 0,
              "foreign descriptor preserves native simple two-pass recipe and "
              "scale");
        check(!std::strcmp(info.dlists[0], COMBO_IKANA_GI_OPA_MM) &&
                  !std::strcmp(info.dlists[1], COMBO_IKANA_GI_XLU_MM),
              "foreign descriptor uses the actual 2Ship get-item paths");
        ComboForeignDrawInfoOOT foreign{};
        foreign.count = 2;
        foreign.xluStart = 1;
        foreign.scale = info.scale;
        foreign.drawKind = info.drawKind;
        foreign.opCount = info.opCount;
        std::copy(std::begin(info.ops), std::end(info.ops),
                  std::begin(foreign.ops));
        foreign.dls[0] = info.dlists[0];
        foreign.dls[1] = info.dlists[1];
        poseX = poseZ = 0;
        scale = 1;
        rotations.clear();
        drawnPaths.clear();
        MM_DrawForeignSimple(&foreign);
        check(drawnPaths == std::vector<std::string>(
                                {COMBO_IKANA_GI_OPA_MM, COMBO_IKANA_GI_XLU_MM}),
              "actual MM consumer submits both native GI passes");
        check(height > depth * 10 && height > width && chinY < -.99f,
              "actual MM consumer applies selected native GI pose");
        std::vector<int> expected;
        if (sideways)
          expected.push_back(CW_OP_ROTATE_Z);
        if (flat)
          expected.push_back(CW_OP_ROTATE_X);
        check(rotations == expected && stack.empty(),
              "MM pose-only recipe has no extra spin and restores matrix");
        check(info.stateDependent == 2,
              "foreign native GI refreshes owner pose across Alt changes");
    }
    // A local worn-model replacement must not displace the get-item model.
    ootMod = true;
    useMod = true;
    drawnPaths.clear();
    Randomizer_DrawExtShieldOfIkana(&play,nullptr);
    check(poseOwner == "mm" && drawnPaths.back() == COMBO_IKANA_GI_XLU_MM,
          "local worn model cannot override native GI");
    donorAvailable = false;
    drawnPaths.clear();
    Randomizer_DrawExtShieldOfIkana(&play, nullptr);
    check(drawnPaths.empty(),
          "unavailable native pair is left to the authored fallback selector");
    donorAvailable = true;
    gComboGameMode = COMBO_GAME_OOT;
    Ship::CrossRMRegistry::owners.erase("mm");
    drawnPaths.clear();
    Randomizer_DrawExtShieldOfIkana(&play, nullptr);
    check(drawnPaths == std::vector<std::string>(
                            {COMBO_IKANA_GI_OPA, COMBO_IKANA_GI_XLU}) &&
              strictBindings == 1,
          "Solo OoT uses both strict optional-archive GI pointers and texture "
          "bindings");
    gComboGameMode = COMBO_GAME_BOTH;
    Ship::CrossRMRegistry::owners["mm"] = mm;
    check(ComboLoadTextureResource(COMBO_IKANA_SHIELD_ICON) == own,
          "Ikana icon uses MM while OoT active");
    check(ComboLoadTextureResource(OotCatalogIcon) == own,
          "native receipt catalog keeps MM icon ownership");
    for (const auto *icon :
         {MmEquip::ExtEquip_GetIcon(1, 3), OotEquip::ExtEquip_GetIcon(1, 3)})
      check(ComboLoadTextureResource(static_cast<const char *>(icon)) == own,
            "both equipment getters retain MM icon");
    gComboGameMode = COMBO_GAME_OOT;
    for (const auto *icon :
         {MmEquip::ExtEquip_GetIcon(1, 3), OotEquip::ExtEquip_GetIcon(1, 3)})
      check(!std::strcmp(static_cast<const char *>(icon),
                         COMBO_IKANA_SHIELD_ICON_NATIVE),
            "runtime Solo OoT uses canonical local MM key");
    gComboGameMode = COMBO_GAME_BOTH;
    nativeIconPresent=false;donorQueried=false;
    const auto *grid =
        static_cast<const char *>(KaleidoEquip_OotTex(COMBO_IKANA_SHIELD_ICON));
    check(!donorQueried && grid && ComboLoadTextureResource(grid) == own,
          "cold MM grid preserves MM ownership");
    std::cout << "Native Ikana GI and icon integration: " << failures
              << " failures\n";
    return failures?1:0;
}
