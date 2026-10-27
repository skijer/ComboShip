#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
extern "C" {
#include "functions.h"
#include "macros.h"
#include "z64.h"
}
#include "ComboItemDrawABI.h"
#include "ComboCollectibleShimmerDraw.h"
extern "C" void NeiGi_DrawTintedShimmerOverlay(PlayState*, const uint8_t[4], const char*) {
  assert(false && "sword fixture unexpectedly selected collectible shimmer");
}
#include "combo/menu/ComboBottleContents.h"
#include "combo/menu/ComboElementalArrowGi.h"
#include "combo/menu/ComboRewardGi.h"
#include "soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include "soh/Enhancements/randomizer/NeiGiFrameFit.h"
#include "combo/DinSwordGiResources.h"
extern "C" void NeiGi_DrawElementalArrow(PlayState*, int) {
  assert(false && "sword fixture must not dispatch elemental arrows");
}
extern "C" void NeiGi_DrawElementalArrowForOwner(PlayState*, int, int, int) {
  assert(false && "sword fixture must not dispatch elemental arrows");
}
extern "C" void NeiGi_DrawRewardMaterial(PlayState*, int, const char*, const char*, const char*) {
  assert(false && "sword fixture must not dispatch reward materials");
}
extern "C" int NeiGi_DrawElementalSpell(PlayState*, int, int, int) {
  assert(false && "sword fixture must not dispatch elemental spells");return 0;
}
extern "C" int ComboBottleContents_Draw(PlayState*, int) {
  assert(false && "sword fixture must not dispatch bottle contents");return 0;
}
#include <libultraship/bridge/consolevariablebridge.h>
/* PRODUCTION_INFO */
static Gfx opa[1024], xlu[1024];
static GraphicsContext gfx;
static PlayState play;
struct Pose {
  float scale = 2, x = 11, y = 17, z = -9, rx = 0, ry = 0, rz = 0;
  bool billboard = false;
  bool operator==(const Pose &) const = default;
};
static Pose pose;
static std::vector<Pose> stack;
static int dinColorReads;
static std::deque<Mtx> matrices;
static std::map<Mtx *, Pose> captured;
static std::vector<std::vector<Gfx>> arena;
static Gfx scrolls[4][12];
static std::vector<std::vector<int32_t>> scrollParams;
static bool flameAvailable = true;
static bool dinLayers;
static bool fitModel;
static bool shelfDraw;
static int fittedRoots;
static float fittedDrawScale, fittedTilt;
static int interpolation, shimmers, sentinels, identityDraws;
static NeiGi::Mesh identityMesh;
static float identityParticleSpan;
static ComboForeignDrawInfo recipe;
void FrameInterpolation_RecordOpenChild(const void *, int) { ++interpolation; }
void FrameInterpolation_RecordCloseChild() {
  assert(interpolation > 0);
  --interpolation;
}
extern "C" {
void gSPSegment(void *cmd, int segment, uintptr_t address) {
  __gSPSegment((Gfx *)cmd, segment, address);
}
void gSPDisplayList(Gfx *cmd, Gfx *path) { __gSPDisplayList(cmd, path); }
void Matrix_Push() { stack.push_back(pose); }
void Matrix_Pop() {
  assert(!stack.empty());
  pose = stack.back();
  stack.pop_back();
}
void Matrix_Scale(float x, float y, float z, u8) {
  assert(x == y && y == z);
  pose.scale *= x;
}
void Matrix_Translate(float x, float y, float z, u8) {
  pose.x += x * pose.scale;
  pose.y += y * pose.scale;
  pose.z += z * pose.scale;
}
void Matrix_RotateX(float r, u8) { pose.rx += r; }
void Matrix_RotateY(float r, u8) { pose.ry += r; }
void Matrix_RotateZ(float r, u8) { pose.rz += r; }
void Matrix_ReplaceRotation(MtxF *) { pose.billboard = true; }
Mtx *Matrix_NewMtx(GraphicsContext *, char *, s32) {
  matrices.emplace_back();
  captured[&matrices.back()] = pose;
  return &matrices.back();
}
void Gfx_SetupDL_25Opa(GraphicsContext *) {}
void Gfx_SetupDL_25Xlu(GraphicsContext *) {}
void Graph_OpenDisps(Gfx **, GraphicsContext *, const char *, s32) {}
void Graph_CloseDisps(Gfx **, GraphicsContext *, const char *, s32) {}
void *Graph_Alloc(GraphicsContext *, size_t bytes) {
  arena.emplace_back((bytes + sizeof(Gfx) - 1) / sizeof(Gfx));
  return arena.back().data();
}
Gfx *Gfx_TwoTexScrollEx(GraphicsContext *, s32, u32 x1, u32 y1, s32 w1, s32 h1,
                        s32, u32 x2, u32 y2, s32 w2, s32 h2, s32 sx1, s32 sy1,
                        s32 sx2, s32 sy2) {
  assert(scrollParams.size() < 4);
  scrollParams.push_back({int32_t(x1), int32_t(y1), w1, h1, int32_t(x2),
                          int32_t(y2), w2, h2, sx1, sy1, sx2, sy2});
  return scrolls[scrollParams.size() - 1];
}
void GetItem_Draw(PlayState *, s16) { ++sentinels; }
}
static uint8_t ResourceMgr_FileExists(const char *path) {
  assert(strstr(path, "gGiBlueFireFlameDL"));
  return flameAvailable;
}
static uint8_t ResourceMgr_FileAltExists(const char *) { return false; }
static bool ResourceMgr_IsAltAssetsEnabled() { return false; }
static RandomizerCheck OOT_GetQueuedDrawCheck() { return RandomizerCheck(1); }
static const ComboForeignDrawInfo *
ComboResolveForeignDrawInfo(RandomizerCheck) {
  return &recipe;
}
static bool ComboForeignAnim_Draw(const CwItemAnimDrawInfo *, const char *,
                                  PlayState *) {
  assert(false);
  return false;
}
extern "C" int32_t CVarGetInteger(const char *, int32_t) { return dinLayers; }
extern "C" Color_RGB8 CVarGetColor24(const char* key,Color_RGB8) {
#ifdef HOST_MM_ROUTE
  assert(false && "MM Din GI must use its changed-color/suppression-aware cosmetics API");
#endif
  ++dinColorReads;
  if(!strcmp(key,"gCosmetics.Custom.DinFireSwordCore.Value"))return {17,31,47};
  assert(!strcmp(key,"gCosmetics.Custom.DinFireSwordOuter.Value"));return {53,67,79};
}
#ifdef HOST_MM_ROUTE
extern "C" Color_RGBA8 CosmeticEditor_GetChangedColor(u8,u8,u8,u8,const char* id) {
  ++dinColorReads;
  if(!strcmp(id,"Custom.DinFireSwordCore"))return {17,31,47,255};
  assert(!strcmp(id,"Custom.DinFireSwordOuter"));return {53,67,79,255};
}
#endif
extern "C" bool NeiGi_CanDrawLayers(PlayState*,size_t,size_t,size_t) {return true;}
extern "C" int ResourceMgr_GetDinSwordGiProfileForGame(const char* owner,const char* path) {
  assert(!strcmp(owner,"oot"));
  if(!strncmp(path,"__OTR__@oot:",12))path+=12;
  else if(!strncmp(path,"__OTR__",7))path+=7;
  return DinSwordGi::SelectedProfile(path,dinLayers,true,[](const char*){return true;});
}
static void ComboSwordGi_ApplyFit(const char*,const char*,float scale,float tilt,bool,int=0) {
  fittedRoots=1;fittedDrawScale=scale;fittedTilt=tilt;
  if(fitModel){Matrix_Translate(0,-20,0,MTXMODE_APPLY);Matrix_Scale(.5f,.5f,.5f,MTXMODE_APPLY);}
}
static void ComboSwordGi_ApplyModelsFit(const char*,const char* const* paths,int count,float scale,float tilt,bool,int=0) {
  for(int i=0;i<count;++i)assert(paths[i]);
  fittedRoots=count;fittedDrawScale=scale;fittedTilt=tilt;
  if(fitModel){Matrix_Translate(0,-20,0,MTXMODE_APPLY);Matrix_Scale(.5f,.5f,.5f,MTXMODE_APPLY);}
}
#include "ComboSwordGiEffectFit.h"
static void ComboDrawMaskShimmer(PlayState *, const char *,
                                 const uint8_t *color, const char *owner) {
  assert(pose == Pose{} && (!strcmp(owner, "mm") || !strcmp(owner,"oot")));
  assert(color[0] == 220 && color[1] == 225 && color[2] == 240);
  ++shimmers;
}
template <class... Args> static void NeiGi_DrawPresentation(Args...) {
  assert(false);
}
template <class... Args> static void ComboDrawSpinAttackGi(Args...) {
  assert(false);
}
#define UNUSED_HANDLER(name)                                                   \
  static void name(PlayState *, const ComboForeignDrawInfo *) { assert(false); }
