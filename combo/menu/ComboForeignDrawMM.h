/* combo/menu/ComboForeignDrawMM.h — ComboShip: cross-game foreign-item rendering, MM (host) side.
 * The exact mirror of the foreign block in soh/soh/Enhancements/randomizer/draw.cpp
 * (ComboResolveForeignDrawInfo + Randomizer_DrawComboForeign), in the opposite direction: an MM check
 * holding RI_COMBO_FOREIGN actually holds an OOT item, so we render the REAL OOT model by asking
 * soh.dll (OOT_GetItemDrawInfo, C ABI in combo/menu/ComboItemDrawABI.h) which display lists draw it,
 * then submitting them as "__OTR__@oot:"-routed paths that the shared Fast3D interpreter resolves
 * against OOT's ResourceManager (CrossRMRegistry — OOT's RM stays resident while MM runs).
 *
 * Unlike OOT, MM passes the originating RandoCheckId straight into Rando::DrawItem at every world
 * draw site (freestanding, chest, grass/pot, shop), so the check identity is available directly — no
 * GetItemEntry-stamping mechanism is needed (OOT's comboForeignCheck field has no MM analog here).
 *
 * TU-GLUE HEADER (menu-extraction pattern): include ONCE from mm/2s2h/Rando/DrawItem.cpp, inside its
 * #ifdef COMBO_BUILD, AFTER the engine headers (OPEN_DISPS, Gfx_SetupDL25 Opa/Xlu, the Matrix_ and
 * gbi macros, gPlayState, gSaveContext, GetItem_Draw) and Rando headers are in scope. Not
 * standalone. Lives in
 * combo/menu/ because that directory is already on 2ship's include path (zero CMake churn).
 *
 * The animated cross-game class (combo/menu/ComboForeignAnim.h) is wired here symmetrically with
 * ComboForeignDrawOOT.h: when the static export has no DL row, OOT_GetItemAnimDrawInfo describes a
 * skeletal recipe (the boss souls' real boss skeletons, issue #86) and that header drives MM's own
 * SkelAnime on OOT's resources.
 */
#ifndef COMBO_FOREIGN_DRAW_MM_H
#define COMBO_FOREIGN_DRAW_MM_H
#include "ComboMagicMirrorForeign.h"

#ifndef OPEN_DISPS
#error "ComboForeignDrawMM.h is TU-glue: include the host engine headers before it"
#endif

#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "ComboItemDrawABI.h"
#include "ComboCollectibleShimmerDraw.h"
#include "ComboRewardGi.h"
#include "ComboMmNativeImports.h"
#include "ComboFairyBottle.h"
#include "ComboBottleContents.h"
#include "ComboSwordGiFit.h"
#define COMBO_DIN_SWORD_GI_HOST_MM
#include "ComboDinSwordGi.h"
#undef COMBO_DIN_SWORD_GI_HOST_MM
#define COMBO_FAIRY_HOST_MM
#include "ComboFairyBottleDraw.h"
#undef COMBO_FAIRY_HOST_MM
#include "Rando/NeiGiPresentation.h"
#include "Rando/NeiResourceRouting.h"
#define COMBO_MORPHA_GI_HOST_MM
#include "ComboMorphaGi.h"
#undef COMBO_MORPHA_GI_HOST_MM
// ComboShip: the animated class, with 2ship.dll as the host (see the shim in ComboForeignAnim.h).
#define COMBO_FOREIGN_ANIM_HOST_MM 1
#include "ComboForeignAnim.h"
#define COMBO_MASK_SHIMMER_HOST_MM
#include "ComboMaskShimmer.h"
#undef COMBO_MASK_SHIMMER_HOST_MM
#include "2s2h/Rando/MiscBehavior/MiscBehavior.h" // Rando::MiscBehavior::MM_LookupForeign
#include "rando/CrossForeign.h"                   // ComboRando::ForeignItem / GAME_OOT
#include "ComboResolve.h"                         // Combo_ResolveSym (process-wide combo-ABI resolution)
#include "ComboSwordGiAssetSelection.h"

void DrawOotSlateRuneFlame(uint8_t r, uint8_t g, uint8_t b);

