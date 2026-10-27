#include "din_fire_shield.h"
/**
 * @file z_draw.c
 * @brief Draw get-item models
 */
#include "global.h"
void NeiGi_ReportBlackMarketModel(const char* owner, const char* opaque, const char* translucent);
#include "ComboElementalArrowGi.h"
#include "2s2h/BenGui/CosmeticEditor.h"
#include "2s2h/Enhancements/ItemVisuals.h"
#include <libultraship/bridge/consolevariablebridge.h>
#include <libultraship/bridge/resourcebridge.h>
#include "ComboFairyBottle.h"
#include "ComboBottleGi.h"
#include "ComboBottleContents.h"
#include "assets/objects/gameplay_keep/gameplay_keep.h"
#include "assets/objects/object_gi_arrow/object_gi_arrow.h"
#include "assets/objects/object_gi_arrowcase/object_gi_arrowcase.h"
#include "assets/objects/object_gi_bean/object_gi_bean.h"
#include "assets/objects/object_gi_bigbomb/object_gi_bigbomb.h"
#include "assets/objects/object_gi_bomb_1/object_gi_bomb_1.h"
#include "assets/objects/object_gi_bomb_2/object_gi_bomb_2.h"
#include "assets/objects/object_gi_bombpouch/object_gi_bombpouch.h"
#include "assets/objects/object_gi_bosskey/object_gi_bosskey.h"
#include "assets/objects/object_gi_bottle_04/object_gi_bottle_04.h"
#include "assets/objects/object_gi_bottle_15/object_gi_bottle_15.h"
#include "assets/objects/object_gi_bottle_16/object_gi_bottle_16.h"
#include "assets/objects/object_gi_bottle_21/object_gi_bottle_21.h"
#include "assets/objects/object_gi_bottle_22/object_gi_bottle_22.h"
#include "assets/objects/object_gi_bottle_red/object_gi_bottle_red.h"
#include "assets/objects/object_gi_bottle/object_gi_bottle.h"
#include "assets/objects/object_gi_bow/object_gi_bow.h"
#include "assets/objects/object_gi_camera/object_gi_camera.h"
#include "assets/objects/object_gi_compass/object_gi_compass.h"
#include "assets/objects/object_gi_fieldmap/object_gi_fieldmap.h"
#include "assets/objects/object_gi_fish/object_gi_fish.h"
#include "assets/objects/object_gi_ghost/object_gi_ghost.h"
#include "assets/objects/object_gi_glasses/object_gi_glasses.h"
#include "assets/objects/object_gi_gold_dust/object_gi_gold_dust.h"
#include "assets/objects/object_gi_golonmask/object_gi_golonmask.h"
#include "assets/objects/object_gi_heart/object_gi_heart.h"
#include "assets/objects/object_gi_hearts/object_gi_hearts.h"
#include "assets/objects/object_gi_hookshot/object_gi_hookshot.h"
#include "assets/objects/object_gi_insect/object_gi_insect.h"
#include "assets/objects/object_gi_key/object_gi_key.h"
#include "assets/objects/object_gi_ki_tan_mask/object_gi_ki_tan_mask.h"
#include "assets/objects/object_gi_liquid/object_gi_liquid.h"
#include "assets/objects/object_gi_loach/object_gi_loach.h"
#include "assets/objects/object_gi_longsword/object_gi_longsword.h"
#include "assets/objects/object_gi_m_arrow/object_gi_m_arrow.h"
#include "assets/objects/object_gi_magicmushroom/object_gi_magicmushroom.h"
#include "assets/objects/object_gi_magicpot/object_gi_magicpot.h"
#include "assets/objects/object_gi_map/object_gi_map.h"
#include "assets/objects/object_gi_mask03/object_gi_mask03.h"
#include "assets/objects/object_gi_mask05/object_gi_mask05.h"
#include "assets/objects/object_gi_mask06/object_gi_mask06.h"
#include "assets/objects/object_gi_mask09/object_gi_mask09.h"
#include "assets/objects/object_gi_mask10/object_gi_mask10.h"
#include "assets/objects/object_gi_mask11/object_gi_mask11.h"
#include "assets/objects/object_gi_mask12/object_gi_mask12.h"
#include "assets/objects/object_gi_mask13/object_gi_mask13.h"
#include "assets/objects/object_gi_mask14/object_gi_mask14.h"
#include "assets/objects/object_gi_mask15/object_gi_mask15.h"
#include "assets/objects/object_gi_mask16/object_gi_mask16.h"
#include "assets/objects/object_gi_mask17/object_gi_mask17.h"
#include "assets/objects/object_gi_mask18/object_gi_mask18.h"
#include "assets/objects/object_gi_mask20/object_gi_mask20.h"
#include "assets/objects/object_gi_mask21/object_gi_mask21.h"
#include "assets/objects/object_gi_mask22/object_gi_mask22.h"
#include "assets/objects/object_gi_mask23/object_gi_mask23.h"
#include "assets/objects/object_gi_melody/object_gi_melody.h"
#include "assets/objects/object_gi_milk/object_gi_milk.h"
#include "assets/objects/object_gi_mssa/object_gi_mssa.h"
#include "assets/objects/object_gi_nuts/object_gi_nuts.h"
#include "assets/objects/object_gi_nutsmask/object_gi_nutsmask.h"
#include "assets/objects/object_gi_ocarina/object_gi_ocarina.h"
#include "assets/objects/object_gi_purse/object_gi_purse.h"
#include "assets/objects/object_gi_rabit_mask/object_gi_rabit_mask.h"
#include "assets/objects/object_gi_reserve_b_00/object_gi_reserve_b_00.h"
#include "assets/objects/object_gi_reserve_b_01/object_gi_reserve_b_01.h"
#include "assets/objects/object_gi_reserve_c_00/object_gi_reserve_c_00.h"
#include "assets/objects/object_gi_reserve_c_01/object_gi_reserve_c_01.h"
#include "assets/objects/object_gi_reserve00/object_gi_reserve00.h"
#include "assets/objects/object_gi_reserve01/object_gi_reserve01.h"
#include "assets/objects/object_gi_rupy/object_gi_rupy.h"
#include "assets/objects/object_gi_schedule/object_gi_schedule.h"
#include "assets/objects/object_gi_seahorse/object_gi_seahorse.h"
#include "assets/objects/object_gi_shield_2/object_gi_shield_2.h"
#include "assets/objects/object_gi_shield_3/object_gi_shield_3.h"
#include "assets/objects/object_gi_soldout/object_gi_soldout.h"
#include "assets/objects/object_gi_soul/object_gi_soul.h"
#include "assets/objects/object_gi_stick/object_gi_stick.h"
#include "assets/objects/object_gi_stonemask/object_gi_stonemask.h"
#include "assets/objects/object_gi_sutaru/object_gi_sutaru.h"
#include "assets/objects/object_gi_sword_1/object_gi_sword_1.h"
#include "assets/objects/object_gi_sword_2/object_gi_sword_2.h"
#include "assets/objects/object_gi_sword_3/object_gi_sword_3.h"
#include "assets/objects/object_gi_sword_4/object_gi_sword_4.h"
#include "assets/objects/object_gi_truth_mask/object_gi_truth_mask.h"
#include "assets/objects/object_gi_zoramask/object_gi_zoramask.h"
#include "assets/objects/object_bsmask/object_bsmask.h"
#include "assets/objects/object_st/object_st.h"

#include "2s2h/BenPort.h"

void GetItem_DrawBombchu(PlayState* play, s16 drawId);
void GetItem_DrawPoes(PlayState* play, s16 drawId);
void GetItem_DrawFairyBottle(PlayState* play, s16 drawId);
void GetItem_DrawSkullToken(PlayState* play, s16 drawId);
void GetItem_DrawCompass(PlayState* play, s16 drawId);
void GetItem_DrawPotion(PlayState* play, s16 drawId);
void GetItem_DrawGoronSword(PlayState* play, s16 drawId);
void GetItem_DrawDekuNuts(PlayState* play, s16 drawId);
void GetItem_DrawRecoveryHeart(PlayState* play, s16 drawId);
void GetItem_DrawFish(PlayState* play, s16 drawId);
void GetItem_DrawOpa0(PlayState* play, s16 drawId);
void GetItem_DrawOpa0Xlu1(PlayState* play, s16 drawId);
void GetItem_DrawOpa01(PlayState* play, s16 drawId);
void GetItem_DrawXlu01(PlayState* play, s16 drawId);
void GetItem_DrawSeahorse(PlayState* play, s16 drawId);
void GetItem_DrawFairyContainer(PlayState* play, s16 drawId);
void GetItem_DrawMoonsTear(PlayState* play, s16 drawId);
void GetItem_DrawMagicArrow(PlayState* play, s16 drawId);
void GetItem_DrawUpgrades(PlayState* play, s16 drawId);
void GetItem_DrawRupee(PlayState* play, s16 drawId);
void GetItem_DrawSmallRupee(PlayState* play, s16 drawId);
void GetItem_DrawWallet(PlayState* play, s16 drawId);
void GetItem_DrawRemains(PlayState* play, s16 drawId);

typedef struct {
    /* 0x0 */ void (*drawFunc)(PlayState*, s16);
    /* 0x4 */ void* drawResources[8]; // Either display lists (Gfx*) or matrices (Mtx*)
} DrawItemTableEntry;                 // size = 0x24

static const ALIGN_ASSET(2) char gComboMushroomBottleGi[] = "__OTR__objects/combo_bottle_gi/MushroomBottle";
static const ALIGN_ASSET(2) char gComboPrincessBottleGi[] = "__OTR__objects/combo_bottle_gi/PrincessBottle";
static const ALIGN_ASSET(2) char gComboGoldDustBottleGi[] = "__OTR__objects/combo_bottle_gi/GoldDustBottle";

void GetItem_DrawBottleContents(PlayState* play, s16 drawId);
void GetItem_DrawSeahorseBottle(PlayState* play, s16 drawId);

