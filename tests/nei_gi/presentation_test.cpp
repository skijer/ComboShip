// Exercise the real renderer. Only the game/graphics boundary is replaced.
#include "overlays/actors/ovl_En_GirlA/z_en_girla.h"
#include "soh/Enhancements/randomizer/NeiGiPresentation.cpp"
#include "variables.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <limits>
#include <set>
#include <string>
#include <vector>

#ifdef COMBO_BUILD
extern bool ownerAlt;
#endif
namespace Fixture {
int enabled, alt, dinSword, loads, allocations, fallback, interpolation, vanilla, trap,
    triforce;
float matrix = 1;
float matrixY = 0;
float matrixYaw = 0;
bool sunProbe = false;
std::vector<float> yawStack;
std::vector<float> submittedYaw;
int modelFitContext = -1;
std::vector<std::pair<float, float>> stack;
std::vector<std::pair<float, float>> submitted;
std::vector<std::pair<float, float>> effectSubmitted;
float selectedFitScale=1, selectedFitLift=0;
std::vector<std::array<unsigned, 3>> flameColors;
std::vector<std::vector<Vtx>> arena;
bool portableSongLists=false;
bool selectedCustomLists=false;
std::map<Gfx *, std::vector<Vtx>> vertexLoads;
std::set<std::string> files;
std::map<std::string, int> cosmeticFlags;
std::map<std::string, Color_RGB8> cosmeticColors;
std::set<std::pair<std::string,std::string>> modFiles;
std::set<std::pair<std::string,std::string>> invalidGiModels;
alignas(16) Gfx opa[0x2FC0], xlu[0x1000], overlay[0x800];
Gfx setupDl{};
Vtx *pendingVertices;
size_t pendingVertexCount;
GraphicsContext gfx{};
PlayState play{};
GetItemEntry shopEntry{};
void Reset() {
  enabled = alt = dinSword = loads = allocations = fallback = interpolation = 0;
  vanilla = trap = triforce = 0;
  matrix = 1;
  matrixY = 0;
  matrixYaw = 0;
  sunProbe = false;
  yawStack.clear();
  submittedYaw.clear();
  modelFitContext = -1;
  submitted.clear();
  effectSubmitted.clear();
  selectedFitScale=1; selectedFitLift=0;
  flameColors.clear();
  arena.clear();
  portableSongLists=false;
  selectedCustomLists=false;
  pendingVertices = nullptr;
  vertexLoads.clear();
  std::memset(opa, 0, sizeof(opa));
  std::memset(xlu, 0, sizeof(xlu));
  std::memset(overlay, 0, sizeof(overlay));
  stack.clear();
  files.clear();
  cosmeticFlags.clear();
  cosmeticColors.clear();
  modFiles.clear();
  invalidGiModels.clear();
  gfx.polyOpa.p = opa;
  gfx.polyOpa.d = std::end(opa);
  gfx.polyXlu.p = xlu;
  gfx.polyXlu.d = std::end(xlu);
  gfx.overlay.p = overlay;
  gfx.overlay.d = std::end(overlay);
  play.state.gfxCtx = &gfx;
  play.gameplayFrames = 42;
  play.billboardMtxF = {};
  play.billboardMtxF.xx = play.billboardMtxF.yy = play.billboardMtxF.zz = 1;
}
void Original() {
  ++fallback;
  Matrix_Scale(7, 7, 7, MTXMODE_APPLY);
}
std::vector<std::string> Drawn(bool retainBaseRoute = false) {
  std::vector<std::string> paths;
  for (auto range :
       {std::pair(opa, gfx.polyOpa.p), std::pair(xlu, gfx.polyXlu.p)}) {
    for (Gfx *p = range.first; p != range.second; ++p) {
      if (((p->words.w0 >> 24) & 255) == G_DL_OTR_FILEPATH) {
        const char* path = reinterpret_cast<const char *>(p->words.w1);
        // Existing model-identity checks compare the authored slug. Dedicated
        // routing regressions retain and assert the exact archive owner marker.
        if (!retainBaseRoute && !std::strncmp(path,"__OTR__@oot-gi-base:",20))
          paths.emplace_back(std::string("__OTR__")+(path+20));
        else paths.emplace_back(path);
      }
    }
  }
  return paths;
}
void ExpectBoleroShimmerOnly() {
  assert(arena.size()==1 && "Bolero must submit only its shared red shimmer, without flame particles");
}
} // namespace Fixture

extern "C" {
int ResourceMgr_GetGiModelBoundsForGame(const char*,const char* const*,int,float[3],float[3]) { return 0; }
#ifndef NEI_GI_REWARD_FIXTURE
int ResourceMgr_GetRewardSurfaceForGame(const char*,const char*,NeiGi::Mesh*) { return 0; }
#endif
void NeiUsedMagic_DrawChargeFocus(PlayState*, int) {}
uintptr_t gSegments[NUM_SEGMENTS];
GameInfo gameInfo{};
GameInfo *gGameInfo = &gameInfo;
void GetItem_Draw(PlayState *, s16) { ++Fixture::vanilla; }
void Player_DrawGetItemIceTrap(PlayState *, Player *, Vec3f *, s32, f32) {
  ++Fixture::trap;
}
void Randomizer_DrawTriforcePieceGI(PlayState *, GetItemEntry) {
  ++Fixture::triforce;
}
f32 Math_SinS(s16) { return 0; }
f32 Math_CosS(s16) { return 1; }
void Matrix_RotateZYX(s16, s16, s16, u8) {}
int32_t CVarGetInteger(const char *name, int32_t) {
  if (auto it=Fixture::cosmeticFlags.find(name);it!=Fixture::cosmeticFlags.end()) return it->second;
  if (std::strcmp(name, CVAR_NEI_GI_EFFECTS) == 0) return Fixture::enabled;
  if (std::strcmp(name, CVAR_ENHANCEMENT("DinFireSword")) == 0) return Fixture::dinSword;
  return 0;
}
Color_RGB8 CVarGetColor24(const char* name,Color_RGB8 value) {
  auto it=Fixture::cosmeticColors.find(name);
  return it==Fixture::cosmeticColors.end()?value:it->second;
}
int ResourceMgr_GetDinSwordGiProfileForGame(const char*,const char* path) {
  if(!std::strncmp(path,"__OTR__@oot:",12))path+=12;
  else if(!std::strncmp(path,"__OTR__",7))path+=7;
  return DinSwordGi::SelectedProfile(path,Fixture::dinSword,Fixture::alt,
    [](const char* dependency) {
      const std::string key=!std::strncmp(dependency,"__OTR__",7)?dependency:std::string("__OTR__")+dependency;
      return Fixture::files.contains(key);
    });
}
ShopItemIdentity Randomizer_IdentifyShopItem(s32, u8) { return {}; }
GetItemEntry
Randomizer_GetItemFromKnownCheckWithoutObtainabilityCheck(RandomizerCheck,
                                                          GetItemID) {
  return Fixture::shopEntry;
}
GetItemEntry GetItemMystery() { return {}; }
void EnItem00_CustomItemsParticles(Actor *, PlayState *, GetItemEntry) {}
void func_80A3C498(Actor *, PlayState *, s32) {}
uint8_t ResourceMgr_FileExists(const char *path) {
  return Fixture::files.contains(path);
}
uint8_t ResourceMgr_FileAltExists(const char *path) {
  return Fixture::files.contains(std::string("alt/") + path);
}
bool ResourceMgr_IsAltAssetsEnabled() { return Fixture::alt; }
int ResourceMgr_IsModAssetForGame(const char* game,const char* path) {
  bool alt=Fixture::alt;
#ifdef COMBO_BUILD
  if(std::strcmp(game,"oot")==0)alt=ownerAlt;
#endif
  const std::string canonical=std::strncmp(path,"__OTR__",7)==0?std::string(path):std::string("__OTR__")+path;
  const std::string selected=alt&&Fixture::files.contains(std::string("alt/")+canonical)?std::string("alt/")+canonical:canonical;
  return Fixture::modFiles.contains({game,selected});
}
int ResourceMgr_IsModAsset(const char* path) {return ResourceMgr_IsModAssetForGame("oot",path);}
int ResourceMgr_IsGiModelAvailableForGame(const char* game,const char* path) {
  bool selectedAlt=Fixture::alt;
#ifdef COMBO_BUILD
  if(!std::strcmp(game,"oot"))selectedAlt=ownerAlt;
#endif
  const std::string key=std::strncmp(path,"__OTR__",7)?std::string("__OTR__")+path:path;
  const std::string selected=selectedAlt&&Fixture::files.contains("alt/"+key)?"alt/"+key:key;
  return Fixture::files.contains(selected) && !Fixture::invalidGiModels.contains({game,selected});
}
extern "C" int ResourceMgr_IsIkanaShieldGiAvailable() {
  return ResourceMgr_IsGiModelAvailableForGame(
             "mm",
             "__OTR__objects/object_gi_shield_3/gGiMirrorShieldEmptyDL") &&
         ResourceMgr_IsGiModelAvailableForGame(
             "mm", "__OTR__objects/object_gi_shield_3/gGiMirrorShieldDL");
}
int ResourceMgr_GetGiModelFitForGame(const char*,const char*,float,float,int context,float fit[2]) {
  Fixture::modelFitContext=context;
  fit[0]=Fixture::selectedFitScale;fit[1]=Fixture::selectedFitLift;
  return Fixture::selectedFitScale!=1 || Fixture::selectedFitLift!=0;
}
int ResourceMgr_GetGiModelsFitForGame(const char* game,const char* const* paths,int count,float scale,float tilt,int context,float fit[2]) {
  assert(paths && count > 0);
  return ResourceMgr_GetGiModelFitForGame(game,paths[0],scale,tilt,context,fit);
}
uint8_t MmAssets_IsAvailable() { return 1; }

Gfx *ResourceMgr_LoadGfxByName(const char *path) {
  ++Fixture::loads;
  return Fixture::files.contains(path)
             ? reinterpret_cast<Gfx *>(const_cast<char *>(path))
             : nullptr;
}
void Matrix_Push() {
  Fixture::stack.emplace_back(Fixture::matrix, Fixture::matrixY);
  Fixture::yawStack.push_back(Fixture::matrixYaw);
}
void Matrix_Pop() {
  assert(!Fixture::stack.empty());
  Fixture::matrix = Fixture::stack.back().first;
  Fixture::matrixY = Fixture::stack.back().second;
  Fixture::stack.pop_back();
  Fixture::matrixYaw = Fixture::yawStack.back();
  Fixture::yawStack.pop_back();
}
void Matrix_Scale(float x, float, float, uint8_t) { Fixture::matrix *= x; }
void Matrix_RotateX(float, uint8_t) {}
void Matrix_RotateY(float, uint8_t) {}
void Matrix_RotateZ(float, uint8_t) {}
void Matrix_Translate(float, float y, float, uint8_t mode) {
  if (mode == MTXMODE_NEW) {
    Fixture::matrix = 1;
    Fixture::matrixY = y;
  } else
    Fixture::matrixY += y * Fixture::matrix;
}
void Matrix_ReplaceRotation(MtxF *) {
  if (Fixture::sunProbe) {
    Fixture::matrixYaw = 0;
  }
}
void Matrix_Put(MtxF *m) { Fixture::matrix=m->xx; Fixture::matrixY=m->yw; }
void Matrix_Get(MtxF *m) {
  *m = {};
  m->xx = m->yy = m->zz = Fixture::matrix;
  m->yw = Fixture::matrixY;
  if (Fixture::sunProbe) {
    m->xx = m->zz = std::cos(Fixture::matrixYaw) * Fixture::matrix;
    m->xz = std::sin(Fixture::matrixYaw) * Fixture::matrix;
    m->zx = -m->xz;
  }
}
void *DebitTail(GraphicsContext *context, size_t size) {
  const auto tail =
      (reinterpret_cast<uintptr_t>(context->polyOpa.d) & ~uintptr_t(15)) -
      ((size + 15) & ~size_t(15));
  assert(tail >= reinterpret_cast<uintptr_t>(context->polyOpa.p) &&
         "production renderer tried to cross the real OPA head/tail");
  context->polyOpa.d = reinterpret_cast<Gfx *>(tail);
  return reinterpret_cast<void *>(tail);
}
void *Graph_Alloc(GraphicsContext *context, size_t size) {
  assert(size <= 1536 * sizeof(Vtx) && size % sizeof(Vtx) == 0);
  Fixture::arena.emplace_back(size / sizeof(Vtx));
  Fixture::pendingVertices = static_cast<Vtx *>(DebitTail(context, size));
  Fixture::pendingVertexCount = size / sizeof(Vtx);
  return Fixture::pendingVertices;
}
Mtx *Matrix_NewMtx(GraphicsContext *context, char *, int32_t) {
  if (Fixture::pendingVertices) {
    Fixture::effectSubmitted.emplace_back(Fixture::matrix, Fixture::matrixY);
    std::copy_n(Fixture::pendingVertices, Fixture::pendingVertexCount,
                Fixture::arena.back().data());
    Fixture::pendingVertices = nullptr;
  }
  Fixture::submitted.emplace_back(Fixture::matrix, Fixture::matrixY);
  if (Fixture::sunProbe) Fixture::submittedYaw.push_back(Fixture::matrixYaw);
  ++Fixture::allocations;
  return static_cast<Mtx *>(DebitTail(context, sizeof(Mtx)));
}
#include "nei_gi_graph.inc"
void Gfx_SetupDL_25Opa(GraphicsContext *context) {
  OPEN_DISPS(context);
  __gSPDisplayList(POLY_OPA_DISP++, &Fixture::setupDl);
  CLOSE_DISPS(context);
}
void Gfx_SetupDL_26Opa(GraphicsContext *context) {
  Gfx_SetupDL_25Opa(context);
}
void Gfx_SetupDL_25Xlu(GraphicsContext *context) {
  OPEN_DISPS(context);
  __gSPDisplayList(POLY_XLU_DISP++, &Fixture::setupDl);
  CLOSE_DISPS(context);
}
void FrameInterpolation_RecordOpenChild(const void *, int) {
  ++Fixture::interpolation;
}
void FrameInterpolation_RecordCloseChild() { --Fixture::interpolation; }
void gSPVertex(Gfx *cmd, uintptr_t data, int n, int v0) {
  assert(n > 0 && n + v0 <= 32);
  const auto *vertices = reinterpret_cast<const Vtx *>(data);
  Fixture::vertexLoads[cmd] = std::vector<Vtx>(vertices, vertices + n);
  cmd->words.w0 = cmd->words.w1 = 0;
}
void gSPDisplayList(Gfx *packet, Gfx *list) {
  assert((Fixture::portableSongLists || Fixture::selectedCustomLists) &&
         "authored GI paths must be deferred, not resolved through the legacy wrapper");
  gDma1p(packet, G_DL_OTR_FILEPATH, list, 0, G_DL_PUSH);
}
#define ORIGINAL(name)                                                         \
  void name(PlayState *, GetItemEntry *) { Fixture::Original(); }
ORIGINAL(Randomizer_DrawRocsFeatherSkijer)
ORIGINAL(Randomizer_DrawRocsFeather)
ORIGINAL(Randomizer_DrawWhip)
ORIGINAL(Randomizer_DrawFireRod)
ORIGINAL(Randomizer_DrawIceRod)
ORIGINAL(Randomizer_DrawLightRod)
ORIGINAL(Randomizer_DrawDekuLeaf)
ORIGINAL(Randomizer_DrawSwitchHook)
ORIGINAL(Randomizer_DrawMogmaMitts)
ORIGINAL(Randomizer_DrawGustJar)
ORIGINAL(Randomizer_DrawBallAndChain)
ORIGINAL(Randomizer_DrawTimeGate)
ORIGINAL(Randomizer_DrawBeetle)
ORIGINAL(Randomizer_DrawShovel)
ORIGINAL(Randomizer_DrawHyliaGrace)
ORIGINAL(Randomizer_DrawZonaiPermafrost)
ORIGINAL(Randomizer_DrawDemiseDestruction)
ORIGINAL(Randomizer_DrawRocsCape)
ORIGINAL(Randomizer_DrawSpinner)
ORIGINAL(Randomizer_DrawBombArrows)
ORIGINAL(Randomizer_DrawCaneOfSomaria)
ORIGINAL(Randomizer_DrawCaneSomariaUpgrade)
ORIGINAL(Randomizer_DrawCanePacci)
ORIGINAL(Randomizer_DrawCanePacciUpgrade)
ORIGINAL(Randomizer_DrawCanePacciUltrahand)
ORIGINAL(Randomizer_DrawExtCaneOfByrna)
ORIGINAL(Randomizer_DrawMinishCap)
ORIGINAL(Randomizer_DrawDominionRod)
ORIGINAL(Randomizer_DrawMagnesis)
ORIGINAL(Randomizer_DrawStasis)
ORIGINAL(Randomizer_DrawLantern)
ORIGINAL(Randomizer_DrawMarioMask)
ORIGINAL(Randomizer_DrawPokeball)
ORIGINAL(Randomizer_DrawCryonis)
ORIGINAL(Randomizer_DrawElementalWand)
ORIGINAL(Randomizer_DrawExtFourSword)
void Randomizer_DrawExtFourSwordPresentation(PlayState* play, GetItemEntry* entry, int) {
  Randomizer_DrawExtFourSword(play, entry);
}
ORIGINAL(Randomizer_DrawExtDivineShield)
ORIGINAL(Randomizer_DrawExtSheikahShield)
ORIGINAL(Randomizer_DrawExtShieldOfIkana)
ORIGINAL(Randomizer_DrawExtMagicCape)
ORIGINAL(Randomizer_DrawExtSpiritBreastplate)
ORIGINAL(Randomizer_DrawExtSagesTunic)
ORIGINAL(Randomizer_DrawExtChampionsTunic)
ORIGINAL(Randomizer_DrawExtPegasusAnklet)
ORIGINAL(Randomizer_DrawExtTrident)
ORIGINAL(Randomizer_DrawExtClimbBoots)
ORIGINAL(Randomizer_DrawExtRocBoots)
ORIGINAL(Randomizer_DrawExtPendantOfMemories)
ORIGINAL(Randomizer_DrawMmTradeQuest)
ORIGINAL(Randomizer_DrawNeiSheikahSlate)
ORIGINAL(Randomizer_DrawSlateRuneBomb)
ORIGINAL(Randomizer_DrawSlateRuneMasterCycle)
ORIGINAL(Randomizer_DrawSlateRuneStasis)
ORIGINAL(Randomizer_DrawSlateRuneCryonis)
ORIGINAL(Randomizer_DrawSlateRuneSensor)
ORIGINAL(Randomizer_DrawNeiPhantomHourglass)
ORIGINAL(Randomizer_DrawNeiShadowCrystal)
ORIGINAL(Randomizer_DrawNeiRodOfSeasons)
ORIGINAL(Randomizer_DrawSeasonSpring)
ORIGINAL(Randomizer_DrawSeasonSummer)
ORIGINAL(Randomizer_DrawSeasonAutumn)
ORIGINAL(Randomizer_DrawSeasonWinter)
ORIGINAL(Randomizer_DrawProgressiveKokiriSword)
ORIGINAL(Randomizer_DrawProgressiveMasterSword)
ORIGINAL(Randomizer_DrawProgressiveBGS)
ORIGINAL(Randomizer_DrawGiantsKnife)
ORIGINAL(Randomizer_DrawMasterSword)
ORIGINAL(Randomizer_DrawRazorSword)
ORIGINAL(Randomizer_DrawGildedSword)
ORIGINAL(Randomizer_DrawTrueMasterSword)
ORIGINAL(Randomizer_DrawGreatFairySword)
ORIGINAL(Randomizer_DrawIronKnuckleAxe)
ORIGINAL(UnrelatedDraw)
#undef ORIGINAL
static void DrawWeaponFlameOverlay(PlayState *, u8 r, u8 g, u8 b) {
  Fixture::flameColors.push_back({r, g, b});
}
#define gSPSegment(pkt, seg, target) ((void)(pkt))
#include "nei_gi_dispatch.inc"
#undef gSPSegment
}