namespace {

struct ComboForeignDrawInfoOOT {
    bool ok = false;
    int32_t nativeMmItem = -1;
    int32_t count = 0;
    int32_t xluStart = -1; // first XLU entry in dls[] order; -1 = all OPA
    float scale = 0.0f;    // extra uniform model scale; 0 = none (OOT rupees: 0.7)
    int32_t itemShimmer = CW_ITEM_SHIMMER_NONE;
    uint8_t itemShimmerColor[4] = {};
    int32_t neiShimmer = 0;
    bool hasEnvColor = false;
    uint8_t envColor[4] = { 0, 0, 0, 0 };
    int32_t drawKind = CW_DRAW_KIND_SIMPLE;   // non-SIMPLE = replicate a specific OOT draw func
    uint8_t primColorXlu[4] = { 0, 0, 0, 0 }; // JEWEL gem prim / MUSIC_NOTE tint
    uint8_t envColorXlu[4] = { 0, 0, 0, 0 };  // JEWEL gem env
    uint8_t primColorOpa[4] = { 0, 0, 0, 0 }; // JEWEL setting prim
    uint8_t envColorOpa[4] = { 0, 0, 0, 0 };  // JEWEL setting env
    // CW_DRAW_KIND_COLOR_LAYERS: per-DL prim/env colors; bit i of each mask = dls[i] sets it.
    uint8_t layerPrimColor[CW_DRAW_MAX_DLISTS][4] = {};
    uint8_t layerEnvColor[CW_DRAW_MAX_DLISTS][4] = {};
    int32_t layerPrimMask = 0;
    int32_t layerEnvMask = 0;
    const char* dls[CW_DRAW_MAX_DLISTS] = { nullptr }; // interned "__OTR__@oot:..." routed paths
    // OOT's own setup DL for each stream (raw Gfx* in soh.dll), or null for our 25 Opa/Xlu.
    const void* setupDlOpa = nullptr;
    const void* setupDlXlu = nullptr;
    const char* segTexPath = nullptr; // OOT's own (unrouted) seg-8 texture path, or null
    // ComboShip: animated class (no static DL row — OOT boss souls' real skeletons). When animOk,
    // anim describes the item and ComboForeignAnim_Draw renders it; paths point at soh.dll statics.
    bool animOk = false;
    CwItemAnimDrawInfo anim{};
    // Recipe chosen from live save state (progressive tier, Triforce shard, junk/trap) — re-resolve
    // every frame instead of caching, or the first model drawn sticks for the whole save slot.
    bool stateDependent = false;
    bool appearanceDependent = false; // Keep cosmetic palettes live after grant latching.
    // Resolved receipt identity: the actual progressive tier or the base item name.
    std::string resolvedName;
    int32_t neiEffect = 0;
    float neiEffectCenter[3] = {};
    int32_t neiSomariaUpgrade = 0;
    int32_t neiLegacyCane = 0;
    int32_t opCount = 0;
    CwDrawOp ops[CW_DRAW_MAX_OPS] = {};
};

// Routed path strings must outlive the frame (the GBI wrapper emits the raw pointer into the display
// list; the interpreter dereferences it later), so intern them for the process lifetime.
inline const char* ComboInternRoutedPathOOT(const std::string& s) {
    static std::unordered_set<std::string> sPool; // node-based: c_str() stable across rehash
    return sPool.insert(s).first->c_str();
}

// One resolution attempt's outcome. NotReady = a producer/lookup that simply isn't up yet (soh.dll
// not resident, OOT's rando context null while dormant), which must NEVER be negative-cached.
enum class ComboForeignResolveOOT { Ok, Unknown, NotReady };

inline ComboForeignResolveOOT ComboFillForeignDrawInfoOOT(RandoCheckId rc, ComboForeignDrawInfoOOT& info,
                                                          const char* namedItem = nullptr) {
    const ComboRando::ForeignItem* fi = namedItem ? nullptr : Rando::MiscBehavior::MM_LookupForeign(rc);
    if (!namedItem && (fi == nullptr || fi->itemGame != ComboRando::GAME_OOT)) {
        return ComboForeignResolveOOT::Unknown;
    }

    const char* drawName =
        namedItem ? namedItem : (fi->HasDisguise() ? fi->fakeItemName.c_str() : fi->itemName.c_str());
    const int32_t nativeItem = ComboNativeMmImport(drawName);
    if (nativeItem >= 0) {
        info.nativeMmItem = nativeItem;
        info.resolvedName = drawName;
        info.ok = true;
        return ComboForeignResolveOOT::Ok;
    }

    static Fn_GetItemDrawInfo sGetItemDrawInfo = nullptr;
    if (sGetItemDrawInfo == nullptr) {
        // soh already loaded by the exe (ComboMenuModel pattern); resolution is process-wide.
        sGetItemDrawInfo = (Fn_GetItemDrawInfo)Combo_ResolveSym("soh", "OOT_GetItemDrawInfo");
    }
    if (sGetItemDrawInfo == nullptr) {
        return ComboForeignResolveOOT::NotReady; // soh.dll may simply not be resident yet
    }
    static Fn_SetGiCosmeticFrame sSetGiCosmeticFrame = nullptr;
    if (!sSetGiCosmeticFrame)
        sSetGiCosmeticFrame = (Fn_SetGiCosmeticFrame)Combo_ResolveSym("soh", "OOT_SetGiCosmeticFrame");
    if (sSetGiCosmeticFrame && gPlayState)
        sSetGiCosmeticFrame(static_cast<uint32_t>(gPlayState->gameplayFrames));
    // A disguised trap must draw the item it pretends to be. Same namespace, so the itemGame dispatch
    // above is unaffected. Not state-dependent: like OOT, the disguise holds until the get-item cutscene.
    CwItemDrawInfo raw{};
    int32_t rcStatic = sGetItemDrawInfo(drawName, &raw);
    const int32_t baseChoice = ComboSwordGi_SelectBaseAssets(raw, ResourceMgr_IsAltAssetsEnabled());
    if (baseChoice == CW_DRAW_NOT_READY)
        return ComboForeignResolveOOT::NotReady;
    if (baseChoice == 2)
        rcStatic = 1;
    if (rcStatic == CW_DRAW_NOT_READY) {
        return ComboForeignResolveOOT::NotReady; // OOT dormant / rando context null — retry next frame
    }
    if (rcStatic == 0 || (raw.dlistCount <= 0 && raw.drawKind != CW_DRAW_KIND_NEI_CANE &&
                          raw.drawKind != CW_DRAW_KIND_OOT_NATIVE_EQUIPMENT && raw.drawKind != CW_DRAW_KIND_SEASON_GI &&
                          raw.drawKind != CW_DRAW_KIND_MAGIC_MIRROR)) {
        // ComboShip: no static DL row — try the animated ABI (OOT boss souls' real skeletons). OOT
        // only describes the item; ComboForeignAnim_Draw loads + draws it (mirror of the OOT side).
        static Fn_GetItemAnimDrawInfo sGetItemAnimDrawInfo = nullptr;
        if (sGetItemAnimDrawInfo == nullptr) {
            sGetItemAnimDrawInfo = (Fn_GetItemAnimDrawInfo)Combo_ResolveSym("soh", "OOT_GetItemAnimDrawInfo");
        }
        if (sGetItemAnimDrawInfo == nullptr) {
            return ComboForeignResolveOOT::NotReady;
        }
        int32_t rcAnim = sGetItemAnimDrawInfo(drawName, &info.anim);
        if (rcAnim == CW_DRAW_NOT_READY) {
            return ComboForeignResolveOOT::NotReady;
        }
        if (rcAnim != 0) {
            info.animOk = true;
            info.stateDependent = info.anim.stateDependent != 0;
            info.ok = true;
            return ComboForeignResolveOOT::Ok;
        }
        return ComboForeignResolveOOT::Unknown; // unknown item or non-portable draw func -> sentinel
    }

    if (raw.drawKind == CW_DRAW_KIND_MAGIC_MIRROR && (raw.dlistCount != 0 || raw.opCount != 0))
        return ComboForeignResolveOOT::Unknown;
    static constexpr char kOtrPrefix[] = "__OTR__";
    int32_t n = raw.dlistCount < CW_DRAW_MAX_DLISTS ? raw.dlistCount : CW_DRAW_MAX_DLISTS;
    if (n < CwMinDlistsForKind(raw.drawKind)) {
        return ComboForeignResolveOOT::Unknown; // handler would blind-index a missing slot
    }
    for (int32_t i = 0; i < n; i++) {
        const char* p = raw.dlists[i];
        if (p == nullptr || strncmp(p, kOtrPrefix, sizeof(kOtrPrefix) - 1) != 0) {
            return ComboForeignResolveOOT::Unknown; // not an OTR path literal — can't route it
        }
        const char* ownerPrefix = (raw.drawKind == CW_DRAW_KIND_MM_MASK || raw.drawKind == CW_DRAW_KIND_MM_REMAINS ||
                                   raw.drawKind == CW_DRAW_KIND_MM_SPIN_ATTACK)
                                      ? "__OTR__@mm:"
                                      : "__OTR__@oot:";
        // A concrete OoT weapon tier may use MM's native GI mesh. Preserve only recognized
        // explicit owner routes; ordinary paths still belong to the producing game.
        const bool routedMm = strncmp(p, "__OTR__@mm:", 11) == 0;
        const bool routedOot = strncmp(p, "__OTR__@oot:", 12) == 0;
        const bool routedBase = strncmp(p, "__OTR__@oot-gi-base:", 20) == 0;
        if (p[7] == '@' && !routedMm && !routedOot && !routedBase)
            return ComboForeignResolveOOT::Unknown;
        info.dls[i] = ComboInternRoutedPathOOT(routedMm || routedOot || routedBase
                                                   ? std::string(p)
                                                   : std::string(ownerPrefix) + (p + sizeof(kOtrPrefix) - 1));
    }
    info.count = n;
    info.xluStart = raw.xluStartIndex;
    info.scale = raw.scale;
    info.setupDlOpa = raw.setupDlOpa;
    info.setupDlXlu = raw.setupDlXlu;
    if (raw.segTexPath != nullptr) {
        if (strncmp(raw.segTexPath, kOtrPrefix, sizeof(kOtrPrefix) - 1) != 0) {
            return ComboForeignResolveOOT::Unknown; // bound under OOT's RM below; must be a path literal
        }
        info.segTexPath = raw.segTexPath;
    }
    info.hasEnvColor = raw.hasEnvColor != 0;
    info.drawKind = raw.drawKind;
    info.neiEffect = raw.neiEffect;
    memcpy(info.neiEffectCenter, raw.neiEffectCenter, sizeof(info.neiEffectCenter));
    info.neiSomariaUpgrade = raw.neiSomariaUpgrade;
    info.neiLegacyCane = raw.neiLegacyCane;
    if (raw.opCount < 0 || raw.opCount > CW_DRAW_MAX_OPS)
        return ComboForeignResolveOOT::Unknown;
    if (raw.drawKind == CW_DRAW_KIND_SEASON_GI &&
        (raw.neiEffect < 1 || raw.neiEffect > 6 || (raw.neiEffect != 5 ? n != 0 || raw.opCount != 0 : n < 1)))
        return ComboForeignResolveOOT::Unknown;
    if (raw.drawKind == CW_DRAW_KIND_CUSTOM_GI || raw.drawKind == CW_DRAW_KIND_SEASON_GI) {
        if (raw.xluStartIndex < -1 || raw.xluStartIndex > n)
            return ComboForeignResolveOOT::Unknown;
        for (int i = 0; i < raw.opCount; ++i) {
            const int op = raw.ops[i].op;
            if (op != CW_OP_ROTATE_X && op != CW_OP_ROTATE_Z && op != CW_OP_SCALE && op != CW_OP_TRANSLATE &&
                op != CW_OP_FRAME_PAIR && op != CW_OP_NO_CULL)
                return ComboForeignResolveOOT::Unknown;
            if (op == CW_OP_FRAME_PAIR &&
                (n != 2 || raw.xluStartIndex != -1 || !(raw.ops[i].a >= 0 && raw.ops[i].a <= 30) ||
                 raw.ops[i].a != static_cast<int>(raw.ops[i].a)))
                return ComboForeignResolveOOT::Unknown;
        }
    }
    if (raw.drawKind == CW_DRAW_KIND_OOT_NATIVE_EQUIPMENT) {
        if (raw.opCount != 1 || raw.ops[0].op != CW_OP_NATIVE_EQUIPMENT ||
            !(raw.ops[0].a >= static_cast<float>(CW_OOT_EQUIP_AXE) &&
              raw.ops[0].a <= static_cast<float>(CW_OOT_EQUIP_ROC_BOOTS)) ||
            raw.ops[0].a != static_cast<int32_t>(raw.ops[0].a))
            return ComboForeignResolveOOT::Unknown;
    }
    if (raw.drawKind == CW_DRAW_KIND_ELEMENTAL_ARROW &&
        (raw.neiEffect < 1 || raw.neiEffect > 3 || raw.dlistCount != 1 || raw.opCount != 0))
        return ComboForeignResolveOOT::Unknown;
    if (raw.drawKind == CW_DRAW_KIND_SIMPLE) {
        if (raw.opCount > 2)
            return ComboForeignResolveOOT::Unknown;
        for (int i = 0; i < raw.opCount; ++i)
            if (raw.ops[i].op != CW_OP_ROTATE_X && raw.ops[i].op != CW_OP_ROTATE_Z)
                return ComboForeignResolveOOT::Unknown;
    }
    info.opCount = raw.opCount;
    memcpy(info.ops, raw.ops, sizeof(info.ops));
    info.stateDependent = raw.stateDependent != 0;
    info.appearanceDependent = raw.stateDependent == 2;
    info.itemShimmer = raw.itemShimmer;
    info.neiShimmer = raw.neiShimmer;
    memcpy(info.itemShimmerColor, raw.itemShimmerColor, sizeof(info.itemShimmerColor));
    if (raw.resolvedName != nullptr) {
        info.resolvedName = raw.resolvedName;
    }
    info.layerPrimMask = raw.layerPrimMask;
    info.layerEnvMask = raw.layerEnvMask;
    memcpy(info.layerPrimColor, raw.layerPrimColor, sizeof(info.layerPrimColor));
    memcpy(info.layerEnvColor, raw.layerEnvColor, sizeof(info.layerEnvColor));
    for (int32_t i = 0; i < 4; i++) {
        info.envColor[i] = raw.envColor[i];
        info.primColorXlu[i] = raw.primColorXlu[i];
        info.envColorXlu[i] = raw.envColorXlu[i];
        info.primColorOpa[i] = raw.primColorOpa[i];
        info.envColorOpa[i] = raw.envColorOpa[i];
    }
    info.ok = true;
    return ComboForeignResolveOOT::Ok;
}

// Recipe cache, swept per save slot and per foreign-map generation. Shared by the resolver and the
// grant-time latch below so both observe the same sweep.
struct ComboForeignDrawCacheOOT {
    std::unordered_map<int32_t, ComboForeignDrawInfoOOT> map;
    std::unordered_map<int32_t, std::string> receiptNames;
    std::unordered_map<int32_t, std::string> swordReceiptNames;
    int slot = -1;
    uint64_t gen = (uint64_t)-1;
};

inline ComboForeignDrawCacheOOT& ComboForeignDrawCacheOOTGet() {
    static ComboForeignDrawCacheOOT c;
    int slot = gSaveContext.fileNum;
    uint64_t gen = Rando::MiscBehavior::ComboRandoGen();
    if (slot != c.slot || gen != c.gen) {
        c.map.clear();
        c.receiptNames.clear();
        c.swordReceiptNames.clear();
        c.slot = slot;
        c.gen = gen;
    }
    return c;
}

// Full lookup chain (foreign map -> OOT export -> routed strings), cached per check per slot per
// foreign-map generation so it runs once per check instead of every frame.
inline const ComboForeignDrawInfoOOT* ComboResolveForeignDrawInfoOOT(RandoCheckId rc) {
    ComboForeignDrawCacheOOT& c = ComboForeignDrawCacheOOTGet();
    auto cached = c.map.find(rc);
    if (cached != c.map.end() && !cached->second.stateDependent) {
        return cached->second.ok ? &cached->second : nullptr;
    }
    // A state-dependent recipe (progressive tier, Triforce shard, junk/trap) is re-resolved every
    // frame; caching it would freeze whichever model happened to be correct on the first draw.
    ComboForeignDrawInfoOOT info{}; // built locally: a failure must not clobber a live cached recipe
    // Once granted, refresh only the frozen concrete sword's appearance.
    // Resolving its progressive placeholder again would select the NEXT tier.
    auto receipt = c.swordReceiptNames.find(rc);
    const char* namedItem = receipt != c.swordReceiptNames.end() ? receipt->second.c_str() : nullptr;
    if (ComboFillForeignDrawInfoOOT(rc, info, namedItem) == ComboForeignResolveOOT::NotReady) {
        c.map.erase(rc); // transient — retry next frame instead of freezing the sentinel in
        return nullptr;
    }
    ComboForeignDrawInfoOOT& entry = c.map[rc]; // Unknown caches ok=false: one lookup, then sentinel
    entry = info;
    return entry.ok ? &entry : nullptr;
}

// ComboShip: freeze this check's recipe at the tier it is ABOUT to grant. The cross-grant mutates
// OOT's dormant save mid-presentation, so a live re-resolve would flip the held-up model next frame.
inline void ComboLatchForeignDrawOOT(RandoCheckId rc) {
    if (rc == RC_UNKNOWN) {
        return;
    }
    ComboForeignDrawCacheOOT& c = ComboForeignDrawCacheOOTGet();
    ComboForeignDrawInfoOOT info{};
    if (ComboFillForeignDrawInfoOOT(rc, info) != ComboForeignResolveOOT::Ok) {
        return; // nothing written, nothing erased: the draw stays live, i.e. no worse than before
    }
    if (!info.resolvedName.empty())
        c.receiptNames[rc] = info.resolvedName;
    if (info.animOk) {
        return; // that class's state-dependence is a CVar (SimplerBossSoulModels), not save state
    }
    // Progressive producers use stateDependent=1 to freeze the awarded tier.
    // Its sword mesh must still follow the host's live Alt Assets selection.
    if (!info.resolvedName.empty() && info.neiShimmer > 0 &&
        NeiGi::IsSword(static_cast<NeiGi::Kind>(info.neiShimmer - 1))) {
        c.swordReceiptNames[rc] = info.resolvedName;
        info.appearanceDependent = true;
    }
    info.stateDependent = info.appearanceDependent; // Freeze tiers while keeping appearance live.
    c.map[rc] = info;
}

// The receipt identity remains frozen even when the model's cosmetic recipe is
// live. Both caches reset together when the save slot or foreign map changes.
inline const char* ComboForeignLatchedNameOOT(RandoCheckId rc) {
    ComboForeignDrawCacheOOT& c = ComboForeignDrawCacheOOTGet();
    auto it = c.receiptNames.find(rc);
    if (it == c.receiptNames.end() || it->second.empty()) {
        return nullptr;
    }
    return it->second.c_str();
}

// Live tier name for previews: runs the same per-frame resolver the shelf model uses.
inline const char* ComboForeignLiveNameOOT(RandoCheckId rc) {
    const ComboForeignDrawInfoOOT* info = ComboResolveForeignDrawInfoOOT(rc);
    if (info == nullptr || info->resolvedName.empty()) {
        return nullptr;
    }
    return info->resolvedName.c_str();
}

} // namespace

