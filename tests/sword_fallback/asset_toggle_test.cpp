#include <cassert>
#include <cstring>
#include <iostream>
#include <set>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include "combo/menu/ComboItemDrawABI.h"
#include "combo/menu/ComboCollectibleShimmer.h"
#include "soh/soh/Enhancements/randomizer/NeiGiFrameFit.h"
using NeiGi::Kind;
#define COMBO_EXPORT
#define CVAR_ENHANCEMENT(x) x
#define CVAR_NEI_GI_EFFECTS "test.ItemEffects"
/* ITEM_IDS */
bool ootAlt=true,mmAlt=false,legacyMod=false,mmLegacyMod=false;
bool dinFireSword=false;
bool ResourceMgr_IsAltAssetsEnabled(){return mmAlt;}
int32_t OOT_NeiAltAssetsEnabled(){return ootAlt;}
int CVarGetInteger(const char* key,int){return !strcmp(key,"DinFireSword")&&dinFireSword;}
const char* NeiResource_Route(const char* path){return path;}
int ResourceMgr_IsModAssetForGame(const char*,const char*){return mmLegacyMod;}
int ResourceMgr_IsIkanaShieldGiAvailable(){assert(false && "sword toggle must not query the native shield pair");return 0;}
std::set<std::string> resources;
int32_t OOT_NeiEnsureGiBaseOwner(){return 1;}
int32_t OOT_NeiResourceExists(const char* path){
    if(!path)return 0;
    if(!strncmp(path,"__OTR__@oot-gi-base:",20))return resources.contains(std::string("__OTR__")+(path+20));
    return resources.contains(path);
}
struct Presentation {
    const char* opaque;const char* translucent;float scale;Kind effect;
    struct{float x,y,z;}effectCenter{};bool alwaysShimmer=true;void(*draw)()=nullptr;
    Kind shimmer=Kind::Neutral;
};
const Presentation kMmKokiriPresentation={"__OTR__objects/nei_gi_redesign/mm_kokiri_sword/gi_dl",nullptr,1,Kind::KokiriSword};
const Presentation kPresentations[]={
    {"__OTR__objects/nei_gi_redesign/kokiri_sword/gi_dl",nullptr,1,Kind::KokiriSword},
    {"__OTR__objects/nei_gi_redesign/master_sword/gi_dl",nullptr,1,Kind::MasterSword},
    {"__OTR__objects/nei_gi_redesign/true_master_sword/gi_dl",nullptr,1,Kind::SwordAura},
    {"__OTR__objects/nei_gi_redesign/razor_sword/gi_dl",nullptr,1,Kind::RazorSword},
    {"__OTR__objects/nei_gi_redesign/gilded_sword/gi_dl",nullptr,1,Kind::GildedSword},
    {"__OTR__objects/nei_gi_redesign/biggoron_sword/gi_dl",nullptr,1,Kind::BiggoronSword},
    {"__OTR__objects/nei_gi_redesign/giants_knife/gi_dl",nullptr,1,Kind::GiantsKnife},
    {"__OTR__objects/nei_gi_redesign/great_fairy_sword/gi_dl",nullptr,1,Kind::GreatFairySword},
    {"__OTR__objects/nei_gi_redesign/fire_rod/gi_dl",nullptr,1,Kind::Fire},
};
struct{const char* slug;}kSeasons[]={{"season_spring"}};
void Randomizer_DrawCaneSomariaUpgrade(){}
bool NeiGi_FillSeasonInfo(int,CwItemDrawInfo*){return false;}
bool HasLegacyGiMod(const Presentation&,bool){return legacyMod;}
bool HasRedesignGiMod(const Presentation&){return false;}
/* OWNER_SELECTION */
/* OWNER_ASSET_QUERY */
/* OWNER_QUERY */
#define RANDO_ENUM_BEGIN(x) enum x {
#define RANDO_ENUM_ITEM(x) x,
#define RANDO_ENUM_END(x) };
#include "soh/soh/Enhancements/randomizer/randomizerEnums/RandomizerGet.h"
#undef RANDO_ENUM_BEGIN
#undef RANDO_ENUM_ITEM
#undef RANDO_ENUM_END
/* OWNER_CUSTOM_RECIPE */
/* OWNER_DEPENDENCY */
struct SwordAward {RandomizerGet id;const char* name;const char* slug;Kind identity;const char* progressiveName;RandomizerGet progressiveId;};
const SwordAward swordAwards[]={
    {RG_KOKIRI_SWORD,"Kokiri Sword","kokiri_sword",Kind::KokiriSword,"Progressive Kokiri Sword",RG_PROGRESSIVE_KOKIRI_SWORD},
    {RG_RAZOR_SWORD,"Razor Sword","razor_sword",Kind::RazorSword,"Progressive Kokiri Sword",RG_PROGRESSIVE_KOKIRI_SWORD},
    {RG_GILDED_SWORD,"Gilded Sword","gilded_sword",Kind::GildedSword,"Progressive Kokiri Sword",RG_PROGRESSIVE_KOKIRI_SWORD},
    {RG_MASTER_SWORD,"Master Sword","master_sword",Kind::MasterSword,"Progressive Master Sword",RG_PROGRESSIVE_MASTER_SWORD},
    {RG_TRUE_MASTER_SWORD,"True Master Sword","true_master_sword",Kind::SwordAura,"Progressive Master Sword",RG_PROGRESSIVE_MASTER_SWORD},
    {RG_BIGGORON_SWORD,"Biggoron's Sword","biggoron_sword",Kind::BiggoronSword,"Progressive Biggoron's Sword",RG_PROGRESSIVE_BGS},
    {RG_GREAT_FAIRY_SWORD,"Great Fairy's Sword","great_fairy_sword",Kind::GreatFairySword,"Progressive Biggoron's Sword",RG_PROGRESSIVE_BGS},
    {RG_GIANTS_KNIFE,"Giant's Knife","giants_knife",Kind::GiantsKnife,nullptr,RG_GIANTS_KNIFE},
};
int donorSwordTier=0,progressiveRequests=0;
int OwnerItemDescribe(const char* name,CwItemDrawInfo* out){
    const bool progressive=!strncmp(name,"Progressive ",12);
    const SwordAward* award=nullptr;
    if(progressive){award=&swordAwards[donorSwordTier];++progressiveRequests;}
    else for(const auto& candidate:swordAwards)if(!strcmp(name,candidate.name)){award=&candidate;break;}
    if(!award)return 0;
    const int result=CwAltSwordGi(award->id,out)?1:OOT_GetNeiGiDrawInfoForAssets(award->slug,ootAlt,out);
    if(result!=1)return result;
    out->resolvedName=award->name;
    out->stateDependent=OOT_DrawDependency(progressive?award->progressiveId:award->id,*out);
    return 1;
}
void* Combo_ResolveSym(const char*,const char* name){
    if(!strcmp(name,"OOT_GetItemDrawInfo"))return reinterpret_cast<void*>(OwnerItemDescribe);
    /* ASSET_QUERY_SYMBOL */
    if(!strcmp(name,"OOT_GetNeiGiDrawInfo"))return reinterpret_cast<void*>(OOT_GetNeiGiDrawInfo);
    return nullptr;
}
/* MM_BINDINGS */
/* MM_SELECTION */
namespace ComboRando {
constexpr int GAME_OOT=0;
struct ForeignItem {int itemGame=0;std::string itemName="Master Sword",fakeItemName;bool HasDisguise()const{return false;}} foreignItem;
}
uint64_t foreignGeneration=1;
namespace Rando::MiscBehavior {
const ComboRando::ForeignItem* MM_LookupForeign(int){return &ComboRando::foreignItem;}
uint64_t ComboRandoGen(){return foreignGeneration;}
}
using RandoCheckId=int;
constexpr int RC_UNKNOWN=-1;
struct {int fileNum=0;} gSaveContext;
struct {uint32_t gameplayFrames=42;} fakePlay;
auto* gPlayState=&fakePlay;
int ComboNativeMmImport(const char*){return -1;}
enum class ComboForeignResolveOOT{Ok,Unknown,NotReady};
const char* ComboInternRoutedPathOOT(const std::string& s){static std::unordered_set<std::string> paths;return paths.insert(s).first->c_str();}
#include "combo/menu/ComboSwordGiAssetSelection.h"
/* FOREIGN_RESOLVER */
/* FOREIGN_CACHE */
int main(){
    resources.insert(kMmKokiriPresentation.opaque);
    for(const auto& item:kPresentations)resources.insert(item.opaque);
    resources.insert("__OTR__alt/objects/object_custom_equip/gCustomKokiriSwordDL");
    resources.insert("__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL");
    resources.insert("__OTR__alt/objects/object_custom_equip/gCustomLongswordDL");
    for(const char* family:{"child","adult","bgs"})
      resources.insert(std::string("__OTR__objects/din_fire_sword/progressive/")+family+"/SwordDL");
    struct Award{RandoItemId id;const char* slug;Kind identity;};
    const Award awards[]={{RI_SWORD_KOKIRI,"mm_kokiri_sword",Kind::KokiriSword},
      {RI_SWORD_RAZOR,"razor_sword",Kind::RazorSword},{RI_SWORD_GILDED,"gilded_sword",Kind::GildedSword},
      {RI_OOT_MASTER_SWORD,"master_sword",Kind::MasterSword},{RI_OOT_TRUE_MASTER_SWORD,"true_master_sword",Kind::SwordAura},
      {RI_OOT_BIGGORON_SWORD,"biggoron_sword",Kind::BiggoronSword},
      {RI_GREAT_FAIRY_SWORD,"great_fairy_sword",Kind::GreatFairySword}};
    // The active MM mode must win over an independently enabled OoT donor.
    // Repeated transitions also exercise the descriptor's static function cache.
    for(bool donor:{true,false})for(bool host:{false,true,false,true,false})for(const auto& award:awards){
        ootAlt=donor;mmAlt=host;legacyMod=mmLegacyMod=false;
        CwItemDrawInfo info{};assert(MM_DescribeNeiGi(award.id,&info));
        assert(info.itemShimmer&&info.neiShimmer==static_cast<int>(award.identity)+1&&info.stateDependent==2);
        if(!host||!donor){
            const std::string authored=std::string(!host?"__OTR__@oot-gi-base:":"__OTR__")+"objects/nei_gi_redesign/"+award.slug+"/gi_dl";
            assert(info.drawKind==CW_DRAW_KIND_NEI_GI&&!strcmp(info.dlists[0],authored.c_str())&&
                   "vanilla MM sword GI retained a custom OoT donor model");
            assert(NeiGi::FindFrameBounds(info.dlists[0])&&"base-owner route lost the approved sword framing");
        }else assert(info.drawKind==CW_DRAW_KIND_CUSTOM_GI&&strstr(info.dlists[0],"object_custom_equip"));
        assert(ootAlt==donor&&mmAlt==host&&"GI selection mutated either owner's global asset state");
    }
    // A foreign OoT award must honor MM's independent Alt state, including
    // repeated draws through the resolver's cached producer function.
    for(const auto& award:{std::pair{"Master Sword","master_sword"},std::pair{"Giant's Knife","giants_knife"}})
    for(bool host:{true,false,true,false}) {
      ootAlt=true;mmAlt=host;legacyMod=mmLegacyMod=false;
      ComboForeignDrawInfoOOT foreign{};
      assert(ComboFillForeignDrawInfoOOT(1,foreign,award.first)==ComboForeignResolveOOT::Ok);
      if(host)assert(foreign.drawKind==CW_DRAW_KIND_CUSTOM_GI && strstr(foreign.dls[0],"object_custom_equip"));
      else assert(foreign.drawKind==CW_DRAW_KIND_NEI_GI &&
                  foreign.dls[0]==std::string("__OTR__@oot-gi-base:objects/nei_gi_redesign/")+award.second+"/gi_dl" &&
                  "foreign sword ignored the active MM vanilla asset setting");
      assert(foreign.neiShimmer==int(!strcmp(award.second,"giants_knife")?Kind::GiantsKnife:Kind::MasterSword)+1);
      assert(foreign.appearanceDependent && foreign.stateDependent);
    }
    // Missing shipped data is retryable, so a later host toggle cannot remain
    // frozen in the foreign renderer's negative cache.
    ootAlt=true;mmAlt=false;legacyMod=mmLegacyMod=false;
    resources.erase("__OTR__objects/nei_gi_redesign/master_sword/gi_dl");
    ComboForeignDrawInfoOOT missing{};
    assert(ComboFillForeignDrawInfoOOT(1,missing,"Master Sword")==ComboForeignResolveOOT::NotReady &&
           "missing vanilla sword was permanently cached as an unknown foreign item");
    mmAlt=true;missing={};
    assert(ComboFillForeignDrawInfoOOT(1,missing,"Master Sword")==ComboForeignResolveOOT::Ok);
    resources.insert("__OTR__objects/nei_gi_redesign/master_sword/gi_dl");
    // A surviving/base-path mod must not suppress the authored vanilla sword.
    ootAlt=true;mmAlt=false;legacyMod=mmLegacyMod=true;
    CwItemDrawInfo info{};assert(MM_DescribeNeiGi(RI_SWORD_RAZOR,&info)&&info.drawKind==CW_DRAW_KIND_NEI_GI);
    mmAlt=true;assert(!MM_DescribeNeiGi(RI_SWORD_RAZOR,&info)&&info.itemShimmer);
    // Non-sword legacy priority keeps its existing behavior.
    mmAlt=false;mmLegacyMod=false;legacyMod=true;
    assert(!MM_DescribeNeiGi(RI_OOT_NEI_FIRE_ROD,&info));
    legacyMod=false;assert(MM_DescribeNeiGi(RI_OOT_NEI_FIRE_ROD,&info));
    // A grant freezes the tier, not its cosmetic recipe. Exercise the actual
    // dependency classifier, latch and cache, unlike the direct queries above.
    dinFireSword=true;legacyMod=mmLegacyMod=false;
    for(bool grantAlt:{true,false})for(int tier=0;tier<7;++tier){
      ++foreignGeneration;ootAlt=true;mmAlt=grantAlt;donorSwordTier=tier;
      const auto& award=swordAwards[tier];
      ComboRando::foreignItem.itemName=award.progressiveName;
      const auto* preview=ComboResolveForeignDrawInfoOOT(17);
      assert(preview&&preview->drawKind==(grantAlt?CW_DRAW_KIND_CUSTOM_GI:CW_DRAW_KIND_NEI_GI));
      if(grantAlt)assert(strstr(preview->dls[0],"din_fire_sword"));
      ComboLatchForeignDrawOOT(17);
      assert(!strcmp(ComboForeignLatchedNameOOT(17),award.name));
      donorSwordTier=(tier+1)%7; // The cross-game grant advanced the donor save.
      const int requestsAtGrant=progressiveRequests;
      for(bool host:{false,true,false,true,false}){
        mmAlt=host;
        const auto* drawn=ComboResolveForeignDrawInfoOOT(17);
        assert(drawn&&drawn->neiShimmer==static_cast<int>(award.identity)+1);
        if(!host){
          const std::string expected=std::string("__OTR__@oot-gi-base:objects/nei_gi_redesign/")+award.slug+"/gi_dl";
          assert(drawn->drawKind==CW_DRAW_KIND_NEI_GI&&!strcmp(drawn->dls[0],expected.c_str())&&
                 "latched progressive sword kept Din's mesh after switching to vanilla");
        }else assert(drawn->drawKind==CW_DRAW_KIND_CUSTOM_GI&&strstr(drawn->dls[0],"din_fire_sword"));
        assert(drawn->resolvedName==award.name&&drawn->itemShimmer);
        assert(!strcmp(ComboForeignLatchedNameOOT(17),award.name));
        assert(ootAlt&&"host toggle changed the donor asset mode");
      }
      assert(progressiveRequests==requestsAtGrant&&"receipt re-resolved the next progressive tier");
    }
    // Uncollected previews remain progression dependent; a generation or
    // save-slot change also discards the previous receipt's concrete identity.
    ++foreignGeneration;ootAlt=mmAlt=true;donorSwordTier=0;
    ComboRando::foreignItem.itemName="Progressive Kokiri Sword";
    assert(ComboResolveForeignDrawInfoOOT(17)->resolvedName=="Kokiri Sword");
    donorSwordTier=1;
    assert(ComboResolveForeignDrawInfoOOT(17)->resolvedName=="Razor Sword");
    ComboLatchForeignDrawOOT(17);
    donorSwordTier=2;++gSaveContext.fileNum;
    assert(ComboForeignLatchedNameOOT(17)==nullptr);
    assert(ComboResolveForeignDrawInfoOOT(17)->resolvedName=="Gilded Sword");
    // Keep the frozen award name across NotReady and retry it, rather than
    // resolving the donor's next tier or retaining the old custom sword.
    ++foreignGeneration;donorSwordTier=3;ootAlt=mmAlt=true;
    ComboRando::foreignItem.itemName="Progressive Master Sword";
    ComboLatchForeignDrawOOT(17);donorSwordTier=4;mmAlt=false;
    resources.erase("__OTR__objects/nei_gi_redesign/master_sword/gi_dl");
    assert(ComboResolveForeignDrawInfoOOT(17)==nullptr);
    assert(!strcmp(ComboForeignLatchedNameOOT(17),"Master Sword"));
    resources.insert("__OTR__objects/nei_gi_redesign/master_sword/gi_dl");
    const auto* retry=ComboResolveForeignDrawInfoOOT(17);
    assert(retry&&retry->drawKind==CW_DRAW_KIND_NEI_GI&&retry->resolvedName=="Master Sword");
    assert(strstr(retry->dls[0],"@oot-gi-base:objects/nei_gi_redesign/master_sword/"));
    std::cout<<"PASS seven native sword identities and distinct foreign Giant's Knife, independent MM/OoT asset states, repeated vanilla/custom transitions, surviving legacy mods and unchanged non-sword priority\n";
    std::cout<<"PASS all seven progressive foreign sword receipts retain their awarded tier while switching Din/authored GIs after grant\n";
    std::cout<<"PASS both starting asset modes, live uncollected previews, generation/slot resets and missing shipped-resource retry of the frozen tier\n";
}
