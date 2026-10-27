#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/Rando/ItemReceiptText.h"
#include "2s2h/Rando/Rando.h"
#include "2s2h/ShipInit.hpp"
#include "libultraship/bridge/consolevariablebridge.h"

namespace {
void BuildNativeRandomRupeeName(u16* textId, bool* loadFromMessageTable) {
    if (!textId || !loadFromMessageTable || !*loadFromMessageTable || !gPlayState ||
        !gPlayState->msgCtx.messageTableNES || gSaveContext.fileNum == 0xFF || !IS_RANDO ||
        !CVarGetInteger("gRandoEnhancements.RandomizeRupeeNames", 1))
        return;
    auto entry = CustomMessage::LoadVanillaMessageTableEntry(*textId);
    if (!Rando::ApplyNativeRandomRupeeReceipt(*textId, entry))
        return;
    CustomMessage::LoadCustomMessageIntoFont(std::move(entry));
    *loadFromMessageTable = false;
}

void RegisterNativeRandomRupeeNames() {
    // Exact native award IDs; settings/save guards remain live in the callback.
    COND_ID_HOOK(OnOpenText, 0xC4, true, BuildNativeRandomRupeeName);
    COND_ID_HOOK(OnOpenText, 0x2, true, BuildNativeRandomRupeeName);
    COND_ID_HOOK(OnOpenText, 0x3, true, BuildNativeRandomRupeeName);
    COND_ID_HOOK(OnOpenText, 0x4, true, BuildNativeRandomRupeeName);
    COND_ID_HOOK(OnOpenText, 0x5, true, BuildNativeRandomRupeeName);
    COND_ID_HOOK(OnOpenText, 0x6, true, BuildNativeRandomRupeeName);
    COND_ID_HOOK(OnOpenText, 0x7, true, BuildNativeRandomRupeeName);
}
static RegisterShipInitFunc initFunc(RegisterNativeRandomRupeeNames);
} // namespace
