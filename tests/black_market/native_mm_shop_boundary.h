#ifndef NATIVE_MM_SHOP_TEST_BOUNDARY_H
#define NATIVE_MM_SHOP_TEST_BOUNDARY_H
#include "black_market_shop_adapter.h"
#include "global.h"
#include "overlays/actors/ovl_En_Ossan/z_en_ossan.h"
#ifdef __cplusplus
extern "C" {
#endif
u8 FixtureMessageState(MessageContext *);
s32 FixtureAdvance(PlayState *);
void FixtureContinue(PlayState *, u16);
void FixtureStart(PlayState *, u16, Actor *);
void FixtureClose(PlayState *);
s32 FixtureTalk(Actor *, GameState *);
s32 FixtureOfferTalk(Actor *, PlayState *, f32);
void FixtureSkeleton(PlayState *, SkelAnime *, FlexSkeletonHeader *,
                     AnimationHeader *, Vec3s *, Vec3s *, s32);
void EnOssan_Init(Actor *, PlayState *);
void EnOssan_InitShop(EnOssan *, PlayState *);
void EnOssan_Update(Actor *, PlayState *);
void EnOssan_Idle(EnOssan *, PlayState *);
void EnOssan_BeginInteraction(EnOssan *, PlayState *);
void EnOssan_Hello(EnOssan *, PlayState *);
void EnOssan_StartShopping(PlayState *, EnOssan *);
void EnOssan_FaceShopkeeper(EnOssan *, PlayState *);
void EnOssan_TalkToShopkeeper(EnOssan *, PlayState *);
void EnOssan_LookToLeftShelf(EnOssan *, PlayState *);
void EnOssan_LookToRightShelf(EnOssan *, PlayState *);
void EnOssan_BrowseLeftShelf(EnOssan *, PlayState *);
void EnOssan_BrowseRightShelf(EnOssan *, PlayState *);
void EnOssan_LookToShopkeeperFromShelf(EnOssan *, PlayState *);
void EnOssan_SelectItem(EnOssan *, PlayState *);
void EnOssan_CannotBuy(EnOssan *, PlayState *);
void EnOssan_CanBuy(EnOssan *, PlayState *);
void EnOssan_BuyItemWithFanfare(EnOssan *, PlayState *);
void EnOssan_SetupItemPurchased(EnOssan *, PlayState *);
void EnOssan_ItemPurchased(EnOssan *, PlayState *);
void EnOssan_ContinueShopping(EnOssan *, PlayState *);
void EnOssan_WaitForBlink(EnOssan *);
void EnOssan_DrawCursor(PlayState *, EnOssan *, f32, f32, f32, u8);
void EnOssan_DrawStickDirectionPrompts(PlayState *, EnOssan *);
void EnOssan_EndInteraction(PlayState *, EnOssan *);
void EnGirlA_DoNothing(EnGirlA *, PlayState *);
void EnGirlA_Update2(EnGirlA *, PlayState *);
void EnGirlA_Update(Actor *, PlayState *);
#ifdef __cplusplus
}
#endif
#define Message_GetState FixtureMessageState
#define Message_ShouldAdvance FixtureAdvance
#define Message_ContinueTextbox FixtureContinue
#define Message_StartTextbox FixtureStart
#define Message_CloseTextbox FixtureClose
#define Actor_TalkOfferAccepted FixtureTalk
#define Actor_OfferTalk FixtureOfferTalk
#define SkelAnime_InitFlex FixtureSkeleton
#define SkelAnime_DrawFlexOpa(...) ((void)0)
#endif