#ifndef NEI_GI_FIXTURE_BOUNDARY_ONLY
int main() {
  using namespace Fixture;
#include "tests/black_market/frame_renderer_checks.inc"
  #include "tests/nei_gi/sword_particle_readability_test.inc"
#include "tests/nei_gi/giants_knife_presentation_test.inc"
#ifdef COMBO_BUILD
#include "tests/sword_fallback/effects_toggle_checks.inc"
#endif
  // This allocator uses the actual OPA tail for vertices and matrices,
  // including OoT's alignment loss; setup functions also emit their real one
  // command.
  const NeiGi::TextureMaterial arenaMaterial{
      "__OTR__objects/private/arena-test", false, false};
  for (int draw = 0; draw < 7; ++draw)
    for (int shortArena = 0; shortArena < 6; ++shortArena) {
      Reset();
      files.insert(arenaMaterial.path);
      gfx.polyOpa.p = opa + 17;
      gfx.polyXlu.p = xlu + 23;
      if (shortArena == 0)
        gfx.polyOpa.d = opa + 18;
      if (shortArena == 1)
        gfx.polyXlu.d = xlu + 24;
      if (shortArena == 2)
        gfx.polyOpa.d =
            reinterpret_cast<Gfx *>(reinterpret_cast<uintptr_t>(opa + 17) + 15);
      if (shortArena == 3)
        gfx.polyXlu.d = xlu + 22;
      if (shortArena == 4)
        gfx.overlay.d = overlay + 1;
      if (shortArena == 5)
        gfx.polyOpa.d = opa + 16;
      const auto opaHead = gfx.polyOpa.p, opaTail = gfx.polyOpa.d;
      const auto xluHead = gfx.polyXlu.p, xluTail = gfx.polyXlu.d;
      const auto mesh = NeiGi::SampleSong(CW_SONG_OOT_ZELDA, 42);
      const uint8_t color[4] = {80, 180, 240, 255};
      const float center[3] = {};
      if (draw == 0)
        NeiGi_DrawMesh(&play, mesh);
      if (draw == 1)
        assert(!NeiGi_DrawTexturedMesh(&play, mesh, arenaMaterial));
      if (draw == 2)
        NeiGi_DrawSeasonOverlay(&play, 6, "oot");
      if (draw == 3)
        NeiGi_DrawShimmerOverlay(&play, color, "oot");
      if (draw == 4)
        NeiGi_DrawSongOverlay(&play, CW_SONG_OOT_ZELDA, "oot");
      if (draw == 5)
        NeiGi_DrawPresentation(
            &play, "__OTR__objects/nei_gi_redesign/fire_rod/gi_dl", nullptr, 1,
            int(Kind::Fire), center, true, "oot");
      if (draw == 6)
        NeiGi_DrawExternalPresentation(&play, "__OTR__objects/arbitrary/gi_dl",
                                       nullptr, 1, int(Kind::Fire), true,
                                       "oot");
      assert(gfx.polyOpa.p == opaHead && gfx.polyOpa.d == opaTail &&
             gfx.polyXlu.p == xluHead && gfx.polyXlu.d == xluTail);
      assert(arena.empty() && submitted.empty() && stack.empty() &&
             matrix == 1 && matrixY == 0);
    }
  // Exact conservative mesh budget: three packed vertices, one matrix, two
  // transient OPA debug nodes, one load/triangle and 32 fixed XLU commands.
  NeiGi::Mesh arenaTriangle;
  arenaTriangle.count = 3;
  arenaTriangle.vertices[0] = {{0, 0, 0}, 0xFFFFFF, 255};
  arenaTriangle.vertices[1] = {{1, 0, 0}, 0xFFFFFF, 255};
  arenaTriangle.vertices[2] = {{0, 1, 0}, 0xFFFFFF, 255};
  for (int padding : {0, 15, -1}) {
    Reset();
    gfx.polyOpa.p = opa + 17;
    gfx.polyXlu.p = xlu + 23;
    const auto needed =
        3 * sizeof(Vtx) + ((sizeof(Mtx) + 15) & ~size_t(15)) + 2 * sizeof(Gfx);
    gfx.polyOpa.d = reinterpret_cast<Gfx *>(
        reinterpret_cast<uintptr_t>(gfx.polyOpa.p) + needed + padding);
    gfx.polyXlu.d = gfx.polyXlu.p + 34;
    gfx.overlay.d = overlay + 2;
    const auto head = gfx.polyOpa.p, tail = gfx.polyOpa.d;
    NeiGi_DrawMesh(&play, arenaTriangle);
    if (padding == -1)
      assert(arena.empty() && submitted.empty() && gfx.polyOpa.p == head &&
             gfx.polyOpa.d == tail && gfx.polyXlu.p == xlu + 23);
    else
      assert(arena.size() == 1 && submitted.size() == 1 &&
             gfx.polyOpa.d == head + 2);
    assert(gfx.polyOpa.p <= gfx.polyOpa.d && gfx.polyXlu.p <= gfx.polyXlu.d &&
           gfx.overlay.p == overlay && stack.empty());
  }
  // Once geometry has been submitted, expensive effects may decline, while the
  // actual caller still has room for the shell, owner pop and restore matrix.
  Reset();
  gfx.polyOpa.d = opa + 40;
  gfx.polyXlu.d = xlu + 128;
  const float reservedCenter[3] = {};
  NeiGi_DrawPresentation(
      &play, "__OTR__objects/nei_gi_redesign/zonai_permafrost/gi_dl",
      "__OTR__objects/nei_gi_redesign/zonai_permafrost/gi_xlu_dl", 1,
      int(Kind::Zonai), reservedCenter, true, "oot");
  assert(arena.empty() && submitted.size() == 3 && Drawn().size() == 2 &&
         gfx.polyOpa.p <= gfx.polyOpa.d && gfx.polyXlu.p <= gfx.polyXlu.d &&
         stack.empty());
  Reset();
  enabled = 1;
  gfx.polyOpa.d = opa + 40;
  gfx.polyXlu.d = xlu + 128;
  files.insert("__OTR__objects/nei_gi_redesign/zonai_permafrost/gi_dl");
  files.insert("__OTR__objects/nei_gi_redesign/zonai_permafrost/gi_xlu_dl");
  GetItemEntry arenaEntry{};
  arenaEntry.drawFunc = Randomizer_DrawZonaiPermafrost;
  assert(NeiGi_Draw(&play, &arenaEntry));
  assert(arena.empty() && submitted.size() == 2 && Drawn().size() == 2 &&
         gfx.polyOpa.p <= gfx.polyOpa.d && gfx.polyXlu.p <= gfx.polyXlu.d &&
         stack.empty());
  // Repeated high-vertex songs fill a real arena; subsequent calls skip before
  // all commands/allocations once even the wrapper no longer fits.
  Reset();
  gfx.polyOpa.d = opa + 4096;
  gfx.polyXlu.d = xlu + 512;
  for (int i = 0; i < 32; ++i) {
    NeiGi_DrawSongOverlay(&play, CW_SONG_OOT_ZELDA, "oot");
    assert(gfx.polyOpa.p <= gfx.polyOpa.d && gfx.polyXlu.p <= gfx.polyXlu.d &&
           gfx.overlay.p == overlay && stack.empty());
  }
  assert(!arena.empty());
  Reset();
  NeiGi_DrawSongOverlay(&play, CW_SONG_OOT_ZELDA, "oot");
  assert(arena.size() == 1 && !submitted.empty() && stack.empty());
  std::cout
      << "PASS real OPA-tail vertices/matrices, actual debug/setup commands: "
         "short/invalid/overlay/exact/misaligned budgets, caller shell/restore "
         "reserves and repeated approved-song exhaustion\n";
  const auto untouched = [] {
    assert(arena.empty() && submitted.empty() && gfx.polyOpa.p==opa && gfx.polyXlu.p==xlu);
    assert(stack.empty() && matrix==1 && matrixY==0 && interpolation==0);
  };
  for(size_t count:{size_t(0),size_t(1),size_t(2),size_t(1537),std::numeric_limits<size_t>::max()}) {
    Reset(); NeiGi::Mesh malformed;malformed.count=count;
    NeiGi_DrawMesh(&play,malformed);untouched();
  }
  for(float value:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),
                   -std::numeric_limits<float>::infinity(),1e30f,-1e30f,2048.f,-2049.f}) {
    for(int field=0;field<5;++field) {
      Reset(); NeiGi::Mesh malformed;malformed.count=3;
      if(field==0)malformed.vertices[0].p.x=value;
      if(field==1)malformed.vertices[0].p.y=value;
      if(field==2)malformed.vertices[0].p.z=value;
      if(field==3)malformed.vertices[0].u=value;
      if(field==4)malformed.vertices[0].v=value;
      NeiGi_DrawMesh(&play,malformed);untouched();
    }
  }
  Reset(); const auto validMesh=NeiGi::SampleShimmer(42,true);
  NeiGi_DrawMesh(nullptr,validMesh);untouched();
  play.state.gfxCtx=nullptr;NeiGi_DrawMesh(&play,validMesh);play.state.gfxCtx=&gfx;untouched();
  const float origin[3]={};
  const char* guarded="__OTR__objects/nei_gi_redesign/fire_rod/gi_dl";
  for(float scale:{0.f,-1.f,std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),1e30f}) {
    Reset(); NeiGi_DrawPresentation(&play,guarded,nullptr,scale,int(Kind::Fire),origin,true,"oot");untouched();
    NeiGi_DrawExternalPresentation(&play,guarded,nullptr,scale,int(Kind::Fire),true,"oot");untouched();
  }
  for(int axis=0;axis<3;++axis) {
    Reset();float center[3]={};center[axis]=std::numeric_limits<float>::quiet_NaN();
    NeiGi_DrawPresentation(&play,guarded,nullptr,1,int(Kind::Fire),center,true,"oot");untouched();
  }
  Reset(); NeiGi_DrawPresentation(&play,guarded,nullptr,1,int(Kind::Ice),origin,true,"oot");untouched();
  for(int identity:{-2,int(Kind::Last)+1}) {
    Reset();NeiGi_DrawPresentation(&play,guarded,nullptr,1,int(Kind::Fire),origin,true,"oot",false,0,identity);untouched();
  }
  NeiGi_DrawPresentation(&play,"__OTR__objects/arbitrary/gi_dl",nullptr,1,int(Kind::Fire),origin,true,"oot");untouched();
  Reset(); NeiGi::Mesh maximum;
  for(size_t i=0;i<maximum.vertices.size();++i)maximum.vertices[i]={{float(i%32),float(i/32),0},0xC8A0FF,255};
  maximum.count=maximum.vertices.size();NeiGi_DrawMesh(&play,maximum);
  assert(arena.size()==1 && arena.front().size()<=1536 && gfx.polyXlu.p<xlu+4096 && stack.empty());
  std::cout<<"PASS real renderer malformed mesh/count/NaN/Inf/range/scale/center/path guards and maximum vertex batch\n";
  Reset();
  GetItemEntry fourSwordEntry{};
  fourSwordEntry.drawFunc = Randomizer_DrawExtFourSword;
  files.insert("__OTR__objects/nei_gi_redesign/four_sword/gi_dl");
  assert(NeiGi_Draw(&play, &fourSwordEntry));
  assert(!submitted.empty() && submitted[0].second == -18.f);
  Reset();
  files.insert("__OTR__objects/nei_gi_redesign/four_sword/gi_dl");
  assert(NeiGi_DrawShop(&play, &fourSwordEntry));
  const auto* fourFrame = NeiGi::FindFrameBounds("__OTR__objects/nei_gi_redesign/four_sword/gi_dl");
  assert(fourFrame && !submitted.empty());
  assert(submitted[0].second == NeiGi::FrameFit(*fourFrame,1,true).lift);
  // The Four Sword's shelf height was small enough, but its high pivot still
  // clipped the top. Check both ends of the real serialized envelope.
  struct FourBounds {
    CustomDrawFunc draw;
    const char *slug, *name, *callback;
    std::array<float,3> minimum,maximum;
    float width, drawScale;
    bool translucent;
    int identity = 0, gid = -1;
  };
  const FourBounds fourBounds[] = {
#include "nei_gi_bounds.inc"
  };
  const auto& four = *std::find_if(std::begin(fourBounds),std::end(fourBounds),
      [](const auto& b) {return std::strcmp(b.slug,"four_sword")==0;});
  assert(6.f + .25f * (submitted[0].second + submitted[0].first * four.maximum[1]) <= 19.f &&
         "Four Sword clips the shelf's upper edge despite passing its height test");
  for (auto song : { std::pair{ RG_MM_SONG_SONATA, CW_SONG_SONATA },
                     std::pair{ RG_MM_SONG_LULLABY_INTRO, CW_SONG_LULLABY_INTRO },
                     std::pair{ RG_MM_SONG_LULLABY, CW_SONG_LULLABY },
                     std::pair{ RG_MM_SONG_NOVA, CW_SONG_NOVA },
                     std::pair{ RG_MM_SONG_HEALING, CW_SONG_HEALING },
                     std::pair{ RG_MM_SONG_ELEGY, CW_SONG_ELEGY },
                     std::pair{ RG_MM_SONG_OATH, CW_SONG_OATH },
                     std::pair{ RG_MM_SONG_DOUBLE_TIME, CW_SONG_DOUBLE_TIME },
                     std::pair{ RG_MM_SONG_INVERTED_TIME, CW_SONG_INVERTED_TIME } }) {
    Reset();
    NeiGi_DrawSongOverlay(&play,song.second,nullptr);
    const auto expectedSong=arena;
    assert(expectedSong.size()==2);
    Reset();
    GetItemEntry songEntry{};
    songEntry.tableId = TABLE_RANDOMIZER;
    songEntry.drawItemId = song.first;
    uint8_t color[4];
    assert(ComboSongShimmerColor(song.second, color));
    assert(NeiGi_Draw(&play, &songEntry));
    assert(Drawn() == std::vector<std::string>{ gGiSongNoteDL });
    assert(arena.size()==expectedSong.size() && stack.empty() && "MM song must submit its shimmer and selected particle profile");
    for(size_t i=0;i<expectedSong.size();++i)assert(arena[i].size()==expectedSong[i].size() &&
        !memcmp(arena[i].data(),expectedSong[i].data(),expectedSong[i].size()*sizeof(Vtx)));
#ifdef COMBO_BUILD
    CwItemDrawInfo songInfo{};
    assert(NeiGi_DescribeEntry(&songEntry, &songInfo));
    assert(songInfo.drawKind == CW_DRAW_KIND_SONG_GI && songInfo.neiEffect == song.second && songInfo.itemShimmer);
    assert(songInfo.dlistCount==1 && songInfo.xluStartIndex==0);
    assert(std::memcmp(color, songInfo.itemShimmerColor, 4) == 0);
#endif
  }
  Reset();
  NeiGi_DrawMesh(&play,NeiGi::SampleSong(CW_SONG_SOARING,42,NeiGi_CameraBasis(&play)));
  const auto expectedFeathers=arena;
  assert(!expectedFeathers.empty() && expectedFeathers.front().size()>400);
  Reset();
  GetItemEntry soaringEntry{};
  soaringEntry.tableId=TABLE_RANDOMIZER;soaringEntry.drawItemId=RG_MM_SONG_SOARING;
  assert(NeiGi_Draw(&play,&soaringEntry));
  assert(Drawn()==std::vector<std::string>{gGiSongNoteDL} && arena.size()==expectedFeathers.size() && stack.empty());
  for(size_t i=0;i<arena.size();++i)assert(arena[i].size()==expectedFeathers[i].size() &&
      !memcmp(arena[i].data(),expectedFeathers[i].data(),arena[i].size()*sizeof(Vtx)));
