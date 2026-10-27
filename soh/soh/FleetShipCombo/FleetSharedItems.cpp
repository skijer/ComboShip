#include "../../../combo/ComboGameMode.h"
#include "FleetSharedItems.h"
#include "FleetComboItems.h"
#include "FleetComboItemsGlue.h"
#include "FleetComboRando.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/ShipInit.hpp"

#include <cstring>
#include <spdlog/spdlog.h>
#ifdef COMBO_BUILD
#include "ComboResolve.h"
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
    if (!Combo_IsCombinedGame() || sReceiveDepth > 0) {
        sReceiveDepth--;
    }
}

extern "C" int FleetShared_IsReceiving(void) {
    return sReceiveDepth > 0;
}

#ifdef COMBO_BUILD
typedef void (*FnGrantSharedItem)(const char*);

static void ShareNativeMagicFloor(int tier) {
    typedef void (*FnMagicFloor)(int);
    static FnMagicFloor apply = nullptr;
    if (apply == nullptr)
        apply = reinterpret_cast<FnMagicFloor>(Combo_ResolveSym("2ship", "MM_ApplySharedMagicFloor"));
    if (apply != nullptr)
        apply(tier);
}

// 2ship.dll is loaded in this process by the launcher before either game boots; resolve its export
// lazily so this module never depends on link order.
static FnGrantSharedItem ResolvePeerGrant() {
    static FnGrantSharedItem sGrant = nullptr;
    static bool sTried = false;
    if (!sTried) {
        sTried = true;
        sGrant = reinterpret_cast<FnGrantSharedItem>(Combo_ResolveSym("2ship", "MM_GrantSharedItem"));
    }
    return sGrant;
}

extern "C" void FleetShared_OnNativeObtained(int nativeId) {
    if (sReceiveDepth > 0) {
        return;
    }
    // Native magic is an absolute capacity floor. A progressive peer grant can
    // infer another tier from RPG-derived flags or a repeated delivery.
    if (nativeId == RG_MAGIC_SINGLE || nativeId == RG_MAGIC_DOUBLE) {
        ShareNativeMagicFloor(nativeId == RG_MAGIC_DOUBLE ? 2 : 1);
        return;
    }
    int chain = FleetCombo_ChainAliasFor(nativeId);
    int fcId = FcCombo_ItemForNative(chain != 0 ? chain : nativeId);
    if (fcId == FCI_NO_ITEM) {
        return;
    }
    // soh's peer token is the "RI_*" enum name, which is exactly 2ship's spoiler name.
    const char* peerName = FcCombo_PeerNameForItem(fcId);
    if (std::strcmp(peerName, "FCI_NO_ITEM") == 0) {
        return;
    }
    FnGrantSharedItem grant = ResolvePeerGrant();
    if (grant == nullptr) {
        SPDLOG_WARN("[FleetShared] 2ship.dll has no MM_GrantSharedItem; '{}' not shared", peerName);
        return;
    }
    SPDLOG_INFO("[FleetShared] OoT obtained fc {} -> MM '{}'", fcId, peerName);
    grant(peerName);
}

typedef const char* (*FnExtractSharedState)(void);

// Deferred to a tick with a loaded file: ApplyShared writes the live save, and the switch that
// requested the pull happens before this game's PlayState exists again.
static void PullFromPeerWhenReady() {
    if (!Combo_IsCombinedGame() || !sPullPending || gPlayState == NULL || gSaveContext.fileNum > 2) {
        return;
    }
    sPullPending = false;
    static FnExtractSharedState sExtract = nullptr;
    static bool sTried = false;
    if (!sTried) {
        sTried = true;
        sExtract = reinterpret_cast<FnExtractSharedState>(Combo_ResolveSym("2ship", "MM_ExtractSharedState"));
    }
    if (sExtract == nullptr) {
        SPDLOG_WARN("[FleetShared] 2ship.dll has no MM_ExtractSharedState; no reconciliation");
        return;
    }
    FleetSharedReceiveGuard receiveGuard;
    FleetSync_ApplySharedState(sExtract());
    SPDLOG_INFO("[FleetShared] pulled MM's shared state into OoT");
}

static void RegisterFleetSharedItems() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGameFrameUpdate>(PullFromPeerWhenReady);
}

static RegisterShipInitFunc initFleetSharedItems(RegisterFleetSharedItems, {});
#else
extern "C" void FleetShared_OnNativeObtained(int nativeId) {
    (void)nativeId;
}
#endif