static DrawItemTableEntry sDrawItemTable[] = {
    // GID_BOTTLE, OBJECT_GI_BOTTLE
    { GetItem_DrawOpa0Xlu1, { gGiEmptyBottleCorkDL, gGiEmptyBottleGlassDL } },
    // GID_KEY_SMALL, OBJECT_GI_KEY
    { GetItem_DrawOpa0, { gGiSmallKeyDL } },
    // GID_MASK_KAMARO, OBJECT_GI_MASK17
    { GetItem_DrawOpa01, { gGiKamaroMaskDL, gGiKamaroMaskEmptyDL } },
    // GID_MASK_COUPLE, OBJECT_GI_MASK13
    { GetItem_DrawOpa0Xlu1, { gGiCouplesMaskFullDL, gGiCouplesMaskHalfDL } },
    // GID_04, OBJECT_GI_MELODY
    { GetItem_DrawXlu01, { gGiSerenadeColorDL, gGiSongNoteDL } },
    // GID_05, OBJECT_GI_MELODY
    { GetItem_DrawXlu01, { gGiRequiemColorDL, gGiSongNoteDL } },
    // GID_06, OBJECT_GI_MELODY
    { GetItem_DrawXlu01, { gGiNocturneColorDL, gGiSongNoteDL } },
    // GID_07, OBJECT_GI_MELODY
    { GetItem_DrawXlu01, { gGiPreludeColorDL, gGiSongNoteDL } },
    // GID_RECOVERY_HEART, OBJECT_GI_HEART
    { GetItem_DrawRecoveryHeart, { gGiRecoveryHeartDL } },
    // GID_KEY_BOSS, OBJECT_GI_KEY_BOSS
    { GetItem_DrawOpa0Xlu1, { gGiBossKeyDL, gGiBossKeyGemDL } },
    // GID_COMPASS, OBJECT_GI_COMPASS
    { GetItem_DrawCompass, { gGiCompassDL, gGiCompassGlassDL } },
    // GID_BOMBERS_NOTEBOOK, OBJECT_GI_SCHEDULE
    { GetItem_DrawOpa0Xlu1, { gGiBombersNotebookEmptyDL, gGiBombersNotebookDL } },
    // GID_MASK_STONE, OBJECT_GI_STONEMASK
    { GetItem_DrawOpa0Xlu1, { gGiStoneMaskEmptyDL, gGiStoneMaskDL } },
    // GID_MASK_KAFEIS_MASK, OBJECT_GI_MASK05
    { GetItem_DrawOpa01, { gGiKafeiMaskDL, gGiKafeiMaskEmptyDL } },
    // GID_MASK_CIRCUS_LEADER, OBJECT_GI_MASK11
    { GetItem_DrawOpa0Xlu1, { gGiCircusLeaderMaskFaceDL, gGiCircusLeaderMaskEyebrowsDL } },
    // GID_MASK_BREMEN, OBJECT_GI_MASK20
    { GetItem_DrawOpa01, { gGiBremenMaskDL, gGiBremenMaskEmptyDL } },
    // GID_MASK_ALL_NIGHT, OBJECT_GI_MASK06
    { GetItem_DrawOpa0Xlu1, { gGiAllNightMaskEyesDL, gGiAllNightMaskFaceDL } },
    // GID_DEKU_NUTS, OBJECT_GI_NUTS
    { GetItem_DrawDekuNuts, { gGiNutDL } },
    // GID_HEART_CONTAINER, OBJECT_GI_HEARTS
    { GetItem_DrawXlu01, { gGiHeartBorderDL, gGiHeartContainerDL } },
    // GID_HEART_PIECE, OBJECT_GI_HEARTS
    { GetItem_DrawXlu01, { gGiHeartBorderDL, gGiHeartPieceDL } },
    // GID_QUIVER_30, OBJECT_GI_ARROWCASE
    { GetItem_DrawUpgrades, { gGiQuiverInnerDL, gGiQuiver30InnerColorDL, gGiQuiver30OuterColorDL, gGiQuiverOuterDL } },
    // GID_QUIVER_40, OBJECT_GI_ARROWCASE
    { GetItem_DrawUpgrades, { gGiQuiverInnerDL, gGiQuiver40InnerColorDL, gGiQuiver40OuterColorDL, gGiQuiverOuterDL } },
    // GID_QUIVER_50, OBJECT_GI_ARROWCASE
    { GetItem_DrawUpgrades, { gGiQuiverInnerDL, gGiQuiver50InnerColorDL, gGiQuiver50OuterColorDL, gGiQuiverOuterDL } },
    // GID_BOMB_BAG_20, OBJECT_GI_BOMBPOUCH
    { GetItem_DrawUpgrades, { gGiBombBagDL, gGiBombBag20BagColorDL, gGiBombBag20RingColorDL, gGiBombBagRingDL } },
    // GID_BOMB_BAG_30, OBJECT_GI_BOMBPOUCH
    { GetItem_DrawUpgrades, { gGiBombBagDL, gGiBombBag30BagColorDL, gGiBombBag30RingColorDL, gGiBombBagRingDL } },
    // GID_BOMB_BAG_40, OBJECT_GI_BOMBPOUCH
    { GetItem_DrawUpgrades, { gGiBombBagDL, gGiBombBag40BagColorDL, gGiBombBag40RingColorDL, gGiBombBagRingDL } },
    // GID_DEKU_STICK, OBJECT_GI_STICK
    { GetItem_DrawOpa0, { gGiStickDL } },
    // GID_DUNGEON_MAP, OBJECT_GI_MAP
    { GetItem_DrawOpa0, { gGiDungeonMapDL } },
    // GID_MAGIC_JAR_SMALL, OBJECT_GI_MAGICPOT
    { GetItem_DrawOpa0, { gGiMagicJarSmallDL } },
    // GID_MAGIC_JAR_BIG, OBJECT_GI_MAGICPOT
    { GetItem_DrawOpa0, { gGiMagicJarLargeDL } },
    // GID_BOMB, OBJECT_GI_BOMB_1
    { GetItem_DrawOpa0, { gGiBombDL } },
    // GID_STONE_OF_AGONY, OBJECT_GI_MAP
    { GetItem_DrawOpa0, { gGiStoneOfAgonyDL } },
    // GID_WALLET_ADULT, OBJECT_GI_PURSE
    { GetItem_DrawWallet,
      { gGiWalletDL, gGiAdultWalletColorDL, gGiAdultWalletRupeeOuterColorDL, gGiWalletRupeeOuterDL,
        gGiAdultWalletStringColorDL, gGiWalletStringDL, gGiAdultWalletRupeeInnerColorDL, gGiWalletRupeeInnerDL } },
    // GID_WALLET_GIANT, OBJECT_GI_PURSE
    { GetItem_DrawWallet,
      { gGiWalletDL, gGiGiantsWalletColorDL, gGiGiantsWalletRupeeOuterColorDL, gGiWalletRupeeOuterDL,
        gGiGiantsWalletStringColorDL, gGiWalletStringDL, gGiGiantsWalletRupeeInnerColorDL, gGiWalletRupeeInnerDL } },
    // GID_MASK_DON_GERO, OBJECT_GI_MASK16
    { GetItem_DrawOpa0Xlu1, { gGiDonGeroMaskFaceDL, gGiDonGeroMaskBodyDL } },
    // GID_ARROWS_SMALL, OBJECT_GI_ARROW
    { GetItem_DrawOpa0, { gGiArrowSmallDL } },
    // GID_ARROWS_MEDIUM, OBJECT_GI_ARROW
    { GetItem_DrawOpa0, { gGiArrowMediumDL } },
    // GID_ARROWS_LARGE, OBJECT_GI_ARROW
    { GetItem_DrawOpa0, { gGiArrowLargeDL } },
    // GID_BOMBCHU, OBJECT_GI_BOMB_2
    { GetItem_DrawBombchu, { gGiBombchuDL } },
    // GID_SHIELD_HERO, OBJECT_GI_SHIELD_2
    { GetItem_DrawOpa0Xlu1, { gGiHerosShieldEmblemDL, gGiHerosShieldDL } },
    // GID_HOOKSHOT, OBJECT_GI_HOOKSHOT
    { GetItem_DrawOpa01, { gGiHookshotEmptyDL, gGiHookshotDL } },
    // GID_29, OBJECT_GI_HOOKSHOT
    { GetItem_DrawOpa01, { gGiHookshotEmptyDL, gGiHookshotDL } },
    // GID_OCARINA, OBJECT_GI_OCARINA
    { GetItem_DrawOpa0Xlu1, { gGiOcarinaOfTimeDL, gGiOcarinaOfTimeHolesDL } },
    // GID_MILK, OBJECT_GI_MILK
    { GetItem_DrawOpa0Xlu1, { gGiMilkBottleContentsDL, gGiMilkBottleGlassDL } },
    // GID_MASK_KEATON, OBJECT_GI_KI_TAN_MASK
    { GetItem_DrawOpa0Xlu1, { gGiKeatonMaskDL, gGiKeatonMaskEyesDL } },
    // GID_TINGLE_MAP, OBJECT_GI_FIELDMAP
    { GetItem_DrawOpa01, { gGiTingleMapDL, gGiTingleMapEmptyDL } },
    // GID_BOW, OBJECT_GI_BOW
    { GetItem_DrawOpa01, { gGiBowHandleDL, gGiBowStringDL } },
    // GID_LENS, OBJECT_GI_GLASSES
    { GetItem_DrawOpa0Xlu1, { gGiLensDL, gGiLensGlassDL } },
    // GID_POTION_GREEN, OBJECT_GI_LIQUID
    { GetItem_DrawPotion,
      { gGiPotionContainerPotDL, gGiPotionContainerGreenPotColorDL, gGiPotionContainerGreenLiquidColorDL,
        gGiPotionContainerLiquidDL, gGiPotionContainerGreenPatternColorDL, gGiPotionContainerPatternDL } },
    // GID_POTION_RED, OBJECT_GI_LIQUID
    { GetItem_DrawPotion,
      { gGiPotionContainerPotDL, gGiPotionContainerRedPotColorDL, gGiPotionContainerRedLiquidColorDL,
        gGiPotionContainerLiquidDL, gGiPotionContainerRedPatternColorDL, gGiPotionContainerPatternDL } },
    // GID_POTION_BLUE, OBJECT_GI_LIQUID
    { GetItem_DrawPotion,
      { gGiPotionContainerPotDL, gGiPotionContainerBluePotColorDL, gGiPotionContainerBlueLiquidColorDL,
        gGiPotionContainerLiquidDL, gGiPotionContainerBluePatternColorDL, gGiPotionContainerPatternDL } },
    // GID_SHIELD_MIRROR, OBJECT_GI_SHIELD_3
    { GetItem_DrawOpa0Xlu1, { gGiMirrorShieldEmptyDL, gGiMirrorShieldDL } },
    // GID_MAGIC_BEANS, OBJECT_GI_BEAN
    { GetItem_DrawOpa0, { gGiBeanDL } },
    // GID_FISH, OBJECT_GI_FISH
    { GetItem_DrawFish, { gGiFishContainerDL } },
    // GID_LETTER_MAMA, OBJECT_GI_RESERVE_B_01
    { GetItem_DrawOpa0Xlu1, { gGiLetterToMamaEnvelopeLetterDL, gGiLetterToMamaInscriptionsDL } },
    // GID_37
    { GetItem_DrawOpa0, { NULL } },
    // GID_SWORD_BGS, OBJECT_GI_LONGSWORD
    { GetItem_DrawGoronSword, { gGiBiggoronSwordDL } },
    // GID_MASK_SUN, OBJECT_GI_MSSA
    { GetItem_DrawOpa0Xlu1, { gGiSunMaskFaceDL, gGiSunMaskEyesSidePatternsDL } },
    // GID_MASK_BLAST, OBJECT_GI_MASK21
    { GetItem_DrawOpa0Xlu1, { gGiBlastMaskEmptyDL, gGiBlastMaskDL } },
    // GID_FAIRY, OBJECT_GI_BOTTLE_04
    { GetItem_DrawFairyContainer,
      { gGiFairyBottleEmptyDL, gGiFairyBottleGlassCorkDL, gGiFairyBottleContentsDL, &gGiFairyBottleBillboardRotMtx } },
    // GID_MASK_SCENTS, OBJECT_GI_MASK22
    { GetItem_DrawOpa01, { gGiMaskOfScentsFaceDL, gGiMaskOfScentsTeethDL } },
    // GID_MASK_CAPTAIN, OBJECT_GI_MASK18
    { GetItem_DrawOpa01, { gGiCaptainsHatBodyDL, gGiCaptainsHatFaceDL } },
    // GID_MASK_BUNNY, OBJECT_GI_RABIT_MASK
    { GetItem_DrawOpa0Xlu1, { gGiBunnyHoodDL, gGiBunnyHoodEyesDL } },
    // GID_MASK_TRUTH, OBJECT_GI_TRUTH_MASK
    { GetItem_DrawOpa0Xlu1, { gGiMaskOfTruthDL, gGiMaskOfTruthAccentsDL } },
    // GID_DEED_SWAMP, OBJECT_GI_RESERVE01
    { GetItem_DrawOpa01, { gGiTitleDeedEmptyDL, gGiTitleDeedSwampColorDL } },
    // GID_DEED_MOUNTAIN, OBJECT_GI_RESERVE01
    { GetItem_DrawOpa01, { gGiTitleDeedEmptyDL, gGiTitleDeedMountainColorDL } },
    // GID_RUPEE_HUGE, OBJECT_GI_RUPY
    { GetItem_DrawRupee, { gGiRupeeInnerDL, gGiGoldRupeeInnerColorDL, gGiRupeeOuterDL, gGiGoldRupeeOuterColorDL } },
    // GID_DEED_OCEAN, OBJECT_GI_RESERVE01
    { GetItem_DrawOpa01, { gGiTitleDeedEmptyDL, gGiTitleDeedOceanColorDL } },
    // GID_MASK_GORON, OBJECT_GI_GOLONMASK
    { GetItem_DrawOpa0Xlu1, { gGiGoronMaskEmptyDL, gGiGoronMaskDL } },
    // GID_MASK_ZORA, OBJECT_GI_ZORAMASK
    { GetItem_DrawOpa01, { gGiZoraMaskEmptyDL, gGiZoraMaskDL } },
    // GID_46
    { GetItem_DrawBombchu, { NULL } },
    // GID_ARROW_FIRE, OBJECT_GI_M_ARROW
    { GetItem_DrawMagicArrow, { gGiMagicArrowAmmoDL, gGiMagicArrowFireColorDL, gGiMagicArrowGlowDL } },
    // GID_ARROW_ICE, OBJECT_GI_M_ARROW
    { GetItem_DrawMagicArrow, { gGiMagicArrowAmmoDL, gGiMagicArrowIceColorDL, gGiMagicArrowGlowDL } },
    // GID_ARROW_LIGHT, OBJECT_GI_M_ARROW
    { GetItem_DrawMagicArrow, { gGiMagicArrowAmmoDL, gGiMagicArrowLightColorDL, gGiMagicArrowGlowDL } },
    // GID_SKULL_TOKEN, OBJECT_GI_SUTARU
    { GetItem_DrawSkullToken, { gGiSkulltulaTokenDL, gGiSkulltulaTokenFlameDL } },
    // GID_BUG, OBJECT_GI_INSECT
    { GetItem_DrawOpa0Xlu1, { gGiBugContainerContentsDL, gGiBugContainerGlassDL } },
    // GID_4C
    { GetItem_DrawOpa0Xlu1, { NULL } },
    // GID_POE, OBJECT_GI_GHOST
    { GetItem_DrawPoes,
      { gGiPoeContainerLidDL, gGiPoeContainerGlassDL, gGiPoeContainerContentsDL, gGiPoeContainerPoeColorDL } },
    // GID_FAIRY_2, OBJECT_GI_SOUL
    { GetItem_DrawFairyBottle, { gGiFairyContainerBaseCapDL, gGiFairyContainerGlassDL, gGiFairyContainerContentsDL } },
    // GID_RUPEE_GREEN, OBJECT_GI_RUPY
    { GetItem_DrawSmallRupee,
      { gGiRupeeInnerDL, gGiGreenRupeeInnerColorDL, gGiRupeeOuterDL, gGiGreenRupeeOuterColorDL } },
    // GID_RUPEE_BLUE, OBJECT_GI_RUPY
    { GetItem_DrawSmallRupee,
      { gGiRupeeInnerDL, gGiBlueRupeeInnerColorDL, gGiRupeeOuterDL, gGiBlueRupeeOuterColorDL } },
    // GID_RUPEE_RED, OBJECT_GI_RUPY
    { GetItem_DrawSmallRupee, { gGiRupeeInnerDL, gGiRedRupeeInnerColorDL, gGiRupeeOuterDL, gGiRedRupeeOuterColorDL } },
    // GID_BIG_POE, OBJECT_GI_GHOST
    { GetItem_DrawPoes,
      { gGiPoeContainerLidDL, gGiPoeContainerGlassDL, gGiPoeContainerContentsDL, gGiPoeContainerBigPoeColorDL } },
    // GID_RUPEE_PURPLE, OBJECT_GI_RUPY
    { GetItem_DrawRupee, { gGiRupeeInnerDL, gGiPurpleRupeeInnerColorDL, gGiRupeeOuterDL, gGiPurpleRupeeOuterColorDL } },
    // GID_RUPEE_SILVER, OBJECT_GI_RUPY
    { GetItem_DrawRupee, { gGiRupeeInnerDL, gGiSilverRupeeInnerColorDL, gGiRupeeOuterDL, gGiSilverRupeeOuterColorDL } },
    // GID_SWORD_KOKIRI, OBJECT_GI_SWORD_1
    { GetItem_DrawOpa01, { gGiKokiriSwordBladeHiltDL, gGiKokiriSwordGuardDL } },
    // GID_SKULL_TOKEN_2, OBJECT_ST
    { GetItem_DrawSkullToken, { gSkulltulaTokenDL, gSkulltulaTokenFlameDL } },
    // GID_57, OBJECT_GI_BOTTLE_RED
    { GetItem_DrawOpa0Xlu1, { gGiRedPotionBottleEmptyDL, gGiRedPotionBottleDL } },
    // GID_ZORA_EGG, OBJECT_GI_BOTTLE_15
    { GetItem_DrawOpa0Xlu1, { gGiZoraEggBottleContentsDL, gGiZoraEggBottleGlassAndCorkDL } },
    // GID_MOONS_TEAR, OBJECT_GI_RESERVE00
    { GetItem_DrawMoonsTear, { gGiMoonsTearItemDL, gGiMoonsTearGlowDL } },
    // GID_DEED_LAND, OBJECT_GI_RESERVE01
    { GetItem_DrawOpa01, { gGiTitleDeedEmptyDL, gGiTitleDeedLandColorDL } },
    // GID_MASK_DEKU, OBJECT_GI_NUTSMASK
    { GetItem_DrawOpa0Xlu1, { gGiDekuMaskEmptyDL, gGiDekuMaskDL } },
    // GID_REMAINS_ODOLWA, OBJECT_BSMASK
    { GetItem_DrawRemains, { gRemainsOdolwaDL, gRemainsOdolwaDL } },
    // GID_POWDER_KEG, OBJECT_GI_BIGBOMB
    { GetItem_DrawOpa0Xlu1, { gGiPowderKegBarrelDL, gGiPowderKegGoronSkullAndFuseDL } },
    // GID_GOLD_DUST, OBJECT_GI_GOLD_DUST
    { GetItem_DrawOpa01, { gGiGoldDustPowderDL, gGiGoldDustPowderEmptyDL } },
    // GID_SEAHORSE, OBJECT_GI_BOTTLE_16
    { GetItem_DrawSeahorseBottle, { gGiSeahorseBottleEmptyDL, gGiSeahorseBottleGlassAndCorkDL } },
    // GID_60, OBJECT_GI_BOTTLE_22
    { GetItem_DrawOpa0Xlu1, { gGiHylianLoachBottleContentsDL, gGiHylianLoachBottleGlassCorkWaterDL } },
    // GID_HYLIAN_LOACH, OBJECT_GI_LOACH
    { GetItem_DrawOpa0Xlu1, { gGiHylianLoachEmptyDL, gGiHylianLoachDL } },
    // GID_SEAHORSE_CAUGHT, OBJECT_GI_SEAHORSE
    { GetItem_DrawSeahorse, { gGiSeahorseEmptyDL, gGiSeahorseBodyDL, gGiSeahorseGlowDL, &gGiSeahorseBillboardRotMtx } },
    // GID_REMAINS_GOHT, OBJECT_BSMASK
    { GetItem_DrawRemains, { gRemainsGohtDL, gRemainsGohtDL } },
    // GID_REMAINS_GYORG, OBJECT_BSMASK
    { GetItem_DrawRemains, { gRemainsGyorgDL, gRemainsGyorgDL } },
    // GID_REMAINS_TWINMOLD, OBJECT_BSMASK
    { GetItem_DrawRemains, { gRemainsTwinmoldDL, gRemainsTwinmoldDL } },
    // GID_SWORD_RAZOR, OBJECT_GI_SWORD_2
    { GetItem_DrawOpa01, { gGiRazorSwordDL, gGiRazorSwordEmptyDL } },
    // GID_SWORD_GILDED, OBJECT_GI_SWORD_3
    { GetItem_DrawOpa01, { gGiGildedSwordDL, gGiGildedSwordEmptyDL } },
    // GID_SWORD_GREAT_FAIRY, OBJECT_GI_SWORD_4
    { GetItem_DrawOpa0Xlu1, { gGiGreatFairysSwordBladeDL, gGiGreatFairysSwordHiltEmblemDL } },
    // GID_MASK_GARO, OBJECT_GI_MASK09
    { GetItem_DrawOpa0Xlu1, { gGiGarosMaskCloakDL, gGiGarosMaskFaceDL } },
    // GID_MASK_GREAT_FAIRY, OBJECT_GI_MASK14
    { GetItem_DrawOpa0Xlu1, { gGiGreatFairyMaskFaceDL, gGiGreatFairyMaskLeavesDL } },
    // GID_MASK_GIBDO, OBJECT_GI_MASK15
    { GetItem_DrawOpa0Xlu1, { gGiGibdoMaskEmptyDL, gGiGibdoMaskDL } },
    // GID_ROOM_KEY, OBJECT_GI_RESERVE_B_00
    { GetItem_DrawOpa0Xlu1, { gGiRoomKeyEmptyDL, gGiRoomKeyDL } },
    // GID_LETTER_TO_KAFEI, OBJECT_GI_RESERVE_C_00
    { GetItem_DrawOpa0Xlu1, { gGiLetterToKafeiEnvelopeLetterDL, gGiLetterToKafeiInscriptionsDL } },
    // GID_PENDANT_OF_MEMORIES, OBJECT_GI_RESERVE_C_01
    { GetItem_DrawOpa0Xlu1, { gGiPendantOfMemoriesEmptyDL, gGiPendantOfMemoriesDL } },
    // GID_MUSHROOM, OBJECT_GI_MAGICMUSHROOM
    { GetItem_DrawOpa0Xlu1, { gGiMagicMushroomEmptyDL, gGiMagicMushroomDL } },
    // GID_MASK_ROMANI, OBJECT_GI_MASK10
    { GetItem_DrawOpa0Xlu1, { gGiRomaniMaskCapDL, gGiRomaniMaskNoseEyeDL } },
    // GID_MASK_POSTMAN, OBJECT_GI_MASK12
    { GetItem_DrawOpa0Xlu1, { gGiPostmanHatCapDL, gGiPostmanHatBunnyLogoDL } },
    // GID_MASK_GIANT, OBJECT_GI_MASK23
    { GetItem_DrawOpa01, { gGiGiantMaskEmptyDL, gGiGiantMaskDL } },
    // GID_CHATEAU, OBJECT_GI_BOTTLE_21
    { GetItem_DrawOpa0Xlu1, { gGiChateauRomaniBottleEmptyDL, gGiChateauRomaniBottleDL } },
    // GID_PICTOGRAPH_BOX, OBJECT_GI_CAMERA
    { GetItem_DrawOpa0Xlu1, { gGiPictoBoxFrameDL, gGiPictoBoxBodyAndLensDL } },
    // GID_MASK_FIERCE_DEITY, OBJECT_GI_MASK03
    { GetItem_DrawOpa01, { gGiFierceDeityMaskFaceDL, gGiFierceDeityMaskHairAndHatDL } },
    { GetItem_DrawBottleContents, { (void*)gComboMushroomBottleGi } },
    { GetItem_DrawBottleContents, { (void*)gComboPrincessBottleGi } },
    { GetItem_DrawBottleContents, { (void*)gComboGoldDustBottleGi } },
};