#ifdef COMBO_BUILD
  CwItemDrawInfo soaringInfo{};
  assert(NeiGi_DescribeEntry(&soaringEntry,&soaringInfo) && soaringInfo.itemShimmer && soaringInfo.neiEffect==CW_SONG_SOARING);
#endif
  Reset();
  GetItemEntry stormEntry{};
  stormEntry.gid = GID_SONG_STORM;
  assert(NeiGi_Draw(&play, &stormEntry));
  assert(Drawn() == std::vector<std::string>{gGiSongNoteDL} && arena.size() == 1 && stack.empty());
  const auto shownRain=arena.front();
  arena.clear();
  NeiGi_DrawMesh(&play,NeiGi::SampleSeason(42,1,NeiGi_CameraBasis(&play)));
  assert(arena.front().size()==shownRain.size() && !std::memcmp(arena.front().data(),shownRain.data(),shownRain.size()*sizeof(Vtx)) &&
         "Storms must render only the existing rain, without lightning geometry");
  struct SongFixture {int rg,gid,song;const char* colorDl;uint32_t hue;};
  const SongFixture shimmerSongs[]={
    {RG_MINUET_OF_FOREST,GID_SONG_MINUET,CW_SONG_OOT_MINUET,gGiMinuetColorDL,0x62FF62},
    {RG_BOLERO_OF_FIRE,GID_SONG_BOLERO,CW_SONG_OOT_BOLERO,gGiBoleroColorDL,0xFF3C00},
    {RG_SERENADE_OF_WATER,GID_SONG_SERENADE,CW_SONG_OOT_SERENADE,gGiSerenadeColorDL,0x55B4DF},
    {RG_REQUIEM_OF_SPIRIT,GID_SONG_REQUIEM,CW_SONG_OOT_REQUIEM,gGiRequiemColorDL,0xDE9E2F},
    {RG_NOCTURNE_OF_SHADOW,GID_SONG_NOCTURNE,CW_SONG_OOT_NOCTURNE,gGiNocturneColorDL,0xA028D2},
    {RG_PRELUDE_OF_LIGHT,GID_SONG_PRELUDE,CW_SONG_OOT_PRELUDE,gGiPreludeColorDL,0xEDE73E},
    {RG_EPONAS_SONG,GID_SONG_EPONA,CW_SONG_OOT_EPONA,nullptr,0xD96E30},
    {RG_SUNS_SONG,GID_SONG_SUN,CW_SONG_SUN,nullptr,0xEDE73E}};
  for(const auto& song:shimmerSongs)for(int effects:{0,1})for(int selectedAlt:{0,1})for(int mod:{0,1}) {
    Reset();enabled=effects;alt=selectedAlt;
#ifdef COMBO_BUILD
    ownerAlt=selectedAlt;
#endif
    const std::string selected=(selectedAlt?"alt/":"")+std::string(gGiSongNoteDL);
    files.insert(gGiSongNoteDL);
    if(selectedAlt)files.insert(selected);
    if(mod)modFiles.insert({"oot",selected});
    const uint8_t rgba[]={uint8_t(song.hue>>16),uint8_t(song.hue>>8),uint8_t(song.hue),255};
    NeiGi_DrawShimmerOverlay(&play,rgba,nullptr);
    const auto expected=arena.front();
    Reset();enabled=effects;alt=selectedAlt;
    files.insert(gGiSongNoteDL);if(selectedAlt)files.insert(selected);
    if(mod)modFiles.insert({"oot",selected});
    GetItemEntry songEntry{};songEntry.tableId=TABLE_RANDOMIZER;songEntry.drawItemId=song.rg;songEntry.gid=song.gid;
    assert(NeiGi_Draw(&play,&songEntry));
    const auto paths=Drawn();
    const std::vector<std::string> wanted=song.colorDl?std::vector<std::string>{song.colorDl,gGiSongNoteDL}:
                                                           std::vector<std::string>{gGiSongNoteDL};
    assert(paths==wanted && "all warp songs preserve their native color display list and clef, including selected Alt/mod assets");
    size_t copies=0;
    for(const auto& submitted:arena)if(submitted.size()==expected.size() && !std::memcmp(submitted.data(),expected.data(),expected.size()*sizeof(Vtx)))++copies;
    assert(copies==1 && stack.empty() && "one matching shared shimmer is mandatory even with ItemEffects off or no mod present");
    if(song.song==CW_SONG_OOT_BOLERO)ExpectBoleroShimmerOnly();
#ifdef COMBO_BUILD
    CwItemDrawInfo info{};assert(NeiGi_DescribeEntry(&songEntry,&info));
    assert(info.drawKind==CW_DRAW_KIND_SONG_GI && info.neiEffect==song.song && info.itemShimmer);
    assert(info.dlistCount==(song.colorDl?2:1) && info.xluStartIndex==0);
    assert(!std::memcmp(info.itemShimmerColor,rgba,4));
    assert(std::strcmp(info.dlists[info.dlistCount-1],gGiSongNoteDL)==0);
    if(song.colorDl)assert(std::strcmp(info.dlists[0],song.colorDl)==0);
#endif
  }
  std::cout << "PASS song presentations: normal MM notes, Storms note/rain, native warp palettes and mandatory matching shimmer across toggle/Alt/mod paths\n";
  Reset();
  GetItemEntry entry{};
  assert(!NeiGi_Draw(nullptr, &entry));
  assert(!NeiGi_Draw(&play, nullptr));
  assert(!NeiGi_Draw(&play, &entry));
  entry.drawFunc = UnrelatedDraw;
  assert(!NeiGi_Draw(&play, &entry));
  assert(allocations == 0 && fallback == 0);

  // The concrete sword binding must select its approved palette and emit
  // intrinsic particles through both native and shared host draw boundaries.
  // A neutral binding or a stale enum gate silently loses this presentation.
  for (const auto& sword : {
      std::pair{&Randomizer_DrawProgressiveKokiriSword, 0x78C850u},
      std::pair{&Randomizer_DrawRazorSword, 0xBDD6EAu},
      std::pair{&Randomizer_DrawGildedSword, 0xFFD45Au},
      std::pair{&Randomizer_DrawMasterSword, 0x6F8FFFu},
      std::pair{&Randomizer_DrawTrueMasterSword, 0xFFF4D6u},
      std::pair{&Randomizer_DrawProgressiveBGS, 0xFF9A42u},
      std::pair{&Randomizer_DrawGreatFairySword, 0x79BE84u},
      std::pair{&Randomizer_DrawExtFourSword, 0x315B2Fu}}) {
    Reset();
    entry = {};
    entry.drawFunc = sword.first;
    const auto* binding = FindPresentation(&entry);
    assert(binding && NeiGi::ColorHex(binding->effect) == sword.second);
    files.insert(binding->opaque);
    assert(NeiGi_Draw(&play, &entry));
    assert(Drawn() == std::vector<std::string>{binding->opaque});
    assert(!arena.empty()); // Intrinsic particles, even with optional shimmer off.
    Reset();
    const float center[] = {0, 0, 0};
    NeiGi_DrawPresentation(&play, binding->opaque, nullptr, binding->scale,
                          int(binding->effect), center, false, nullptr);
    assert(Drawn() == std::vector<std::string>{binding->opaque});
    assert(!arena.empty() && stack.empty());
  }
  std::cout << "PASS sword native/shared bindings: approved palettes and intrinsic particles\n";

