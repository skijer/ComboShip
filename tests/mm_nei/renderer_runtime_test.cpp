#define MM_REAL_RENDERER
#include "../../soh/soh/Enhancements/randomizer/NeiGiRender.h"
#include "2s2h/Rando/NeiHeldPresentation.h"
#include "2s2h/Rando/NeiGiPresentation.h"
#include "2s2h/Rando/NeiLanternPresentation.h"
#include "2s2h/Rando/NeiResourceRouting.h"
#include "2s2h/Rando/NeiUsedMagicPresentation.h"
#include "ComboMaskShimmer.h"
#include "ComboSongDrawMM.h"
#include "ComboIkanaShieldResources.h"
#include "objects/object_gi_melody/object_gi_melody.h"
#include "rod_runtime_test.cpp"
#include <set>
#ifndef NEI_GI_REWARD_FIXTURE
extern "C" int ResourceMgr_GetRewardSurfaceForGame(const char*,const char*,NeiGi::Mesh*) { return 0; }
#endif
std::set<std::string> ownerBase, ownerAlt;
bool ownerAltEnabled = false, mmAltEnabled = false, ownerRegistered = true;
// Authored fallback cases below have no native shield graph. Its availability
// is a controlled archive boundary; actual typed/owner queries have their own gate.
static bool nativeShieldPairAvailable = false;
extern "C" int ResourceMgr_IsIkanaShieldGiAvailable() { return nativeShieldPairAvailable; }
extern "C" bool ResourceMgr_IsAltAssetsEnabled() { return mmAltEnabled; }
static std::set<std::string> descriptorPaths;
static int descriptorCalls;
extern "C" __attribute__((visibility("default"))) int32_t
OOT_NeiResourceExists(const char *path) {
  constexpr char stockPrefix[]="__OTR__@oot-gi-base:";
  if (!strncmp(path,stockPrefix,sizeof(stockPrefix)-1))
    return ownerRegistered && ownerBase.contains(std::string("__OTR__")+(path+sizeof(stockPrefix)-1));
  return ownerRegistered && (ownerBase.contains(path) ||
                             (ownerAltEnabled && ownerAlt.contains(path)));
}
extern "C" __attribute__((visibility("default"))) int32_t
OOT_GetNeiGiDrawInfoForAssets(const char* slug, int32_t altAssets, CwItemDrawInfo* out) {
  ++descriptorCalls;
  std::string prefix=std::string("__OTR__objects/nei_gi_redesign/")+slug;
  const char* opa=descriptorPaths.insert(prefix+"/gi_dl").first->c_str();
  const bool split=!strcmp(slug,"phantom_hourglass");
  const char* skin=split ? descriptorPaths.insert(prefix+"/gi_xlu_dl").first->c_str() : nullptr;
  *out = {};
  const auto *bounds = NeiGi::FindFrameBounds(opa);
  assert(bounds);
  out->neiEffect = static_cast<int>(bounds->effect);
  out->neiShimmer = out->neiEffect + 1;
  // The donor ABI carries palette separately from the protected model/energy
  // kind. Its real descriptor is exercised by the producer/bridge fixtures.
  const std::pair<const char*,NeiGi::Kind> equipmentPalettes[] = {
    {"divine_shield",NeiGi::Kind::DivineShield},
    {"sheikah_shield",NeiGi::Kind::SheikahShield},
    {"shield_of_ikana",NeiGi::Kind::IkanaShield},
    {"magic_cape",NeiGi::Kind::MagicCape},
    {"spirit_breastplate",NeiGi::Kind::SpiritBreastplate},
    {"sages_tunic",NeiGi::Kind::SagesTunic},
    {"champions_tunic",NeiGi::Kind::ChampionsTunic},
    {"pegasus_anklet",NeiGi::Kind::PegasusAnklet},
    {"climb_boots",NeiGi::Kind::ClimbBoots},
    {"pendant_of_memories",NeiGi::Kind::PendantOfMemories}
  };
  for (const auto& [name,palette] : equipmentPalettes)
    if (!strcmp(slug,name)) out->neiShimmer=static_cast<int>(palette)+1;
  if (!altAssets && NeiGi::IsSword(bounds->effect))
    opa=descriptorPaths.insert(std::string("__OTR__@oot-gi-base:")+(opa+7)).first->c_str();
  if(!OOT_NeiResourceExists(opa) || (skin && !OOT_NeiResourceExists(skin))) return 0;
  out->drawKind = CW_DRAW_KIND_NEI_GI;
  out->dlistCount = split ? 2 : 1;
  out->dlists[0]=opa; out->dlists[1]=skin; out->xluStartIndex=split ? 1 : -1;
  out->scale=1; out->itemShimmer=1;
  return 1;
}
extern "C" __attribute__((visibility("default"))) int32_t
OOT_GetNeiGiDrawInfo(const char* slug, CwItemDrawInfo* out) {
  return OOT_GetNeiGiDrawInfoForAssets(slug,1,out);
}
static bool itemEffects;
extern "C" int32_t CVarGetInteger(const char* name,int32_t value) {
  return !strcmp(name,"gEnhancements.SkijerNEI.ItemEffects") ? itemEffects : value;
}
extern "C" Color_RGB8 CVarGetColor24(const char*,Color_RGB8 value) {return value;}
// Selected third-party resource graphs and Din layer eligibility have dedicated
// production fixtures. This native NEI fixture supplies neither resource family.
extern "C" int ResourceMgr_GetGiModelFitForGame(const char*,const char*,float,float,int,float[2]) {return 0;}
extern "C" int ResourceMgr_GetGiModelsFitForGame(const char*,const char* const*,int,float,float,int,float[2]) {return 0;}
// No optional resource graph is installed in this renderer fixture, including
// the bounds observer used by Black Market displays.
extern "C" int ResourceMgr_GetGiModelBoundsForGame(const char*,const char* const*,int,float[3],float[3]) {return 0;}
extern "C" uint8_t MmAssets_IsAvailable() {return 1;}
extern "C" int ResourceMgr_GetDinSwordGiProfileForGame(const char*,const char*) {return 0;}
extern "C" Color_RGBA8 CosmeticEditor_GetChangedColor(u8,u8,u8,u8,const char*) {
  assert(false && "native NEI fixture selected an absent Din layer"); return {};
}
void DrawOotSlateRuneFlame(u8,u8,u8) {assert(false && "concrete non-Somaria GI must not borrow a flame");}
#include "mm_song_draw.inc"
extern "C" void gSPVertex(Gfx *cmd, uintptr_t addr, int count, int v0) {
  cmd->words.w0 = (G_VTX << 24) | (count << 12) | ((v0 + count) << 1);
  cmd->words.w1 = addr;
}
u8 slots[8]{};
extern "C" u8 ItemEquip_GetItemOnSlot(u8 i) { return slots[i]; }
int main() {
  PlayState play{};
  Player p{};
  GraphicsContext gfx{};
  alignas(16) static Gfx opa[0x6700], xlu[0x1000], overlay[0x800];
  play.state.gfxCtx = &gfx;
  gPlayState=&play;
  play.billboardMtxF.xx = play.billboardMtxF.yy = play.billboardMtxF.zz = 1;
  auto reset = [&]() {
    gfx.polyOpa.p = opa;
    gfx.polyOpa.d = std::end(opa);
    gfx.polyXlu.p = xlu;
    gfx.polyXlu.d = std::end(xlu);
    gfx.overlay.p = overlay;
    gfx.overlay.d = std::end(overlay);
    nativeDisplayLists.clear();
    Matrix_Translate(0, 0, 0, MTXMODE_NEW);
  };
  const auto packedEffects = [&]() {
    std::vector<Vtx> vertices;
    for(Gfx* cmd=xlu;cmd<gfx.polyXlu.p;++cmd)if(cmd->words.w0>>24==G_VTX) {
      const size_t count=(cmd->words.w0>>12)&255;
      const auto* batch=reinterpret_cast<const Vtx*>(cmd->words.w1);
      vertices.insert(vertices.end(),batch,batch+count);
    }
    return vertices;
  };
  struct Song {RandoItemId item;int profile;uint32_t hue;};
  const Song songs[]={
    {RI_OOT_SONG_MINUET_OF_FOREST,CW_SONG_OOT_MINUET,0x62FF62},
    {RI_OOT_SONG_BOLERO_OF_FIRE,CW_SONG_OOT_BOLERO,0xFF3C00},
    {RI_OOT_SONG_SERENADE_OF_WATER,CW_SONG_OOT_SERENADE,0x55B4DF},
    {RI_OOT_SONG_REQUIEM_OF_SPIRIT,CW_SONG_OOT_REQUIEM,0xDE9E2F},
    {RI_OOT_SONG_NOCTURNE_OF_SHADOW,CW_SONG_OOT_NOCTURNE,0xA028D2},
    {RI_OOT_SONG_PRELUDE_OF_LIGHT,CW_SONG_OOT_PRELUDE,0xEDE73E},
    {RI_SONG_EPONA,CW_SONG_EPONA,0xD96E30},{RI_SONG_SUN,CW_SONG_SUN,0xEDE73E},
    {RI_SONG_HEALING,CW_SONG_HEALING,0xFF96E6},{RI_SONG_SONATA,CW_SONG_SONATA,0x62FF62},
    {RI_SONG_LULLABY_INTRO,CW_SONG_LULLABY_INTRO,0xFF6464},{RI_SONG_LULLABY,CW_SONG_LULLABY,0xFF1414},
    {RI_SONG_NOVA,CW_SONG_NOVA,0x1414FF},{RI_SONG_ELEGY,CW_SONG_ELEGY,0xFF6200},
    {RI_SONG_OATH,CW_SONG_OATH,0x620062},{RI_SONG_DOUBLE_TIME,CW_SONG_DOUBLE_TIME,0x80D8F0},
    {RI_SONG_INVERTED_TIME,CW_SONG_INVERTED_TIME,0x4A70CA}};
  for(const auto& song:songs)for(bool effects:{false,true})for(bool alt:{false,true})for(bool donor:{false,true}) {
    reset();play.gameplayFrames=42;itemEffects=effects;mmAltEnabled=alt;ownerRegistered=donor;
    const uint8_t color[]={uint8_t(song.hue>>16),uint8_t(song.hue>>8),uint8_t(song.hue),255};
    NeiGi_DrawShimmerOverlay(&play,color,nullptr);
    const auto expected=packedEffects();assert(!expected.empty());
    reset();const int descriptionsBefore=descriptorCalls;
    assert(MM_TryDrawNeiGi(song.item) && descriptorCalls==descriptionsBefore);
    const auto shown=packedEffects();
    assert(shown.size()>=expected.size() && !memcmp(shown.data(),expected.data(),expected.size()*sizeof(Vtx)) &&
           "native MM songs must submit the matching shared shimmer even without optional effects or donor/mod resources");
    if(song.profile==CW_SONG_EPONA || song.profile==CW_SONG_SUN)assert(shown.size()==expected.size());
    if(song.profile==CW_SONG_OOT_BOLERO)assert(shown.size()==expected.size() && "Bolero must retain only red shimmer");
    if(song.profile==CW_SONG_OOT_PRELUDE) {
      reset();NeiGi_DrawMesh(&play,NeiGi::SampleSong(song.profile,42,NeiGi_CameraBasis(&play)));
      const auto motes=packedEffects();
      assert(shown.size()==expected.size()+motes.size() &&
             !memcmp(shown.data()+expected.size(),motes.data(),motes.size()*sizeof(Vtx)));
      assert(motes.size()>0 && motes.size()<300 && "Prelude light particles must replace its broad ring/disc");
      // Restore the note draw used by the shared assertion below.
      reset();assert(MM_TryDrawNeiGi(song.item));
    }
    assert(nativeDisplayLists.size()==1 && !strcmp(reinterpret_cast<const char*>(nativeDisplayLists.front()),
                                                  "__OTR__objects/object_gi_melody/gGiSongNoteDL") && matrices.empty());
  }
  reset();
  assert(MM_TryDrawNeiGi(RI_SONG_STORMS));
  const auto rainOnly=packedEffects();
  reset();NeiGi_DrawMesh(&play,NeiGi::SampleSeason(42,1,NeiGi_CameraBasis(&play)));
  const auto expectedRain=packedEffects();
  assert(rainOnly.size()==expectedRain.size() && !memcmp(rainOnly.data(),expectedRain.data(),expectedRain.size()*sizeof(Vtx)));
  for(RandoItemId song:{RI_SONG_HEALING,RI_SONG_SONATA,RI_SONG_NOVA,
                       RI_SONG_LULLABY,RI_SONG_LULLABY_INTRO,RI_SONG_ELEGY,RI_SONG_OATH,RI_SONG_DOUBLE_TIME,RI_SONG_INVERTED_TIME}) {
    reset();NeiGi_DrawSongOverlay(&play,ComboSongForMmItem(song),nullptr);
    const auto expected=packedEffects();assert(!expected.empty());
    reset();assert(MM_TryDrawNeiGi(song));
    const auto shown=packedEffects();
    assert(shown.size()==expected.size() && !memcmp(shown.data(),expected.data(),expected.size()*sizeof(Vtx)) && matrices.empty());
  }
  reset();assert(MM_TryDrawNeiGi(RI_SONG_TIME));assert(packedEffects().empty() && matrices.empty());
  for(bool effects:{false,true})for(bool alt:{false,true})for(bool donor:{false,true}) {
    reset();itemEffects=effects;mmAltEnabled=alt;ownerRegistered=donor;
    NeiGi_DrawMesh(&play,NeiGi::SampleSong(CW_SONG_SOARING,42,NeiGi_CameraBasis(&play)));
    const auto feathers=packedEffects();assert(feathers.size()>400);
    reset();assert(MM_TryDrawNeiGi(RI_SONG_SOARING));
    const auto shown=packedEffects();
    assert(shown.size()==feathers.size() && !memcmp(shown.data(),feathers.data(),shown.size()*sizeof(Vtx)) && matrices.empty());
    assert(nativeDisplayLists.size()==1 && "Soaring feathers must accompany the native song note");
  }
  ownerRegistered=true;mmAltEnabled=false;itemEffects=false;
  std::cout << "PASS real native MM song submission: mandatory colored shimmer, original note, rain-only Storms, plain regular songs and donor/Alt independence\n";
  const auto firstOpaquePath = [&]() {
    for (Gfx *cmd = opa; cmd < gfx.polyOpa.p; ++cmd)
      if (cmd->words.w0 >> 24 == G_DL_OTR_FILEPATH)
        return reinterpret_cast<const char *>(cmd->words.w1);
    assert(false && "missing deferred opaque path");
    return static_cast<const char *>(nullptr);
  };
  const NeiGi::TextureMaterial guardedMaterial{
      "__OTR__objects/nei_used_magic/ice_fracture", true, true};
  ownerBase.insert(guardedMaterial.path);
  for (int draw = 0; draw < 3; ++draw)
    for (int shortArena = 0; shortArena < 4; ++shortArena) {
      reset();
      if (shortArena == 0)
        gfx.polyOpa.d = opa + 1;
      if (shortArena == 1)
        gfx.polyXlu.d = xlu + 1;
      if (shortArena == 2)
        gfx.overlay.d = overlay + 1;
      if (shortArena == 3)
        gfx.polyOpa.p = opa + 2, gfx.polyOpa.d = opa + 1;
      const auto opaHead = gfx.polyOpa.p, xluHead = gfx.polyXlu.p;
      const auto opaTail = gfx.polyOpa.d, xluTail = gfx.polyXlu.d;
      const auto mesh = NeiGi::SampleSong(CW_SONG_OOT_ZELDA, 42);
      if (draw == 0)
        NeiGi_DrawMesh(&play, mesh);
      if (draw == 1)
        assert(!NeiGi_DrawTexturedMesh(&play, mesh, guardedMaterial));
      if (draw == 2)
        NeiGi_DrawSongOverlay(&play, CW_SONG_OOT_ZELDA, "oot");
      assert(gfx.polyOpa.p == opaHead && gfx.polyOpa.d == opaTail &&
             gfx.polyXlu.p == xluHead && gfx.polyXlu.d == xluTail &&
             gfx.overlay.p == overlay && matrices.empty());
    }
  reset();
  assert(NeiGi_DrawTexturedMesh(&play, NeiGi::SampleSong(CW_SONG_OOT_ZELDA, 42),
                                guardedMaterial));
  int ownerDepth = 0;
  bool texturedVertices = false;
  for (Gfx *cmd = xlu; cmd < gfx.polyXlu.p; ++cmd) {
    const unsigned op = cmd->words.w0 >> 24;
    if (op == G_COMBO_RM_PUSH)
      ++ownerDepth;
    if (op == G_VTX)
      assert(ownerDepth == 1), texturedVertices = true;
    if (op == G_COMBO_RM_POP)
      --ownerDepth;
    assert(ownerDepth >= 0);
  }
  assert(texturedVertices && ownerDepth == 0 &&
         gfx.polyOpa.p <= gfx.polyOpa.d && gfx.polyXlu.p <= gfx.polyXlu.d &&
         matrices.empty());
  std::cout << "PASS actual MM GRAPH_ALLOC/matrix/debug/setup arena guards, "
               "native textured owner balance and no commands on decline\n";
  // Execute the real MM binding/description code and its owner-routed draw.
  // Unknown items and missing/incomplete resources retain the native fallback.
  const std::pair<RandoItemId,const char*> candidates[] = {
    {RI_OOT_NEI_ELEMENTAL_WAND,"elemental_wand"},
    {RI_OOT_NEI_WAND_SAND_ROD,"sand_rod"}, {RI_OOT_NEI_WAND_TORNADO_ROD,"tornado_rod"},
    {RI_OOT_NEI_WAND_WATER_ROD,"water_rod"}, {RI_OOT_NEI_WAND_METEOR_ROD,"meteor_rod"},
    {RI_OOT_NEI_WAND_STORM_ROD,"storm_rod"}, {RI_OOT_NEI_WAND_SHADOW_SCEPTER,"shadow_scepter"},
    {RI_OOT_EXT_DIVINE_SHIELD,"divine_shield"}, {RI_OOT_EXT_SHEIKAH_SHIELD,"sheikah_shield"},
    {RI_SHIELD_MIRROR,"shield_of_ikana"}, {RI_OOT_EXT_MAGIC_CAPE,"magic_cape"},
    {RI_OOT_EXT_SPIRIT_BREASTPLATE,"spirit_breastplate"},
    {RI_OOT_EXT_WATER_DRAGON_SCALE,"sages_tunic"}, {RI_OOT_EXT_CHAMPIONS_TUNIC,"champions_tunic"},
    {RI_OOT_EXT_PEGASUS_ANKLET,"pegasus_anklet"}, {RI_OOT_EXT_TRIDENT,"trident"},
    {RI_OOT_EXT_CLIMB_BOOTS,"climb_boots"}, {RI_OOT_EXT_ROC_BOOTS,"roc_boots"},
    {RI_OOT_EXT_CANE_OF_BYRNA,"cane_of_byrna"}, {RI_OOT_EXT_FOUR_SWORD,"four_sword"},
    {RI_PENDANT_OF_MEMORIES,"pendant_of_memories"}, {RI_OOT_NEI_SHEIKAH_SLATE,"sheikah_slate"},
    {RI_OOT_NEI_SLATE_RUNE_BOMB,"slate_bomb"}, {RI_OOT_NEI_SLATE_RUNE_MASTER_CYCLE,"slate_master_cycle"},
    {RI_OOT_NEI_SLATE_RUNE_STASIS,"slate_stasis"}, {RI_OOT_NEI_SLATE_RUNE_CRYONIS,"slate_cryonis"},
    {RI_OOT_NEI_DESIRE_SENSOR,"slate_sensor"}, {RI_OOT_NEI_PHANTOM_HOURGLASS,"phantom_hourglass"},
    {RI_OOT_NEI_SHADOW_CRYSTAL,"shadow_crystal"}, {RI_OOT_NEI_ROD_OF_SEASONS,"rod_of_seasons"},
    {RI_SWORD_KOKIRI,"mm_kokiri_sword"}, {RI_SWORD_RAZOR,"razor_sword"}, {RI_SWORD_GILDED,"gilded_sword"},
    {RI_OOT_MASTER_SWORD,"master_sword"}, {RI_OOT_TRUE_MASTER_SWORD,"true_master_sword"},
    {RI_OOT_BIGGORON_SWORD,"biggoron_sword"}, {RI_GREAT_FAIRY_SWORD,"great_fairy_sword"},
    {RI_OOT_IRON_KNUCKLE_AXE,"iron_knuckle_axe"}
  };
  CwItemDrawInfo described{};
  assert(!MM_DescribeNeiGi(RI_BOW,&described) && descriptorCalls==0);
  nativeShieldPairAvailable=true;
  ownerBase.insert("__OTR__objects/nei_gi_redesign/shield_of_ikana/gi_dl");
  ownerAlt.insert("__OTR__objects/nei_gi_redesign/shield_of_ikana/gi_dl");
  for (bool ownerAssets : {false,true}) for (bool hostAssets : {false,true}) {
    reset();ownerAltEnabled=ownerAssets;mmAltEnabled=hostAssets;
    Gfx* beforeOpa=gfx.polyOpa.p;Gfx* beforeXlu=gfx.polyXlu.p;
    assert(!MM_DescribeNeiGi(RI_SHIELD_MIRROR,&described));
    assert(!MM_TryDrawNeiGi(RI_SHIELD_MIRROR));
    assert(described.neiShimmer==int(NeiGi::Kind::IkanaShield)+1 && described.itemShimmer &&
           "declined authored descriptors must retain native shield identity");
    assert(gfx.polyOpa.p==beforeOpa && gfx.polyXlu.p==beforeXlu &&
           "native shield availability must bypass authored geometry emission");
  }
  nativeShieldPairAvailable=false;
  puts("PASS native MM shield priority with both asset states; authored fixture emission remains fallback only");
  for(auto [id,slug] : candidates) {
    const std::string raw=std::string("__OTR__objects/nei_gi_redesign/")+slug;
    const bool vanillaSword=NeiGi::IsSword(NeiGi::FindFrameBounds((raw+"/gi_dl").c_str())->effect);
    reset(); ownerBase.clear(); ownerAlt.clear(); mmAltEnabled=false;
    assert(MM_TryDrawNeiGi(id)==vanillaSword && gfx.polyOpa.p==opa && gfx.polyXlu.p==xlu);
    ownerAlt.insert(raw+"/gi_dl"); ownerAlt.insert(raw+"/gi_xlu_dl");
    ownerAltEnabled=false; mmAltEnabled=true;
    assert(!MM_DescribeNeiGi(id,&described));
    ownerAltEnabled=true; mmAltEnabled=false;
    if(vanillaSword) {
      assert(!MM_DescribeNeiGi(id,&described));
      ownerBase.insert(raw+"/gi_dl");
    }
    assert(MM_DescribeNeiGi(id,&described));
    const std::string ownerPrefix=vanillaSword?"__OTR__@oot-gi-base:":"__OTR__@oot:";
    assert(std::string(described.dlists[0])==ownerPrefix+raw.substr(7)+"/gi_dl");
    assert(MM_TryDrawNeiGi(id));
    int bodies=0,skins=0;
    for(auto range : {std::pair(opa,gfx.polyOpa.p),std::pair(xlu,gfx.polyXlu.p)})
      for(Gfx* cmd=range.first;cmd<range.second;++cmd)
        if(cmd->words.w0>>24==G_DL_OTR_FILEPATH) {
          std::string path=(const char*)cmd->words.w1;
          assert(path.starts_with(ownerPrefix+"objects/nei_gi_redesign/"));
          if(path.ends_with("/gi_dl")) ++bodies; else ++skins;
        }
    assert(bodies==1 && skins==(described.dlistCount==2) && matrices.empty());
    if(described.dlistCount==2) {
      ownerAlt.erase(raw+"/gi_xlu_dl"); reset();
      assert(!MM_TryDrawNeiGi(id) && gfx.polyOpa.p==opa && gfx.polyXlu.p==xlu);
    }
  }
  // Native MM's CPU availability probe must use the inactive OoT donor and
  // its own Alt setting before the GPU owner scope is ever submitted.
  for (bool donorAlt : {false, true}) for (bool hostAlt : {false, true}) {
    reset(); ownerBase.clear(); ownerAlt.clear(); ownerRegistered = true;
    ownerAltEnabled = donorAlt; mmAltEnabled = hostAlt; itemEffects = false;
    play.state.frames = play.gameplayFrames = 9;
    auto& donorFiles = donorAlt ? ownerAlt : ownerBase;
    donorFiles.insert("__OTR__objects/nei_gi_redesign/sages_tunic/gi_dl");
    donorFiles.insert("__OTR__objects/object_gi_medal/gGiMedallionDL");
    std::set<std::string> expectedFaces;
    for (const char* name : {"Forest", "Fire", "Water", "Spirit", "Shadow", "Light"}) {
      expectedFaces.insert(std::string("__OTR__objects/object_gi_medal/gGi") + name + "MedallionFaceDL");
    }
    donorFiles.insert(expectedFaces.begin(), expectedFaces.end());
    assert(MM_TryDrawNeiGi(RI_OOT_EXT_WATER_DRAGON_SCALE));
    std::set<std::string> faces;
    int depth = 0, medalBodies = 0, tunics = 0;
    for (Gfx* cmd = opa; cmd < gfx.polyOpa.p; ++cmd) {
      const auto op = cmd->words.w0 >> 24;
      if (op == G_COMBO_RM_PUSH) {
        assert(!strcmp(reinterpret_cast<const char*>(cmd->words.w1), "oot"));
        ++depth;
      }
      if (op == G_COMBO_RM_POP) assert(--depth >= 0);
      if (op != G_DL_OTR_FILEPATH) continue;
      const std::string path = reinterpret_cast<const char*>(cmd->words.w1);
      if (path.find("/object_gi_medal/") != std::string::npos) {
        assert(depth == 1 && "deferred medal textures must retain their OoT resource owner");
        if (path.ends_with("MedallionFaceDL")) faces.insert(path);
        else if (path.ends_with("gGiMedallionDL")) ++medalBodies;
      } else if (path.ends_with("/sages_tunic/gi_dl")) ++tunics;
    }
    assert(faces == expectedFaces && medalBodies == 6 && tunics == 1 && depth == 0 && matrices.empty());
  }
  std::cout << "PASS native MM Sage fountain: inactive donor availability, divergent host/donor Alt and six owner-scoped medals\n";
  // Missing authored resources still use the shared shimmer around the
  // existing model, without inheriting a legacy drawer's scale/translation.
  for (auto [id,slug] : candidates) {
    const bool sword =
        id == RI_SWORD_KOKIRI || id == RI_SWORD_RAZOR ||
        id == RI_SWORD_GILDED || id == RI_GREAT_FAIRY_SWORD ||
        id == RI_OOT_MASTER_SWORD || id == RI_OOT_TRUE_MASTER_SWORD ||
        id == RI_OOT_BIGGORON_SWORD || id == RI_OOT_IRON_KNUCKLE_AXE ||
        id == RI_OOT_EXT_FOUR_SWORD;
    bool intrinsic = sword;
    for (const auto equipment : {RI_OOT_EXT_DIVINE_SHIELD, RI_OOT_EXT_SHEIKAH_SHIELD,
         RI_SHIELD_MIRROR, RI_OOT_EXT_MAGIC_CAPE, RI_OOT_EXT_SPIRIT_BREASTPLATE,
         RI_OOT_EXT_WATER_DRAGON_SCALE, RI_OOT_EXT_CHAMPIONS_TUNIC,
         RI_OOT_EXT_PEGASUS_ANKLET, RI_OOT_EXT_TRIDENT, RI_OOT_EXT_CLIMB_BOOTS,
         RI_OOT_EXT_ROC_BOOTS, RI_OOT_EXT_CANE_OF_BYRNA, RI_PENDANT_OF_MEMORIES})
      intrinsic |= id == equipment;
    for (bool enabled : {false,true}) {
      reset(); itemEffects = enabled;
      const auto before = current;
      {
        MM_NeiGiFallbackShimmer shimmer(id);
        Matrix_Scale(7,7,7,MTXMODE_APPLY);
        Matrix_Translate(1,2,3,MTXMODE_APPLY);
      }
      if (enabled || intrinsic) {
        assert(gfx.polyXlu.p > xlu);
        assert(!memcmp(&current,&before,sizeof(current)));
      } else assert(gfx.polyXlu.p == xlu);
      assert(matrices.empty());
    }
  }
  reset(); itemEffects = true;
  { MM_NeiGiFallbackShimmer unrelated(RI_BOW); }
  assert(gfx.polyXlu.p == xlu && matrices.empty());
  for (int season = RI_OOT_NEI_SEASON_SPRING; season <= RI_OOT_NEI_SEASON_WINTER; ++season) {
    reset();
    { MM_NeiGiFallbackShimmer weather(static_cast<RandoItemId>(season)); }
    assert(gfx.polyXlu.p == xlu && matrices.empty());
  }
  itemEffects = false;
  std::cout << "PASS native MM fallback equipment/item shimmer, toggle, sword invariance, pose isolation and weather exclusion\n";
  ownerBase.clear(); ownerAlt.clear(); ownerAltEnabled=false;
  std::cout << "PASS native MM bindings: all 38 new meshes, explicit OoT ownership, divergent Alt, atomic missing-resource fallback\n";
  const char *body = NEI_HELD_PATH("fire_rod");
  const char *glass = NEI_HELD_PATH("fake_glass");
  reset();
  assert(!NeiHeld_DrawModel(&play, body, glass));
  assert(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
  ownerBase.insert(body);
  assert(!NeiHeld_DrawModel(&play, body, glass));
  assert(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
  ownerAlt.insert(glass);
  mmAltEnabled = true;
  assert(!NeiHeld_DrawModel(&play, body, glass));
  ownerAltEnabled = true;
  mmAltEnabled = false;
  assert(NeiHeld_DrawModel(&play, body, glass));
  assert(std::string(firstOpaquePath()) ==
         "__OTR__@oot:objects/nei_held_redesign/fire_rod/gi_dl");
  const char *retained = firstOpaquePath();
  for (int i = 0; i < 10000; i++) {
    std::string temp = "__OTR__objects/transient/" + std::to_string(i);
    NeiResource_Route(temp.c_str());
  }
  assert(std::string(retained) ==
         "__OTR__@oot:objects/nei_held_redesign/fire_rod/gi_dl");
  ownerAltEnabled = false;
  reset();
  assert(!NeiHeld_DrawModel(&play, body, glass));
  assert(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
  ownerBase.erase(body);
  assert(!NeiHeld_DrawRod(&play, 0));
  assert(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
  // Native texture commands retain raw paths inside a balanced owner bracket.
  const char *texture = "__OTR__objects/nei_rod_attack/fire_surge";
  NeiGi::TextureMaterial mat{texture, false, false};
  const auto mesh =
      NeiGi::SampleOrb(NeiGi::Kind::Fire, {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}});
  reset();
  assert(!NeiGi_DrawTexturedMesh(&play, mesh, mat));
  assert(gfx.polyXlu.p == xlu);
  ownerBase.insert(texture);
  assert(NeiGi_DrawTexturedMesh(&play, mesh, mat));
  std::string active = "mm";
  int textures = 0, depth = 0;
  for (Gfx *c = xlu; c != gfx.polyXlu.p; c++) {
    unsigned op = (c->words.w0 >> 24) & 255;
    if (op == G_COMBO_RM_PUSH) {
      assert(active == "mm");
      active = (const char *)c->words.w1;
      ++depth;
    }
    if (op == G_SETTIMG || op == G_SETTIMG_OTR_FILEPATH) {
      assert(active == "oot");
      assert(std::string((const char *)c->words.w1) == texture);
      ++textures;
    }
    if (op == G_COMBO_RM_POP) {
      assert(depth == 1);
      active = "mm";
      --depth;
    }
  }
  assert(textures == 1 && depth == 0 && active == "mm");

  // Real native paired Mitts and charge-colored Jar: no partial replacements.
  for (auto pair :
       {std::pair{NEI_HELD_PATH("mitt_left"), NEI_HELD_PATH("mitt_right")},
        std::pair{NEI_HELD_PATH("gust_jar"), NEI_HELD_PATH("gust_jar_band")}}) {
    reset();
    ownerBase.insert(pair.first);
    bool mitts = std::string(pair.first).find("mitt") != std::string::npos;
    assert(!(mitts ? NeiHeld_DrawMitts(&p, &play)
                   : NeiHeld_DrawGustJar(&p, &play, 0, .5f)));
    assert(gfx.polyOpa.p == opa);
    ownerBase.insert(pair.second);
    assert(mitts ? NeiHeld_DrawMitts(&p, &play)
                 : NeiHeld_DrawGustJar(&p, &play, 0, .5f));
    int paths = 0;
    for (Gfx *c = opa; c < gfx.polyOpa.p; c++)
      if ((c->words.w0 >> 24) == G_DL_OTR_FILEPATH)
        paths++;
    assert(paths == 2);
  }
  const char *lantern = "__OTR__objects/nei_gi_redesign/lantern/gi_dl";
  const char *lanternGlass = "__OTR__objects/nei_gi_redesign/lantern/gi_xlu_dl";
  reset();
  ownerBase.insert(lantern);
  assert(!NeiLantern_DrawHeld(&p, &play, 0));
  assert(gfx.polyOpa.p == opa && gfx.polyXlu.p == xlu);
  ownerBase.insert(lanternGlass);
  for (int fire = 0; fire <= 4; fire++) {
    reset();
    assert(NeiLantern_DrawHeld(&p, &play, fire));
    assert(std::string(firstOpaquePath()) ==
           "__OTR__@oot:objects/nei_gi_redesign/lantern/gi_dl");
    int verts = 0, glassDraws = 0;
    for (Gfx *c = xlu; c < gfx.polyXlu.p; c++) {
      if ((c->words.w0 >> 24) == G_VTX)
        verts++;
      if ((c->words.w0 >> 24) == G_DL_OTR_FILEPATH) {
        assert(std::string((char *)c->words.w1) ==
               "__OTR__@oot:objects/nei_gi_redesign/lantern/gi_xlu_dl");
        glassDraws++;
      }
    }
    assert((fire == 0 ? verts == 0 : verts > 0) && glassDraws == 1 &&
           matrices.empty());
  }
  p.transformation = PLAYER_FORM_HUMAN;
  p.heldItemAction = PLAYER_IA_NONE;
  gCustomItemState.lanternEquipped = 1;
  assert(!NeiLantern_UsesGrip(&p));
  for (int slot = 0; slot < 8; slot++) {
    slots[slot] = ITEM_LANTERN;
    assert(NeiLantern_UsesGrip(&p));
    slots[slot] = ITEM_NONE;
  }
  slots[7] = ITEM_LANTERN;
  p.heldItemAction = PLAYER_IA_BOW;
  assert(!NeiLantern_UsesGrip(&p));
  p.heldItemAction = PLAYER_IA_NONE;
  p.transformation = PLAYER_FORM_DEKU;
  assert(!NeiLantern_UsesGrip(&p));
  ownerRegistered = false;
  reset();
  assert(!NeiGi_DrawTexturedMesh(&play, mesh, mat));
  assert(gfx.polyXlu.p == xlu);
  std::cout << "PASS native MM DL ownership, retained deferred paths, atomic "
               "and late fallback, owner Alt, raw texture owner brackets\n";
}
