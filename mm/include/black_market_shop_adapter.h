#ifndef BLACK_MARKET_SHOP_ADAPTER_H
#define BLACK_MARKET_SHOP_ADAPTER_H

#include "global.h"

struct EnOssan;

// Only the private Black Market provides this adapter. The original actor owns
// interaction, input, cursor, item tween, cutscene and purchase state transitions.
typedef struct ComboBlackMarketShopAdapter {
    // Called from native InitShop after shared UI/cutscene initialization.
    // Initialize presentation, blinkFunc and eight typed stock slots; 0 kills.
    s32 (*init)(struct EnOssan*, PlayState*);
    s32 (*canOfferTalk)(struct EnOssan*, PlayState*);
    // Accepted talk has not hidden the player yet; 0 closes it and stays idle.
    s32 (*beginTalk)(struct EnOssan*, PlayState*);
    u16 (*resolveText)(struct EnOssan*, u16);
    s32 (*canStartCutscene)(struct EnOssan*, PlayState*);
    void (*cutsceneStarted)(struct EnOssan*, PlayState*);
    // Supply shelf origin and selected position; native tween math is shared.
    void (*itemPositions)(struct EnOssan*, s32, Vec3f*, Vec3f*);
    // Reserve once before native camera/UI changes. The adapter owns delivery,
    // payment and persistence; native stock callbacks must not grant or charge.
    s32 (*offerReceipt)(struct EnOssan*, PlayState*);
    // -1: cancelled, 0: pending, 1: acknowledged delivery.
    s32 (*receiptAccepted)(struct EnOssan*, PlayState*);
    // True only after the entire receipt/tutorial/choice chain has finished.
    s32 (*receiptComplete)(struct EnOssan*, PlayState*);
    // Optional private conversation while the native action faces the keeper.
    // Return true to resume the native keeper menu; false retains this action.
    s32 (*keeperDialogue)(struct EnOssan*, PlayState*);
    // Private OoT shop camera: pivot the existing owned camera and report when
    // its native turn completes. NULL retains MM's linked-cutscene behavior.
    s32 (*turnCamera)(struct EnOssan*, PlayState*, f32 targetDegrees);
} ComboBlackMarketShopAdapter;

#ifdef __cplusplus
extern "C" {
#endif
#ifdef COMBO_BUILD
const ComboBlackMarketShopAdapter* ComboBlackMarketShop_GetAdapter(struct EnOssan*, PlayState*);
#endif
#ifdef __cplusplus
}
#endif

#endif
