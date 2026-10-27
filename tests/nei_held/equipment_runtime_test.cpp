#include <cassert>
#include <set>
#include <string>
#include <cstring>
#include "mods/equipment/nei_equipment_presentation.h"
#include "mods/equipment/nei_equipment_resources.inc"
#include "../../combo/NeiHeldSword.h"
std::set<std::string> resources, alternate, modded, donorModded, foreignModded;
bool altEnabled=false;
bool equipped=true;
Gfx nativeDL[1]{};
extern "C" {
SaveContext gSaveContext{};
s32 CVarGetInteger(const char*, s32 fallback){return fallback;}
uint8_t ResourceMgr_FileExists(const char* p){return resources.contains(p);}
uint8_t ResourceMgr_FileAltExists(const char* p){return alternate.contains(p);}
bool ResourceMgr_IsAltAssetsEnabled(){return altEnabled;}
int ResourceMgr_IsModAsset(const char* p){return modded.contains(p) || (altEnabled && alternate.contains(p));}
int ResourceMgr_IsModAssetForGame(const char*, const char* p){return foreignModded.contains(p);}
int32_t OOT_NeiEnsureGiBaseOwner(){return 1;}
int32_t OOT_NeiResourceExists(const char* p){
 const char* prefix="__OTR__@oot-gi-base:";
 return std::string(p).starts_with(prefix) && resources.contains(std::string("__OTR__")+(p+std::strlen(prefix)));
}
Gfx* ResourceMgr_LoadGfxByName(const char* p){assert(resources.contains(p));return nativeDL;}
Gfx* OotAssets_LoadGfxDirect(const char*){return nativeDL;}
u8 FourSword_IsEquipped(){return equipped;}
#ifdef NEI_EQUIPMENT_MM
u8 AdultLink_IsActive(){return 0;}
#endif
#ifdef NEI_EQUIPMENT_MM
int NeiResource_EnsureGiBaseOwner(){return 1;}
int NeiResource_Available(const char* p){return std::string(p).starts_with("__OTR__@oot-gi-base:")?OOT_NeiResourceExists(p):resources.contains(p);}
int NeiResource_IsMod(const char* p){return donorModded.contains(p);}
const char* NeiResource_Route(const char* p){static std::set<std::string> paths;return paths.insert(p[7]=='@'?std::string(p):std::string("__OTR__@oot:")+(p+7)).first->c_str();}
#endif
}
#define FOURSWORD_BLADE_DL "__OTR__objects/object_nei_four_sword/gNeiFourSwordBladeDL"
#define FOURSWORD_HILT_DL "__OTR__objects/object_nei_four_sword/gNeiFourSwordHiltDL"
#include "equipment_bindings.inc"
int main(int argc,char**){
 if(argc>1){void* b=nullptr;void* h=nullptr;assert(!FourSword_HeldSwordDL(&b,&h));return 0;}
 const char* legacy="__OTR__objects/legacy/model";
 for(int i=0;i<NEI_EQUIPMENT_COUNT;i++){
  const char* legacy = i == NEI_EQUIPMENT_IKANA_SHIELD
      ? "__OTR__objects/object_link_child/gLinkHumanMirrorShieldDL" : "__OTR__objects/legacy/model";
  resources.clear();alternate.clear();altEnabled=false;
  assert(NeiEquipment_ModelDL(i,legacy)==nullptr);
  resources.insert(NeiEquipment_ModelPath(i));
  assert(!NeiEquipment_ModelDL(i,legacy)); // incomplete graph must fall back atomically
  for (const char* const* r=sNeiEquipmentResources[i]; *r; ++r) resources.insert(*r);
  Gfx* d=NeiEquipment_ModelDL(i,legacy);
  assert(d!=nullptr);
  for (const char* const* r=sNeiEquipmentResources[i]; *r; ++r) {
   resources.erase(*r);assert(!NeiEquipment_ModelDL(i,legacy));resources.insert(*r);
  }
  assert((d[0].words.w0>>24)==G_DL_OTR_FILEPATH);
  const char* path=(const char*)d[0].words.w1;
#ifdef NEI_EQUIPMENT_MM
  assert(std::string(path).starts_with("__OTR__@oot:objects/nei_held_redesign/"));
#else
  assert(std::string(path).starts_with("__OTR__objects/nei_held_redesign/"));
#endif
  modded.insert(legacy);
  assert(NeiEquipment_ModelDL(i,legacy)==nullptr);
  Gfx* selectedMod=NeiEquipment_LegacyDL(i,legacy);
  assert(selectedMod && std::string((const char*)selectedMod[0].words.w1)==legacy);
  modded.clear();
  alternate.insert(legacy);altEnabled=true;
  assert(NeiEquipment_ModelDL(i,legacy)==nullptr); // caller retains selected legacy mod
  altEnabled=false;assert(NeiEquipment_ModelDL(i,legacy));
  resources.clear();assert(NeiEquipment_ModelDL(i,legacy)==nullptr);
 }
 resources={NeiEquipment_ModelPath(NEI_EQUIPMENT_FOUR_BLADE)};
 assert(!NeiEquipment_HasFourSword());
 for (int m : {NEI_EQUIPMENT_FOUR_BLADE,NEI_EQUIPMENT_FOUR_HILT})
  for (const char* const* r=sNeiEquipmentResources[m]; *r; ++r) resources.insert(*r);
 assert(NeiEquipment_HasFourSword());
 assert(!NeiEquipment_ModelDL(-1,legacy));
 assert(!NeiEquipment_ModelDL(NEI_EQUIPMENT_COUNT,legacy));
 // Execute source-extracted production getter bodies, including original caches.
 const char* originals[]={"__OTR__objects/object_somaria/g_byrna_cane_dl",
  "__OTR__objects/object_gnd/gPhantomGanonSkelLimbsLimb_00C610DL_009298",
  "__OTR__objects/object_nei_kite_shield/g_kite_shield_dl",
  "__OTR__objects/object_nei_divine_shield/g_divine_shield_dl"};
 Gfx* (*getters[])()={Byrna_GetCaneDL,Trident_GetLanceDL,ExtEquip_GetKiteShieldDL,ExtEquip_GetDivineShieldDL};
 const int models[]={NEI_EQUIPMENT_BYRNA,NEI_EQUIPMENT_TRIDENT,NEI_EQUIPMENT_SHEIKAH_SHIELD,NEI_EQUIPMENT_DIVINE_SHIELD};
 for(int i=0;i<4;i++){
  resources={originals[i]};alternate.clear();altEnabled=false;
  assert(getters[i]()==nativeDL);
  for (const char* const* r=sNeiEquipmentResources[models[i]]; *r; ++r) resources.insert(*r);
  assert(getters[i]()!=nativeDL);
  altEnabled=true;alternate.insert(originals[i]);
  assert(std::string((const char*)getters[i]()[0].words.w1)==originals[i]);
  altEnabled=false;alternate.clear();modded.insert(originals[i]);
  assert(std::string((const char*)getters[i]()[0].words.w1)==originals[i]);
  modded.clear();assert(getters[i]()!=nativeDL);
 }
 void* blade=nullptr;void* hilt=nullptr;altEnabled=false;alternate.clear();
 resources={FOURSWORD_BLADE_DL,FOURSWORD_HILT_DL,NeiEquipment_ModelPath(NEI_EQUIPMENT_FOUR_BLADE)};
 assert(FourSword_HeldSwordDL(&blade,&hilt));assert(blade==nativeDL && hilt==nativeDL);
 for (int m : {NEI_EQUIPMENT_FOUR_BLADE,NEI_EQUIPMENT_FOUR_HILT})
  for (const char* const* r=sNeiEquipmentResources[m]; *r; ++r) resources.insert(*r);
 assert(FourSword_HeldSwordDL(&blade,&hilt));assert(blade!=nativeDL && hilt!=nativeDL);
 altEnabled=true;alternate.insert(FOURSWORD_HILT_DL);
 assert(FourSword_HeldSwordDL(&blade,&hilt));
 assert(std::string((const char*)((Gfx*)blade)[0].words.w1)==FOURSWORD_BLADE_DL);
 assert(std::string((const char*)((Gfx*)hilt)[0].words.w1)==FOURSWORD_HILT_DL);
 // A complete redesigned GI mesh in a held frame supersedes the older split
 // authored model. No second hilt is drawn over the exact complete sword.
 altEnabled=false;alternate.clear();
 for (const char* name : {"edge_tex", "gi_dl", "gold_edge_tex", "gold_tex", "guard_groove_tex",
                         "ivory_cloth_tex", "mesh_opa_vtx", "pommel_blue_tex", "ruby_tex",
                         "scale_mtx", "steel_tex"})
  resources.insert(std::string("__OTR__objects/nei_gi_redesign/four_sword/")+name);
#ifdef NEI_EQUIPMENT_MM
 const char* redesignedFour="__OTR__objects/nei_held_swords/four_sword/mm_human/held_dl";
 resources.insert("__OTR__objects/nei_held_swords/four_sword/mm_human/grip_mtx");
#else
 gSaveContext.linkAge=LINK_AGE_ADULT;
 const char* redesignedFour="__OTR__objects/nei_held_swords/four_sword/oot_adult/held_dl";
 resources.insert("__OTR__objects/nei_held_swords/four_sword/oot_adult/grip_mtx");
#endif
 resources.insert(redesignedFour);
 assert(FourSword_HeldSwordDL(&blade,&hilt));
 assert(hilt==nullptr && "Four Sword still draws its older authored hilt over the redesigned whole GI mesh");
#ifdef NEI_EQUIPMENT_MM
 assert(std::string((const char*)((Gfx*)blade)[0].words.w1)==
        "__OTR__@oot-gi-base:objects/nei_held_swords/four_sword/mm_human/held_dl");
#else
 assert(std::string((const char*)((Gfx*)blade)[0].words.w1)==std::string("__OTR__@oot-gi-base:")+(redesignedFour+7));
#endif
 // An active mod of either old half retains the whole selected legacy pair.
 modded.insert(FOURSWORD_HILT_DL);
 assert(FourSword_HeldSwordDL(&blade,&hilt));
 assert(hilt!=nullptr);
 assert(std::string((const char*)((Gfx*)hilt)[0].words.w1)==FOURSWORD_HILT_DL);
 modded.clear();
 altEnabled=false;alternate={FOURSWORD_HILT_DL};modded={FOURSWORD_BLADE_DL};resources={FOURSWORD_BLADE_DL};
 assert(!FourSword_HeldSwordDL(&blade,&hilt)); // inactive Alt cannot complete a legacy pair
 modded.clear();
 equipped=false;assert(!FourSword_HeldSwordDL(&blade,&hilt));
 alternate.clear();altEnabled=false;modded.clear();resources.clear();
 for (const char* const* r=sNeiEquipmentResources[NEI_EQUIPMENT_IKANA_SHIELD]; *r; ++r) resources.insert(*r);
 const char* body="__OTR__objects/object_link_child/gLinkHumanMirrorShieldDL";
 const char* hand="__OTR__objects/object_link_child/gLinkHumanRightHandHoldingMirrorShieldDL";
#ifdef NEI_EQUIPMENT_MM
 modded.insert(hand); // A hand-only mod preserves native back as well.
#else
 foreignModded.insert(hand);
#endif
 assert(!NeiEquipment_ModelDL(NEI_EQUIPMENT_IKANA_SHIELD,body));
 auto* shieldBody=NeiEquipment_LegacyDL(NEI_EQUIPMENT_IKANA_SHIELD,body);
 auto* shieldHand=NeiEquipment_LegacyDL(NEI_EQUIPMENT_IKANA_SHIELD,hand);
 assert(shieldBody && shieldHand && shieldBody!=shieldHand);
#ifdef NEI_EQUIPMENT_MM
 assert(std::string((const char*)shieldBody[0].words.w1)==body);
 assert(std::string((const char*)shieldHand[0].words.w1)==hand);
 modded.clear();donorModded.insert(originals[0]);
 for (const char* const* r=sNeiEquipmentResources[NEI_EQUIPMENT_BYRNA]; *r; ++r) resources.insert(*r);
 assert(!NeiEquipment_ModelDL(NEI_EQUIPMENT_BYRNA,originals[0]));
 assert(std::string((const char*)NeiEquipment_LegacyDL(NEI_EQUIPMENT_BYRNA,originals[0])[0].words.w1)==
    "__OTR__@oot:objects/object_somaria/g_byrna_cane_dl");
#else
 assert(std::string((const char*)shieldBody[0].words.w1)=="__OTR__@mm:objects/object_link_child/gLinkHumanMirrorShieldDL");
 assert(std::string((const char*)shieldHand[0].words.w1)=="__OTR__@mm:objects/object_link_child/gLinkHumanRightHandHoldingMirrorShieldDL");
 // A local OoT hand-only mod selects the local owner for BOTH native halves.
 foreignModded.clear();modded.insert(hand);
 shieldBody=NeiEquipment_LegacyDL(NEI_EQUIPMENT_IKANA_SHIELD,body);
 shieldHand=NeiEquipment_LegacyDL(NEI_EQUIPMENT_IKANA_SHIELD,hand);
 assert(std::string((const char*)shieldBody[0].words.w1)==body);
 assert(std::string((const char*)shieldHand[0].words.w1)==hand);

#endif


}