extern int ResourceMgr_IsModAsset(const char* path);
extern int ResourceMgr_IsModAssetForGame(const char* game, const char* path);
extern int ResourceMgr_IsCustomAssetForGame(const char* game, const char* path);
static ComboFairyBottleShell GetItem_EmptyBottleShell(s16 drawId) {
    ComboFairyBottleShell shell = { (const char*)sDrawItemTable[drawId].drawResources[0],
                                    (const char*)sDrawItemTable[drawId].drawResources[1] };
    if (shell.opaque != gGiEmptyBottleCorkDL || shell.glass != gGiEmptyBottleGlassDL) {
        return shell; // Other two-pass items retain their native recipe.
    }
    // Explicit empty-bottle mods retain their complete casing.
    return ComboFairyBottle_BundledShell(shell, shell.opaque, shell.glass, ResourceMgr_IsModAsset,
                                         ResourceMgr_FileExists("__OTR__objects/combo_bottle_gi/EmptyOpaque") &&
                                             ResourceMgr_FileExists("__OTR__objects/combo_bottle_gi/EmptyXlu"));
}

static ComboFairyBottleShell GetItem_FairyBottleShell(s16 drawId) {
    // Content models such as Blue Fire can bake a flame into their shell.
    // Reuse the empty bottle's selected casing without inheriting any content.
    return ComboFairyBottle_ReadableShell(GetItem_EmptyBottleShell(GID_BOTTLE), ResourceMgr_IsModAsset,
                                          ResourceMgr_FileExists("__OTR__objects/combo_bottle_gi/FairyXlu"));
}