UNUSED_HANDLER(OOT_DrawForeignDekuNuts)
UNUSED_HANDLER(OOT_DrawForeignRecoveryHeart)
UNUSED_HANDLER(OOT_DrawForeignFish)
UNUSED_HANDLER(OOT_DrawForeignPotion)
UNUSED_HANDLER(OOT_DrawForeignBlueFire)
UNUSED_HANDLER(OOT_DrawForeignPoes)
UNUSED_HANDLER(OOT_DrawForeignFairyBottle)
UNUSED_HANDLER(OOT_DrawForeignMagicSpell)
UNUSED_HANDLER(OOT_DrawForeignSoulFlame)
UNUSED_HANDLER(OOT_DrawForeignOps)
static constexpr int kMaxMatEntries=8;
template<class...Args>void ComboForeignTexAnim_Run(Args...){assert(false);}
template<class...Args>void ComboForeignTexAnim_Restore(Args...){assert(false);}
static Gfx* MM_DrawForeignMagicJarDList(Gfx*,const char*,const uint8_t*){assert(false);return nullptr;}
static void NeiGi_DrawSeasonOverlay(PlayState*,int,const char*) {
  assert(false && "sword fixture must not select weather");
}
static bool OOT_DrawForeignFairyContainer(PlayState*,const ComboForeignDrawInfo*) {assert(false);return false;}
static void NeiGi_DrawSongOverlay(PlayState*,int,const char*) {assert(false);}
static NeiGi::Basis NeiGi_CameraBasis(PlayState*) {return {};}
static void NeiGi_DrawMesh(PlayState*,const NeiGi::Mesh& mesh) {
  Pose expected{};
  if(recipe.neiShimmer>0 && NeiGi::IsSword(static_cast<NeiGi::Kind>(recipe.neiShimmer-1))) {
    expected.scale=shelfDraw?2.f:2.3f;
    expected.y=shelfDraw?17.f:17.6f;
    // Master Sword's authored world fit shifts its tip below the 48-unit edge,
    // regardless of the arbitrary .5/-20 model correction used by this fixture.
    if(shelfDraw) {
      const auto fit=NeiGi::FrameFit(*NeiGi::FindSwordFrameBounds(
          static_cast<NeiGi::Kind>(recipe.neiShimmer-1)),1.f,true);
      expected.y+=2.f*fit.lift;
      expected.scale*=fit.scale;
    } else if(recipe.neiShimmer==int(NeiGi::Kind::MasterSword)+1 ||
              recipe.neiShimmer==int(NeiGi::Kind::SwordAura)+1)
      // Independent quantized TP blade tip and serialized source scale.
      expected.y+=2.3f*(48.f-106.375f*.659912109375f);
    else expected=pose; // The full production renderer gate checks every other award.
  }
  assert(std::abs(pose.scale-expected.scale)<.00001f && std::abs(pose.y-expected.y)<.0001f &&
         "foreign binary sword particles/shimmer inherited model coordinate fitting");
  if(identityDraws==0 && recipe.neiShimmer==int(NeiGi::Kind::KokiriSword)+1) {
    const auto d=mesh.vertices[0].p-mesh.vertices[2].p;
    identityParticleSpan=std::sqrt(d.x*d.x+d.y*d.y+d.z*d.z)*pose.scale;
  }
  identityMesh=mesh;++identityDraws;
}
#ifdef HOST_MM_ROUTE
#define COMBO_DIN_SWORD_GI_HOST_MM
#define Matrix_Finalize(gfx) Matrix_NewMtx(gfx,(char*)__FILE__,__LINE__)
#define Gfx_SetupDL25_Opa Gfx_SetupDL_25Opa
#define Gfx_SetupDL25_Xlu Gfx_SetupDL_25Xlu
#endif
#include "ComboDinSwordGi.h"
#undef COMBO_DIN_SWORD_GI_HOST_MM
/* PRODUCTION_HANDLERS */