#include "tests/nei_gi/equipment_shimmer_checks.inc"
  // These items retain their gold halo as intrinsic equipment shimmer. A neutral
  // binding would produce blue vertices even though the meshes themselves are gold.
  for (CustomDrawFunc draw : {Randomizer_DrawExtTrident, Randomizer_DrawExtRocBoots}) {
    Reset();
    enabled = 1;
    entry = {};
    entry.drawFunc = draw;
    const auto* binding = FindPresentation(&entry);
    assert(binding);
    files.insert(binding->opaque);
    assert(NeiGi_Draw(&play, &entry) && arena.size() == 1);
    bool gold = false;
    for (const auto& vertex : arena.front())
      gold |= vertex.v.cn[0] == 255 && vertex.v.cn[1] == 212 && vertex.v.cn[2] == 90;
    assert(gold && "Trident and Roc's Boots must emit gold shimmer vertices");
    Reset();
    enabled = 1;
    const float center[] = {0, 0, 0};
    NeiGi_DrawPresentation(&play, binding->opaque, nullptr, binding->scale,
                          int(binding->effect), center, true, nullptr);
    gold = false;
    for (const auto& vertices : arena)
      for (const auto& vertex : vertices)
        gold |= vertex.v.cn[0] == 255 && vertex.v.cn[1] == 212 && vertex.v.cn[2] == 90;
    assert(gold && stack.empty());
    Reset();
    files.insert(binding->opaque);
    assert(NeiGi_Draw(&play, &entry) && arena.size()==1);
  }
  std::cout << "PASS Trident/Roc's Boots gold shimmer in native/shared draws with either optional setting\n";

  // Rune identification survives the shared Slate silhouette and animation.
  std::set<uint32_t> runeHues;
  for (CustomDrawFunc draw : {Randomizer_DrawSlateRuneBomb,
                             Randomizer_DrawSlateRuneMasterCycle,
                             Randomizer_DrawSlateRuneStasis,
                             Randomizer_DrawSlateRuneCryonis,
                             Randomizer_DrawSlateRuneSensor}) {
    entry.drawFunc = draw;
    const auto* rune = FindPresentation(&entry);
    assert(rune != nullptr);
    const auto hue = NeiGi::ColorHex(rune->effect);
    assert(runeHues.insert(hue).second);
    const auto shimmer = NeiGi::SampleShimmer(45, true, {}, rune->effect);
    bool carriesHue = false;
    for (size_t i = 0; i < shimmer.count; ++i)
      carriesHue |= shimmer.vertices[i].rgb == hue;
    assert(carriesHue);
    assert(NeiGi::SampleSpecial(rune->effect, 45).count > 0);
    // MM consumes the shared integer effect through this renderer boundary.
    Reset();
    const float center[] = {0, 0, 4};
    NeiGi_DrawPresentation(&play, rune->opaque, nullptr, 1.f,
                           static_cast<int>(rune->effect), center, true, nullptr);
    assert(gfx.polyOpa.p > opa && !arena.empty());
    assert(stack.empty());
  }

  // The summer sun's local vertices must stay fixed while the held item
  // spins. Native matrix billboarding is interpolated; CPU counter-rotated
  // vertices snap at game ticks under an interpolated parent rotation.
  std::vector<std::vector<Vtx>> summerGeometry;
  unsigned previousScroll = 0;
  for (unsigned tick = 0; tick < 3; ++tick) {
    Reset(); sunProbe = true; matrixYaw = float(tick) * 0.7f;
    const float callerYaw = matrixYaw;
    play.gameplayFrames = 180 + tick;
    NeiGi_DrawSeasonOverlay(&play, 5, "oot");
    assert(arena.size() >= 2);
    assert(submittedYaw.size() >= 3);
    for (size_t i = 0; i + 1 < submittedYaw.size(); ++i)
      assert(submittedYaw[i] == 0 && "both sun passes must face the camera independently of the parent spin");
    assert(submittedYaw.back() == callerYaw);
    if (tick == 0) summerGeometry = arena;
    else {
      assert(arena.size() == summerGeometry.size());
      for (size_t pass = 0; pass < arena.size(); ++pass) {
        assert(arena[pass].size() == summerGeometry[pass].size());
        for (size_t vertex = 0; vertex < arena[pass].size(); ++vertex)
          assert(!std::memcmp(arena[pass][vertex].v.ob, summerGeometry[pass][vertex].v.ob,
                              sizeof(arena[pass][vertex].v.ob)) &&
                 "summer sun counter-rotation must not be baked into tick-dependent vertices");
      }
    }
    assert(matrixYaw == callerYaw && stack.empty());
    bool foundScroll = false;
    unsigned scroll = 0;
    for (Gfx* command = xlu; command < gfx.polyXlu.p; ++command) {
      if ((command->words.w0 >> 24) == G_SETTILESIZE) {
        scroll = (command->words.w0 >> 12) & 0xFFF;
        foundScroll = true;
      }
    }
    assert(foundScroll);
    if (tick) assert((scroll + 128 - previousScroll) % 128 == 1);
    previousScroll = scroll;
  }
  Reset();

  // Seasons are intrinsic weather in the common, shop and overhead routes,
  // even with optional effects disabled and authored season/rod assets present.
  const std::array<CustomDrawFunc, 4> seasons = {
      Randomizer_DrawSeasonSpring, Randomizer_DrawSeasonSummer,
      Randomizer_DrawSeasonAutumn, Randomizer_DrawSeasonWinter};
  const char *seasonSlugs[] = {"season_spring", "season_summer", "season_autumn",
                              "season_winter"};
  for (size_t season = 0; season < seasons.size(); ++season) {
    for (int assets = 0; assets < 3; ++assets) {
      for (int route = 0; route < 3; ++route) {
        Reset();
        entry = {};
        entry.drawFunc = seasons[season];
        alt = assets == 2;
        if (assets) {
          const std::string prefix = assets == 2 ? "alt/" : "";
          files.insert(prefix + "__OTR__objects/nei_gi_redesign/" +
                       seasonSlugs[season] + "/gi_dl");
          files.insert(prefix +
                       "__OTR__objects/nei_gi_redesign/rod_of_seasons/gi_dl");
        }
        if (route == 0) {
          GetItemEntry_Draw(&play, entry);
        } else if (route == 1) {
          EnGirlA shop{};
          shop.actor.params = SI_RANDOMIZED_ITEM;
          shopEntry = entry;
          EnGirlA_Draw(&shop.actor, &play);
        } else {
          Player seasonalPlayer{}; seasonalPlayer.giObjectSegment=reinterpret_cast<void*>(uintptr_t(0x80000000));
          Vec3f reference{};
          seasonalPlayer.getItemEntry = entry;
          Player_DrawGetItemImpl(&play, &seasonalPlayer, &reference, 1);
        }
        assert(!arena.empty() && gfx.polyXlu.p > xlu);
        assert(Drawn().empty() && fallback == 0);
        assert(stack.empty() && interpolation == 0 && loads == 0);
      }
    }
  }
  Reset();
  entry = {};

  // A missing model retains the original draw and restores its matrix edits.
  entry.drawFunc = Randomizer_DrawWhip;
  assert(NeiGi_Draw(&play, &entry));
  assert(fallback == 1 && allocations == 0 && matrix == 1 && stack.empty());

  Reset();
  files.insert("__OTR__objects/nei_gi_redesign/whip/gi_dl");
  assert(NeiGi_Draw(&play, &entry));
  assert(fallback == 0 && allocations == 1 && Drawn().size() == 1);
  assert(gfx.polyXlu.p ==
         xlu); // Disabled effects emit no translucent commands.
  assert(matrix == 1 && stack.empty() && interpolation == 0);
  assert(loads ==
         0); // Deferred paths must not evict/reload Alt resources per draw.
  enabled = 1;
  assert(NeiGi_Draw(&play, &entry));
  assert(allocations > 2 && allocations <= 6 && gfx.polyXlu.p > xlu);
  assert(matrix == 1 && stack.empty() && interpolation == 0);

  // Both spell passes are required; otherwise draw the intact original.
  Reset();
  entry.drawFunc = Randomizer_DrawDemiseDestruction;
  files.insert("__OTR__objects/nei_gi_redesign/demise_destruction/gi_dl");
  assert(NeiGi_Draw(&play, &entry));
  assert(fallback == 1 && Drawn().empty());
  files.insert("__OTR__objects/nei_gi_redesign/demise_destruction/gi_xlu_dl");
  assert(NeiGi_Draw(&play, &entry));
  assert(fallback == 1 && Drawn().size() == 2 && allocations == 6);
  assert(arena.size() == 3 &&
         arena.front().size() > 30); // Intrinsic energy + facet sheen, shimmer OFF.
  assert(Drawn()[1].ends_with("/gi_xlu_dl") && gfx.polyXlu.p > xlu);
  assert(matrix == 1 && stack.empty() && interpolation == 0);

  // A missing spell pass must never expose half a replacement or apply its
  // shelf lift to the legacy fallback, in either resource namespace.
  for (const auto &[draw, slug] : std::vector<std::pair<CustomDrawFunc, const char *>>{
           {Randomizer_DrawHyliaGrace, "hylia_grace"},
           {Randomizer_DrawZonaiPermafrost, "zonai_permafrost"},
           {Randomizer_DrawDemiseDestruction, "demise_destruction"}}) {
    for (bool altOnly : {false, true}) {
      for (int passes = 0; passes < 4; ++passes) {
        Reset();
        entry.drawFunc = draw;
        alt = altOnly;
        const auto prefix = std::string(altOnly ? "alt/" : "") +
                            "__OTR__objects/nei_gi_redesign/" + slug;
        if (passes & 1)
          files.insert(prefix + "/gi_dl");
        if (passes & 2)
          files.insert(prefix + "/gi_xlu_dl");
        matrix = .25f;
        matrixY = 6;
        assert(NeiGi_DrawShop(&play, &entry));
        assert(fallback == (passes == 3 ? 0 : 1));
        assert(Drawn().size() == (passes == 3 ? 2 : 0));
        assert(matrix == .25f && matrixY == 6 && stack.empty() && loads == 0);
        if (passes != 3)
          assert(submitted.empty());
      }
    }
  }

  Reset();
  entry.drawFunc = Randomizer_DrawFireRod;
  files.insert("__OTR__objects/nei_gi_redesign/fire_rod/gi_dl");
  assert(NeiGi_Draw(&play, &entry));
  assert(allocations == 3 && arena.size() == 2 && arena.front().size() > 30);
  bool texture = false;
  for (Gfx *cmd = xlu; cmd != gfx.polyXlu.p; ++cmd) {
    if (((cmd->words.w0 >> 24) & 255) == G_SETTIMG) {
      texture = true;
      assert(((cmd->words.w0 >> 21) & 7) == G_IM_FMT_I);
      assert(cmd->words.w1 ==
             reinterpret_cast<uintptr_t>(
                 NeiGi::OrbTexture(NeiGi::Kind::Fire, play.gameplayFrames)
                     .data()));
    }
  }
  assert(texture); // Dense energy exists even with the checkbox disabled.
  enabled = 1;
  assert(NeiGi_Draw(&play, &entry));
  assert(allocations == 7 &&
         arena.size() == 5); // Orb + accents + independent shimmer.
  assert(matrix == 1 && stack.empty() && interpolation == 0 && loads == 0);

  Reset();
  alt = 1;
  entry.drawFunc = Randomizer_DrawWhip;
  files.insert("alt/__OTR__objects/nei_gi_redesign/whip/gi_dl");
  assert(NeiGi_Draw(&play, &entry));
  assert(fallback == 0 && Drawn().size() == 1 && loads == 0);

  // Exercise the actual overhead animation entry point, not a surrogate
  // dispatch.
  Reset();
  Player player{}; player.giObjectSegment=reinterpret_cast<void*>(uintptr_t(0x80000000));
  Vec3f ref{};
  player.getItemEntry.drawFunc = Randomizer_DrawWhip;
  files.insert("__OTR__objects/nei_gi_redesign/whip/gi_dl");
  Player_DrawGetItemImpl(&play, &player, &ref, 1);
  assert(fallback == 0 && Drawn().size() == 1);
  player.getItemEntry.drawFunc = nullptr;
  Player_DrawGetItemImpl(&play, &player, &ref, 1);
  assert(vanilla == 1);
  player.getItemEntry.modIndex = MOD_RANDOMIZER;
  player.getItemEntry.getItemId = RG_ICE_TRAP;
  Player_DrawGetItemImpl(&play, &player, &ref, 1);
  assert(trap == 1 && vanilla == 1);
  player.getItemEntry.getItemId = RG_TRIFORCE_PIECE;
  Player_DrawGetItemImpl(&play, &player, &ref, 1);
  assert(triforce == 1 && vanilla == 1);

  // Both item-table identities use the approved feather in every real draw
  // route. This must work for stock Roc's Feather, without plando substitutions.
  const std::vector<std::string> featherPaths = {
      "__OTR__objects/nei_gi_redesign/rocs_feather/gi_dl"};
  for (const auto draw : {Randomizer_DrawRocsFeatherSkijer,
                          Randomizer_DrawRocsFeather}) {
    for (int route = 0; route < 3; ++route) {
      for (bool altOnly : {false, true}) {
        Reset();
        entry = {};
        entry.drawFunc = draw;
        alt = altOnly;
        files.insert((altOnly ? "alt/" : "") + featherPaths.front());
        if (route == 0) {
          GetItemEntry_Draw(&play, entry);
        } else if (route == 1) {
          EnGirlA shop{};
          shop.actor.params = SI_RANDOMIZED_ITEM;
          shopEntry = entry;
          EnGirlA_Draw(&shop.actor, &play);
        } else {
          player = {}; player.giObjectSegment=reinterpret_cast<void*>(uintptr_t(0x80000000));
          player.getItemEntry = entry;
          Player_DrawGetItemImpl(&play, &player, &ref, 1);
        }
        assert(Drawn() == featherPaths && fallback == 0);
        assert(stack.empty() && interpolation == 0 && loads == 0);
      }
    }
    Reset();
    entry.drawFunc = draw;
    assert(NeiGi_DrawShop(&play, &entry));
    assert(fallback == 1 && Drawn().empty() && matrix == 1 && matrixY == 0);
    std::cout << (draw == Randomizer_DrawRocsFeather ? "Stock Roc's Feather" : "NEI Progressive Roc")
              << ": common/shop/overhead base+Alt use approved feather; missing resource falls back\n";
  }

  // Newly authored models reach every GI caller, with complete-resource base
  // and Alt fallback. Per-skill Somaria shares red geometry; other cane modes
  // retain their own original callbacks and presentation identities.
  for (const auto &[draw, slug] : std::vector<std::pair<CustomDrawFunc, const char *>>{
           {Randomizer_DrawSpinner, "spinner"},
           {Randomizer_DrawCaneOfSomaria, "cane_of_somaria"},
           {Randomizer_DrawCaneSomariaUpgrade, "cane_of_somaria"},
           {Randomizer_DrawMinishCap, "minish_cap"},
           {Randomizer_DrawRocsCape, "rocs_cape"}}) {
    const auto path = std::string("__OTR__objects/nei_gi_redesign/") + slug + "/gi_dl";
    for (int route = 0; route < 3; ++route) {
      for (bool altOnly : {false, true}) {
        Reset();
        entry = {};
        entry.drawFunc = draw;
        alt = altOnly;
        files.insert((altOnly ? "alt/" : "") + path);
        if (route == 0) {
          GetItemEntry_Draw(&play, entry);
        } else if (route == 1) {
          EnGirlA shop{};
          shop.actor.params = SI_RANDOMIZED_ITEM;
          shopEntry = entry;
          EnGirlA_Draw(&shop.actor, &play);
        } else {
          player = {}; player.giObjectSegment=reinterpret_cast<void*>(uintptr_t(0x80000000));
          player.getItemEntry = entry;
          Player_DrawGetItemImpl(&play, &player, &ref, 1);
        }
        assert(Drawn() == std::vector<std::string>{path} && fallback == 0);
        assert(flameColors.size() == (draw == Randomizer_DrawCaneSomariaUpgrade ? 1 : 0));
        if (!flameColors.empty())
          assert((flameColors.front() == std::array<unsigned, 3>{255, 60, 60}));
        assert(stack.empty() && interpolation == 0 && loads == 0);
      }
    }
    Reset();
    entry.drawFunc = draw;
    files.insert("alt/" + path); // Disabled Alt cannot hide the legacy model.
    assert(NeiGi_DrawShop(&play, &entry));
    assert(fallback == 1 && Drawn().empty() && matrix == 1 && matrixY == 0);
  }
  for (auto draw : {Randomizer_DrawCanePacci, Randomizer_DrawCanePacciUpgrade,
                   Randomizer_DrawCanePacciUltrahand}) {
    Reset();
    entry.drawFunc = draw;
    files.insert("__OTR__objects/nei_gi_redesign/cane_of_somaria/gi_dl");
    enabled = true;
    assert(NeiGi_Draw(&play, &entry));
    assert(arena.size() == 1 && fallback == 1);
    Reset(); enabled = true;
    assert(NeiGi_DrawShop(&play, &entry));
    assert(arena.size() == 1 && fallback == 1);
    Reset(); enabled = true;
    GetItemEntry_Draw(&play, entry);
    assert(fallback == 1 && Drawn().empty());
  }

  // Every authored equipment/item uses the original NEI shimmer sampler.
  // Verify actual packed submissions through common, shop and acquisition
  // entry points; intrinsic energy and both mesh passes must survive toggling.
  size_t covered = 0;
  for (const auto &item : kPresentations) {
    if (!item.opaque) continue;
    for (bool altOnly : {false, true}) {
      for (int route = 0; route < 3; ++route) {
        std::vector<std::vector<Vtx>> offMeshes;
        std::vector<std::string> offPaths;
        for (bool effects : {false, true}) {
          Reset();
          enabled = effects;
          alt = altOnly;
          entry = {};
          entry.drawFunc = item.draw;
          entry.drawItemId = item.identity;
          entry.gid = item.nativeGid;
          const std::string prefix = altOnly ? "alt/" : "";
          files.insert(prefix + item.opaque);
          if (item.translucent) files.insert(prefix + item.translucent);
          if (route == 0) GetItemEntry_Draw(&play, entry);
          else if (route == 1) {
            EnGirlA shop{};
            shop.actor.params = SI_RANDOMIZED_ITEM;
            shopEntry = entry;
            EnGirlA_Draw(&shop.actor, &play);
          } else {
            player = {}; player.giObjectSegment=reinterpret_cast<void*>(uintptr_t(0x80000000));
            player.getItemEntry = entry;
            Player_DrawGetItemImpl(&play, &player, &ref, item.draw ? 1 : item.nativeGid + 1);
          }
          if (fallback || vanilla) std::cerr << "coverage " << item.opaque << " route=" << route << " alt=" << altOnly << " effects=" << effects << " fallback=" << fallback << " vanilla=" << vanilla << "\n";
          assert(fallback == 0 && vanilla == 0);
          assert(stack.empty() && interpolation == 0 && loads == 0);
          if (!effects) {
            offMeshes = arena;
            offPaths = Drawn();
          } else {
            assert(Drawn() == offPaths);
            assert(arena.size() == offMeshes.size() + (item.alwaysShimmer ? 0 : 1));
            auto drawnMeshes = arena;
            // Render the shared sampler alone to compare its packed vertices,
            // including alpha/UV/color. Camera and frame are the same.
            arena.clear();
            NeiGi_DrawMesh(&play, NeiGi::SampleShimmer(play.gameplayFrames, true,
                               NeiGi_CameraBasis(&play), PresentationShimmer(item)));
            assert(arena.size() == 1);
            const auto &want = arena.front();
            size_t matching = 0;
            for (const auto &got : drawnMeshes)
              if (got.size() == want.size() &&
                  !std::memcmp(got.data(), want.data(), want.size() * sizeof(Vtx))) ++matching;
            assert(matching == 1); // Exactly one shared shimmer, never a duplicate.
          }
        }
      }
    }
    ++covered;
  }
  std::cout << "PASS all " << covered
            << " authored bindings: shared shimmer, base/Alt, common/shop/acquisition, toggle and no duplicates\n";

  // Selected swords keep their original drawer AND the shared shimmer. The
  // callback deliberately changes the model matrix to catch a misplaced overlay.
  Reset();
  alt = 1;
  entry = {};
  entry.drawFunc = Randomizer_DrawMasterSword;
  files.insert("__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL");
  assert(NeiGi_Draw(&play, &entry));
  assert(fallback == 0 && Drawn() == std::vector<std::string>{"__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL"} && arena.size() == 2);
  assert(matrix == 1 && stack.empty() && interpolation == 0);

  Reset(); alt=1; entry={}; entry.drawFunc=Randomizer_DrawTrueMasterSword;
  files.insert("__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL");
  assert(NeiGi_Draw(&play,&entry));
  assert(Drawn()==std::vector<std::string>{"__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL"});
  assert((flameColors==std::vector<std::array<unsigned,3>>{{120,180,255}}));
  assert(arena.size()==2 && fallback==0 && stack.empty() && matrix==1);

  // Plain native swords and each concrete callback use their own GI. Selected
  // standalone sword packs and the protected Din GI retain the existing route.
  for (const auto test : {std::pair{GID_SWORD_KOKIRI,"kokiri_sword"},
                         std::pair{GID_SWORD_BGS,"biggoron_sword"}}) {
    Reset(); entry={}; entry.gid=test.first;
    assert(NeiGi_Draw(&play,&entry));
#ifdef COMBO_BUILD
    assert(vanilla == 0 && arena.empty()); // Absent shipped data cannot borrow a mod-capable fallback.
#else
    assert(vanilla == 1 && arena.size() == 2);
#endif
    Reset(); entry={}; entry.gid=test.first;
    const std::string path=std::string("__OTR__objects/nei_gi_redesign/")+test.second+"/gi_dl";
    files.insert(path); assert(NeiGi_Draw(&play,&entry));
    assert(Drawn()==std::vector<std::string>{path} && !arena.empty() && stack.empty());
    const char* selected=test.first==GID_SWORD_KOKIRI ?
      "__OTR__alt/objects/object_custom_equip/gCustomKokiriSwordDL" :
      "__OTR__alt/objects/object_custom_equip/gCustomLongswordDL";
    files.insert(selected); alt=1;
    const int nativeBefore = vanilla;
    const size_t shimmerBefore = arena.size();
    assert(NeiGi_Draw(&play,&entry));
    assert(vanilla == nativeBefore && Drawn().back() == selected && arena.size() == shimmerBefore + 2);
    files.erase(selected);
    const char* fire=test.first==GID_SWORD_KOKIRI ?
      "__OTR__objects/din_fire_sword/progressive/child/SwordDL" :
      "__OTR__objects/din_fire_sword/progressive/bgs/SwordDL";
    files.insert(fire); dinSword=1;
    assert(NeiGi_Draw(&play,&entry));
    assert(vanilla == nativeBefore && Drawn().back() == fire && arena.size() == shimmerBefore + 4);
    dinSword=0; assert(NeiGi_Draw(&play,&entry));
  }
  // A selected mod can also replace a redesigned GI pass itself. Exact path
  // ownership does not make its arbitrary mesh inherit our authored fit/FX.
  for(const auto& model:kPresentations) {
    if(!model.opaque)continue;
    for(bool useAlt:{false,true}) for(int effects:{0,1}) for(int pass:{0,1}) {
      if(pass && !model.translucent)continue;
      Reset(); alt=useAlt; enabled=effects;
#ifdef COMBO_BUILD
      ownerAlt=useAlt;
#endif
      files.insert(model.opaque); if(model.translucent)files.insert(model.translucent);
      const char* overridden=pass?model.translucent:model.opaque;
      if(useAlt)files.insert(std::string("alt/")+overridden);
      modFiles.insert({"oot",(useAlt?"alt/":"")+std::string(overridden)});
      GetItemEntry modEntry{}; modEntry.drawFunc=model.draw; modEntry.drawItemId=model.identity;modEntry.gid=model.nativeGid;
      assert(NeiGi_Draw(&play,&modEntry));
      const float expectedSize=NeiGi::IsSword(model.effect)?1.15f:1.f;
      const float expectedLift=NeiGi::IsSword(model.effect)?.3f:0.f;
#ifdef COMBO_BUILD
      if(!useAlt && NeiGi::IsSword(model.effect))
        assert(Drawn(true)==std::vector<std::string>{std::string("__OTR__@oot-gi-base:")+(model.opaque+7)});
      else
#endif
      assert(!submitted.empty() && std::abs(submitted.front().first-model.scale*expectedSize)<.00001f &&
             std::abs(submitted.front().second-expectedLift)<.0001f);
      assert(arena.size()==size_t(model.alwaysShimmer||effects)+NeiGi::IsSword(model.effect) && "selected sword lost its intrinsic particles");
      assert(flameColors.empty() && fallback==0 && vanilla==0 && stack.empty() && matrix==1 && matrixY==0);
#ifdef COMBO_BUILD
      CwItemDrawInfo modInfo{}; assert(NeiGi_DescribeEntry(&modEntry,&modInfo));
      assert(modInfo.drawKind==CW_DRAW_KIND_CUSTOM_GI && modInfo.neiEffect==0 && modInfo.neiShimmer==int(PresentationShimmer(model))+1);
      assert(!modInfo.neiSomariaUpgrade && !modInfo.primColorOpa[3] && !modInfo.primColorXlu[3]);
      const std::string slug=std::string(model.opaque).substr(std::strlen("__OTR__objects/nei_gi_redesign/"));
      CwItemDrawInfo ownerInfo{}; assert(OOT_GetNeiGiDrawInfo(slug.substr(0,slug.find('/')).c_str(),&ownerInfo));
      assert(ownerInfo.drawKind==CW_DRAW_KIND_CUSTOM_GI && ownerInfo.neiEffect==0);
      submitted.clear();arena.clear();gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;
      MM_DrawNeiGi(ownerInfo);
      assert(std::abs(submitted.front().first-model.scale*expectedSize)<.00001f &&
             std::abs(submitted.front().second-expectedLift)<.0001f);
      assert(arena.size()==size_t(model.alwaysShimmer||effects)+NeiGi::IsSword(model.effect));
      assert(stack.empty() && matrix==1 && matrixY==0);
#endif
    }
  }