#define COMBO_FAIRY_HOST_MM
#include "ComboFairyBottleDraw.h"
#undef COMBO_FAIRY_HOST_MM

/**
 * Draw "Get Item" Model
 * Calls the corresponding draw function for the given draw ID
 */
// Resolve an O2R display list by its __OTR__ path (defined in BenPort.cpp). Used to verify a get-item
// model actually loaded before we feed it to the interpreter.
extern Gfx* ResourceMgr_LoadGfxByName(const char* path);

// Palette resolution is independent of replacement assets. Both hosts use these helpers
// for the vanilla GI lists as well as the existing TP map/compass replacements.
s32 GetItem_GetDungeonItemTint(s16 drawId, s32 owner, Color_RGBA8* color, u8* strength) {
    static const Color_RGBA8 defaults[4] = {
        { 236, 120, 186, 255 },
        { 129, 173, 70, 255 },
        { 99, 90, 183, 255 },
        { 177, 165, 83, 255 },
    };
    static const char* ids[4] = { "Items.Woodfall", "Items.Snowhead", "Items.GreatBay", "Items.StoneTower" };
    if (owner < 0 || owner >= 4 || color == NULL || strength == NULL ||
        !CVarGetInteger("gEnhancements.DungeonItemColors", 0)) {
        return false;
    }
    switch (drawId) {
        case GID_KEY_SMALL:
        case GID_KEY_BOSS:
            *strength = 192; // Preserve the established neutral highlights.
            break;
        case GID_DUNGEON_MAP:
            *strength = 96; // Retain the parchment markings.
            break;
        case GID_COMPASS:
            *strength = 176;
            break;
        default:
            return false;
    }
    *color = CosmeticEditor_GetChangedColor(defaults[owner].r, defaults[owner].g, defaults[owner].b, 255, ids[owner]);
    return true;
}

// A vanilla boss key has a separate gem layer. An edited emblem row colors that
// layer too; untouched rows preserve the original native gem, even with no key pack.
s32 GetItem_GetDungeonKeyEmblemTint(s32 owner, Color_RGBA8* color) {
    static const Color_RGBA8 defaults[4] = {
        { 236, 120, 186, 255 },
        { 129, 173, 70, 255 },
        { 99, 90, 183, 255 },
        { 201, 38, 41, 255 },
    };
    static const char* ids[4] = { "Items.WoodfallEmblem", "Items.SnowheadEmblem", "Items.GreatBayEmblem",
                                  "Items.StoneTowerEmblem" };
    static const char* changed[4] = {
        "gCosmetic.Items.WoodfallEmblem.Changed",
        "gCosmetic.Items.SnowheadEmblem.Changed",
        "gCosmetic.Items.GreatBayEmblem.Changed",
        "gCosmetic.Items.StoneTowerEmblem.Changed",
    };
    if (owner < 0 || owner >= 4 || color == NULL || !CVarGetInteger(changed[owner], 0)) {
        return false;
    }
    *color = CosmeticEditor_GetChangedColor(defaults[owner].r, defaults[owner].g, defaults[owner].b, 255, ids[owner]);
    return true;
}

// The repaired key models belong to MM's bundled assets. Canonical paths also
// allow normal Alt replacements without requiring the retired POC3 archive.
extern bool ResourceMgr_IsAltAssetsEnabled(void);
extern u8 ResourceMgr_FileAltExists(const char* path);

s32 GetItem_GetDungeonKeyModel(s16 drawId, s32 owner, const char** metalPath, const char** emblemPath,
                               Color_RGBA8* metalColor, Color_RGBA8* emblemColor) {
    static const Color_RGBA8 metalDefaults[2] = { { 213, 224, 236, 255 }, { 233, 191, 66, 255 } };
    static const Color_RGBA8 emblemDefaults[4] = {
        { 236, 120, 186, 255 },
        { 129, 173, 70, 255 },
        { 99, 90, 183, 255 },
        { 201, 38, 41, 255 },
    };
    static const char* ids[4] = { "Items.Woodfall", "Items.Snowhead", "Items.GreatBay", "Items.StoneTower" };
    static const char* emblemIds[4] = { "Items.WoodfallEmblem", "Items.SnowheadEmblem", "Items.GreatBayEmblem",
                                        "Items.StoneTowerEmblem" };
    static const char* paths[4][2][2] = {
        { { "__OTR__objects/combo_dungeon_keys/WoodfallSmallKeyMetalDL",
            "__OTR__objects/combo_dungeon_keys/WoodfallSmallKeyEmblemDL" },
          { "__OTR__objects/combo_dungeon_keys/WoodfallBossKeyMetalDL",
            "__OTR__objects/combo_dungeon_keys/WoodfallBossKeyEmblemDL" } },
        { { "__OTR__objects/combo_dungeon_keys/SnowheadSmallKeyMetalDL",
            "__OTR__objects/combo_dungeon_keys/SnowheadSmallKeyEmblemDL" },
          { "__OTR__objects/combo_dungeon_keys/SnowheadBossKeyMetalDL",
            "__OTR__objects/combo_dungeon_keys/SnowheadBossKeyEmblemDL" } },
        { { "__OTR__objects/combo_dungeon_keys/GreatBaySmallKeyMetalDL",
            "__OTR__objects/combo_dungeon_keys/GreatBaySmallKeyEmblemDL" },
          { "__OTR__objects/combo_dungeon_keys/GreatBayBossKeyMetalDL",
            "__OTR__objects/combo_dungeon_keys/GreatBayBossKeyEmblemDL" } },
        { { "__OTR__objects/combo_dungeon_keys/StoneTowerSmallKeyMetalDL",
            "__OTR__objects/combo_dungeon_keys/StoneTowerSmallKeyEmblemDL" },
          { "__OTR__objects/combo_dungeon_keys/StoneTowerBossKeyMetalDL",
            "__OTR__objects/combo_dungeon_keys/StoneTowerBossKeyEmblemDL" } },
    };
    s32 kind;
    if (owner < 0 || owner >= 4 || metalPath == NULL || emblemPath == NULL || metalColor == NULL ||
        emblemColor == NULL) {
        return false;
    }
    if (drawId == GID_KEY_SMALL) {
        kind = 0;
    } else if (drawId == GID_KEY_BOSS) {
        kind = 1;
    } else {
        return false;
    }
    *metalPath = paths[owner][kind][0];
    *emblemPath = paths[owner][kind][1];
    const bool altEnabled = ResourceMgr_IsAltAssetsEnabled();
    if ((!ResourceMgr_FileExists(*metalPath) && !(altEnabled && ResourceMgr_FileAltExists(*metalPath))) ||
        (!ResourceMgr_FileExists(*emblemPath) && !(altEnabled && ResourceMgr_FileAltExists(*emblemPath)))) {
        return false;
    }
    *metalColor = metalDefaults[kind];
    *emblemColor = emblemDefaults[owner];
    if (CVarGetInteger("gEnhancements.DungeonItemColors", 0)) {
        // Unchanged rows keep neutral steel/gold; a selected dungeon hex colors the metal.
        *metalColor = CosmeticEditor_GetChangedColor(metalColor->r, metalColor->g, metalColor->b, 255, ids[owner]);
    }
    // Emblems have their own picker, independent of key metal and the general checkbox.
    *emblemColor =
        CosmeticEditor_GetChangedColor(emblemColor->r, emblemColor->g, emblemColor->b, 255, emblemIds[owner]);
    return true;
}