// ---- Non-portable OOT draw funcs: each handler is a 1:1 port of the OOT get-item func
// (soh/src/code/z_draw.c), re-binding the segment(s) it needs in MM's frame before submitting the
// routed @oot: DLs. See docs/deviations/rando.md for the invariants.

// Pin prim+env to white right after a setup DL, before the handler's own colour commands: a layer
// the recipe doesn't tint must not inherit MM's continuously-interpolated scene material colour.
#define MM_FOREIGN_PIN_OPA()                                        \
    do {                                                            \
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255); \
        gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);        \
    } while (0)
#define MM_FOREIGN_PIN_XLU()                                        \
    do {                                                            \
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255); \
        gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);        \
    } while (0)

// Restore the segments a handler bound (8 and/or 9) to a benign empty DL so later same-frame commands
// don't sample our scroll DLs. Restores on both streams (harmless where a segment wasn't bound).
// Mirrors the segment hygiene in ComboForeignAnim.h.
inline void MM_RestoreForeignSegs(const int32_t* segs, int32_t count) {
    GraphicsContext* gfxCtx = gPlayState->state.gfxCtx;
    // Array of ENDDLs: DLs may call through a bound segment at an index > 0 — see CfaEmptyDL.
    Gfx* empty = (Gfx*)GRAPH_ALLOC(gfxCtx, 8 * sizeof(Gfx));
    Gfx* e = empty;
    for (int32_t iEmpty = 0; iEmpty < 8; iEmpty++) {
        gSPEndDisplayList(e++);
    }
    OPEN_DISPS(gfxCtx);
    for (int32_t i = 0; i < count; i++) {
        gSPSegment(POLY_OPA_DISP++, segs[i], (uintptr_t)empty);
        gSPSegment(POLY_XLU_DISP++, segs[i], (uintptr_t)empty);
    }
    CLOSE_DISPS(gfxCtx);
}

inline void MM_DrawForeignSpinAttack(const ComboForeignDrawInfoOOT* info) {
    ComboDrawSpinAttackGi(gPlayState, info->dls[0], info->dls[1], info->scale, info->primColorXlu, "mm");
}

// The owner already classified its selected Alt asset, so custom jars get the
// same grayscale tint scope as the native OoT draw without recoloring native DLs.
inline Gfx* MM_DrawForeignMagicJarDList(Gfx* gfx, const char* dlist, const uint8_t* color) {
    if (color[3]) {
        gDPSetGrayscaleColor(gfx++, color[0], color[1], color[2], 255);
        gSPGrayscale(gfx++, true);
    }
    gSPDisplayList(gfx++, (Gfx*)dlist);
    if (color[3])
        gSPGrayscale(gfx++, false);
    return gfx;
}

// Simple path: OPA layers then XLU layers (self-contained funcs, rupees, wallets, Triforce/rod scale).
inline void MM_DrawForeignSimple(const ComboForeignDrawInfoOOT* info) {
    int32_t n = info->count;
    int32_t xs = (info->xluStart < 0 || info->xluStart > n) ? n : info->xluStart;
    GraphicsContext* gfxCtx = gPlayState->state.gfxCtx;
    const bool pose = info->drawKind == CW_DRAW_KIND_SIMPLE && info->opCount > 0;
    if (pose) {
        Matrix_Push();
        for (int i = 0; i < info->opCount; ++i) {
            const auto& op = info->ops[i];
            if (op.op == CW_OP_ROTATE_Z)
                Matrix_RotateZF(op.a * (3.14159265358979323846f / 32768.f), MTXMODE_APPLY);
            else if (op.op == CW_OP_ROTATE_X)
                Matrix_RotateXF(op.a * (3.14159265358979323846f / 32768.f), MTXMODE_APPLY);
        }
    }
    OPEN_DISPS(gfxCtx);
    if (info->scale > 0.0f) {
        Matrix_Scale(info->scale, info->scale, info->scale, MTXMODE_APPLY);
    }
    if (xs > 0) {
        // OOT's own setup when the row uses one other than 25 (masks/bombchu/medallions = 26, which
        // is 1-CYCLE without fog). Under MM's 2-cycle 25 those lists' duplicated second cycle wins
        // and samples TEXEL1 — whatever tile MM last bound — instead of the item's own texture.
        if (info->setupDlOpa != nullptr) {
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->setupDlOpa);
        } else {
            Gfx_SetupDL25_Opa(gfxCtx);
        }
        MM_FOREIGN_PIN_OPA();
        if (info->drawKind == CW_DRAW_KIND_MM_MASK || info->drawKind == CW_DRAW_KIND_MM_REMAINS)
            gDPSetTextureLUT(POLY_OPA_DISP++, G_TT_NONE);
        MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx);
        if (info->hasEnvColor) {
            gDPSetEnvColor(POLY_OPA_DISP++, info->envColor[0], info->envColor[1], info->envColor[2], info->envColor[3]);
        }
        for (int32_t i = 0; i < xs; i++) {
            if (info->drawKind == CW_DRAW_KIND_MAGIC_JAR)
                POLY_OPA_DISP = MM_DrawForeignMagicJarDList(POLY_OPA_DISP, info->dls[i], info->primColorOpa);
            else
                gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[i]);
        }
    }
    if (xs < n) {
        if (info->setupDlXlu != nullptr) { // sold-out sign / compass glass: setup 5, not 25
            gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->setupDlXlu);
        } else {
            Gfx_SetupDL25_Xlu(gfxCtx);
        }
        MM_FOREIGN_PIN_XLU();
        if (info->drawKind == CW_DRAW_KIND_MM_MASK || info->drawKind == CW_DRAW_KIND_MM_REMAINS)
            gDPSetTextureLUT(POLY_XLU_DISP++, G_TT_NONE);
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
        if (info->hasEnvColor) {
            gDPSetEnvColor(POLY_XLU_DISP++, info->envColor[0], info->envColor[1], info->envColor[2], info->envColor[3]);
        }
        for (int32_t i = xs; i < n; i++) {
            gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[i]);
        }
    }
    CLOSE_DISPS(gfxCtx);
    if (pose)
        Matrix_Pop();
}

