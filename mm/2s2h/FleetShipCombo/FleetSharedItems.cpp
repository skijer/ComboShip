#include "FleetSharedItems.h"
#include "FleetComboItems.h"
#include "FleetComboItemsGlue.h"
#include "2s2h/Rando/Types.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/ShipInit.hpp"

#include <spdlog/spdlog.h>
#ifdef _WIN32
#include <windows.h>
#endif

extern "C" {
#include "z64.h"
#include "variables.h"
extern PlayState* gPlayState;
void FleetSync_ApplySharedState(const char* json); // FleetSync.cpp
}

static int sReceiveDepth = 0;
static bool sPullPending = false;

extern "C" void FleetShared_RequestPullFromPeer(void) {
    sPullPending = true;
}

extern "C" void FleetShared_BeginReceive(void) {
    sReceiveDepth++;
}

extern "C" void FleetShared_EndReceive(void) {
    if (sReceiveDepth > 0) {
        sReceiveDepth--;
    }
}

extern "C" int FleetShared_IsReceiving(void) {
    return sReceiveDepth > 0;
}

#ifdef COMBO_BUILD
// The give choke sees the CONVERTED tier (Rando::ConvertItem), but the FC table pairs the chain, so
// each tier is folded back onto the chain that owns it. Mirrors ConvertItem's chain cases.
static const struct {
    RandoItemId tier;
    RandoItemId chain;
} kChainAliases[] = {
    { RI_WALLET_ADULT, RI_PROGRESSIVE_WALLET },
    { RI_WALLET_GIANT, RI_PROGRESSIVE_WALLET },
    { RI_WALLET_TYCOON, RI_PROGRESSIVE_WALLET },
    { RI_SWORD_KOKIRI, RI_PROGRESSIVE_SWORD },
    { RI_SWORD_RAZOR, RI_PROGRESSIVE_SWORD },
    { RI_SWORD_GILDED, RI_PROGRESSIVE_SWORD },
    { RI_BOMB_BAG_20, RI_PROGRESSIVE_BOMB_BAG },
    { RI_BOMB_BAG_30, RI_PROGRESSIVE_BOMB_BAG },
    { RI_BOMB_BAG_40, RI_PROGRESSIVE_BOMB_BAG },
    { RI_BOW, RI_PROGRESSIVE_BOW },
    { RI_QUIVER_40, RI_PROGRESSIVE_BOW },
    { RI_QUIVER_50, RI_PROGRESSIVE_BOW },
    { RI_SINGLE_MAGIC, RI_PROGRESSIVE_MAGIC },
    { RI_DOUBLE_MAGIC, RI_PROGRESSIVE_MAGIC },
    { RI_OOT_HAMMER, RI_OOT_PROGRESSIVE_HAMMER },
    { RI_OOT_IRON_KNUCKLE_AXE, RI_OOT_PROGRESSIVE_HAMMER },
    { RI_OOT_MASTER_SWORD, RI_OOT_PROGRESSIVE_MASTER_SWORD },
    { RI_OOT_TRUE_MASTER_SWORD, RI_OOT_PROGRESSIVE_MASTER_SWORD },
    { RI_OOT_BIGGORON_SWORD, RI_OOT_PROGRESSIVE_BGS },
    { RI_OOT_GORONS_BRACELET, RI_OOT_PROGRESSIVE_STRENGTH },
    { RI_OOT_SILVER_GAUNTLETS, RI_OOT_PROGRESSIVE_STRENGTH },
    { RI_OOT_GOLDEN_GAUNTLETS, RI_OOT_PROGRESSIVE_STRENGTH },
    { RI_OOT_NEI_ROCS_FEATHER, RI_OOT_PROGRESSIVE_ROC },
    { RI_OOT_NEI_ROCS_CAPE, RI_OOT_PROGRESSIVE_ROC },
    { RI_OOT_QUARTZ_OF_MOTION, RI_OOT_STONE_OF_AGONY },
};

static int ChainAliasFor(int nativeId) {
    for (const auto& alias : kChainAliases) {
        if (alias.tier == nativeId) {
            return alias.chain;
        }
    }
    return 0;
}

typedef void (*FnGrantSharedItem)(const char*);

// soh.dll is loaded in this process by the launcher before either game boots; resolve its export
// lazily so this module never depends on link order.
static FnGrantSharedItem ResolvePeerGrant() {
    static FnGrantSharedItem sGrant = nullptr;
    static bool sTried = false;
    if (!sTried) {
        sTried = true;
        if (HMODULE peer = GetModuleHandleA("soh.dll")) {
            sGrant = (FnGrantSharedItem)GetProcAddress(peer, "SOH_GrantSharedItem");
        }
    }
    return sGrant;
}

extern "C" void FleetShared_OnNativeObtained(int nativeId) {
    if (sReceiveDepth > 0) {
        return;
    }
    int chain = ChainAliasFor(nativeId);
    int fcId = FcCombo_ItemForNative(chain != 0 ? chain : nativeId);
    if (fcId == FCI_NO_ITEM || fcId < 0 || fcId >= FC_COMBO_ITEM_COUNT) {
        return;
    }
    // soh resolves cross grants by its English itemTable name, which is what the FC row carries.
    const char* ootName = gFcComboItems[fcId].ootName;
    if (ootName == nullptr || ootName[0] == '\0') {
        return;
    }
    FnGrantSharedItem grant = ResolvePeerGrant();
    if (grant == nullptr) {
        SPDLOG_WARN("[FleetShared] soh.dll has no SOH_GrantSharedItem; '{}' not shared", ootName);
        return;
    }
    SPDLOG_INFO("[FleetShared] MM obtained fc {} -> OoT '{}'", fcId, ootName);
    grant(ootName);
}

typedef const char* (*FnExtractSharedState)(void);

// Deferred to a tick with a loaded file: ApplyShared writes the live save, and the switch that
// requested the pull happens before this game's PlayState exists again.
static void PullFromPeerWhenReady() {
    if (!sPullPending || gPlayState == NULL || gSaveContext.fileNum < 0 || gSaveContext.fileNum > 2) {
        return;
    }
    sPullPending = false;
    static FnExtractSharedState sExtract = nullptr;
    static bool sTried = false;
    if (!sTried) {
        sTried = true;
        if (HMODULE peer = GetModuleHandleA("soh.dll")) {
            sExtract = (FnExtractSharedState)GetProcAddress(peer, "SOH_ExtractSharedState");
        }
    }
    if (sExtract == nullptr) {
        SPDLOG_WARN("[FleetShared] soh.dll has no SOH_ExtractSharedState; no reconciliation");
        return;
    }
    FleetShared_BeginReceive();
    FleetSync_ApplySharedState(sExtract());
    FleetShared_EndReceive();
    SPDLOG_INFO("[FleetShared] pulled OoT's shared state into MM");
}

static void RegisterFleetSharedItems() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameStateUpdate>(PullFromPeerWhenReady);
}

static RegisterShipInitFunc initFleetSharedItems(RegisterFleetSharedItems, {});
#else
extern "C" void FleetShared_OnNativeObtained(int nativeId) {
    (void)nativeId;
}
#endif