static s32 GetItem_TryDrawDungeonKey(PlayState* play, s16 drawId, s32 owner) {
    const char *metalPath, *emblemPath;
    Color_RGBA8 metalColor, emblemColor;
    Gfx *metal, *emblem;
    if (!GetItem_GetDungeonKeyModel(drawId, owner, &metalPath, &emblemPath, &metalColor, &emblemColor)) {
        return false;
    }
    metal = ResourceMgr_LoadGfxByName(metalPath);
    emblem = ResourceMgr_LoadGfxByName(emblemPath);
    if (metal == NULL || emblem == NULL) {
        return false;
    }
    Matrix_Push();
    // Quarter-unit detail is stored as integer vertices at 4x scale in the O2R.
    Matrix_Scale(0.25f, 0.25f, 0.25f, MTXMODE_APPLY);
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    gSPGrayscale(POLY_OPA_DISP++, false);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0x80, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_OPA_DISP++, metalColor.r, metalColor.g, metalColor.b, 255);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, metal);
    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetEnvColor(POLY_OPA_DISP++, emblemColor.r, emblemColor.g, emblemColor.b, 255);
    gSPDisplayList(POLY_OPA_DISP++, emblem);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);
    CLOSE_DISPS(play->state.gfxCtx);
    Matrix_Pop();
    return true;
}

s32 GetItem_DrawDungeonItem(PlayState* play, s16 drawId, s32 owner) {
    Color_RGBA8 color, emblem;
    u8 strength = 0;
    s32 tintBody, tintGem;
    if (GetItem_TryDrawDungeonKey(play, drawId, owner)) {
        return true;
    }
    tintBody = GetItem_GetDungeonItemTint(drawId, owner, &color, &strength);
    tintGem = drawId == GID_KEY_BOSS && GetItem_GetDungeonKeyEmblemTint(owner, &emblem);
    if ((!tintBody && !tintGem) ||
        ResourceMgr_LoadGfxByName((const char*)sDrawItemTable[drawId].drawResources[0]) == NULL) {
        return false;
    }
    OPEN_DISPS(play->state.gfxCtx);
    if (tintBody) {
        gDPSetGrayscaleColor(POLY_OPA_DISP++, color.r, color.g, color.b, strength);
        gSPGrayscale(POLY_OPA_DISP++, true);
    }
    if (tintGem) {
        gDPSetGrayscaleColor(POLY_XLU_DISP++, emblem.r, emblem.g, emblem.b, 192);
        gSPGrayscale(POLY_XLU_DISP++, true);
    }
    CLOSE_DISPS(play->state.gfxCtx);
    // The real table draw preserves selected GI geometry, materials and pass routing.
    sDrawItemTable[drawId].drawFunc(play, drawId);
    OPEN_DISPS(play->state.gfxCtx);
    gSPGrayscale(POLY_OPA_DISP++, false);
    if (tintGem) {
        gSPGrayscale(POLY_XLU_DISP++, false);
    }
    CLOSE_DISPS(play->state.gfxCtx);
    return true;
}

#define COMBO_MASK_SHIMMER_HOST_MM
#include "ComboMaskShimmer.h"
#undef COMBO_MASK_SHIMMER_HOST_MM

#define COMBO_COLLECTIBLE_SHIMMER_HOST_MM
#include "ComboCollectibleShimmerHost.h"
#undef COMBO_COLLECTIBLE_SHIMMER_HOST_MM

s32 GetItem_GetShimmerColor(s16 drawId, uint8_t color[4]) {
    const int collectible = ComboCollectibleShimmer_ColorForDrawId(drawId, color);
    if (collectible)
        return collectible;
    switch (drawId) {
        case GID_CHATEAU:
            color[0] = 0x4A;
            color[1] = 0x0C;
            color[2] = 0x41;
            color[3] = 255;
            return true;
        case GID_BOTTLE_MUSHROOM:
            return ComboBottleContents_Color(CW_BOTTLE_MUSHROOM, color);
        case GID_BOTTLE_DEKU_PRINCESS:
            return ComboBottleContents_Color(CW_BOTTLE_PRINCESS, color);
        case GID_BOTTLE_GOLD_DUST:
            return ComboBottleContents_Color(CW_BOTTLE_GOLD_DUST, color);
        case GID_SEAHORSE:
            return ComboBottleContents_Color(CW_BOTTLE_SEAHORSE, color);
        case GID_FAIRY:
        case GID_FAIRY_2:
            return ComboBottleShimmer_Color(CW_SHIMMER_FAIRY, color);
        case GID_MASK_POSTMAN:
            return ComboMmMaskShimmerColor(0, color);
        case GID_MASK_ALL_NIGHT:
            return ComboMmMaskShimmerColor(1, color);
        case GID_MASK_BLAST:
            return ComboMmMaskShimmerColor(2, color);
        case GID_MASK_STONE:
            return ComboMmMaskShimmerColor(3, color);
        case GID_MASK_GREAT_FAIRY:
            return ComboMmMaskShimmerColor(4, color);
        case GID_MASK_DEKU:
            return ComboMmMaskShimmerColor(5, color);
        case GID_MASK_KEATON:
            return ComboMmMaskShimmerColor(6, color);
        case GID_MASK_BREMEN:
            return ComboMmMaskShimmerColor(7, color);
        case GID_MASK_BUNNY:
            return ComboMmMaskShimmerColor(8, color);
        case GID_MASK_DON_GERO:
            return ComboMmMaskShimmerColor(9, color);
        case GID_MASK_SCENTS:
            return ComboMmMaskShimmerColor(10, color);
        case GID_MASK_GORON:
            return ComboMmMaskShimmerColor(11, color);
        case GID_MASK_ROMANI:
            return ComboMmMaskShimmerColor(12, color);
        case GID_MASK_CIRCUS_LEADER:
            return ComboMmMaskShimmerColor(13, color);
        case GID_MASK_KAFEIS_MASK:
            return ComboMmMaskShimmerColor(14, color);
        case GID_MASK_COUPLE:
            return ComboMmMaskShimmerColor(15, color);
        case GID_MASK_TRUTH:
            return ComboMmMaskShimmerColor(16, color);
        case GID_MASK_ZORA:
            return ComboMmMaskShimmerColor(17, color);
        case GID_MASK_KAMARO:
            return ComboMmMaskShimmerColor(18, color);
        case GID_MASK_GIBDO:
            return ComboMmMaskShimmerColor(19, color);
        case GID_MASK_GARO:
            return ComboMmMaskShimmerColor(20, color);
        case GID_MASK_CAPTAIN:
            return ComboMmMaskShimmerColor(21, color);
        case GID_MASK_GIANT:
            return ComboMmMaskShimmerColor(22, color);
        case GID_MASK_FIERCE_DEITY:
            return ComboMmMaskShimmerColor(23, color);
        case GID_REMAINS_ODOLWA:
            return ComboMmRemainsShimmerColor(0, color);
        case GID_REMAINS_GOHT:
            return ComboMmRemainsShimmerColor(1, color);
        case GID_REMAINS_GYORG:
            return ComboMmRemainsShimmerColor(2, color);
        case GID_REMAINS_TWINMOLD:
            return ComboMmRemainsShimmerColor(3, color);
        case GID_MASK_SUN:
            color[0] = 255;
            color[1] = 205;
            color[2] = 70;
            color[3] = 255;
            return true;
        default:
            return false;
    }
}

static void GetItem_DrawShimmer(PlayState* play, const uint8_t color[4]) {
    ComboDrawMaskShimmer(play, NULL, color, NULL);
}

static s32 GetItem_BottleShimmerColor(s16 drawId, Color_RGBA8* color) {
    switch (drawId) {
        case GID_POTION_RED:
            *color = CosmeticEditor_GetChangedColor(255, 70, 50, 255, "HUD.Hearts");
            return true;
        case GID_POTION_GREEN:
            *color = CosmeticEditor_GetChangedColor(0, 200, 0, 255, "HUD.Magic");
            return true;
        case GID_POTION_BLUE:
            *color = (Color_RGBA8){ 100, 160, 255, 255 };
            return true;
        case GID_FAIRY:
        case GID_FAIRY_2:
            *color = (Color_RGBA8){ 255, 160, 235, 255 };
            return true;
        case GID_POE:
            *color = (Color_RGBA8){ 100, 0, 200, 255 };
            return true;
        case GID_BIG_POE:
            *color = (Color_RGBA8){ 150, 200, 0, 255 };
            return true;
        default:
            return false;
    }
}