#ifdef COMBO_BUILD
  ownerAlt=false;
#endif
  std::cout<<"PASS every redesigned GI override: both OPA/XLU selections, base/Alt, native/owner/MM, independent identity shimmer and no authored fit/energy\n";

  // A Poe actor replacement is not a replacement for the authored Lantern
  // item. Both passes must survive generic actor mods, including MM-first
  // selection with independent donor/host Alt settings.
  for(int ootAlt:{0,1}) for(int mmAlt:{0,1}) for(int effects:{0,1})
  for(const char* modOwner:{"oot","mm"}) for(bool altPack:{false,true}) {
    Reset();alt=ootAlt;enabled=effects;
#ifdef COMBO_BUILD
    ownerAlt=ootAlt;
#endif
    entry={};entry.drawFunc=Randomizer_DrawLantern;
    const std::vector<std::string> authored={
      "__OTR__objects/nei_gi_redesign/lantern/gi_dl",
      "__OTR__objects/nei_gi_redesign/lantern/gi_xlu_dl"};
    for(const auto& path:authored)files.insert(path);
    const std::string poe="__OTR__objects/object_poh/gPoeLanternDL";
    files.insert(poe);
    const auto selectedPoe=altPack?"alt/"+poe:poe;
    files.insert(selectedPoe);modFiles.insert({modOwner,selectedPoe});
    assert(NeiGi_Draw(&play,&entry) && fallback==0 && Drawn()==authored &&
           "a generic Poe actor mod must not suppress the authored Lantern GI");
    gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;arena.clear();submitted.clear();
    assert(NeiGi_DrawShop(&play,&entry) && fallback==0 && Drawn()==authored);
    assert(stack.empty() && matrix==1 && matrixY==0 && loads==0);
#ifdef COMBO_BUILD
    alt=mmAlt;
    CwItemDrawInfo info{};
    assert(OOT_GetNeiGiDrawInfoForAssets("lantern",mmAlt,&info)==1 &&
           info.drawKind==CW_DRAW_KIND_NEI_GI && info.dlistCount==2 && info.xluStartIndex==1);
    assert(std::string(info.dlists[0])==authored[0] && std::string(info.dlists[1])==authored[1]);
    assert(MM_DescribeNeiGi(RI_OOT_NEI_LANTERN,&info) &&
           info.drawKind==CW_DRAW_KIND_NEI_GI && info.dlistCount==2 && info.xluStartIndex==1);
    const std::vector<std::string> routed={
      "__OTR__@oot:objects/nei_gi_redesign/lantern/gi_dl",
      "__OTR__@oot:objects/nei_gi_redesign/lantern/gi_xlu_dl"};
    assert(std::string(info.dlists[0])==routed[0] && std::string(info.dlists[1])==routed[1]);
    for(bool shop:{false,true}) {
      gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;arena.clear();submitted.clear();
      assert(MM_TryDrawNeiGi(RI_OOT_NEI_LANTERN,shop,shop?0:1) && Drawn()==routed);
      assert(arena.size()==size_t(effects) && stack.empty() && matrix==1 && matrixY==0 && loads==0);
    }
#endif
    // A missing metal or glass pass still preserves the established fallback;
    // the selection correction must not emit a partial authored model.
    for(const auto& missing:authored) {
      files.erase(missing);gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;fallback=0;arena.clear();
      assert(NeiGi_Draw(&play,&entry) && fallback==1 && Drawn().empty());
#ifdef COMBO_BUILD
      assert(OOT_GetNeiGiDrawInfoForAssets("lantern",mmAlt,&info)==0);
#endif
      files.insert(missing);
    }
  }