struct Draw {
  std::string path;
  Pose pose;
  bool gray;
  uint32_t color;
  int owner;
  uint32_t prim, env;
};
static std::vector<Draw> CheckStream(Gfx *begin, Gfx *end, bool restore) {
  Pose gpu;
  int owner = 0;
  bool gray = false;
  uint32_t color = 0;
  uint32_t prim = 0, env = 0;
  uintptr_t segment8 = 0;
  std::vector<Draw> draws;
  for (auto *cmd = begin; cmd < end; ++cmd) {
    auto op = cmd->words.w0 >> 24;
    if (op == G_COMBO_RM_PUSH) {
      assert(!strcmp((const char *)cmd->words.w1, "oot"));
      ++owner;
    }
    if (op == G_COMBO_RM_POP) {
      assert(owner > 0);
      --owner;
    }
    if (op == G_SETGRAYSCALE)
      gray = cmd->words.w1;
    if (op == G_SETINTENSITY)
      color = cmd->words.w1;
    if (op == G_SETPRIMCOLOR)prim = cmd->words.w1;
    if (op == G_SETENVCOLOR)env = cmd->words.w1;
    if (op == G_MTX)
      gpu = captured.at((Mtx *)cmd->words.w1);
    if (op == G_MOVEWORD && ((cmd->words.w0 >> 16) & 255) == G_MW_SEGMENT &&
        (cmd->words.w0 & 65535) == 8 * 4)
      segment8 = cmd->words.w1;
    if (op == G_DL || op == G_DL_OTR_FILEPATH)
      draws.push_back({(const char *)cmd->words.w1, gpu, gray, color, owner, prim, env});
  }
  assert(owner == 0 && !gray);
  if (!draws.empty()) {
#ifndef HOST_MM_ROUTE
    Pose expected{};
    if(recipe.neiShimmer>0 && NeiGi::IsSword(static_cast<NeiGi::Kind>(recipe.neiShimmer-1))) {
      expected.scale=shelfDraw?2.f:2.3f;expected.y=shelfDraw?17.f:17.6f;
    }
    if(fitModel){expected.y-=20*expected.scale;expected.scale*=.5f;}
    assert(std::abs(gpu.scale-expected.scale)<.00001f && std::abs(gpu.y-expected.y)<.0001f &&
           gpu.x==expected.x && gpu.z==expected.z && gpu.rx==0 && gpu.ry==0 && gpu.rz==0);
#endif
  }
  if (restore && segment8)
    assert(((Gfx *)segment8)->words.w0 >> 24 == G_ENDDL);
  return draws;
}
static void Reset(int kind, bool trueTier, bool shimmer) {
  memset(opa, 0, sizeof(opa));
  memset(xlu, 0, sizeof(xlu));
  gfx.polyOpa.p = opa;
  gfx.polyOpa.d = opa + 1024;
  gfx.polyXlu.p = xlu;
  gfx.polyXlu.d = xlu + 1024;
  play.state.gfxCtx = &gfx;
  play.state.frames = 47;
  play.gameplayFrames = 47;
  pose = {};
  stack.clear();
  captured.clear();
  matrices.clear();
  arena.clear();
  scrollParams.clear();
  interpolation = shimmers = sentinels = identityDraws = 0;
  identityParticleSpan=0.f;
  flameAvailable = true;
  dinLayers = false;
  fitModel = shelfDraw = false;
  fittedRoots=0;fittedDrawScale=fittedTilt=0;
  recipe = {};
  recipe.count = 1;
  recipe.xluStart = -1;
  recipe.drawKind = kind;
  recipe.itemShimmer = shimmer;
  const uint8_t color[4] = {220, 225, 240, 255};
  memcpy(recipe.itemShimmerColor, color, 4);
  recipe.dls[0] =
      kind == CW_DRAW_KIND_CUSTOM_GI
          ? "__OTR__@oot:alt/objects/object_custom_equip/gCustomMasterSwordDL"
          : "__OTR__@oot:objects/object_toki_objects/"
            "object_toki_objects_DL_001BD0";
  if (kind == CW_DRAW_KIND_CUSTOM_GI) {
    recipe.scale = .04f;
    recipe.opCount = 1;
    recipe.ops[0] = {CW_OP_ROTATE_Z, 18774.682f, 0, 0, {}};
  }
  if (trueTier) {
    const uint8_t flame[4] = {120, 180, 255, 255};
    memcpy(recipe.primColorXlu, flame, 4);
    if (kind == CW_DRAW_KIND_MASTER_SWORD) {
      const uint8_t gold[4] = {255, 215, 110, 255};
      memcpy(recipe.primColorOpa, gold, 4);
    }
  }
}
static void Dispatch(bool shop=false) {
  shelfDraw=shop;
  GetItemEntry entry{};
  entry.comboForeignCheck = 1;
  OOT_DrawComboForeign(&play, &entry, shop);
  assert(pose == Pose{} && stack.empty() && interpolation == 0 && !sentinels);
}
int main() {
  for (int kind : {CW_DRAW_KIND_MASTER_SWORD, CW_DRAW_KIND_CUSTOM_GI})
    for (bool trueTier : {false, true})
      for (bool shimmer : {false, true}) {
        Reset(kind, trueTier, shimmer);
        Dispatch();
        auto body = CheckStream(opa, gfx.polyOpa.p, true),
             flame = CheckStream(xlu, gfx.polyXlu.p, true);
        assert(body.size() == 1 && body[0].path == recipe.dls[0]);
#ifndef HOST_MM_ROUTE
        assert(body[0].owner == 1);
#endif
        assert(body[0].gray == (trueTier && kind == CW_DRAW_KIND_MASTER_SWORD));
        if (body[0].gray)
          assert(body[0].color == 0xFFD76EFFu);
        assert(std::abs(body[0].pose.scale -
                        (kind == CW_DRAW_KIND_MASTER_SWORD ? .1f : .08f)) <
               .00001f);
        assert(std::abs(body[0].pose.rz -
                        (kind == CW_DRAW_KIND_MASTER_SWORD ? 2.1f : 1.8f)) <
               .0001f);
        if (kind == CW_DRAW_KIND_CUSTOM_GI)
          assert(body[0].pose.rx == 0 &&
                 body[0].pose.ry == .94f);
        assert(flame.size() == size_t(trueTier) && shimmers == int(shimmer));
        if (trueTier) {
          assert(flame[0].gray && flame[0].color == 0x78B4FFFFu &&
                 flame[0].owner == 1);
          assert(flame[0].pose.billboard && flame[0].pose.y == -123.f &&
                 flame[0].pose.scale == 10.f);
          assert(scrollParams[0] ==
                 std::vector<int32_t>(
                     {0, 0, 16, 32, 47, -376, 16, 32, 0, 0, 1, -8}));
        }
        if (kind == CW_DRAW_KIND_MASTER_SWORD)
          assert(
              scrollParams.back() ==
              std::vector<int32_t>({47, 0, 32, 32, 0, 0, 32, 32, 1, 0, 0, 0}));
      }
  for(auto kind:{NeiGi::Kind::KokiriSword,NeiGi::Kind::RazorSword,NeiGi::Kind::GildedSword,
                 NeiGi::Kind::MasterSword,NeiGi::Kind::SwordAura,NeiGi::Kind::BiggoronSword,NeiGi::Kind::GreatFairySword,
                 NeiGi::Kind::GiantsKnife}) {
    Reset(CW_DRAW_KIND_CUSTOM_GI,false,true);recipe.neiShimmer=int(kind)+1;
    Dispatch();assert(identityDraws==2 && shimmers==0);
    assert(std::abs(CheckStream(opa,gfx.polyOpa.p,false)[0].pose.rz-1.5707963267948966f)<.0001f &&
           "older selected sword descriptor retained its slanted pose");
    const auto wanted=NeiGi::SampleShimmer(play.gameplayFrames,true,{},kind);
    assert(identityMesh.count==wanted.count);
    for(size_t i=0;i<wanted.count;++i)assert(identityMesh.vertices[i].rgb==wanted.vertices[i].rgb &&
        identityMesh.vertices[i].alpha==wanted.vertices[i].alpha && identityMesh.vertices[i].p.x==wanted.vertices[i].p.x);
    assert(CheckStream(opa,gfx.polyOpa.p,false).size()==1 && CheckStream(xlu,gfx.polyXlu.p,false).empty());
  }
  Reset(CW_DRAW_KIND_CUSTOM_GI, true, true);
  recipe.neiShimmer=int(NeiGi::Kind::SwordAura)+1;fitModel=true;
  Dispatch();
  const auto presentationFlame=CheckStream(xlu,gfx.polyXlu.p,false);
  assert(!presentationFlame.empty() && std::abs(presentationFlame.front().pose.scale-11.5f)<.00001f &&
         "True Master presentation flame inherited the binary geometry scale");
  Reset(CW_DRAW_KIND_CUSTOM_GI, false, true);
  recipe.neiShimmer = int(NeiGi::Kind::MasterSword) + 1;
  dinLayers = true;
  dinColorReads = 0;
  Dispatch();
  const auto dinBody = CheckStream(opa,gfx.polyOpa.p,false);
  const auto dinFlame = CheckStream(xlu,gfx.polyXlu.p,false);
  assert(dinBody.size() == 2 && dinFlame.size() == 1 && "selected Din GI lost its core/flame layers");
  assert(dinBody[1].path == DinSwordGi::profiles[0].core && dinFlame[0].path == DinSwordGi::profiles[0].flame);
  assert(dinBody[0].pose == dinBody[1].pose && dinBody[0].pose == dinFlame[0].pose);
  assert(dinColorReads == 2 && dinBody[1].prim == 0x111F2FFF && dinBody[1].env == 0x35434FFF);
  assert((dinFlame[0].prim & 0xFFFFFF00u) == 0x111F2F00 && dinFlame[0].env == 0x35434FFF);
  assert(identityDraws == 2 && "Din layers replaced the awarded sword particles/shimmer");
  Reset(CW_DRAW_KIND_CUSTOM_GI,false,true);
  recipe.neiShimmer=int(NeiGi::Kind::MasterSword)+1;
  fitModel=true;dinLayers=true;
  Dispatch();
  const auto fittedBody=CheckStream(opa,gfx.polyOpa.p,false);
  const auto fittedFlame=CheckStream(xlu,gfx.polyXlu.p,false);
  assert(fittedBody[0].pose==fittedBody[1].pose && fittedBody[0].pose==fittedFlame[0].pose);
  assert(std::abs(fittedBody[0].pose.scale-.046f)<.00001f && std::abs(fittedBody[0].pose.y+28.4f)<.0001f);
  Reset(CW_DRAW_KIND_CUSTOM_GI, true, true);
  flameAvailable = false;
  Dispatch();
  assert(CheckStream(xlu, gfx.polyXlu.p, true).empty());
  assert(CheckStream(opa, gfx.polyOpa.p, true).size() == 1 && shimmers == 1);
  Reset(CW_DRAW_KIND_CUSTOM_GI, false, true);
  recipe.count = 2;
  recipe.xluStart = 1;
  recipe.dls[1] = "__OTR__@oot:objects/test/skinDL";
  Dispatch();
  assert(CheckStream(opa, gfx.polyOpa.p, false).size() == 1 &&
         CheckStream(xlu, gfx.polyXlu.p, false).size() == 1);
  Reset(CW_DRAW_KIND_CUSTOM_GI,false,true);
  recipe.neiShimmer=int(NeiGi::Kind::MasterSword)+1;fitModel=true;
  recipe.count=2;recipe.xluStart=1;recipe.dls[1]="__OTR__@oot:objects/test/oversizedSwordSkinDL";
  Dispatch();
  assert(fittedRoots==2&&"foreign selected sword fit omitted its independent translucent root");
  assert(CheckStream(opa,gfx.polyOpa.p,false)[0].path==recipe.dls[0]);
  assert(CheckStream(xlu,gfx.polyXlu.p,false)[0].path==recipe.dls[1]);
  Reset(CW_DRAW_KIND_SIMPLE,false,true);
  recipe.neiShimmer=int(NeiGi::Kind::RazorSword)+1;fitModel=true;
  recipe.scale=0;recipe.count=2;recipe.xluStart=-1;
  recipe.opCount=1;recipe.ops[0]={CW_OP_ROTATE_Z,18774.682f,0,0,{}};
  recipe.dls[0]="__OTR__@mm:objects/object_gi_sword_2/gGiRazorSwordDL";
  recipe.dls[1]="__OTR__@mm:objects/object_gi_sword_2/gGiRazorSwordEmptyDL";
  Dispatch();
  assert(fittedRoots==2&&fittedDrawScale==1.f&&fittedTilt==0.f&&"native GI recipe bypassed its effective-scale fit");
  const auto nativeBody=CheckStream(opa,gfx.polyOpa.p,false);
  assert(nativeBody.size()==2&&nativeBody[0].path==recipe.dls[0]&&nativeBody[1].path==recipe.dls[1]);
  assert(identityDraws==2&&nativeBody[0].pose==nativeBody[1].pose);
  for(int rotation:{CW_OP_ROTATE_X,CW_OP_ROTATE_Z}) {
    Reset(CW_DRAW_KIND_CUSTOM_GI,false,false);
    recipe.ops[0]={rotation,16384.f,0,0,{}};
    recipe.dls[0]="__OTR__@mm:objects/non_sword/quarter_turn_model";
    Dispatch(true);
    const auto genericShelf=CheckStream(opa,gfx.polyOpa.p,false);
    assert(genericShelf.size()==1 && std::abs(genericShelf[0].pose.scale-.08f)<.000001f);
    const float angle=rotation==CW_OP_ROTATE_X ? genericShelf[0].pose.rx : genericShelf[0].pose.rz;
    assert(std::abs(angle-1.5707963267948966f)<.000001f &&
           "sword shelf correction changed a non-sword quarter turn, including Ikana shield");
  }
#ifdef HOST_MM_ROUTE
  Reset(CW_DRAW_KIND_CUSTOM_GI,false,true);
  recipe.neiShimmer=int(NeiGi::Kind::KokiriSword)+1;
  Dispatch();
  const float fittedParticleSpan=identityParticleSpan;
  Reset(CW_DRAW_KIND_CUSTOM_GI,false,true);
  recipe.neiShimmer=int(NeiGi::Kind::KokiriSword)+1;
  recipe.opCount=2;
  recipe.ops[1]={CW_OP_TRANSLATE,0,0,0,{}};
  Dispatch();
  assert(std::abs(identityParticleSpan-fittedParticleSpan)<.0001f &&
         "custom sword particles compensated for a presentation scale that was never applied");
  Reset(CW_DRAW_KIND_CUSTOM_GI,false,false);
  fitModel=true;
  recipe.dls[0]="__OTR__@oot:objects/non_sword/slanted_receipt_model";
  MM_DrawComboForeign(1,false,1);
  const auto genericReceipt=CheckStream(opa,gfx.polyOpa.p,false);
  assert(genericReceipt.size()==1 && std::abs(genericReceipt[0].pose.rz-1.8f)<.0001f);
  assert(std::abs(fittedTilt-genericReceipt[0].pose.rz)<.0001f &&
         "MM receipt fit changed a non-sword model's selected pose");
  assert(pose==Pose{} && stack.empty() && interpolation==0);
#endif
  // Exercise both real foreign dispatchers with the pickup descriptor. Shops
  // use the upright selected model pose and retain matching Din layers.
  Reset(CW_DRAW_KIND_CUSTOM_GI,false,true);
  recipe.neiShimmer=int(NeiGi::Kind::MasterSword)+1;
  recipe.ops[0].a=16384.f;fitModel=true;dinLayers=true;
  Dispatch(true);
  const auto shelfBody=CheckStream(opa,gfx.polyOpa.p,false);
  const auto shelfFlame=CheckStream(xlu,gfx.polyXlu.p,false);
  assert(std::abs(fittedTilt-1.5707963267948966f)<.00001f);
  assert(shelfBody.size()==2 && shelfFlame.size()==1);
  assert(shelfBody[0].pose==shelfBody[1].pose && shelfBody[0].pose==shelfFlame[0].pose);
  assert(std::abs(shelfBody[0].pose.scale-.04f)<.00001f && shelfBody[0].pose.y==-23.f &&
         std::abs(shelfBody[0].pose.rz-1.5707963267948966f)<.00001f &&
         "selected sword shelf lost its upright pose, framing or Din layer placement");
  std::cout << "PASS actual OoT sword fallback dispatch: owner scopes, signed "
               "transforms, independent blade/flame, shimmer pose, stream "
               "cursors and segment cleanup\n";
}