static void GetItem_DrawBottleShimmer(PlayState* play, s16 drawId) {
    Color_RGBA8 color;
    s32 i;

    if (!CVarGetInteger("gEnhancements.BottleShimmer", 0) || !GetItem_BottleShimmerColor(drawId, &color) ||
        ResourceMgr_LoadGfxByName(gEffSparklesDL) == NULL) {
        return;
    }
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    gSPClearGeometryMode(POLY_XLU_DISP++, G_FOG | G_LIGHTING);

    // Three local-space motes, evaluated from game time. Draw frequency and number of
    // visible bottles cannot advance an effect pool, consume RNG, or accelerate the shimmer.
    for (i = 0; i < 3; i++) {
        u32 phase = (play->gameplayFrames + i * 43) & 127;
        s16 angle = (s16)(play->gameplayFrames * 384 + i * 21845);
        f32 fade = (phase < 64 ? phase : 128 - phase) / 64.0f;
        f32 radius = (drawId == GID_FAIRY || drawId == GID_FAIRY_2) ? 23.0f : 29.0f;
        f32 scale = 0.055f + 0.04f * fade;

        Matrix_Push();
        Matrix_Translate(Math_SinS(angle) * radius, -28.0f + phase * 0.5f, Math_CosS(angle) * radius, MTXMODE_APPLY);
        Matrix_ReplaceRotation(&play->billboardMtxF);
        Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
        gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, (color.r + 765) / 4, (color.g + 765) / 4, (color.b + 765) / 4,
                        (u8)(112.0f * fade));
        gDPSetEnvColor(POLY_XLU_DISP++, color.r, color.g, color.b, 0);
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
        gSPDisplayList(POLY_XLU_DISP++, gEffSparklesDL);
        Matrix_Pop();
    }
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    CLOSE_DISPS(play->state.gfxCtx);
}

void GetItem_Draw(PlayState* play, s16 drawId) {
    if (DinFireShield_DrawItem(play, drawId))
        return;
    s32 owner = -1;
    Color_RGBA8 shimmerColor;
    s32 shimmer;
    uint8_t maskColor[4];
    s32 itemShimmer;
    // Guard against an out-of-range drawId and a get-item whose model resource does NOT resolve (a
    // missing/incompatible o2r, a missing asset, a rando/combo item without a resident object). Feeding
    // gSPDisplayList a display list the Fast3D interpreter can't resolve dereferences a bad address and
    // crashes 2ship with a 0xC0000005 access violation (this crashed in GetItem_DrawRecoveryHeart ->
    // object_gi_heart/gGiRecoveryHeartDL). Skip the draw instead of crashing — the item just doesn't
    // render, which is recoverable; a crash is not.
    if (drawId < 0 || drawId >= (s32)ARRAY_COUNT(sDrawItemTable) || sDrawItemTable[drawId].drawFunc == NULL) {
        return;
    }
    void* primaryDL = sDrawItemTable[drawId].drawResources[0];
    if (primaryDL != NULL && ResourceMgr_LoadGfxByName((const char*)primaryDL) == NULL) {
        return; // primary model didn't load -> don't hand the interpreter a bad DL
    }
    // Randomizer draws supply the true owner before this entry point. This fallback
    // covers ordinary dungeon rewards; an overworld minimap index is never an owner.
    switch (Play_GetOriginalSceneId(play->sceneId)) {
        case SCENE_MITURIN:
        case SCENE_MITURIN_BS:
            owner = 0;
            break;
        case SCENE_HAKUGIN:
        case SCENE_HAKUGIN_BS:
            owner = 1;
            break;
        case SCENE_SEA:
        case SCENE_SEA_BS:
            owner = 2;
            break;
        case SCENE_INISIE_N:
        case SCENE_INISIE_R:
        case SCENE_INISIE_BS:
            owner = 3;
            break;
    }
    if (GetItem_DrawDungeonItem(play, drawId, owner)) {
        return;
    }
    shimmer = CVarGetInteger("gEnhancements.BottleShimmer", 0) && GetItem_BottleShimmerColor(drawId, &shimmerColor);
    itemShimmer = GetItem_GetShimmerColor(drawId, maskColor);
    if (shimmer || itemShimmer) {
        Matrix_Push();
    }
    sDrawItemTable[drawId].drawFunc(play, drawId);
    if (shimmer || itemShimmer) {
        // Some fairy draws leave a billboard transform behind. Motes use the incoming
        // bottle transform, not the contents' billboard matrix.
        Matrix_Pop();
        if (shimmer) {
            GetItem_DrawBottleShimmer(play, drawId);
        }
        if (itemShimmer) {
            if (itemShimmer == CW_ITEM_SHIMMER_TINTED) {
                NeiGi_DrawTintedShimmerOverlay(play, maskColor, NULL);
            } else if (drawId == GID_CHATEAU) {
                NeiGi_DrawChateauShimmer(play);
            } else {
                GetItem_DrawShimmer(play, maskColor);
            }
        }
    }
}

#ifdef COMBO_BUILD
// ComboShip: expose one sDrawItemTable row for cross-game rendering (combo/menu/ComboItemDrawABI.h).
// outDlists is in submission order, *outXluStart is the first XLU entry (-1 = all OPA); non-portable
// funcs set *outDrawKind and carry the RAW table row. Returns 0 for unsupported rows.
// See docs/deviations/rando.md for what is and isn't portable and why.
s32 GetItem_GetDrawTableEntry(s32 drawId, void** outDlists, s32 maxDlists, s32* outXluStart, f32* outScale,
                              s32* outXluSeg8TexScroll, s32* outDrawKind) {
    // Mirror of CwDrawKind (ABI header is C++/POD; z_draw.c is C so keep local names in sync).
    enum {
        KIND_SIMPLE = 0,
        KIND_GORON_SWORD = 1,
        KIND_DEKU_NUTS = 2,
        KIND_RECOVERY_HEART = 3,
        KIND_FISH = 4,
        KIND_POTION = 5,
        KIND_POES = 8,
        KIND_MM_FAIRY_BOTTLE = 21,
        KIND_MM_FAIRY_CONTAINER = 37,
        KIND_ELEMENTAL_ARROW = 39,
        KIND_BOTTLE_CONTENTS = 40,
    };
    static const s8 sOrder0[] = { 0 };
    static const s8 sOrder01[] = { 0, 1 };
    static const s8 sOrder012[] = { 0, 1, 2 };
    static const s8 sOrder1023[] = { 1, 0, 2, 3 };
    static const s8 sOrder1032[] = { 1, 0, 3, 2 };
    static const s8 sOrderWallet[] = { 1, 0, 2, 3, 4, 5, 6, 7 };
    static const s8 sOrderRaw[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
    static const s8 sBottlePotion[] = { 1, 3, 5 };
    void (*drawFunc)(PlayState*, s16);
    void** res;
    void* seahorsePolish[] = { (void*)"__OTR__objects/combo_bottle_gi/SeahorseBottle" };
    const s8* order;
    s32 count;
    s32 xluStart;
    s32 kind = KIND_SIMPLE;
    s32 i;

    if ((drawId < 0) || (drawId >= ARRAY_COUNT(sDrawItemTable)) || (outDlists == NULL) || (outXluStart == NULL) ||
        (outScale == NULL)) {
        return 0;
    }
    *outScale = 0.0f; // 0 = no extra scale
    if (outXluSeg8TexScroll != NULL) {
        *outXluSeg8TexScroll = 0;
    }
    if (outDrawKind != NULL) {
        *outDrawKind = KIND_SIMPLE;
    }
    drawFunc = sDrawItemTable[drawId].drawFunc;
    res = sDrawItemTable[drawId].drawResources;
    const s32 isEmptyBottle = drawFunc == GetItem_DrawOpa0Xlu1 && (const char*)res[0] == gGiEmptyBottleCorkDL &&
                              (const char*)res[1] == gGiEmptyBottleGlassDL;
    if (isEmptyBottle && maxDlists < 2) {
        return 0;
    }

    if (drawFunc == GetItem_DrawRemains) {
        // Single OPA DL at 0.02 scale; see the portability note in the header comment.
        order = sOrder0;
        count = 1;
        xluStart = -1;
        *outScale = 0.02f;
    } else if (drawFunc == GetItem_DrawOpa0) {
        order = sOrder0;
        count = 1;
        xluStart = -1;
    } else if (drawFunc == GetItem_DrawOpa01) {
        order = sOrder01;
        count = 2;
        xluStart = -1;
    } else if (drawFunc == GetItem_DrawOpa0Xlu1) {
        order = sOrder01;
        count = 2;
        xluStart = 1;
    } else if (drawFunc == GetItem_DrawXlu01) {
        order = sOrder01;
        count = 2;
        xluStart = 0;
    } else if (drawFunc == GetItem_DrawCompass) {
        // The XLU layer really uses SETUPDL_5; the consumer's 25Xlu is a close approximation.
        order = sOrder01;
        count = 2;
        xluStart = 1;
    } else if (drawFunc == GetItem_DrawSeahorseBottle) {
        if (ResourceMgr_IsModAsset((const char*)res[0]) || ResourceMgr_IsModAsset((const char*)res[1])) {
            order = sOrder01;
            count = 2;
            xluStart = 1;
        } else {
            res = seahorsePolish;
            order = sOrder0;
            count = 1;
            xluStart = 0;
            kind = KIND_BOTTLE_CONTENTS;
        }
    } else if (drawFunc == GetItem_DrawBottleContents) {
        order = sOrder0;
        count = 1;
        xluStart = 0;
        kind = KIND_BOTTLE_CONTENTS;
    } else if (drawFunc == GetItem_DrawMagicArrow) {
        order = sOrder0;
        count = 1;
        xluStart = -1;
        kind = KIND_ELEMENTAL_ARROW;
    } else if (drawFunc == GetItem_DrawUpgrades) {
        order = sOrder1023;
        count = 4;
        xluStart = -1;
    } else if (drawFunc == GetItem_DrawWallet) {
        order = sOrderWallet;
        count = 8;
        xluStart = -1;
    } else if ((drawFunc == GetItem_DrawRupee) || (drawFunc == GetItem_DrawSmallRupee)) {
        // SmallRupee also applies a 0.7 model scale we cannot carry across; acceptable mismatch.
        order = sOrder1032;
        count = 4;
        xluStart = 2;
    } else if (drawFunc == GetItem_DrawSkullToken) {
        // Body (OPA) + flame (XLU). The flame draws with an animated segment-8 texture scroll; the
        // consumer replicates it (outXluSeg8TexScroll) so the flame renders across the cross-game frame.
        order = sOrder01;
        count = 2;
        xluStart = 1;
        if (outXluSeg8TexScroll != NULL) {
            *outXluSeg8TexScroll = 1;
        }
    } else if (drawFunc == GetItem_DrawMoonsTear) {
        // Static item (OPA) + glow (XLU). The AnimatedMat texture scroll + glow billboard aren't
        // carried in this DL list; the cross-game consumer replicates them (ComboForeignTexAnim_Run).
        order = sOrder01;
        count = 2;
        xluStart = 1;
    } else if (drawFunc == GetItem_DrawBombchu) {
        // Single OPA DL; the func's SETUPDL_23 is approximated as the consumer's 25Opa.
        order = sOrder0;
        count = 1;
        xluStart = -1;
    } else if (drawFunc == GetItem_DrawFairyContainer) {
        // Keep the native contents and its placement matrix; the consumer loads
        // the matrix through the owning MM resource manager before billboarding.
        kind = KIND_MM_FAIRY_CONTAINER;
        order = sOrderRaw;
        count = 4;
        xluStart = 1;
    } else if (drawFunc == GetItem_DrawGoronSword) {
        kind = KIND_GORON_SWORD;
        order = sOrderRaw;
        count = 1;
        xluStart = -1;
    } else if (drawFunc == GetItem_DrawDekuNuts) {
        kind = KIND_DEKU_NUTS;
        order = sOrderRaw;
        count = 1;
        xluStart = -1;
    } else if (drawFunc == GetItem_DrawRecoveryHeart) {
        kind = KIND_RECOVERY_HEART;
        order = sOrderRaw;
        count = 1;
        xluStart = 0;
    } else if (drawFunc == GetItem_DrawFish) {
        kind = KIND_FISH;
        order = sOrderRaw;
        count = 1;
        xluStart = 0;
    } else if (drawFunc == GetItem_DrawPotion) {
        if (ComboBottleGi_HasPotionRecipe((const char*)res[0], (const char*)res[3], (const char*)res[5],
                                          ResourceMgr_LoadGfxByName)) {
            if (maxDlists < 3) {
                return 0;
            }
            order = sBottlePotion;
            count = 3;
            xluStart = 0;
        } else {
            kind = KIND_POTION;
            order = sOrderRaw;
            count = 6;
            xluStart = 4;
        }
    } else if (drawFunc == GetItem_DrawPoes) {
        kind = KIND_POES;
        order = sOrderRaw;
        count = 4;
        xluStart = 1;
    } else if (drawFunc == GetItem_DrawFairyBottle) {
        kind = KIND_MM_FAIRY_BOTTLE;
        order = sOrderRaw;
        count = 3;
        xluStart = 1;
    } else {
        return 0;
    }

    if ((kind != KIND_SIMPLE) && ((outDrawKind == NULL) || (count > maxDlists))) {
        return 0; // a kind handler indexes the full raw row; truncating it would misdraw
    }
    if (count > maxDlists) {
        count = maxDlists;
    }
    for (i = 0; i < count; i++) {
        if (res[order[i]] == NULL) {
            return 0; // padded/unused rows (e.g. GID_37) are not drawable
        }
        outDlists[i] = res[order[i]];
    }
    if (kind == KIND_MM_FAIRY_BOTTLE || kind == KIND_MM_FAIRY_CONTAINER) {
        const ComboFairyBottleShell shell = GetItem_FairyBottleShell(drawId);
        outDlists[0] = (void*)shell.opaque;
        outDlists[1] = (void*)shell.glass;
    } else if (isEmptyBottle) {
        const ComboFairyBottleShell shell = GetItem_EmptyBottleShell(drawId);
        outDlists[0] = (void*)shell.opaque;
        outDlists[1] = (void*)shell.glass;
    }
    *outXluStart = (xluStart > count) ? count : xluStart;
    if (outDrawKind != NULL) {
        *outDrawKind = kind;
    }
    return count;
}

// ComboShip: the setup DL a row's draw func emits before its display lists, for the funcs that don't
// use plain 25. NULL = 25 Opa/Xlu (the consumer's own). The foreign consumer must submit the SAME
// setup: 23 is 1-CYCLE without fog, and a list authored for it renders through the wrong combiner
// under 2-cycle 25 — the second cycle wins and samples TEXEL1, i.e. the host's leftover tile.
void GetItem_GetDrawSetupDLs(s32 drawId, void** outOpa, void** outXlu) {
    void (*drawFunc)(PlayState*, s16);

    if (outOpa != NULL) {
        *outOpa = NULL;
    }
    if (outXlu != NULL) {
        *outXlu = NULL;
    }
    if ((drawId < 0) || (drawId >= (s32)ARRAY_COUNT(sDrawItemTable))) {
        return;
    }

    drawFunc = sDrawItemTable[drawId].drawFunc;
    if (drawFunc == GetItem_DrawBombchu) {
        if (outOpa != NULL) {
            *outOpa = gSetupDLs[SETUPDL_23];
        }
    } else if (drawFunc == GetItem_DrawCompass) {
        // Compass keeps 25 on OPA and draws its glass with SETUPDL_5.
        if (outXlu != NULL) {
            *outXlu = gSetupDLs[SETUPDL_5];
        }
    }
}
#endif

void GetItem_DrawBombchu(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL23_Opa(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[0]);

    CLOSE_DISPS(play->state.gfxCtx);
}

void GetItem_DrawPoes(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[0]);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[1]);

    gSPSegment(POLY_XLU_DISP++, 0x08,
               Gfx_TwoTexScrollEx(play->state.gfxCtx, G_TX_RENDERTILE, play->state.frames * 0, play->state.frames * 0,
                                  16, 32, 1, play->state.frames, -(play->state.frames * 6), 16, 32, 0, 0, 1, -6));

    Matrix_Push();
    Matrix_ReplaceRotation(&play->billboardMtxF);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[3]);
    gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[2]);

    Matrix_Pop();

    CLOSE_DISPS(play->state.gfxCtx);
}