// Biggoron's / Broken Goron's Sword: seg8 OPA scroll (z_draw.c GetItem_DrawGoronSword).
inline void MM_DrawForeignGoronSword(const ComboForeignDrawInfoOOT* info) {
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Opa(gfxCtx);
    MM_FOREIGN_PIN_OPA();
    gSPSegment(POLY_OPA_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, G_TX_RENDERTILE, play->state.frames * 1, play->state.frames * 0,
                                             32, 32, 1, play->state.frames * 0, play->state.frames * 0, 32, 32, 1, 0, 0,
                                             0));
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
}

// Deku Nuts: seg8 OPA scroll (GetItem_DrawDekuNuts). The rando nut bag carries OOT's 26 Opa.
inline void MM_DrawForeignDekuNuts(const ComboForeignDrawInfoOOT* info) {
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    if (info->setupDlOpa != nullptr) {
        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->setupDlOpa);
    } else {
        Gfx_SetupDL25_Opa(gfxCtx);
    }
    MM_FOREIGN_PIN_OPA();
    gSPSegment(POLY_OPA_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, G_TX_RENDERTILE, play->state.frames * 6, play->state.frames * 6,
                                             32, 32, 1, play->state.frames * 6, play->state.frames * 6, 32, 32, 6, 6, 6,
                                             6));
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
}

// Recovery Heart: seg8 XLU scroll (GetItem_DrawRecoveryHeart; cosmetic grayscale recolor omitted).
inline void MM_DrawForeignRecoveryHeart(const ComboForeignDrawInfoOOT* info) {
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MM_FOREIGN_PIN_XLU();
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, G_TX_RENDERTILE, play->state.frames * 0, -(play->state.frames * 3),
                                             32, 32, 1, play->state.frames * 0, -(play->state.frames * 2), 32, 32, 0,
                                             -3, 0, -2));
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[0]);
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
}

// Fish: seg8 XLU scroll (GetItem_DrawFish).
inline void MM_DrawForeignFish(const ComboForeignDrawInfoOOT* info) {
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MM_FOREIGN_PIN_XLU();
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, G_TX_RENDERTILE, play->state.frames * 0, play->state.frames * 1,
                                             32, 32, 1, play->state.frames * 0, play->state.frames * 1, 32, 32, 0, 1, 0,
                                             1));
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[0]);
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
}

// Potions: seg8 OPA scroll, OPA dl[1,0,2,3] + XLU dl[4,5] (GetItem_DrawPotion).
inline void MM_DrawForeignPotion(const ComboForeignDrawInfoOOT* info) {
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Opa(gfxCtx);
    MM_FOREIGN_PIN_OPA();
    gSPSegment(POLY_OPA_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, G_TX_RENDERTILE, -play->state.frames, play->state.frames, 32, 32,
                                             1, -play->state.frames, play->state.frames, 32, 32, -1, 1, -1, 1));
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[1]);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[2]);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[3]);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MM_FOREIGN_PIN_XLU();
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[4]);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[5]);
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
}

// Mirror Shield: seg8 OPA scroll, OPA dl0 + XLU dl1 (GetItem_DrawMirrorShield).
inline void MM_DrawForeignMirrorShield(const ComboForeignDrawInfoOOT* info) {
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Opa(gfxCtx);
    MM_FOREIGN_PIN_OPA();
    gSPSegment(POLY_OPA_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, G_TX_RENDERTILE, 0, play->state.frames * 2 % 256, 64, 64, 1, 0,
                                             play->state.frames * 1 % 128, 32, 32, 0, 2, 0, 1));
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MM_FOREIGN_PIN_XLU();
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[1]);
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
}

// Blue Fire: OPA dl0; XLU seg8 flame scroll + billboard dl1 (GetItem_DrawBlueFire).
inline void MM_DrawForeignBlueFire(const ComboForeignDrawInfoOOT* info) {
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Opa(gfxCtx);
    MM_FOREIGN_PIN_OPA();
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MM_FOREIGN_PIN_XLU();
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, G_TX_RENDERTILE, 0, 0, 16, 32, 1, play->state.frames * 1,
                                             -(play->state.frames * 8), 16, 32, 0, 0, 1, -8));
    Matrix_Push();
    Matrix_Translate(-8.0f, -2.0f, 0.0f, MTXMODE_APPLY);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[1]);
    Matrix_Pop();
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
}

// Poe / Big Poe: OPA dl0; XLU dl1; seg8 scroll; billboard dl3,dl2 (GetItem_DrawPoes).
inline void MM_DrawForeignPoes(const ComboForeignDrawInfoOOT* info) {
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Opa(gfxCtx);
    MM_FOREIGN_PIN_OPA();
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MM_FOREIGN_PIN_XLU();
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[1]);
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, G_TX_RENDERTILE, 0, 0, 16, 32, 1, play->state.frames * 1,
                                             -(play->state.frames * 6), 16, 32, 0, 0, 1, -6));
    Matrix_Push();
    Matrix_ReplaceRotation(&play->billboardMtxF);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[3]);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[2]);
    Matrix_Pop();
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
}

// Fairy (bottled): OPA dl0; XLU dl1; seg8 scroll; billboard dl2 (GetItem_DrawFairy).
inline void MM_DrawForeignFairy(const ComboForeignDrawInfoOOT* info) {
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    const ComboFairyBottleMotion motion = ComboFairyBottle_SampleForShell(play->gameplayFrames, info->dls[0]);
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Opa(gfxCtx);
    MM_FOREIGN_PIN_OPA();
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MM_FOREIGN_PIN_XLU();
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    if (strcmp(info->dls[0], info->dls[1]) != 0) {
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[1]);
    }
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, G_TX_RENDERTILE, 0, 0, 32, 32, 1, play->state.frames * 1,
                                             -(play->state.frames * 6), 32, 32, 0, 0, 1, -6));
    Matrix_Push();
    if (ComboFairyBottle_IsBlueFireShell(info->dls[0])) {
        Matrix_Translate(-8.0f, -2.0f, 0.0f, MTXMODE_APPLY);
    }
    Matrix_Translate(motion.x, motion.y, motion.z, MTXMODE_APPLY);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    Matrix_Scale(motion.scaleX, motion.scaleY, motion.scaleZ, MTXMODE_APPLY);
    if (!ComboFairyBottle_DrawVfx(play, info->dls[0])) {
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[2]);
    }
    Matrix_Pop();
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
    if (ComboFairyBottle_IsBundledShell(info->dls[0]))
        ComboBottleShimmer_DrawMotes(play, CW_SHIMMER_FAIRY);
}

// Spiritual stones: seg9 XLU + seg8 OPA (static binds), rotate, per-layer prim/env colors, gem dl0
// (XLU) + setting dl1 (OPA) (GetItem_DrawJewel + the Kokiri/Goron/Zora color wrappers).
inline void MM_DrawForeignJewel(const ComboForeignDrawInfoOOT* info) {
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    gSPSegment(POLY_XLU_DISP++, 9,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, 0 % 256, (256 - (0 % 256)) - 1, 64, 64, 1, 0 % 256,
                                             (256 - (0 % 256)) - 1, 16, 16, 0, 0, 0, 0));
    gSPSegment(POLY_OPA_DISP++, 8, (uintptr_t)Gfx_TexScrollEx(gfxCtx, 0, 0, 16, 16, 0, 0));
    Matrix_Push();
    Matrix_RotateZYX(0, -0x4000, 0x4000, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MM_FOREIGN_PIN_XLU();
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 128, info->primColorXlu[0], info->primColorXlu[1], info->primColorXlu[2], 255);
    gDPSetEnvColor(POLY_XLU_DISP++, info->envColorXlu[0], info->envColorXlu[1], info->envColorXlu[2], 255);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[0]);
    Gfx_SetupDL25_Opa(gfxCtx);
    MM_FOREIGN_PIN_OPA();
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 128, info->primColorOpa[0], info->primColorOpa[1], info->primColorOpa[2], 255);
    gDPSetEnvColor(POLY_OPA_DISP++, info->envColorOpa[0], info->envColorOpa[1], info->envColorOpa[2], 255);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[1]);
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08, 0x09 };
    // Complete the native recipe's allocating tail before optional materials.
    MM_RestoreForeignSegs(segs, 2);
    NeiGi_DrawRewardMaterial(play, RewardGi_ProfileForPaths(info->dls[0], nullptr), info->dls[0], info->dls[1], "oot");
    Matrix_Pop();
}