#ifdef COMBO_BUILD
  ownerAlt=false;
#endif
  std::cout<<"PASS authored Lantern ignores generic Poe mods: both passes, native/shop/MM, independent Alt, shimmer and missing-pass fallback\n";

  // A mod at an established model resource outranks authored GI geometry in
  // native common/shop selection and the owner-pinned foreign descriptor.
  struct ModBinding {CustomDrawFunc draw;const char* slug;const char* game;const char* legacy;int identity=0;};
  const ModBinding modBindings[] = {
      {Randomizer_DrawRocsFeatherSkijer, "rocs_feather", "oot",
       "__OTR__objects/object_nei_rocs_feather/rocs_feather_dl"},
      {Randomizer_DrawRocsFeather, "rocs_feather", "oot",
       "__OTR__objects/object_rocs_feather/gGiRocsFeatherDL"},
      {Randomizer_DrawRocsCape, "rocs_cape", "oot",
       "__OTR__objects/object_nei_rocs_cape/rocs_cape_mesh_dl"},
      {Randomizer_DrawWhip, "whip", "oot",
       "__OTR__objects/object_nei_whip/whip_give_opaque_dl"},
      {Randomizer_DrawSpinner, "spinner", "oot",
       "__OTR__objects/object_nei_spinner/n0b0_opaque_dl"},
      {Randomizer_DrawDekuLeaf, "deku_leaf", "oot",
       "__OTR__objects/object_nei_deku_leaf/g_dekuleaf_dl"},
      {Randomizer_DrawSwitchHook, "switch_hook", "oot",
       "__OTR__objects/object_nei_switchhook/gSwitchHookGiveDL"},
      {Randomizer_DrawMogmaMitts, "mogma_mitts", "oot",
       "__OTR__objects/object_nei_mogma_mitts/gMogmaMittsGiveDL"},
      {Randomizer_DrawGustJar, "gust_jar", "oot",
       "__OTR__objects/object_nei_gust_jar/jar_model_dl"},
      {Randomizer_DrawTimeGate, "time_gate", "oot",
       "__OTR__objects/object_nei_time_gate/g_timegate_dl"},
      {Randomizer_DrawMinishCap, "minish_cap", "oot",
       "__OTR__objects/object_nei_minish_cap/Cylinder_opaque_dl"},
      {Randomizer_DrawMarioMask, "mario_mask", "oot",
       "__OTR__objects/object_nei_mario_mask/g_mario_mask_dl"},
      {Randomizer_DrawExtDivineShield, "divine_shield", "oot",
       "__OTR__objects/object_nei_divine_shield/g_divine_shield_dl"},
      {Randomizer_DrawExtSheikahShield, "sheikah_shield", "oot",
       "__OTR__objects/object_nei_kite_shield/g_kite_shield_dl"},
      {Randomizer_DrawExtMagicCape, "magic_cape", "oot",
       "__OTR__objects/object_nei_magic_cape/gNeiMagicCapeWaveDL"},
      {Randomizer_DrawExtSpiritBreastplate, "spirit_breastplate", "oot",
       "__OTR__objects/object_gi_clothes/gGiTunicCollarDL"},
      {Randomizer_DrawExtSagesTunic, "sages_tunic", "oot",
       "__OTR__objects/object_gi_clothes/gGiTunicDL"},
      {Randomizer_DrawExtChampionsTunic, "champions_tunic", "oot",
       "__OTR__objects/object_gi_clothes/gGiTunicDL"},
      {Randomizer_DrawExtPegasusAnklet, "pegasus_anklet", "oot",
       "__OTR__objects/object_gi_hoverboots/gGiHoverBootsDL"},
      {Randomizer_DrawExtTrident, "trident", "oot",
       "__OTR__objects/object_gnd/gPhantomGanonSkelLimbsLimb_00C610DL_009298"},
      {Randomizer_DrawExtClimbBoots, "climb_boots", "oot",
       "__OTR__objects/object_gi_boots_2/gGiIronBootsRivetsDL"},
      {Randomizer_DrawExtRocBoots, "roc_boots", "oot",
       "__OTR__objects/object_gi_hoverboots/gGiHoverBootsDL"},
      {Randomizer_DrawExtCaneOfByrna, "cane_of_byrna", "oot",
       "__OTR__objects/object_somaria/g_byrna_cane_give_dl"},
      {Randomizer_DrawExtFourSword, "four_sword", "oot",
       "__OTR__objects/object_nei_four_sword/gNeiFourSwordHiltDL"},
      {Randomizer_DrawCaneOfSomaria, "cane_of_somaria", "oot",
       "__OTR__objects/object_somaria/g_somaria_cane_give_dl"},
      {Randomizer_DrawExtPendantOfMemories, "pendant_of_memories", "mm",
       "__OTR__objects/object_gi_reserve_c_01/gGiPendantOfMemoriesDL"},
      {Randomizer_DrawProgressiveKokiriSword, "kokiri_sword", "oot",
       "__OTR__objects/object_gi_sword_1/gGiKokiriSwordDL"},
      {Randomizer_DrawMasterSword, "master_sword", "oot",
       "__OTR__objects/object_toki_objects/object_toki_objects_DL_001BD0"},
      {Randomizer_DrawTrueMasterSword, "true_master_sword", "oot",
       "__OTR__objects/object_toki_objects/object_toki_objects_DL_001BD0"},
      {Randomizer_DrawProgressiveBGS, "biggoron_sword", "oot",
       "__OTR__objects/object_gi_longsword/gGiBiggoronSwordDL"},
      {Randomizer_DrawRazorSword, "razor_sword", "mm",
       "__OTR__objects/object_gi_sword_2/gGiRazorSwordDL"},
      {Randomizer_DrawGildedSword, "gilded_sword", "mm",
       "__OTR__objects/object_gi_sword_3/gGiGildedSwordEmptyDL"},
      {Randomizer_DrawGreatFairySword, "great_fairy_sword", "mm",
       "__OTR__objects/object_gi_sword_4/gGiGreatFairysSwordHiltEmblemDL"},
      {Randomizer_DrawNeiSheikahSlate, "sheikah_slate", "oot",
       "__OTR__objects/object_nei_sheikah_slate/gNeiSheikahSlateDL"},
      {Randomizer_DrawSlateRuneMasterCycle, "slate_master_cycle", "oot",
       "__OTR__objects/object_nei_sheikah_slate/gNeiSheikahSlateDL"},
      {Randomizer_DrawSlateRuneStasis, "slate_stasis", "oot",
       "__OTR__objects/object_nei_sheikah_slate/gNeiSheikahSlateDL"},
      {Randomizer_DrawSlateRuneCryonis, "slate_cryonis", "oot",
       "__OTR__objects/object_nei_sheikah_slate/gNeiSheikahSlateDL"},
      {Randomizer_DrawSlateRuneSensor, "slate_sensor", "oot",
       "__OTR__objects/object_nei_sheikah_slate/gNeiSheikahSlateDL"},
      {Randomizer_DrawSlateRuneBomb, "slate_bomb", "oot",
       "__OTR__objects/object_nei_sheikah_slate/gNeiSheikahSlateDL"},
      {Randomizer_DrawNeiRodOfSeasons, "rod_of_seasons", "oot",
       "__OTR__objects/object_nei_rod_of_seasons/gNeiRodOfSeasonsDL"},
      {Randomizer_DrawNeiPhantomHourglass, "phantom_hourglass", "oot",
       "__OTR__objects/object_nei_phantom_hourglass/gNeiPhantomHourglassDL"},
      {Randomizer_DrawNeiShadowCrystal, "shadow_crystal", "oot",
       "__OTR__objects/object_nei_shadow_crystal/gNeiShadowCrystalDL"},
  };
  // A partial legacy tunic pack must not turn a fully available authored GI
  // into an empty native draw plus its independent shimmer. Test each pass,
  // invalid types/empty models, both owners and both asset settings.
  for(const auto& test:modBindings) {
    if(std::strcmp(test.slug,"spirit_breastplate") && std::strcmp(test.slug,"sages_tunic") &&
       std::strcmp(test.slug,"champions_tunic"))continue;
    const char* legacy[] = {"__OTR__objects/object_gi_clothes/gGiTunicCollarDL",
                           "__OTR__objects/object_gi_clothes/gGiTunicDL"};
    for(int selectedAlt:{0,1}) for(bool altPack:{false,true}) for(int missing:{0,1}) for(bool invalid:{false,true}) {
      if(altPack && !selectedAlt)continue;
      Reset();alt=selectedAlt;enabled=1;
#ifdef COMBO_BUILD
      ownerAlt=selectedAlt;
#endif
      entry={};entry.drawFunc=test.draw;
      const auto authored=std::string("__OTR__objects/nei_gi_redesign/")+test.slug+"/gi_dl";
      files.insert(authored);
      const std::string selected[]={(altPack?"alt/":"")+std::string(legacy[0]),
                                    (altPack?"alt/":"")+std::string(legacy[1])};
      for(int pass=0;pass<2;++pass)
        if(pass!=missing || invalid)files.insert(selected[pass]);
      modFiles.insert({"oot",selected[1-missing]});
      if(invalid)invalidGiModels.insert({"oot",selected[missing]});
      assert(NeiGi_Draw(&play,&entry) && fallback==0 && Drawn()==std::vector<std::string>{authored} &&
             "incomplete legacy tunic geometry must retain the authored model, not only shimmer");
      assert(!arena.empty() && stack.empty());
#ifdef COMBO_BUILD
      CwItemDrawInfo info{};
      assert(OOT_GetNeiGiDrawInfoForAssets(test.slug,selectedAlt,&info)==1 && info.dlistCount==1 &&
             std::string(info.dlists[0])==authored && info.neiShimmer>int(NeiGi::Kind::GiantsKnife)+1);
      // An MM replacement still needs the donor pass that its native drawer
      // uses. An unusable donor pass cannot be hidden by a host mod marker.
      invalidGiModels.clear();invalidGiModels.insert({"oot",selected[missing]});
      files.insert(selected[missing]);modFiles.clear();modFiles.insert({"mm",selected[1-missing]});
      assert(OOT_GetNeiGiDrawInfoForAssets(test.slug,selectedAlt,&info)==1 &&
             std::string(info.dlists[0])==authored);
      const auto mmItem=!std::strcmp(test.slug,"spirit_breastplate") ? RI_OOT_EXT_SPIRIT_BREASTPLATE :
                        !std::strcmp(test.slug,"sages_tunic") ? RI_OOT_EXT_WATER_DRAGON_SCALE : RI_OOT_EXT_CHAMPIONS_TUNIC;
      gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;arena.clear();
      const auto routed="__OTR__@oot:"+authored.substr(7);
      assert(MM_TryDrawNeiGi(mmItem,false,1) && Drawn()==std::vector<std::string>{routed} &&
             !arena.empty() && stack.empty() && "MM receipts must draw the recovered model and its shimmer together");
      // A valid donor copy cannot rescue an unusable selected host override:
      // the native drawer would use the MM replacement for this pass.
      invalidGiModels.clear();invalidGiModels.insert({"mm",selected[missing]});
      modFiles.clear();modFiles.insert({"mm",selected[missing]});
      assert(OOT_GetNeiGiDrawInfoForAssets(test.slug,selectedAlt,&info)==1 &&
             std::string(info.dlists[0])==authored);
      gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;arena.clear();
      assert(MM_TryDrawNeiGi(mmItem,false,1) && Drawn()==std::vector<std::string>{routed} &&
             !arena.empty() && stack.empty());
#endif
    }
  }
  std::cout<<"PASS incomplete legacy tunic recipes retain all three authored GIs and their shimmer\n";
  // Vanilla clothes replacements are shared by Goron/Zora tunics. Even a
  // complete pack at those roots is not a replacement for any of these three
  // distinct authored awards. This catches the path the earlier repair left
  // open by rejecting only incomplete generic packs.
  for(const auto& test:modBindings) {
    if(std::strcmp(test.slug,"spirit_breastplate") && std::strcmp(test.slug,"sages_tunic") &&
       std::strcmp(test.slug,"champions_tunic"))continue;
    for(int hostAlt:{0,1}) for(int donorAlt:{0,1}) for(const char* replacementOwner:{"oot","mm"}) {
      Reset();alt=hostAlt;enabled=1;
#ifdef COMBO_BUILD
      ownerAlt=donorAlt;
#else
      if(donorAlt!=hostAlt)continue;
#endif
      entry={};entry.drawFunc=test.draw;
      const auto authored=std::string("__OTR__objects/nei_gi_redesign/")+test.slug+"/gi_dl";
      files.insert(authored);
      for(const auto* legacy:{"__OTR__objects/object_gi_clothes/gGiTunicCollarDL",
                             "__OTR__objects/object_gi_clothes/gGiTunicDL"}) {
        files.insert(legacy);
        files.insert(std::string("alt/")+legacy);
        modFiles.insert({replacementOwner,legacy});
        modFiles.insert({replacementOwner,std::string("alt/")+legacy});
      }
      assert(NeiGi_Draw(&play,&entry) && fallback==0 && Drawn()==std::vector<std::string>{authored} &&
             !arena.empty() && stack.empty() && "complete generic clothes packs must not bypass a distinct authored tunic GI");
      gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;arena.clear();
      assert(NeiGi_DrawShop(&play,&entry) && fallback==0 && Drawn()==std::vector<std::string>{authored});
#ifdef COMBO_BUILD
      CwItemDrawInfo info{};
      assert(OOT_GetNeiGiDrawInfoForAssets(test.slug,hostAlt,&info)==1 && info.drawKind==CW_DRAW_KIND_NEI_GI &&
             std::string(info.dlists[0])==authored);
      const auto mmItem=!std::strcmp(test.slug,"spirit_breastplate")?RI_OOT_EXT_SPIRIT_BREASTPLATE:
                        !std::strcmp(test.slug,"sages_tunic")?RI_OOT_EXT_WATER_DRAGON_SCALE:RI_OOT_EXT_CHAMPIONS_TUNIC;
      gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;arena.clear();
      const auto routed=std::string("__OTR__@oot:")+authored.substr(7);
      assert(MM_TryDrawNeiGi(mmItem,false,1) && Drawn()==std::vector<std::string>{routed} &&
             !arena.empty() && stack.empty());
#endif
    }
  }
  std::cout<<"PASS complete generic clothes packs preserve all three individual tunic GIs\n";
  for(const auto& test:modBindings)for(int selectedAlt:{0,1}){
    Reset();alt=selectedAlt;
#ifdef COMBO_BUILD
    ownerAlt=selectedAlt;
#endif
    entry={};entry.drawFunc=test.draw;entry.drawItemId=test.identity;
    const auto path=std::string("__OTR__objects/nei_gi_redesign/")+test.slug+"/gi_dl";
    files.insert(path);files.insert(test.legacy);
    if(!std::strcmp(test.slug,"spirit_breastplate") || !std::strcmp(test.slug,"sages_tunic") ||
       !std::strcmp(test.slug,"champions_tunic")) {
      files.insert("__OTR__objects/object_gi_clothes/gGiTunicCollarDL");
      files.insert("__OTR__objects/object_gi_clothes/gGiTunicDL");
    }
    const auto* selected=FindPresentation(&entry);assert(selected);
    bool vanillaSword=false;
#ifdef COMBO_BUILD
    vanillaSword=!selectedAlt && NeiGi::IsSword(selected->effect);
#endif
    const bool distinctTunic=!std::strcmp(test.slug,"spirit_breastplate") ||
                             !std::strcmp(test.slug,"sages_tunic") || !std::strcmp(test.slug,"champions_tunic");
    std::vector<std::string> authoredPaths{path};
    if(selected->translucent){files.insert(selected->translucent);authoredPaths.push_back(selected->translucent);}
    if(selectedAlt)files.insert(std::string("alt/")+test.legacy);
    assert(NeiGi_Draw(&play,&entry)&&fallback==0&&Drawn()==authoredPaths);
    modFiles.insert({test.game,selectedAlt?std::string("alt/")+test.legacy:std::string(test.legacy)});
    gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;fallback=0;
    assert(NeiGi_Draw(&play,&entry)&&fallback==((vanillaSword||distinctTunic)?0:1)&&
           Drawn()==((vanillaSword||distinctTunic)?authoredPaths:std::vector<std::string>{}));
    gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;fallback=0;
    assert(NeiGi_DrawShop(&play,&entry)&&fallback==((vanillaSword||distinctTunic)?0:1)&&
           Drawn()==((vanillaSword||distinctTunic)?authoredPaths:std::vector<std::string>{}));
#ifdef COMBO_BUILD
    CwItemDrawInfo info{};assert(bool(OOT_GetNeiGiDrawInfo(test.slug,&info))==distinctTunic);
    modFiles.clear();assert(OOT_GetNeiGiDrawInfo(test.slug,&info));
    if(std::strcmp(test.game,"mm")==0){
      // Imported MM model files can also be replaced in OoT's local archive
      // manager, which TransformMasks_LoadMmDL consults before the donor.
      modFiles.insert({"oot",selectedAlt?std::string("alt/")+test.legacy:std::string(test.legacy)});
      gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;fallback=0;
      assert(NeiGi_Draw(&play,&entry)&&fallback==(vanillaSword?0:1)&&
             Drawn()==(vanillaSword?authoredPaths:std::vector<std::string>{}));
      assert(!OOT_GetNeiGiDrawInfo(test.slug,&info));
      modFiles.clear();assert(OOT_GetNeiGiDrawInfo(test.slug,&info));
    }
    if(std::strcmp(test.game,"oot")==0 && std::strcmp(test.slug,"magic_cape") && std::strcmp(test.slug,"four_sword")){
      // A native MM legacy mod protects its exported descriptor, except for
      // shared clothes packs unrelated to the three distinct authored tunics.
      modFiles.insert({"mm",selectedAlt?std::string("alt/")+test.legacy:std::string(test.legacy)});
      gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;fallback=0;
      assert(NeiGi_Draw(&play,&entry)&&fallback==0&&Drawn()==authoredPaths);
      assert(bool(OOT_GetNeiGiDrawInfo(test.slug,&info))==distinctTunic);
      modFiles.clear();assert(OOT_GetNeiGiDrawInfo(test.slug,&info));
    }
    assert(info.dlistCount==authoredPaths.size()&&std::strcmp(info.dlists[0],path.c_str())==0);
    ownerAlt=false;
#endif
  }
  {
    Reset();
#ifdef COMBO_BUILD
    GetItemEntry room{};
    room.tableId = TABLE_RANDOMIZER;
    room.drawFunc = Randomizer_DrawMmTradeQuest;
    room.drawItemId = RG_MM_ROOM_KEY;
    const char* roomPath = "__OTR__objects/nei_gi_redesign/room_key/gi_dl";
    files.insert(roomPath);
    CwItemDrawInfo roomInfo{};
    assert(NeiGi_DescribeEntry(&room, &roomInfo) == 1 &&
           "Room Key must select its authored GI instead of the generic MM trade callback");
    assert(roomInfo.dlistCount == 1 && !std::strcmp(roomInfo.dlists[0], roomPath));
    assert(NeiGi_Draw(&play, &room) && Drawn() == std::vector<std::string>{roomPath});
    assert(MM_DescribeNeiGi(RI_ROOM_KEY, &roomInfo));
    room.drawItemId = RG_MM_MOONS_TEAR;
    assert(!NeiGi_DescribeEntry(&room, &roomInfo) && "Room Key binding captured another trade item");
    room.drawItemId = RG_MM_ROOM_KEY;
    for (const char* part : {"gGiRoomKeyDL", "gGiRoomKeyEmptyDL"}) {
      Reset(); files.insert(roomPath);
      modFiles.insert({"mm", std::string("__OTR__objects/object_gi_reserve_b_00/") + part});
      assert(NeiGi_Draw(&play, &room) && fallback == 1 && Drawn().empty());
      assert(!MM_DescribeNeiGi(RI_ROOM_KEY, &roomInfo));
    }
    Reset();
    assert(NeiGi_Draw(&play, &room) && fallback==1 && Drawn().empty() &&
           "missing authored Room Key must call its original native GI callback");
#endif
    GetItemEntry optionalCojiro{};
    optionalCojiro.tableId = TABLE_VANILLA;
    optionalCojiro.gid = GID_COJIRO;
    assert(!NeiGi_Draw(&play, &optionalCojiro));
    files.insert("__OTR__objects/nei_gi_redesign/cojiro/gi_dl");
    assert(NeiGi_Draw(&play, &optionalCojiro));
    files.clear();
    assert(!NeiGi_Draw(&play, &optionalCojiro));
  }