void GetItem_DrawFairyBottle(PlayState* play, s16 drawId) {
    s32 pad;
    const ComboFairyBottleShell shell = GetItem_FairyBottleShell(drawId);
    const ComboFairyBottleMotion motion = ComboFairyBottle_SampleForShell(play->gameplayFrames, shell.opaque);

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)shell.opaque);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    if (strcmp(shell.opaque, shell.glass) != 0) {
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)shell.glass);
    }
    gSPSegment(POLY_XLU_DISP++, 0x08,
               Gfx_TwoTexScrollEx(play->state.gfxCtx, G_TX_RENDERTILE, play->state.frames * 0, play->state.frames * 0,
                                  32, 32, 1, play->state.frames, -(play->state.frames * 6), 32, 320, 0, 0, 1, -6));

    Matrix_Push();
    if (ComboFairyBottle_IsBlueFireShell(shell.opaque)) {
        Matrix_Translate(-8.0f, -2.0f, 0.0f, MTXMODE_APPLY);
    }
    Matrix_Translate(motion.x, motion.y, motion.z, MTXMODE_APPLY);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    Matrix_Scale(motion.scaleX, motion.scaleY, motion.scaleZ, MTXMODE_APPLY);

    if (!ComboFairyBottle_DrawVfx(play, shell.opaque)) {
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
        gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[2]);
    }

    Matrix_Pop();

    CLOSE_DISPS(play->state.gfxCtx);
    if (ComboFairyBottle_IsBundledShell(shell.opaque))
        ComboBottleShimmer_DrawMotes(play, CW_SHIMMER_FAIRY);
}

void GetItem_DrawSkullToken(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[0]);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    gSPSegment(POLY_XLU_DISP++, 0x08,
               Gfx_TwoTexScrollEx(play->state.gfxCtx, G_TX_RENDERTILE, play->state.frames * 0,
                                  -(play->state.frames * 5), 32, 32, 1, play->state.frames * 0, play->state.frames * 0,
                                  32, 64, 0, -5, 0, 0));
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[1]);

    CLOSE_DISPS(play->state.gfxCtx);
}

void GetItem_DrawCompass(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[0]);

    POLY_XLU_DISP = Gfx_SetupDL(POLY_XLU_DISP, SETUPDL_5);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[1]);

    CLOSE_DISPS(play->state.gfxCtx);
}

void GetItem_DrawPotion(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    if (ComboBottleGi_HasPotionRecipe(
            (const char*)sDrawItemTable[drawId].drawResources[0], (const char*)sDrawItemTable[drawId].drawResources[3],
            (const char*)sDrawItemTable[drawId].drawResources[5], ResourceMgr_LoadGfxByName)) {
        Gfx_SetupDL25_Xlu(play->state.gfxCtx);
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
        gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[1]);
        gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[3]);
        gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[5]);
    } else {
        Gfx_SetupDL25_Opa(play->state.gfxCtx);
        gSPSegment(POLY_OPA_DISP++, 0x08,
                   Gfx_TwoTexScrollEx(play->state.gfxCtx, G_TX_RENDERTILE, -play->state.frames, play->state.frames, 32,
                                      32, 1, -play->state.frames, play->state.frames, 32, 32, -1, 1, -1, 1));
        MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
        gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[1]);
        gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[0]);
        gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[2]);
        gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[3]);

        Gfx_SetupDL25_Xlu(play->state.gfxCtx);
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
        gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[4]);
        gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[5]);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

void GetItem_DrawGoronSword(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    gSPSegment(POLY_OPA_DISP++, 0x08,
               Gfx_TwoTexScrollEx(play->state.gfxCtx, G_TX_RENDERTILE, play->state.frames * 1, play->state.frames * 0,
                                  32, 32, 1, play->state.frames * 0, play->state.frames * 0, 32, 32, 1, 0, 0, 0));
    NeiGi_ReportBlackMarketModel("mm", (const char*)sDrawItemTable[drawId].drawResources[0], NULL);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[0]);

    CLOSE_DISPS(play->state.gfxCtx);
}

void GetItem_DrawDekuNuts(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    gSPSegment(POLY_OPA_DISP++, 0x08,
               Gfx_TwoTexScrollEx(play->state.gfxCtx, G_TX_RENDERTILE, play->state.frames * 6, play->state.frames * 6,
                                  32, 32, 1, play->state.frames * 6, play->state.frames * 6, 32, 32, 6, 6, 6, 6));
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[0]);

    CLOSE_DISPS(play->state.gfxCtx);
}