// Din's Fire / Farore's Wind / Nayru's Love: XLU seg8 scroll, dl0,1,2 (GetItem_DrawMagicSpell).
inline void MM_DrawForeignMagicSpell(const ComboForeignDrawInfoOOT* info) {
    PlayState* play = gPlayState;
    if (!NeiGi_CanDrawElementalSpellFallback(play))
        return;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MM_FOREIGN_PIN_XLU();
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, G_TX_RENDERTILE, play->state.frames * 2, -(play->state.frames * 6),
                                             32, 32, 1, play->state.frames * 1, -(play->state.frames * 2), 32, 32, 2,
                                             -6, 1, -2));
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[0]);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[1]);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[2]);
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
}

// Silver / Gold Scale: XLU seg8 scroll, dl2,3,1,0 (GetItem_DrawScale).
inline void MM_DrawForeignScale(const ComboForeignDrawInfoOOT* info) {
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MM_FOREIGN_PIN_XLU();
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, G_TX_RENDERTILE, play->state.frames * 2, -(play->state.frames * 2),
                                             64, 64, 1, play->state.frames * 4, -(play->state.frames * 4), 32, 32, 2,
                                             -2, 4, -4));
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[2]);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[3]);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[1]);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[0]);
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
}

// Skulltula Token: body OPA dl0 + XLU seg8 flame dl1 (GetItem_DrawSkullToken, full flame).
inline void MM_DrawForeignSkullToken(const ComboForeignDrawInfoOOT* info) {
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Opa(gfxCtx);
    MM_FOREIGN_PIN_OPA();
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MM_FOREIGN_PIN_XLU();
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, G_TX_RENDERTILE, play->state.frames * 0, -(play->state.frames * 5),
                                             32, 32, 1, play->state.frames * 0, play->state.frames * 0, 32, 64, 0, -5,
                                             0, 0));
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[1]);
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
}

// Generic rando song note: grayscale-tinted note DL (GetItem_DrawGenericMusicNote). No segments.
inline void MM_DrawForeignMusicNote(const ComboForeignDrawInfoOOT* info) {
    if (info->count == 2) {
        OPEN_DISPS(gPlayState->state.gfxCtx);
        gSPGrayscale(POLY_XLU_DISP++, false);
        CLOSE_DISPS(gPlayState->state.gfxCtx);
        MM_DrawForeignSimple(info); // Native warp color DL followed by its original clef.
        return;
    }
    GraphicsContext* gfxCtx = gPlayState->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gDPSetGrayscaleColor(POLY_XLU_DISP++, info->primColorXlu[0], info->primColorXlu[1], info->primColorXlu[2], 255);
    gSPGrayscale(POLY_XLU_DISP++, true);
    Gfx_SetupDL25_Opa(gfxCtx); // OOT's func really does set up Opa state for an XLU submission
    MM_FOREIGN_PIN_XLU();      // pin the stream the DL goes out on, not the setup's
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[0]);
    gSPGrayscale(POLY_XLU_DISP++, false);
    CLOSE_DISPS(gfxCtx);
}

// OoT's simpler boss souls use MM's native flame plus the owner-routed generic skull.
inline void MM_DrawForeignBossSoul(const ComboForeignDrawInfoOOT* info) {
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    const float translate[3] = { 0.0f, -70.0f, 0.0f };
    const float scale[3] = { 5.0f, 5.0f, 5.0f };
    DrawOotSoulFlame(play, info->primColorXlu, translate, scale);
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MM_FOREIGN_PIN_XLU();
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gDPSetEnvColor(POLY_XLU_DISP++, info->envColorXlu[0], info->envColorXlu[1], info->envColorXlu[2], 255);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[1]); // generic soul skull
    CLOSE_DISPS(gfxCtx);
}

// DrawMorpha's two XLU core layers. The flame remains MM-owned, while every inner model
// resource resolves through OoT's live Alt selection under the owner bracket.
inline void MM_DrawForeignMorphaSoul(const ComboForeignDrawInfoOOT* info) {
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    const float translate[3] = { 0.0f, -70.0f, 0.0f };
    const float scale[3] = { 5.0f, 5.0f, 5.0f };
    DrawOotSoulFlame(play, info->primColorXlu, translate, scale);
    Matrix_Push();
    Matrix_Scale(0.015f, 0.015f, 0.015f, MTXMODE_APPLY);
    Matrix_RotateXF(play->state.frames * 0.1f, MTXMODE_APPLY);
    Matrix_RotateZF(play->state.frames * 0.16f, MTXMODE_APPLY);
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MM_FOREIGN_PIN_XLU();
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, play->state.frames * 3, play->state.frames * 3, 32, 32, 1,
                                             play->state.frames * -3, play->state.frames * -3, 32, 32, 3, 3, -3, -3));
    gSPSegment(POLY_XLU_DISP++, 0x09,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, play->state.frames * 3, 0, 32, 32, 1, 0,
                                             play->state.frames * -5, 32, 32, 3, 0, 0, -5));
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 255, 255, 255, 255);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[1]);
    gDPPipeSync(POLY_XLU_DISP++);
    gDPSetEnvColor(POLY_XLU_DISP++, 0, 220, 255, 128);
    gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 255, 255, 255, 255);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[2]);
    CLOSE_DISPS(gfxCtx);
    Matrix_Pop();
    const int32_t segs[] = { 8, 9 };
    MM_RestoreForeignSegs(segs, 2);
}

// Native MM imported boss souls have no foreign check-map entry. Resolve the
// same OoT model recipe by name so both pickup routes honor OoT's Alt selection,
// native animation and simplified-model setting.
inline bool MM_TryDrawOotBossSoul(RandoItemId item) {
    if (!gPlayState || !Ship::CrossRMRegistry::Get("oot"))
        return false;
    const char* name = nullptr;
    switch (item) {
        case RI_SOUL_OOT_BOSS_GOHMA:
            name = "Gohma's Soul";
            break;
        case RI_SOUL_OOT_BOSS_KING_DODONGO:
            name = "King Dodongo's Soul";
            break;
        case RI_SOUL_OOT_BOSS_BARINADE:
            name = "Barinade's Soul";
            break;
        case RI_SOUL_OOT_BOSS_PHANTOM_GANON:
            name = "Phantom Ganon's Soul";
            break;
        case RI_SOUL_OOT_BOSS_VOLVAGIA:
            name = "Volvagia's Soul";
            break;
        case RI_SOUL_OOT_BOSS_MORPHA:
            name = "Morpha's Soul";
            break;
        case RI_SOUL_OOT_BOSS_BONGO_BONGO:
            name = "Bongo Bongo's Soul";
            break;
        case RI_SOUL_OOT_BOSS_TWINROVA:
            name = "Twinrova's Soul";
            break;
        case RI_SOUL_OOT_BOSS_GANON:
            name = "Ganon's Soul";
            break;
        default:
            return false;
    }
    ComboForeignDrawInfoOOT info{};
    if (ComboFillForeignDrawInfoOOT(RC_UNKNOWN, info, name) != ComboForeignResolveOOT::Ok)
        return false;
    bool drawn = false;
    Matrix_Push();
    if (info.animOk) {
        drawn = ComboForeignAnim_Draw(&info.anim, "oot", gPlayState) != 0;
    } else if (info.drawKind == CW_DRAW_KIND_OOT_MORPHA_SOUL) {
        MM_DrawForeignMorphaSoul(&info);
        drawn = true;
    } else if (info.drawKind == CW_DRAW_KIND_BOSS_SOUL) {
        MM_DrawForeignBossSoul(&info);
        drawn = true;
    }
    Matrix_Pop();
    return drawn;
}

// Per-DL prim/env colored layers: the rando map/compass/small-key/boss-key/key-ring/jabber-nut/
// bombchu-bag/overworld-key funcs, which only differ in which DLs they tint and with what. Rows
// authored for another setup (26 Opa, 5 Xlu) carry it in the recipe and it is submitted below.
inline void MM_DrawForeignColorLayers(const ComboForeignDrawInfoOOT* info) {
    int32_t n = info->count;
    int32_t xs = (info->xluStart < 0 || info->xluStart > n) ? n : info->xluStart;
    GraphicsContext* gfxCtx = gPlayState->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    if (xs > 0) {
        if (info->setupDlOpa != nullptr) { // Jabber Nut / Bombchu Bag: 26 Opa, 1-cycle
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->setupDlOpa);
        } else {
            Gfx_SetupDL25_Opa(gfxCtx);
        }
        MM_FOREIGN_PIN_OPA();
        MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx);
        for (int32_t i = 0; i < xs; i++) {
            if (info->layerPrimMask & (1 << i)) {
                gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, info->layerPrimColor[i][0], info->layerPrimColor[i][1],
                                info->layerPrimColor[i][2], info->layerPrimColor[i][3]);
            }
            if (info->layerEnvMask & (1 << i)) {
                gDPSetEnvColor(POLY_OPA_DISP++, info->layerEnvColor[i][0], info->layerEnvColor[i][1],
                               info->layerEnvColor[i][2], info->layerEnvColor[i][3]);
            }
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[i]);
        }
    }
    if (xs < n) {
        if (info->setupDlXlu != nullptr) { // compass glass: 5 Xlu
            gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->setupDlXlu);
        } else {
            Gfx_SetupDL25_Xlu(gfxCtx);
        }
        MM_FOREIGN_PIN_XLU();
        MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
        for (int32_t i = xs; i < n; i++) {
            if (info->layerPrimMask & (1 << i)) {
                gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, info->layerPrimColor[i][0], info->layerPrimColor[i][1],
                                info->layerPrimColor[i][2], info->layerPrimColor[i][3]);
            }
            if (info->layerEnvMask & (1 << i)) {
                gDPSetEnvColor(POLY_XLU_DISP++, info->layerEnvColor[i][0], info->layerEnvColor[i][1],
                               info->layerEnvColor[i][2], info->layerEnvColor[i][3]);
            }
            gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[i]);
        }
    }
    CLOSE_DISPS(gfxCtx);
}