#ifdef COMBO_BUILD
  for (const char* part : {"gGiChickenDL", "gGiCojiroColorDL", "gGiChickenEyesDL"}) {
    for (const char* owner : {"oot", "mm"}) for (int selectedAlt : {0, 1}) {
      Reset(); alt = selectedAlt; ownerAlt = selectedAlt;
      const std::string legacy = std::string("__OTR__objects/object_gi_niwatori/") + part;
      files.insert("__OTR__objects/nei_gi_redesign/cojiro/gi_dl");
      files.insert(legacy);
      if (selectedAlt) files.insert("alt/" + legacy);
      CwItemDrawInfo info{};
      assert(OOT_GetNeiGiDrawInfo("cojiro", &info));
      modFiles.insert({owner, selectedAlt ? "alt/" + legacy : legacy});
      assert(!OOT_GetNeiGiDrawInfo("cojiro", &info));
    }
  }
  for(const char* owner:{"oot","mm"})for(int selectedAlt:{0,1}){
    Reset();alt=selectedAlt;ownerAlt=selectedAlt;
    const char* legacy="__OTR__objects/object_gi_sword_1/gGiKokiriSwordDL";
    files.insert("__OTR__objects/nei_gi_redesign/mm_kokiri_sword/gi_dl");files.insert(legacy);
    if(selectedAlt)files.insert(std::string("alt/")+legacy);
    CwItemDrawInfo info{};assert(OOT_GetNeiGiDrawInfo("mm_kokiri_sword",&info));
    modFiles.insert({owner,selectedAlt?std::string("alt/")+legacy:std::string(legacy)});
    assert(!OOT_GetNeiGiDrawInfo("mm_kokiri_sword",&info));
  }
  ownerAlt=false;
#endif
  // The progressive OoT Master callback actually draws a tinted Kokiri
  // placeholder; the real Master callback and foreign recipe use Temple geometry.
  for(int selectedAlt:{0,1}){
    Reset();alt=selectedAlt;
#ifdef COMBO_BUILD
    ownerAlt=selectedAlt;
#endif
    const char* kokiri="__OTR__objects/object_gi_sword_1/gGiKokiriSwordDL";
    const char* authored="__OTR__objects/nei_gi_redesign/master_sword/gi_dl";
    files.insert(kokiri);files.insert(authored);
    if(selectedAlt)files.insert(std::string("alt/")+kokiri);
    modFiles.insert({"oot",selectedAlt?std::string("alt/")+kokiri:std::string(kokiri)});
    entry={};entry.drawFunc=Randomizer_DrawProgressiveMasterSword;
#ifdef COMBO_BUILD
    assert(NeiGi_Draw(&play,&entry)&&fallback==(selectedAlt?1:0)&&
           Drawn()==(selectedAlt?std::vector<std::string>{}:
                    std::vector<std::string>{"__OTR__objects/nei_gi_redesign/master_sword/gi_dl"}));
#else
    assert(NeiGi_Draw(&play,&entry)&&fallback==1&&Drawn().empty());
#endif
    gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;fallback=0;
    entry.drawFunc=Randomizer_DrawMasterSword;
    assert(NeiGi_Draw(&play,&entry)&&fallback==0&&Drawn()==std::vector<std::string>{authored});
#ifdef COMBO_BUILD
    CwItemDrawInfo info{};assert(OOT_GetNeiGiDrawInfo("master_sword",&info));ownerAlt=false;
#endif
  }
  // Wand mods only own held geometry: their legacy GI is an inline stand-in,
  // so keeping a held mod must not turn the new GI back into that stand-in.
  Reset();entry={};entry.drawFunc=Randomizer_DrawElementalWand;entry.drawItemId=RG_WAND_SHADOW_SCEPTER;
  const char* shadowPath="__OTR__objects/nei_gi_redesign/shadow_scepter/gi_dl";
  files.insert(shadowPath);files.insert("__OTR__objects/object_nei_wand_shadow_scepter/gNeiShadowScepterDL");
  modFiles.insert({"oot","__OTR__objects/object_nei_wand_shadow_scepter/gNeiShadowScepterDL"});
  assert(NeiGi_Draw(&play,&entry)&&fallback==0&&Drawn()==std::vector<std::string>{shadowPath});
#ifdef COMBO_BUILD
  CwItemDrawInfo heldOnlyInfo{};assert(OOT_GetNeiGiDrawInfo("shadow_scepter",&heldOnlyInfo));