// Custom models do not share the native command offsets used by Cosmetic Editor.
// Resolve through the display-list bridge first so the first Alt draw is classified
// correctly, then scope the live MM cosmetic tint to this body draw only.
Gfx* GetItem_DrawDListWithCosmetics(Gfx* gfx, const char* dlist, s16 drawId) {
    Gfx drawCommand;
    const char* cosmeticId = NULL;
    Color_RGBA8 color;

    gSPDisplayList(&drawCommand, (Gfx*)dlist);

    if ((drawId == GID_MAGIC_JAR_SMALL || drawId == GID_MAGIC_JAR_BIG) &&
        CVarGetInteger(CVAR_COSMETIC_CHANGED("HUD.Magic"), 0) && ResourceGetIsCustomByName(dlist)) {
        cosmeticId = "HUD.Magic";
        color = CosmeticEditor_GetChangedColor(0, 200, 0, 255, cosmeticId);
    } else if ((drawId == GID_HEART_PIECE || drawId == GID_HEART_CONTAINER || drawId == GID_RECOVERY_HEART) &&
               CVarGetInteger(CVAR_COSMETIC_CHANGED("HUD.Hearts"), 0) && ResourceGetIsCustomByName(dlist)) {
        cosmeticId = "HUD.Hearts";
        color = CosmeticEditor_GetChangedColor(255, 70, 50, 255, cosmeticId);
    }

    if (cosmeticId != NULL) {
        gDPSetGrayscaleColor(gfx++, color.r, color.g, color.b, 255);
        gSPGrayscale(gfx++, true);
    }
    *gfx++ = drawCommand;
    if (cosmeticId != NULL) {
        gSPGrayscale(gfx++, false);
    }
    return gfx;
}

void GetItem_DrawRecoveryHeart(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    gSPSegment(POLY_XLU_DISP++, 0x08,
               Gfx_TwoTexScrollEx(play->state.gfxCtx, G_TX_RENDERTILE, play->state.frames * 0,
                                  -(play->state.frames * 3), 32, 32, 1, play->state.frames * 0,
                                  -(play->state.frames * 2), 32, 32, 0, -3, 0, -2));
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    POLY_XLU_DISP =
        GetItem_DrawDListWithCosmetics(POLY_XLU_DISP, (const char*)sDrawItemTable[drawId].drawResources[0], drawId);

    CLOSE_DISPS(play->state.gfxCtx);
}

void GetItem_DrawFish(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    gSPSegment(POLY_XLU_DISP++, 0x08,
               Gfx_TwoTexScrollEx(play->state.gfxCtx, G_TX_RENDERTILE, play->state.frames * 0, play->state.frames * 1,
                                  32, 32, 1, play->state.frames * 0, play->state.frames * 1, 32, 32, 0, 1, 0, 1));
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[0]);

    CLOSE_DISPS(play->state.gfxCtx);
}

/**
 * Draw `drawResources[0]` as Opa
 */
void GetItem_DrawOpa0(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    NeiGi_ReportBlackMarketModel("mm", (const char*)sDrawItemTable[drawId].drawResources[0], NULL);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    POLY_OPA_DISP =
        GetItem_DrawDListWithCosmetics(POLY_OPA_DISP, (const char*)sDrawItemTable[drawId].drawResources[0], drawId);

    CLOSE_DISPS(play->state.gfxCtx);
}

/**
 * Draw `drawResources[0]` as Opa and `drawResources[1]` as Xlu
 */
void GetItem_DrawOpa0Xlu1(PlayState* play, s16 drawId) {
    s32 pad;
    const ComboFairyBottleShell shell = GetItem_EmptyBottleShell(drawId);

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    NeiGi_ReportBlackMarketModel("mm", shell.opaque, shell.glass);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)shell.opaque);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)shell.glass);

    CLOSE_DISPS(play->state.gfxCtx);
}

/**
 * Draw `drawResources[0]` and `drawResources[1]` as Opa
 */
void GetItem_DrawOpa01(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    NeiGi_ReportBlackMarketModel("mm", (const char*)sDrawItemTable[drawId].drawResources[0],
                                 (const char*)sDrawItemTable[drawId].drawResources[1]);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[0]);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[1]);

    CLOSE_DISPS(play->state.gfxCtx);
}

/**
 * Draw `drawResources[0]` and `drawResources[1]` as Xlu
 */
void GetItem_DrawXlu01(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    NeiGi_ReportBlackMarketModel("mm", (const char*)sDrawItemTable[drawId].drawResources[0],
                                 (const char*)sDrawItemTable[drawId].drawResources[1]);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[0]);
    POLY_XLU_DISP =
        GetItem_DrawDListWithCosmetics(POLY_XLU_DISP, (const char*)sDrawItemTable[drawId].drawResources[1], drawId);

    CLOSE_DISPS(play->state.gfxCtx);
}

void GetItem_DrawSeahorse(PlayState* play, s16 drawId) {
    s32 pad;
    MtxF mtx;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[0]);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[1]);

    Matrix_MtxToMtxF(ResourceMgr_LoadMtxByName(Lib_SegmentedToVirtual(sDrawItemTable[drawId].drawResources[3])), &mtx);
    Matrix_Mult(&mtx, MTXMODE_APPLY);
    Matrix_ReplaceRotation(&play->billboardMtxF);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[2]);

    CLOSE_DISPS(play->state.gfxCtx);
}

void GetItem_DrawFairyContainer(PlayState* play, s16 drawId) {
    s32 pad;
    MtxF mtx;
    const ComboFairyBottleShell shell = GetItem_FairyBottleShell(drawId);
    const ComboFairyBottleMotion motion = ComboFairyBottle_SampleForShell(play->gameplayFrames, shell.opaque);

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    AnimatedMat_Draw(play, Lib_SegmentedToVirtual(gGiFairyBottleTexAnim));

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)shell.opaque);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    if (strcmp(shell.opaque, shell.glass) != 0) {
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)shell.glass);
    }

    Matrix_MtxToMtxF(ResourceMgr_LoadMtxByName(Lib_SegmentedToVirtual(sDrawItemTable[drawId].drawResources[3])), &mtx);
    Matrix_Push();
    Matrix_Translate(motion.x, motion.y, motion.z, MTXMODE_APPLY);
    if (ComboFairyBottle_IsBlueFireShell(shell.opaque)) {
        Matrix_Translate(-8.0f, -2.0f, 0.0f, MTXMODE_APPLY);
    } else if (!ComboFairyBottle_IsBundledShell(shell.opaque)) {
        // The native billboard matrix sizes a sprite, not an actor skeleton.
        // Keep its anchor for the VFX; apply its full scale only to fallback contents.
        Matrix_Translate(mtx.xw, mtx.yw, mtx.zw, MTXMODE_APPLY);
    }
    Matrix_ReplaceRotation(&play->billboardMtxF);
    Matrix_Scale(motion.scaleX, motion.scaleY, motion.scaleZ, MTXMODE_APPLY);

    if (!ComboFairyBottle_DrawVfx(play, shell.opaque)) {
        Matrix_Pop();
        Matrix_Push();
        Matrix_Translate(motion.x, motion.y, motion.z, MTXMODE_APPLY);
        if (ComboFairyBottle_IsBlueFireShell(shell.opaque)) {
            Matrix_Translate(-8.0f, -2.0f, 0.0f, MTXMODE_APPLY);
        } else {
            Matrix_Mult(&mtx, MTXMODE_APPLY);
        }
        Matrix_ReplaceRotation(&play->billboardMtxF);
        Matrix_Scale(motion.scaleX, motion.scaleY, motion.scaleZ, MTXMODE_APPLY);
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
        gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[2]);
    }
    Matrix_Pop();

    CLOSE_DISPS(play->state.gfxCtx);
    if (ComboFairyBottle_IsBundledShell(shell.opaque))
        ComboBottleShimmer_DrawMotes(play, CW_SHIMMER_FAIRY);
}

void GetItem_DrawMoonsTear(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    Gfx_SetupDL25_Xlu(play->state.gfxCtx);
    AnimatedMat_Draw(play, Lib_SegmentedToVirtual(gGiMoonsTearTexAnim));

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[0]);

    Matrix_ReplaceRotation(&play->billboardMtxF);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[1]);

    CLOSE_DISPS(play->state.gfxCtx);
}

void GetItem_DrawMagicArrow(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[0]);

    CLOSE_DISPS(play->state.gfxCtx);
    NeiGi_DrawElementalArrow(play, NeiArrowGi_ProfileForDrawId(drawId, GID_ARROW_FIRE, GID_ARROW_ICE, GID_ARROW_LIGHT));
}

void GetItem_DrawUpgrades(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[1]);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[0]);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[2]);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[3]);

    CLOSE_DISPS(play->state.gfxCtx);
}

void GetItem_DrawRupee(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[1]);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[0]);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[3]);
    gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[2]);

    CLOSE_DISPS(play->state.gfxCtx);
}

void GetItem_DrawSmallRupee(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Matrix_Scale(0.7f, 0.7f, 0.7f, MTXMODE_APPLY);
    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[1]);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[0]);

    Gfx_SetupDL25_Xlu(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[3]);
    gSPDisplayList(POLY_XLU_DISP++, sDrawItemTable[drawId].drawResources[2]);

    CLOSE_DISPS(play->state.gfxCtx);
}

void GetItem_DrawWallet(PlayState* play, s16 drawId) {
    s32 pad;

    OPEN_DISPS(play->state.gfxCtx);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[1]);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[0]);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[2]);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[3]);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[4]);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[5]);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[6]);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[7]);

    CLOSE_DISPS(play->state.gfxCtx);
}

void GetItem_DrawRemains(PlayState* play, s16 drawId) {
    s32 pad;
    s8 objectSlot;

    OPEN_DISPS(play->state.gfxCtx);

    objectSlot = Object_GetSlot(&play->objectCtx, OBJECT_BSMASK);

    gSPSegment(POLY_OPA_DISP++, 0x06, play->objectCtx.slots[objectSlot].segment);

    Gfx_SetupDL25_Opa(play->state.gfxCtx);
    Matrix_Scale(0.02f, 0.02f, 0.02f, MTXMODE_APPLY);

    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, play->state.gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, sDrawItemTable[drawId].drawResources[0]);

    POLY_OPA_DISP = Play_SetFog(play, POLY_OPA_DISP);

    CLOSE_DISPS(play->state.gfxCtx);
}

// Selected native Seahorse art owns both shell passes; the accepted built-in
// profile is otherwise shared with foreign consumers through its marker recipe.
void GetItem_DrawSeahorseBottle(PlayState* play, s16 drawId) {
#ifdef COMBO_BUILD
    if (!ResourceMgr_IsModAsset((const char*)sDrawItemTable[drawId].drawResources[0]) &&
        !ResourceMgr_IsModAsset((const char*)sDrawItemTable[drawId].drawResources[1]) &&
        ComboBottleContents_Draw(play, CW_BOTTLE_SEAHORSE))
        return;
#endif
    GetItem_DrawOpa0Xlu1(play, drawId);
}

void GetItem_DrawBottleContents(PlayState* play, s16 drawId) {
#ifdef COMBO_BUILD
    if (!ComboBottleContents_Draw(play,
                                  ComboBottleContents_Profile((const char*)sDrawItemTable[drawId].drawResources[0])))
#endif
        GetItem_DrawOpa0Xlu1(play, GID_BOTTLE);
}