// Per-layer grayscale matches native editor tint scopes without tinting neighboring DLs.
inline void MM_DrawForeignGrayscaleLayers(const ComboForeignDrawInfoOOT* info) {
    GraphicsContext* gfxCtx = gPlayState->state.gfxCtx;
    if (info->scale > 0.0f)
        Matrix_Scale(info->scale, info->scale, info->scale, MTXMODE_APPLY);
    const int split = info->xluStart < 0 || info->xluStart > info->count ? info->count : info->xluStart;
    OPEN_DISPS(gfxCtx);
    for (int stream = 0; stream < 2; ++stream) {
        const int begin = stream ? split : 0;
        const int end = stream ? info->count : split;
        if (begin >= end)
            continue;
        if (stream) {
            if (info->setupDlXlu)
                gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->setupDlXlu);
            else
                Gfx_SetupDL25_Xlu(gfxCtx);
            MM_FOREIGN_PIN_XLU();
            MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
            for (int i = begin; i < end; ++i) {
                const bool tint = (info->layerPrimMask & (1 << i)) != 0;
                if (tint)
                    gDPSetGrayscaleColor(POLY_XLU_DISP++, info->layerPrimColor[i][0], info->layerPrimColor[i][1],
                                         info->layerPrimColor[i][2], info->layerPrimColor[i][3]);
                gSPGrayscale(POLY_XLU_DISP++, tint);
                gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[i]);
                gSPGrayscale(POLY_XLU_DISP++, false);
            }
        } else {
            if (info->setupDlOpa)
                gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->setupDlOpa);
            else
                Gfx_SetupDL25_Opa(gfxCtx);
            MM_FOREIGN_PIN_OPA();
            MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx);
            for (int i = begin; i < end; ++i) {
                const bool tint = (info->layerPrimMask & (1 << i)) != 0;
                if (tint)
                    gDPSetGrayscaleColor(POLY_OPA_DISP++, info->layerPrimColor[i][0], info->layerPrimColor[i][1],
                                         info->layerPrimColor[i][2], info->layerPrimColor[i][3]);
                gSPGrayscale(POLY_OPA_DISP++, tint);
                gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[i]);
                gSPGrayscale(POLY_OPA_DISP++, false);
            }
        }
    }
    CLOSE_DISPS(gfxCtx);
}

// Grayscale-tinted XLU glyph: ocarina buttons (Randomizer_DrawOcarinaButton). No segments.
inline void MM_DrawForeignGrayscaleXlu(const ComboForeignDrawInfoOOT* info) {
    GraphicsContext* gfxCtx = gPlayState->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MM_FOREIGN_PIN_XLU();
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gDPSetGrayscaleColor(POLY_XLU_DISP++, info->primColorXlu[0], info->primColorXlu[1], info->primColorXlu[2], 255);
    gSPGrayscale(POLY_XLU_DISP++, true);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[0]);
    gSPGrayscale(POLY_XLU_DISP++, false);
    CLOSE_DISPS(gfxCtx);
}

// Double Defense: grayscale-white heart border dl0, then the plain container dl1 (both XLU).
inline void MM_DrawForeignDoubleDefense(const ComboForeignDrawInfoOOT* info) {
    GraphicsContext* gfxCtx = gPlayState->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MM_FOREIGN_PIN_XLU();
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gDPSetGrayscaleColor(POLY_XLU_DISP++, 255, 255, 255, 255);
    gSPGrayscale(POLY_XLU_DISP++, true);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[0]);
    gSPGrayscale(POLY_XLU_DISP++, false);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[1]);
    CLOSE_DISPS(gfxCtx);
}

// Master Sword: seg8 OPA scroll + fixed scale/rotation (Randomizer_DrawMasterSword).
inline void MM_DrawForeignMasterSword(const ComboForeignDrawInfoOOT* info, bool drawFlame = true) {
    if (drawFlame && info->primColorXlu[3])
        DrawOotSlateRuneFlame(info->primColorXlu[0], info->primColorXlu[1], info->primColorXlu[2]);
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Opa(gfxCtx);
    MM_FOREIGN_PIN_OPA();
    gSPSegment(
        POLY_OPA_DISP++, 0x08,
        (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, play->state.frames * 1, 0, 32, 32, 1, 0, 0, 32, 32, 1, 0, 0, 0));
    Matrix_Scale(0.05f, 0.05f, 0.05f, MTXMODE_APPLY);
    Matrix_RotateZF(2.1f, MTXMODE_APPLY);
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
}

// Bronze Scale: the scale model on the SCALE seg8 scroll, recolored bronze. The OOT func's two color
// DLs are inline Gfx arrays (no OTR resource), so the prim/env pairs are emitted here verbatim.
inline void MM_DrawForeignBronzeScale(const ComboForeignDrawInfoOOT* info) {
    PlayState* play = gPlayState;
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Xlu(gfxCtx);
    MM_FOREIGN_PIN_XLU();
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, 0, play->state.frames * 2, -(play->state.frames * 2), 64, 64, 1,
                                             play->state.frames * 4, -(play->state.frames * 4), 32, 32, 2, -2, 4, -4));
    MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
    gDPPipeSync(POLY_XLU_DISP++);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x80, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 91, 51, 18, 255);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[0]);
    gDPPipeSync(POLY_XLU_DISP++);
    gDPSetPrimColor(POLY_XLU_DISP++, 0, 0x60, 255, 255, 255, 255);
    gDPSetEnvColor(POLY_XLU_DISP++, 255, 123, 0, 255);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[1]);
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
}

// Rotating custom OoT models. The two alpha values express independent, optional
// grayscale (OPA) and weapon-flame (XLU) colors; the flame never tints the model.
inline void MM_DrawForeignCustomGi(const ComboForeignDrawInfoOOT* info, bool shop = false, bool drawFlame = true) {
    if (drawFlame && info->primColorXlu[3])
        DrawOotSlateRuneFlame(info->primColorXlu[0], info->primColorXlu[1], info->primColorXlu[2]);
    Matrix_Push();
    const uint32_t bits = (static_cast<uint32_t>(gPlayState->gameplayFrames) * 2u) & 0xFFFFu;
    const int32_t rotation = bits >= 0x8000u ? static_cast<int32_t>(bits) - 0x10000 : bits;
    Matrix_RotateYF(rotation * .01f, MTXMODE_APPLY);
    const bool sword = info->neiShimmer > 0 && info->neiShimmer <= static_cast<int32_t>(NeiGi::Kind::Last) + 1 &&
                       NeiGi::IsSword(static_cast<NeiGi::Kind>(info->neiShimmer - 1));
    int selectedOpaque = -1;
    bool noCull = false;
    for (int i = 0; i < info->opCount; ++i) {
        const auto& op = info->ops[i];
        switch (op.op) {
            case CW_OP_ROTATE_X:
                Matrix_RotateXF(op.a * (3.14159265358979323846f / 32768.0f), MTXMODE_APPLY);
                break;
            case CW_OP_ROTATE_Z:
                Matrix_RotateZF(sword ? ComboSwordGi_SelectedTilt(op.a * (3.14159265358979323846f / 32768.0f), shop)
                                      : op.a * (3.14159265358979323846f / 32768.0f),
                                MTXMODE_APPLY);
                break;
            case CW_OP_SCALE:
                Matrix_Scale(op.a, op.b, op.c, MTXMODE_APPLY);
                break;
            case CW_OP_TRANSLATE:
                Matrix_Translate(op.a, op.b, op.c, MTXMODE_APPLY);
                break;
            case CW_OP_FRAME_PAIR:
                selectedOpaque = (static_cast<uint32_t>(gPlayState->gameplayFrames) >> static_cast<int>(op.a)) & 1u;
                break;
            case CW_OP_NO_CULL:
                noCull = true;
                break;
            default:
                break;
        }
    }
    if (info->scale > 0)
        Matrix_Scale(info->scale, info->scale, info->scale, MTXMODE_APPLY);
    GraphicsContext* gfxCtx = gPlayState->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    const int split = info->xluStart < 0 ? info->count : info->xluStart;
    for (int stream = 0; stream < 2; ++stream) {
        const int begin = stream ? split : 0;
        const int end = stream ? info->count : split;
        if (begin >= end)
            continue;
        if (stream) {
            Gfx_SetupDL25_Xlu(gfxCtx);
            MM_FOREIGN_PIN_XLU();
            MATRIX_FINALIZE_AND_LOAD(POLY_XLU_DISP++, gfxCtx);
            for (int i = begin; i < end; ++i)
                gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[i]);
        } else {
            Gfx_SetupDL25_Opa(gfxCtx);
            MM_FOREIGN_PIN_OPA();
            MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx);
            if (noCull)
                gSPClearGeometryMode(POLY_OPA_DISP++, G_CULL_BOTH);
            if (info->primColorOpa[3]) {
                gDPSetGrayscaleColor(POLY_OPA_DISP++, info->primColorOpa[0], info->primColorOpa[1],
                                     info->primColorOpa[2], info->primColorOpa[3]);
                gSPGrayscale(POLY_OPA_DISP++, true);
            }
            for (int i = begin; i < end; ++i) {
                if (selectedOpaque < 0 || i == selectedOpaque)
                    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[i]);
            }
            if (info->primColorOpa[3])
                gSPGrayscale(POLY_OPA_DISP++, false);
            if (noCull)
                gSPSetGeometryMode(POLY_OPA_DISP++, G_CULL_BACK);
        }
    }
    CLOSE_DISPS(gfxCtx);
    if (info->opCount == 1 && info->ops[0].op == CW_OP_ROTATE_Z && info->neiShimmer > 0 &&
        info->neiShimmer <= static_cast<int32_t>(NeiGi::Kind::Last) + 1 &&
        NeiGi::IsSword(static_cast<NeiGi::Kind>(info->neiShimmer - 1)))
        ComboDinSwordGi_DrawLayers(gPlayState, "oot", info->dls[0]);
    Matrix_Pop();
}