#endif
  std::cout<<"PASS legacy mod model priority: "<<std::size(modBindings)<<" concrete GI callbacks, base/Alt, native/common/shop, foreign and both imported owners; held-only wand mods retain authored GI\n";
  // Serialized model vertices and resource matrices must clear the shelf by 0.5
  // world units under EnGirlA's real .25 actor scale / 24 local-unit Y offset.
  struct Bounds {
    CustomDrawFunc draw;
    const char *slug;
    const char *name;
    const char *callback;
    std::array<float, 3> minimum, maximum;
    float spinningWidth, drawScale;
    bool translucent;
    int identity = 0;
    int gid = -1;
  };
  const Bounds bounds[] = {
#include "nei_gi_bounds.inc"
  };
  // Front-mounted Slate sigils share the GI's rotation. Camera changes may
  // alter ribbon width, but cannot billboard their ring centerlines. Compare
  // packed effects with the actual serialized model's frontmost relief.
  size_t slateVariants = 0;
  for (const auto& b : bounds) {
    GetItemEntry slateEntry{};
    slateEntry.drawFunc = b.draw;
    const auto* slate = FindPresentation(&slateEntry);
    if (!slate || !NeiGi::IsSlate(slate->effect))
      continue;
    ++slateVariants;
    for (int pitch : {-60, -30, 0, 30, 60}) {
      const float x = pitch * NeiGi::Tau / 360.f;
      for (int yaw = 0; yaw < 360; yaw += 15) {
        const float y = yaw * NeiGi::Tau / 360.f;
        const float sx = std::sin(x), cx = std::cos(x);
        const float sy = std::sin(y), cy = std::cos(y);
        const NeiGi::Basis camera{{cy, 0, sy}, {sx * sy, cx, -sx * cy},
                                  {-cx * sy, sx, cx * cy}};
        for (uint32_t frame : {0u, 45u, 89u}) {
          const auto halo = NeiGi::SampleSpecial(slate->effect, frame, camera);
          const auto faceOn = NeiGi::SampleSpecial(slate->effect, frame);
          assert(halo.count > 0);
          // Vertex zero is the first ribbon's centerline anchor. Its model-local
          // position must not move when only the camera orientation changes.
          assert(halo.vertices[0].p.x == faceOn.vertices[0].p.x);
          assert(halo.vertices[0].p.y == faceOn.vertices[0].p.y);
          assert(halo.vertices[0].p.z == faceOn.vertices[0].p.z);
          for (size_t i = 0; i < halo.count; ++i) {
            const auto& sampled = halo.vertices[i].p;
            const NeiGi::Point p{std::round(sampled.x * 16) / 16,
                                 std::round(sampled.y * 16) / 16,
                                 std::round(sampled.z * 16) / 16};
            assert(p.z + slate->effectCenter.z > b.maximum[2] * b.drawScale + .25f);
            const float radius = std::hypot(p.x, p.y);
            assert(radius > 8.5f && radius < 14.5f);
          }
        }
      }
    }
  }
  assert(slateVariants == 6);
  std::cout << "PASS six Slate sigils: model-attached centerlines and packed vertices clear raised relief across camera poses\n";
  std::ofstream preview;
  if (const auto *path = std::getenv("NEI_SHOP_PREVIEW_EXPORT")) {
    preview.open(path);
    assert(preview.is_open());
    preview << std::setprecision(9)
            << "{\"metadata\":{\"runtime_tested\":false,"
               "\"matrix_convention\":\"row-major, column vector\","
               "\"rotation\":\"normalized to identity by fixture; preview may add GI spin\","
               "\"shop_matrix_includes\":\"GI caller scale, .25 actor scale and +24 local origin (world +6)\","
               "\"nonshop_matrix_includes\":\"GI caller scale\","
               "\"matrix_input\":\"vertices after serialized resource scale_mtx\","
               "\"checkpoint_renderer\":\"already applies resource and caller scales; divide exported matrix scale by draw_scale\","
               "\"counter_world_y\":0,\"actor_scale\":0.25,\"actor_y_offset_local\":24},\"items\":[";
  }
  bool firstPreview = true;
  const auto writeMatrix = [&](float s, float y) {
    preview << "[[" << s << ",0,0,0],[0," << s << ",0," << y
            << "],[0,0," << s << ",0],[0,0,0,1]]";
  };
  const auto writeBounds = [&](const Bounds &b, float s, float y) {
    preview << "{\"min\":[" << s * b.minimum[0] << ',' << y + s * b.minimum[1]
            << ',' << s * b.minimum[2] << "],\"max\":[" << s * b.maximum[0]
            << ',' << y + s * b.maximum[1] << ',' << s * b.maximum[2] << "]}";
  };
  for (const auto &b : bounds) {
    Reset();
    entry = {};
    entry.drawFunc = b.draw;
    entry.drawItemId = b.identity;
    entry.gid = b.gid;
    const auto path = std::string("__OTR__objects/nei_gi_redesign/") + b.slug;
    files.insert(path + "/gi_dl");
    if (b.translucent)
      files.insert(path + "/gi_xlu_dl");
    matrix = .25f;
    matrixY = 6;
    EnGirlA shop{};
    shop.actor.params = SI_RANDOMIZED_ITEM;
    shopEntry = entry;
    EnGirlA_Draw(&shop.actor, &play);
    const auto [scale, y] = submitted.front();
    std::cout << b.slug << ": shelf bottom=" << y + scale * b.minimum[1]
              << ", height=" << scale * (b.maximum[1] - b.minimum[1])
              << ", spinning width=" << scale * b.spinningWidth << '\n'
              << std::flush;
    // Float32 composition at the exact shelf boundary may lose one ULP.
    assert(y + scale * b.minimum[1] >= .5f - .000002f);
    // The approved feather retains its full size; only its shelf lift changes.
    const bool feather = b.draw == Randomizer_DrawRocsFeather || b.draw == Randomizer_DrawRocsFeatherSkijer;
    assert(scale * (b.maximum[1] - b.minimum[1]) <= (feather ? 21.f : 19.f));
    assert(scale * b.spinningWidth <= 19.f);
    if (b.translucent) // The shell uses the same lifted/scaled pose as its core.
      assert(submitted.back() == submitted.front());
    assert(matrix == .25f && matrixY == 6 && stack.empty());
    // The actual common draw keeps its original size outside shops.
    submitted.clear();
    matrix = 1;
    matrixY = 0;
    GetItemEntry_Draw(&play, entry);
    const auto* frameBounds = NeiGi::FindFrameBounds((path + "/gi_dl").c_str());
    assert(frameBounds);
    const auto commonFit = NeiGi::FrameFit(*frameBounds,b.drawScale,false);
    assert(std::abs(submitted.front().first - b.drawScale * commonFit.scale) < .000001f);
    assert(submitted.front().second == commonFit.lift);
    if (preview.is_open()) {
      if (!firstPreview)
        preview << ',';
      firstPreview = false;
      const auto* authored = FindPresentation(&entry);
      assert(authored);
      preview << "{\"slug\":\"" << b.slug << "\",\"name\":\"" << b.name
              << "\",\"callback\":\"Randomizer_Draw" << b.callback
              << "\",\"draw_scale\":" << b.drawScale << ",\"shop_matrix\":";
      writeMatrix(scale, y);
      preview << ",\"nonshop_matrix\":";
      writeMatrix(submitted.front().first, submitted.front().second);
      preview << ",\"nei_effect\":" << int(authored->effect)
              << ",\"effect_center\":[" << authored->effectCenter.x << ',' << authored->effectCenter.y
              << ',' << authored->effectCenter.z << ']' << ",\"always_shimmer\":"
              << (authored->alwaysShimmer ? "true" : "false");
      preview << ",\"bounds\":{\"resource\":";
      writeBounds(b, 1, 0);
      preview << ",\"nonshop\":";
      writeBounds(b, submitted.front().first, submitted.front().second);
      preview << ",\"shop\":";
      writeBounds(b, scale, y);
      preview << "}}";
    }
    submitted.clear();
    player = {}; player.giObjectSegment=reinterpret_cast<void*>(uintptr_t(0x80000000));
    player.getItemEntry = entry;
    ref = {};
    Player_DrawGetItemImpl(&play, &player, &ref, 1);
    assert(std::abs(submitted.front().first - .2f * b.drawScale * commonFit.scale) < .000001f);
    assert(std::abs(submitted.front().second -
                    (14.f + .2f * commonFit.lift)) < .000001f);
    Reset();
    matrix = .25f;
    matrixY = 6;
    assert(NeiGi_DrawShop(&play, &entry));
#ifdef COMBO_BUILD
    assert(fallback == (NeiGi::IsSword(FindPresentation(&entry)->effect)?0:1) && matrix == .25f && matrixY == 6);
#else
    assert(fallback == 1 && matrix == .25f && matrixY == 6);
#endif
  }
  if (preview.is_open())
    preview << "]}\n";
  // Eight occupied potion-shop slots share one frame's XLU buffer. Keep at
  // least a third of its 4096 commands available to the room and other actors.
  Reset();
  enabled = 1;
  for (auto draw : {Randomizer_DrawFireRod, Randomizer_DrawIceRod,
                   Randomizer_DrawLightRod, Randomizer_DrawHyliaGrace,
                   Randomizer_DrawZonaiPermafrost, Randomizer_DrawDemiseDestruction,
                   Randomizer_DrawGustJar, Randomizer_DrawTimeGate}) {
    const auto &item = *std::find_if(std::begin(kPresentations), std::end(kPresentations),
                                   [draw](const auto &candidate) { return candidate.draw == draw; });
    files.insert(item.opaque);
    if (item.translucent)
      files.insert(item.translucent);
    entry.drawFunc = item.draw;
    assert(NeiGi_DrawShop(&play, &entry));
  }
  assert(gfx.polyXlu.p - xlu < 2700);
  size_t vertexBytes = 0;
  for (const auto &vertices : arena)
    vertexBytes += vertices.size() * sizeof(Vtx);
  std::cout << "Potion shop: " << vertexBytes << " arena vertex bytes, "
            << gfx.polyXlu.p - xlu << " XLU commands, " << allocations << " matrices\n"
            << std::flush;
  // Three crystal sheen passes each add one mesh matrix and one restore.
  assert(vertexBytes < 80 * 1024 && allocations <= 38 && stack.empty());

  // Deku Leaf's optional shimmer is green, including glint centers and halos.
  Reset();
  enabled = 1;
  entry.drawFunc = Randomizer_DrawDekuLeaf;
  files.insert("__OTR__objects/nei_gi_redesign/deku_leaf/gi_dl");
  assert(NeiGi_Draw(&play, &entry) && arena.size() == 1);
  for (const auto &vertex : arena.front()) {
    assert(vertex.v.cn[1] > vertex.v.cn[0] && vertex.v.cn[1] > vertex.v.cn[2]);
  }

  // Decode the actual GPU commands after cache packing. Shared positions with
  // different alpha/color must stay distinct, including across cache reloads.
  Reset();
  NeiGi::Mesh mesh;
  for (int i = 0; i < 40; ++i) {
    const NeiGi::EffectVertex a{{float(i), 0, 0}, 0xFF2020, 64};
    const NeiGi::EffectVertex b{{float(i + 1), 0, 0}, 0xFF2020, 64};
    const NeiGi::EffectVertex c{{float(i), 1, 0}, 0x20FF20, 128};
    const NeiGi::EffectVertex d{{float(i + 1), 1, 0}, 0x20FF20, 128};
    mesh.Tri(a, b, c);
    mesh.Tri(b, d, c);
  }
  mesh.Tri({{0, 0, 0}, 0x0000FF, 255}, {{1, 0, 0}, 0x0000FF, 255},
           {{0, 1, 0}, 0x0000FF, 255});
  // Same position/color but different UVs must not collapse at texture seams.
  mesh.Tri({{0, 0, 0}, 0x0000FF, 255, 1, 1}, {{1, 0, 0}, 0x0000FF, 255, 0, 1},
           {{0, 1, 0}, 0x0000FF, 255, 1, 0});
  NeiGi_DrawMesh(&play, mesh);
  std::vector<Vtx> cache;
  size_t decoded = 0;
  auto triangle = [&](uintptr_t word) {
    for (int shift : {16, 8, 0}) {
      const size_t index = ((word >> shift) & 255) / 2;
      assert(index < cache.size() && decoded < mesh.count);
      const auto &got = cache[index].v;
      const auto &want = mesh.vertices[decoded++];
      assert(got.ob[0] == want.p.x * 16 && got.ob[1] == want.p.y * 16 &&
             got.ob[2] == want.p.z * 16);
      assert(got.cn[0] == (want.rgb >> 16) &&
             got.cn[1] == ((want.rgb >> 8) & 255) &&
             got.cn[2] == (want.rgb & 255) && got.cn[3] == want.alpha);
      assert(got.tc[0] == std::lround(want.u * 63 * 32) &&
             got.tc[1] == std::lround(want.v * 63 * 32));
    }
  };
  for (Gfx *cmd = xlu; cmd != gfx.polyXlu.p; ++cmd) {
    if (auto it = vertexLoads.find(cmd); it != vertexLoads.end())
      cache = it->second;
    const auto op = (cmd->words.w0 >> 24) & 255;
    if (op == G_TRI1 || op == G_TRI2)
      triangle(cmd->words.w0);
    if (op == G_TRI2)
      triangle(cmd->words.w1);
  }
  assert(decoded == mesh.count && vertexLoads.size() > 1);
  // Exercise the shared production draw for all 63 serialized models at the
  // actual pickup/shop/freestanding caller scales and both owner routes.
  struct FrameFixture {const char* slug;float low,high,width,drawScale;bool xlu;};
  const FrameFixture frames[] = {
#include "nei_all_frame_bounds.inc"
  };
  assert(std::size(frames)==63);
  for(const auto& f:frames) for(const char* owner:{"","@oot:","@mm:","@oot-gi-base:"}) for(int route:{0,1,2}) {
    Reset();
    const std::string path=std::string("__OTR__")+owner+"objects/nei_gi_redesign/"+f.slug+"/gi_dl";
    const std::string shell=std::string("__OTR__")+owner+"objects/nei_gi_redesign/"+f.slug+"/gi_xlu_dl";
    const auto* b=NeiGi::FindFrameBounds(path.c_str());
    assert(b && std::abs(b->minimum.y-f.low)<.0001f && std::abs(b->maximum.y-f.high)<.0001f &&
           std::abs(b->spinningWidth-f.width)<.0001f);
    matrix=route==1?.25f:route==2?.2f:1.f;
    matrixY=route==1?6.f:route==2?14.f:0.f;
    const auto incoming=std::pair(matrix,matrixY);
    const float center[]={0,0,0};
    NeiGi_DrawPresentation(&play,path.c_str(),f.xlu?shell.c_str():nullptr,f.drawScale,int(b->effect),center,
                          false,nullptr,route==1);
    assert(!submitted.empty());
    const auto [s,y]=submitted.front();
    const float bottom=y+s*f.low, top=y+s*f.high;
    if(route==1) {
      assert(bottom>=.49999f && top<=(std::strcmp(f.slug,"rocs_feather")?19.00001f:21.50001f));
      assert(s*f.width<=19.00001f);
    } else if(route==2) {
      assert(bottom>=3.59999f && top<=23.60001f && s*f.width<=20.80001f);
    } else {
      assert(bottom>=-52.00001f && top<=48.00001f && s*f.width<=104.00001f);
    }
    if(f.xlu) {
      const bool sheen=NeiGi::IsSpell(b->effect);
      // Shell, optional packed sheen matrix, then the original state restore.
      assert(submitted[submitted.size()-(sheen?3:2)]==submitted.front());
      if(sheen) {
        const auto& highlight=submitted[submitted.size()-2];
        assert(std::abs(highlight.first*16-submitted.front().first)<.000001f &&
               highlight.second==submitted.front().second);
      }
    }
    assert(stack.empty() && std::pair(matrix,matrixY)==incoming && interpolation==0);
  }
  assert(!NeiGi::FindFrameBounds("__OTR__@bad:objects/nei_gi_redesign/four_sword/gi_dl"));
  assert(!NeiGi::FindFrameBounds("__OTR__objects/nei_gi_redesign/four_sword/held_dl"));
  std::cout<<"PASS all "<<std::size(frames)<<" serialized GI frames: native/OoT/MM routes, pickup/shop/freestanding bounds and shared shell pose\n";
#ifdef COMBO_BUILD
#include "tests/mm_presentation/pickup_framing_checks.inc"
#endif
  // Optional private USED surfaces must not load global textures or retain an
  // Alt-owned resource pointer. Missing base/Alt materials queue nothing.
  Reset();
  const NeiGi::TextureMaterial material{"__OTR__objects/nei_used_magic/ice_fracture", true, true};
  assert(!NeiGi_DrawTexturedMesh(&play, mesh, material));
  assert(gfx.polyXlu.p == xlu && allocations == 0 && arena.empty());
  files.insert(std::string("alt/") + material.path);
  assert(!NeiGi_DrawTexturedMesh(&play, mesh, material));
  alt = 1;
  assert(NeiGi_DrawTexturedMesh(&play, mesh, material));
  assert(loads == 0 && allocations > 0 && stack.empty() && interpolation == 0);
  for (const auto& batch : arena) for (const auto& vertex : batch) {
    // This fixture's UVs are normalized; private surfaces have a 32px logical tile.
    assert(std::abs(vertex.v.tc[0]) <= 32 * 32 && std::abs(vertex.v.tc[1]) <= 32 * 32);
  }
  alt = 0;
  const auto* after = gfx.polyXlu.p;
  assert(!NeiGi_DrawTexturedMesh(&play, mesh, material) && gfx.polyXlu.p == after);
  std::cout << "NEI production renderer: fallback, OPA/XLU, disabled effects, "
               "Alt paths, and matrix balance passed\n";
  return 0;
}
#endif