// Concrete inline/palette-remapped equipment uses the already-proven host-native renderer.
void DrawOotIronKnuckleAxe();
void DrawOotExtSpiritBreastplate();
void DrawOotExtChampionsTunic();
void DrawOotExtSagesTunic();
void DrawOotExtPegasusAnklet();
void DrawOotExtTrident();
void DrawOotExtClimbBoots();
void DrawOotExtRocBoots();
inline void MM_DrawForeignNativeEquipment(const ComboForeignDrawInfoOOT* info) {
    Matrix_Push();
    switch (static_cast<int32_t>(info->ops[0].a)) {
        case CW_OOT_EQUIP_AXE:
            DrawOotIronKnuckleAxe();
            break;
        case CW_OOT_EQUIP_SPIRIT_TUNIC:
            DrawOotExtSpiritBreastplate();
            break;
        case CW_OOT_EQUIP_CHAMPIONS_TUNIC:
            DrawOotExtChampionsTunic();
            break;
        case CW_OOT_EQUIP_SAGES_TUNIC:
            DrawOotExtSagesTunic();
            break;
        case CW_OOT_EQUIP_PEGASUS_BOOTS:
            DrawOotExtPegasusAnklet();
            break;
        case CW_OOT_EQUIP_TRIDENT:
            DrawOotExtTrident();
            break;
        case CW_OOT_EQUIP_CLIMB_BOOTS:
            DrawOotExtClimbBoots();
            break;
        case CW_OOT_EQUIP_ROC_BOOTS:
            DrawOotExtRocBoots();
            break;
    }
    Matrix_Pop();
}

// Silver rupee (NewDrops off): drop rupee DL on OOT's silver texture (Randomizer_DrawSilverRupee).
// The texture path exists in both archives, so seg 8 is bound under OOT's RM, as CfaBindSeg does.
inline bool MM_DrawForeignSilverRupee(const ComboForeignDrawInfoOOT* info) {
    std::shared_ptr<Ship::ResourceManager> rm = Ship::CrossRMRegistry::Get("oot");
    if (rm == nullptr || info->segTexPath == nullptr) {
        return false;
    }
    GraphicsContext* gfxCtx = gPlayState->state.gfxCtx;
    bool gray = info->primColorOpa[3] != 0;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL25_Opa(gfxCtx);
    MM_FOREIGN_PIN_OPA();
    if (info->scale > 0.0f) {
        Matrix_Scale(info->scale, info->scale, info->scale, MTXMODE_APPLY);
    }
    MATRIX_FINALIZE_AND_LOAD(POLY_OPA_DISP++, gfxCtx);
    {
        Ship::ResourceManagerScope rmScope(rm); // gSPSegment probes the path at record time
        gSPSegment(POLY_OPA_DISP++, 0x08, (uintptr_t)info->segTexPath);
    }
    gSPComboRMPush(POLY_OPA_DISP++, "oot");
    if (gray) {
        gDPSetGrayscaleColor(POLY_OPA_DISP++, info->primColorOpa[0], info->primColorOpa[1], info->primColorOpa[2], 255);
        gSPGrayscale(POLY_OPA_DISP++, true);
    }
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    if (gray) {
        gSPGrayscale(POLY_OPA_DISP++, false);
    }
    gSPComboRMPop(POLY_OPA_DISP++);
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08 };
    MM_RestoreForeignSegs(segs, 1);
    return true;
}

// Draw a foreign (OOT-bound) item's real OOT model at the current model matrix. Any resolution
// failure falls back to the sentinel blue rupee (the RI_COMBO_FOREIGN item's GID_RUPEE_BLUE), so we
// never draw blank. Mirrors Randomizer_DrawComboForeign (soh/.../draw.cpp).
inline void MM_DrawComboForeign(RandoCheckId randoCheckId, bool shop = false, int mmPickup = 0,
                                Actor* actor = nullptr) {
    const ComboForeignDrawInfoOOT* info =
        (randoCheckId != RC_UNKNOWN) ? ComboResolveForeignDrawInfoOOT(randoCheckId) : nullptr;
    if (info == nullptr) {
        GetItem_Draw(gPlayState, GID_RUPEE_BLUE);
        return;
    }
#ifdef COMBO_GI_RECEIPT_TRACE
    ComboGiReceiptTrace::Dispatch(info->resolvedName.c_str(), info->drawKind, info->dls, info->count, info->scale,
                                  info->nativeMmItem, mmPickup);
#endif
    if (ComboDrawMagicMirrorForeign(gPlayState, info->drawKind))
        return;
    if (info->nativeMmItem >= 0) {
        Rando::DrawResolvedItem(static_cast<RandoItemId>(info->nativeMmItem), RC_UNKNOWN, actor);
        return;
    }

    // ComboShip: animated class — combo-owned skeletal draw (any failure -> sentinel, never blank).
    if (info->animOk) {
        if (!ComboForeignAnim_Draw(&info->anim, "oot", gPlayState)) {
            GetItem_Draw(gPlayState, GID_RUPEE_BLUE);
        }
        return;
    }

    if (info->drawKind == CW_DRAW_KIND_NEI_CANE) {
        Matrix_Push();
        static constexpr RandoItemId skills[] = { RI_NONE,
                                                  RI_OOT_NEI_CANE_OF_SOMARIA,
                                                  RI_OOT_NEI_CANE_PACCI_FLIP,
                                                  RI_OOT_NEI_CANE_SOMARIA_BLOCK,
                                                  RI_OOT_NEI_CANE_PACCI_STONE,
                                                  RI_OOT_NEI_CANE_SOMARIA_PLATFORM,
                                                  RI_OOT_NEI_CANE_PACCI_ULTRAHAND };
        if (info->neiLegacyCane == 6)
            DrawOotNeiUltrahand();
        else if (info->neiLegacyCane > 0 && info->neiLegacyCane < 6)
            DrawOotNeiCaneOfSomaria(skills[info->neiLegacyCane]);
        else
            GetItem_Draw(gPlayState, GID_RUPEE_BLUE);
        Matrix_Pop();
        if (info->itemShimmer)
            ComboDrawMaskShimmer(gPlayState, nullptr, info->itemShimmerColor, "oot");
        return;
    }
    if (info->drawKind == CW_DRAW_KIND_NEI_GI) {
        CwItemDrawInfo recipe{};
        for (int i = 0; i < info->count; ++i)
            recipe.dlists[i] = info->dls[i];
        recipe.dlistCount = info->count;
        recipe.xluStartIndex = info->xluStart;
        recipe.scale = info->scale;
        recipe.neiEffect = info->neiEffect;
        recipe.neiShimmer = info->neiShimmer;
        memcpy(recipe.neiEffectCenter, info->neiEffectCenter, sizeof(recipe.neiEffectCenter));
        recipe.neiSomariaUpgrade = info->neiSomariaUpgrade;
        recipe.itemShimmer = info->itemShimmer;
        MM_DrawNeiGi(recipe, shop, mmPickup);
        return;
    }
    const bool swordIdentity = info->neiShimmer > 0 &&
                               info->neiShimmer <= static_cast<int32_t>(NeiGi::Kind::Last) + 1 &&
                               NeiGi::IsSword(static_cast<NeiGi::Kind>(info->neiShimmer - 1));
    const bool fitModel =
        info->count > 0 &&
        ((info->drawKind == CW_DRAW_KIND_CUSTOM_GI &&
          (info->opCount == 0 || (info->opCount == 1 && info->ops[0].op == CW_OP_ROTATE_Z)) &&
          (mmPickup || swordIdentity)) ||
         (swordIdentity && (info->drawKind == CW_DRAW_KIND_SIMPLE || info->drawKind == CW_DRAW_KIND_MASTER_SWORD ||
                            info->drawKind == CW_DRAW_KIND_GORON_SWORD)));
    const bool swordFlame = swordIdentity && info->primColorXlu[3] &&
                            (info->drawKind == CW_DRAW_KIND_CUSTOM_GI || info->drawKind == CW_DRAW_KIND_MASTER_SWORD);
    if (fitModel && swordIdentity) {
        Matrix_Push();
        ComboSwordGi_ApplyPresentationSize(shop, mmPickup);
    }
    if (swordFlame) {
        Matrix_Push();
        ComboSwordGi_ApplyEffectFit(static_cast<NeiGi::Kind>(info->neiShimmer - 1), shop, mmPickup);
        DrawOotSlateRuneFlame(info->primColorXlu[0], info->primColorXlu[1], info->primColorXlu[2]);
        Matrix_Pop();
    }
    if (fitModel) {
        Matrix_Push();
        const float scale = info->drawKind == CW_DRAW_KIND_MASTER_SWORD  ? .05f
                            : info->drawKind == CW_DRAW_KIND_GORON_SWORD ? 1.f
                            : info->scale > 0                            ? info->scale
                                                                         : 1.f;
        const float tilt =
            info->drawKind == CW_DRAW_KIND_MASTER_SWORD ? 2.1f
            : info->drawKind == CW_DRAW_KIND_CUSTOM_GI && info->opCount == 1 && info->ops[0].op == CW_OP_ROTATE_Z
                ? (swordIdentity ? ComboSwordGi_SelectedTilt(info->ops[0].a * (3.14159265358979323846f / 32768.f), shop)
                                 : info->ops[0].a * (3.14159265358979323846f / 32768.f))
                : 0.f;
        ComboSwordGi_ApplyModelsFit("oot", info->dls, info->count, scale, tilt, shop, mmPickup);
    }
    const int reward = RewardGi_ProfileForPaths(info->dls[0], info->count > 1 ? info->dls[1] : nullptr);
    const bool overlayShimmer = info->itemShimmer && !reward && info->drawKind != CW_DRAW_KIND_ELEMENTAL_ARROW &&
                                info->drawKind != CW_DRAW_KIND_MAGIC_SPELL;
    if (overlayShimmer) {
        Matrix_Push();
    }
    switch (info->drawKind) {
        case CW_DRAW_KIND_ELEMENTAL_ARROW: {
            auto core = *info;
            core.count = 1;
            core.xluStart = -1;
            MM_DrawForeignSimple(&core);
            NeiGi_DrawElementalArrowForOwner(gPlayState, info->neiEffect, 0, shop);
            break;
        }
        case CW_DRAW_KIND_GORON_SWORD:
            MM_DrawForeignGoronSword(info);
            break;
        case CW_DRAW_KIND_DEKU_NUTS:
            MM_DrawForeignDekuNuts(info);
            break;
        case CW_DRAW_KIND_RECOVERY_HEART:
            MM_DrawForeignRecoveryHeart(info);
            break;
        case CW_DRAW_KIND_FISH:
            MM_DrawForeignFish(info);
            break;
        case CW_DRAW_KIND_POTION:
            MM_DrawForeignPotion(info);
            break;
        case CW_DRAW_KIND_MIRROR_SHIELD:
            MM_DrawForeignMirrorShield(info);
            break;
        case CW_DRAW_KIND_BLUE_FIRE:
            MM_DrawForeignBlueFire(info);
            break;
        case CW_DRAW_KIND_POES:
            MM_DrawForeignPoes(info);
            break;
        case CW_DRAW_KIND_FAIRY:
            MM_DrawForeignFairy(info);
            break;
        case CW_DRAW_KIND_JEWEL:
            MM_DrawForeignJewel(info);
            break;
        case CW_DRAW_KIND_MAGIC_SPELL:
            if (!NeiGi_DrawElementalSpell(gPlayState, info->neiEffect, 0, shop))
                MM_DrawForeignMagicSpell(info);
            break;
        case CW_DRAW_KIND_SCALE:
            MM_DrawForeignScale(info);
            break;
        case CW_DRAW_KIND_SKULL_TOKEN:
            MM_DrawForeignSkullToken(info);
            break;
        case CW_DRAW_KIND_SONG_GI:
        case CW_DRAW_KIND_MUSIC_NOTE:
            MM_DrawForeignMusicNote(info);
            break;
        case CW_DRAW_KIND_BOSS_SOUL:
            MM_DrawForeignBossSoul(info);
            break;
        case CW_DRAW_KIND_OOT_MORPHA_SOUL:
            MM_DrawForeignMorphaSoul(info);
            break;
        case CW_DRAW_KIND_GRAYSCALE_LAYERS:
            MM_DrawForeignGrayscaleLayers(info);
            break;
        case CW_DRAW_KIND_COLOR_LAYERS:
            MM_DrawForeignColorLayers(info);
            break;
        case CW_DRAW_KIND_GRAYSCALE_XLU:
            MM_DrawForeignGrayscaleXlu(info);
            break;
        case CW_DRAW_KIND_DOUBLE_DEFENSE:
            MM_DrawForeignDoubleDefense(info);
            break;
        case CW_DRAW_KIND_OOT_NATIVE_EQUIPMENT:
            MM_DrawForeignNativeEquipment(info);
            break;
        case CW_DRAW_KIND_BOTTLE_CONTENTS:
            if (!ComboBottleContents_Draw(gPlayState, ComboBottleContents_Profile(info->dls[0]))) {
                MM_DrawForeignSimple(info);
            }
            break;
        case CW_DRAW_KIND_SEASON_GI:
            if (info->neiEffect == 5)
                MM_DrawForeignCustomGi(info, shop, !swordFlame);
            NeiGi_DrawSeasonOverlay(gPlayState, info->neiEffect, "oot");
            break;
        case CW_DRAW_KIND_CUSTOM_GI:
            MM_DrawForeignCustomGi(info, shop, !swordFlame);
            break;
        case CW_DRAW_KIND_MASTER_SWORD:
            MM_DrawForeignMasterSword(info, !swordFlame);
            break;
        case CW_DRAW_KIND_BRONZE_SCALE:
            MM_DrawForeignBronzeScale(info);
            break;
        case CW_DRAW_KIND_SILVER_RUPEE:
            if (!MM_DrawForeignSilverRupee(info)) {
                GetItem_Draw(gPlayState, GID_RUPEE_BLUE);
            }
            break;
        case CW_DRAW_KIND_MM_SPIN_ATTACK:
            MM_DrawForeignSpinAttack(info);
            break;
        case CW_DRAW_KIND_MM_MASK:
        case CW_DRAW_KIND_MM_REMAINS:
        case CW_DRAW_KIND_MAGIC_JAR:
        case CW_DRAW_KIND_SIMPLE:
        default:
            MM_DrawForeignSimple(info);
            break;
    }
    if (reward >= 1 && reward <= 6)
        NeiGi_DrawRewardMaterial(gPlayState, reward, info->dls[0], info->count > 1 ? info->dls[1] : nullptr, "oot");
    if (overlayShimmer)
        Matrix_Pop();
    if (fitModel && swordIdentity)
        Matrix_Pop();
    if (overlayShimmer) {
        if (swordIdentity) {
            Matrix_Push();
            ComboSwordGi_ApplyEffectFit(static_cast<NeiGi::Kind>(info->neiShimmer - 1), shop, mmPickup);
        }
        if (info->neiShimmer > 0 && info->neiShimmer <= static_cast<int32_t>(NeiGi::Kind::Last) + 1 &&
            NeiGi::IsSword(static_cast<NeiGi::Kind>(info->neiShimmer - 1)))
            NeiGi_DrawMesh(gPlayState, NeiGi::SampleSpecial(
                                           static_cast<NeiGi::Kind>(info->neiShimmer - 1), gPlayState->gameplayFrames,
                                           NeiGi_CameraBasis(gPlayState),
                                           ComboSwordGi_ParticleScale(static_cast<NeiGi::Kind>(info->neiShimmer - 1),
                                                                      shop, mmPickup, fitModel)));
        const bool mmOwner = info->drawKind == CW_DRAW_KIND_MM_MASK || info->drawKind == CW_DRAW_KIND_MM_REMAINS;
        if (info->drawKind == CW_DRAW_KIND_SONG_GI)
            NeiGi_DrawSongOverlay(gPlayState, info->neiEffect, "oot");
        else if (info->neiShimmer > 0 && info->neiShimmer <= static_cast<int32_t>(NeiGi::Kind::Last) + 1)
            NeiGi_DrawMesh(gPlayState,
                           NeiGi::SampleShimmer(gPlayState->gameplayFrames, true, NeiGi_CameraBasis(gPlayState),
                                                static_cast<NeiGi::Kind>(info->neiShimmer - 1)));
        else if (info->neiEffect == static_cast<int32_t>(NeiGi::Kind::Pokeball))
            NeiGi_DrawMesh(gPlayState, NeiGi::SampleShimmer(gPlayState->gameplayFrames, true,
                                                            NeiGi_CameraBasis(gPlayState), NeiGi::Kind::Pokeball));
        else if (info->itemShimmer == CW_ITEM_SHIMMER_TINTED)
            NeiGi_DrawTintedShimmerOverlay(gPlayState, info->itemShimmerColor, mmOwner ? "mm" : "oot");
        else
            ComboDrawMaskShimmer(gPlayState, nullptr, info->itemShimmerColor, mmOwner ? "mm" : "oot");
    }
    if (overlayShimmer && swordIdentity)
        Matrix_Pop();
    if (fitModel && !swordIdentity)
        Matrix_Pop();
    if (fitModel && swordIdentity)
        Matrix_Pop();
}

#undef MM_FOREIGN_PIN_OPA
#undef MM_FOREIGN_PIN_XLU

#endif // COMBO_FOREIGN_DRAW_MM_H
