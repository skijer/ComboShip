/* combo/menu/ComboForeignDrawOOT.h — ComboShip: cross-game foreign-item rendering, OOT (host) side.
 * The exact mirror of combo/menu/ComboForeignDrawMM.h in the opposite direction: an OOT check holding
 * RG_COMBO_FOREIGN actually holds an MM item, so we render the REAL MM model by asking 2ship.dll
 * (MM_GetItemDrawInfo, C ABI in combo/menu/ComboItemDrawABI.h) which display lists draw it, then
 * submitting them as "__OTR__@mm:"-routed paths that the shared Fast3D interpreter resolves against
 * MM's ResourceManager (CrossRMRegistry + scoped routing — see libultraship interpreter.cpp).
 *
 * All foreign checks share one sentinel entry, so the check identity rides in the entry itself:
 * Context::GetFinalGIEntry stamps comboForeignCheck (ItemTableTypes.h) into every entry it returns,
 * which covers every draw path that knows its check (shop shelves, freestanding, chests, queued
 * get-item, ...). Entries built without a check carry RC_UNKNOWN_CHECK and degrade to the sentinel.
 *
 * TU-GLUE HEADER (menu-extraction pattern): include ONCE from
 * soh/soh/Enhancements/randomizer/draw.cpp, inside its #ifdef COMBO_BUILD, AFTER the engine headers
 * (OPEN_DISPS, Gfx_SetupDL_25Opa/Xlu, Matrix_*, gbi macros, gPlayState, gSaveContext, GetItem_Draw)
 * and after ComboForeignAnim.h. Not standalone.
 */
#ifndef COMBO_FOREIGN_DRAW_OOT_H
#define COMBO_FOREIGN_DRAW_OOT_H
#include "ComboMagicMirrorForeign.h"

#include "ComboSpinAttackGi.h"
#include "ComboMaskShimmer.h"
#include "ComboFairyBottle.h"
#include "ComboBottleContents.h"
#include "ComboSwordGiFit.h"
#include "ComboDinSwordGi.h"
#include "ComboFairyBottleDraw.h"
#include "soh/Enhancements/randomizer/NeiGiRender.h"

#ifndef OPEN_DISPS
#error "ComboForeignDrawOOT.h is TU-glue: include the host engine headers before it"
#endif

#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "ComboItemDrawABI.h"
#include "ComboCollectibleShimmerDraw.h"
#include "soh/Enhancements/randomizer/hook_handlers.h" // OOT_LookupForeign / OOT_GetQueuedDrawCheck
#include "soh/Enhancements/randomizer/static_data.h"
#include "ComboResolve.h" // Combo_ResolveSym (process-wide combo-ABI symbol resolution)
#include "ComboSwordGiAssetSelection.h"

namespace {
struct ComboForeignDrawInfo {
    bool ok = false;
    int32_t count = 0;
    int32_t xluStart = -1;            // first XLU entry in dls[] order; -1 = all OPA
    float scale = 0.0f;               // extra model scale; 0 = none (MM remains: 0.02)
    bool appearanceDependent = false; // Palette and Alt state remain live after a dungeon-item grant.
    int32_t itemShimmer = CW_ITEM_SHIMMER_NONE;
    uint8_t itemShimmerColor[4] = {};
    int32_t neiShimmer = 0;
    bool hasEnvColor = false; // emit env color before the DLs (MM song notes)
    uint8_t envColor[4] = { 0, 0, 0, 0 };
    bool xluSeg8TexScroll = false;     // bind segment 8 to the flame texscroll before the XLU layer (skull token)
    const char* matAnimPath = nullptr; // MM TextureAnimation resource to replicate before the DLs (Moon's Tear)
    bool matAnimBindOpa = false;       // also bind the animated segment on the OPA layer (item body samples it)
    bool matAnimBillboard = false;     // Matrix_ReplaceRotation(billboardMtxF) before the XLU layer (glow)
    int32_t neiEffect = 0;
    float neiEffectCenter[3] = {};
    int32_t drawKind = CW_DRAW_KIND_SIMPLE;   // non-SIMPLE = replicate a specific MM draw func
    uint8_t primColorXlu[4] = { 0, 0, 0, 0 }; // MM_SOUL_FLAME color
    uint8_t primColorOpa[4] = {};             // independent custom/True Master blade tint
    int32_t opCount = 0;                      // CW_DRAW_KIND_OPS payload
    CwDrawOp ops[CW_DRAW_MAX_OPS] = {};
    const char* dls[CW_DRAW_MAX_DLISTS] = { nullptr }; // interned "__OTR__@mm:..." routed paths
    // MM's own setup DL for each stream (raw Gfx* in 2ship.dll), or null for our 25 Opa/Xlu.
    const void* setupDlOpa = nullptr;
    const void* setupDlXlu = nullptr;
    // ComboShip: animated class (no static DL row — MM stray fairies). When animOk, anim describes
    // the item and ComboForeignAnim_Draw renders it; path strings point at 2ship.dll statics.
    bool animOk = false;
    CwItemAnimDrawInfo anim{};
    // Recipe chosen from live save state (progressive tier, Triforce shard, junk/trap) — re-resolve
    // every frame instead of caching, or the first model drawn sticks for the whole save slot.
    bool stateDependent = false;
    // Resolved tier name (e.g. "Large Quiver") when a progressive placeholder converted, else empty.
    std::string resolvedName;
};

// Routed path strings must outlive the frame (the GBI wrapper emits the raw pointer into the
// display list; the interpreter dereferences it later), so intern them for the process lifetime.
inline const char* ComboInternRoutedPath(const std::string& s) {
    static std::unordered_set<std::string> sPool; // node-based: c_str() stable across rehash
    return sPool.insert(s).first->c_str();
}

// One resolution attempt's outcome. NotReady = a producer/lookup that simply isn't up yet (2ship.dll
// not resident, foreign map not built, MM's rando state null), which must NEVER be negative-cached.
enum class ComboForeignResolve { Ok, Unknown, NotReady };

inline ComboForeignResolve ComboFillForeignDrawInfo(RandomizerCheck rc, int slot, ComboForeignDrawInfo& info) {
    const std::string checkName = Rando::StaticData::GetLocation(rc)->GetName();
    const ComboRando::ForeignItem* fi = OOT_LookupForeign(slot, checkName);
    if (fi == nullptr || fi->itemGame != ComboRando::GAME_MM) {
        return ComboForeignResolve::Unknown;
    }

    static Fn_GetItemDrawInfo sGetItemDrawInfo = nullptr;
    if (sGetItemDrawInfo == nullptr) {
        // 2ship already loaded by the exe (ComboMenuModel pattern); resolution is process-wide.
        sGetItemDrawInfo = (Fn_GetItemDrawInfo)Combo_ResolveSym("2ship", "MM_GetItemDrawInfo");
    }
    if (sGetItemDrawInfo == nullptr) {
        return ComboForeignResolve::NotReady; // 2ship.dll may simply not be resident yet
    }
    // A disguised trap must draw the item it pretends to be. Same namespace, so the itemGame dispatch
    // above is unaffected. Not state-dependent: like OOT, the disguise holds until the get-item cutscene.
    const char* drawName = fi->HasDisguise() ? fi->fakeItemName.c_str() : fi->itemName.c_str();
    CwItemDrawInfo raw{};
    int32_t rcStatic = sGetItemDrawInfo(drawName, &raw);
    const int32_t baseChoice = ComboSwordGi_SelectBaseAssets(raw, ResourceMgr_IsAltAssetsEnabled());
    if (baseChoice == CW_DRAW_NOT_READY)
        return ComboForeignResolve::NotReady;
    if (baseChoice == 2)
        rcStatic = 1;
    if (rcStatic == CW_DRAW_NOT_READY) {
        return ComboForeignResolve::NotReady; // MM's rando state isn't up — retry, don't freeze
    }
    if (rcStatic == 0 ||
        (raw.dlistCount <= 0 && raw.drawKind != CW_DRAW_KIND_SEASON_GI && raw.drawKind != CW_DRAW_KIND_MAGIC_MIRROR)) {
        // ComboShip: no static DL row — try the animated ABI (MM stray fairies). MM only describes
        // the item; ComboForeignAnim_Draw (combo/menu/ComboForeignAnim.h) loads + draws it.
        static Fn_GetItemAnimDrawInfo sGetItemAnimDrawInfo = nullptr;
        if (sGetItemAnimDrawInfo == nullptr) {
            sGetItemAnimDrawInfo = (Fn_GetItemAnimDrawInfo)Combo_ResolveSym("2ship", "MM_GetItemAnimDrawInfo");
        }
        if (sGetItemAnimDrawInfo == nullptr) {
            return ComboForeignResolve::NotReady;
        }
        int32_t rcAnim = sGetItemAnimDrawInfo(drawName, &info.anim);
        if (rcAnim == CW_DRAW_NOT_READY) {
            return ComboForeignResolve::NotReady;
        }
        if (rcAnim != 0) {
            info.animOk = true;
            info.stateDependent = info.anim.stateDependent != 0;
            info.ok = true;
            return ComboForeignResolve::Ok;
        }
        return ComboForeignResolve::Unknown; // unknown item or non-portable draw func -> sentinel
    }

    if (raw.drawKind == CW_DRAW_KIND_MAGIC_MIRROR && (raw.dlistCount != 0 || raw.opCount != 0))
        return ComboForeignResolve::Unknown;
    static constexpr char kOtrPrefix[] = "__OTR__";
    int32_t n = raw.dlistCount < CW_DRAW_MAX_DLISTS ? raw.dlistCount : CW_DRAW_MAX_DLISTS;
    if (n < CwMinDlistsForKind(raw.drawKind)) {
        return ComboForeignResolve::Unknown; // handler would blind-index a missing slot
    }
    for (int32_t i = 0; i < n; i++) {
        const char* p = raw.dlists[i];
        if (p == nullptr || strncmp(p, kOtrPrefix, sizeof(kOtrPrefix) - 1) != 0) {
            return ComboForeignResolve::Unknown; // not an OTR path literal — can't route it
        }
        const bool routedMm = strncmp(p, "__OTR__@mm:", 11) == 0;
        const bool routedOot = strncmp(p, "__OTR__@oot:", 12) == 0;
        const bool routedBase = strncmp(p, "__OTR__@oot-gi-base:", 20) == 0;
        if (p[7] == '@' && !routedMm && !routedOot && !routedBase)
            return ComboForeignResolve::Unknown;
        info.dls[i] = ComboInternRoutedPath(routedMm || routedOot || routedBase
                                                ? std::string(p)
                                                : std::string("__OTR__@mm:") + (p + sizeof(kOtrPrefix) - 1));
    }
    info.count = n;
    info.xluStart = raw.xluStartIndex;
    info.scale = raw.scale;
    info.setupDlOpa = raw.setupDlOpa;
    info.setupDlXlu = raw.setupDlXlu;
    info.hasEnvColor = raw.hasEnvColor != 0;
    info.xluSeg8TexScroll = raw.xluSeg8TexScroll != 0;
    info.matAnimPath = raw.matAnimPath; // 2ship static literal (process-lifetime); loaded, not emitted
    info.matAnimBindOpa = raw.matAnimBindOpa != 0;
    info.matAnimBillboard = raw.matAnimBillboard != 0;
    info.stateDependent = raw.stateDependent != 0;
    info.appearanceDependent = raw.stateDependent == 2;
    info.itemShimmer = raw.itemShimmer;
    info.neiShimmer = raw.neiShimmer;
    memcpy(info.itemShimmerColor, raw.itemShimmerColor, sizeof(info.itemShimmerColor));
    if (raw.resolvedName != nullptr) {
        info.resolvedName = raw.resolvedName;
    }
    info.drawKind = raw.drawKind;
    if (raw.drawKind == CW_DRAW_KIND_ELEMENTAL_ARROW &&
        (raw.neiEffect < 1 || raw.neiEffect > 3 || raw.dlistCount != 1 || raw.opCount != 0))
        return ComboForeignResolve::Unknown;
    if (raw.opCount < 0 || raw.opCount > CW_DRAW_MAX_OPS)
        return ComboForeignResolve::Unknown;
    if (raw.drawKind == CW_DRAW_KIND_SEASON_GI &&
        (raw.neiEffect < 1 || raw.neiEffect > 6 || (raw.neiEffect != 5 ? n != 0 || raw.opCount != 0 : n < 1)))
        return ComboForeignResolve::Unknown;
    info.neiEffect = raw.neiEffect;
    std::memcpy(info.neiEffectCenter, raw.neiEffectCenter, sizeof(info.neiEffectCenter));
    info.opCount = raw.opCount < CW_DRAW_MAX_OPS ? raw.opCount : CW_DRAW_MAX_OPS;
    memcpy(info.ops, raw.ops, sizeof(info.ops));
    for (int32_t i = 0; i < 4; i++) {
        info.envColor[i] = raw.envColor[i];
        info.primColorXlu[i] = raw.primColorXlu[i];
        info.primColorOpa[i] = raw.primColorOpa[i];
    }
    info.ok = true;
    return ComboForeignResolve::Ok;
}

// Recipe cache, swept per save slot and per foreign-map generation. Shared by the resolver and the
// grant-time latch below so both observe the same sweep.
struct ComboForeignDrawCache {
    std::unordered_map<int32_t, ComboForeignDrawInfo> map;
    int slot = -1;
    uint64_t gen = (uint64_t)-1;
};

inline ComboForeignDrawCache& ComboForeignDrawCacheGet() {
    static ComboForeignDrawCache c;
    int slot = gSaveContext.fileNum;
    uint64_t gen = OOT_ForeignMapGen();
    if (slot != c.slot || gen != c.gen) {
        c.map.clear();
        c.slot = slot;
        c.gen = gen;
    }
    return c;
}

// Full lookup chain (foreign map -> MM export -> routed strings), cached per check per slot per
// foreign-map generation so it runs once per check instead of every frame.
inline const ComboForeignDrawInfo* ComboResolveForeignDrawInfo(RandomizerCheck rc) {
    ComboForeignDrawCache& c = ComboForeignDrawCacheGet();
    auto cached = c.map.find(rc);
    if (cached != c.map.end() && !cached->second.stateDependent) {
        return cached->second.ok ? &cached->second : nullptr;
    }
    // A state-dependent recipe (progressive tier, Triforce shard, junk/trap) is re-resolved every
    // frame; caching it would freeze whichever model happened to be correct on the first draw.
    ComboForeignDrawInfo info{}; // built locally: a failure must not clobber a live cached recipe
    if (ComboFillForeignDrawInfo(rc, c.slot, info) == ComboForeignResolve::NotReady) {
        c.map.erase(rc); // transient — retry next frame instead of freezing the sentinel in
        return nullptr;
    }
    ComboForeignDrawInfo& entry = c.map[rc]; // Unknown caches ok=false: one lookup, then sentinel
    entry = info;
    return entry.ok ? &entry : nullptr;
}

// ComboShip: freeze this check's recipe at the tier it is ABOUT to grant. The cross-grant mutates
// MM's dormant save mid-presentation, so a live re-resolve would flip the held-up model next frame.
inline void ComboLatchForeignDraw(RandomizerCheck rc) {
    if (rc == RC_UNKNOWN_CHECK) {
        return;
    }
    ComboForeignDrawCache& c = ComboForeignDrawCacheGet();
    ComboForeignDrawInfo info{};
    if (ComboFillForeignDrawInfo(rc, c.slot, info) != ComboForeignResolve::Ok) {
        return; // nothing written, nothing erased: the draw stays live, i.e. no worse than before
    }
    if (info.animOk) {
        return; // MM's anim class (stray fairies, souls, minifrogs) is never state-dependent
    }
    // The fill's lookup may have built the foreign map, bumping the generation the cache keys on;
    // adopt it (dropping entries resolved against the old map) so this latch survives.
    uint64_t gen = OOT_ForeignMapGen();
    if (gen != c.gen) {
        c.map.clear();
        c.gen = gen;
    }
    info.stateDependent = info.appearanceDependent; // Freeze progressive tiers, retain live cosmetics.
    c.map[rc] = info;
}

// Frozen tier name only: NULL unless latched (stateDependent == false) with a non-empty name. Never
// serves a live entry, so a pickup can't show the tier the NEXT copy would give.
inline const char* ComboForeignLatchedName(RandomizerCheck rc) {
    ComboForeignDrawCache& c = ComboForeignDrawCacheGet();
    auto it = c.map.find(rc);
    if (it == c.map.end() || it->second.stateDependent || it->second.resolvedName.empty()) {
        return nullptr;
    }
    return it->second.resolvedName.c_str();
}

// Live tier name for previews: runs the same per-frame resolver the shelf model uses.
inline const char* ComboForeignLiveName(RandomizerCheck rc) {
    const ComboForeignDrawInfo* info = ComboResolveForeignDrawInfo(rc);
    if (info == nullptr || info->resolvedName.empty()) {
        return nullptr;
    }
    return info->resolvedName.c_str();
}

} // namespace

// ---- Non-portable MM draw funcs. Each handler is a 1:1 port of the MM get-item func
// (mm/src/code/z_draw.c) or bespoke Rando draw func (mm/2s2h/Rando/DrawItem.cpp, DrawFuncs.cpp),
// re-binding the segment(s) it needs in OOT's frame with OOT's own gbi BEFORE submitting the routed
// @mm: DLs, then restoring them. Where MM's func is identical to its OOT twin the shared CwDrawKind
// is reused, so these are ports of MM's code that happen to read like OOT's. Free (non-namespaced)
// inline functions so the OPEN_DISPS block-scope decls keep C linkage — see ComboForeignAnim.h.

// Restore the segments a handler bound to a benign empty DL so later same-frame commands don't
// sample our scroll DLs. Mirrors MM_RestoreForeignSegs / the hygiene in ComboForeignAnim.h.
inline void OOT_RestoreForeignSegs(PlayState* play, const int32_t* segs, int32_t count) {
    // Array of ENDDLs: DLs may call through a bound segment at an index > 0 — see CfaEmptyDL.
    Gfx* empty = (Gfx*)Graph_Alloc(play->state.gfxCtx, 8 * sizeof(Gfx));
    Gfx* e = empty;
    for (int32_t iEmpty = 0; iEmpty < 8; iEmpty++) {
        gSPEndDisplayList(e++);
    }
    OPEN_DISPS(play->state.gfxCtx);
    for (int32_t i = 0; i < count; i++) {
        gSPSegment(POLY_OPA_DISP++, segs[i], (uintptr_t)empty);
        gSPSegment(POLY_XLU_DISP++, segs[i], (uintptr_t)empty);
    }
    CLOSE_DISPS(play->state.gfxCtx);
}

inline void OOT_DrawForeignMagicSpell(PlayState* play, const ComboForeignDrawInfo* info) {
    if (info->count < 3 || !NeiGi_CanDrawElementalSpellFallback(play))
        return;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    // MM's imported spell recipe explicitly carries OoT's diamond/orb assets.
    gSPGrayscale(POLY_XLU_DISP++, false);
    gSPComboRMPush(POLY_XLU_DISP++, "oot");
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, play->state.frames * 2, -(play->state.frames * 6),
                                             32, 32, 1, play->state.frames, -(play->state.frames * 2), 32, 32, 2, -6, 1,
                                             -2));
    gSPMatrix(POLY_XLU_DISP++, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__),
              G_MTX_MODELVIEW | G_MTX_LOAD | G_MTX_NOPUSH);
    for (int i = 0; i < 3; ++i)
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[i]);
    gSPComboRMPop(POLY_XLU_DISP++);
    CLOSE_DISPS(play->state.gfxCtx);
    const int32_t segs[] = { 0x08 };
    OOT_RestoreForeignSegs(play, segs, 1);
}

#define COMBO_FOREIGN_MTX(disp) \
    gSPMatrix(disp, Matrix_NewMtx(play->state.gfxCtx, (char*)__FILE__, __LINE__), G_MTX_MODELVIEW | G_MTX_LOAD)

// Pin prim+env to white right after a setup DL, before the handler's own colour commands: a layer
// the recipe doesn't tint must not inherit whatever the host frame last set.
#define OOT_FOREIGN_PIN_OPA()                                       \
    do {                                                            \
        gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, 255); \
        gDPSetEnvColor(POLY_OPA_DISP++, 255, 255, 255, 255);        \
    } while (0)
#define OOT_FOREIGN_PIN_XLU()                                       \
    do {                                                            \
        gDPSetPrimColor(POLY_XLU_DISP++, 0, 0, 255, 255, 255, 255); \
        gDPSetEnvColor(POLY_XLU_DISP++, 255, 255, 255, 255);        \
    } while (0)

// Biggoron's Sword: seg8 OPA scroll (GetItem_DrawGoronSword).
inline void OOT_DrawForeignGoronSword(PlayState* play, const ComboForeignDrawInfo* info) {
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    OOT_FOREIGN_PIN_OPA();
    gSPSegment(POLY_OPA_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(play->state.gfxCtx, G_TX_RENDERTILE, play->state.frames * 1,
                                             play->state.frames * 0, 32, 32, 1, play->state.frames * 0,
                                             play->state.frames * 0, 32, 32, 1, 0, 0, 0));
    COMBO_FOREIGN_MTX(POLY_OPA_DISP++);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    CLOSE_DISPS(play->state.gfxCtx);
    int32_t segs[] = { 0x08 };
    OOT_RestoreForeignSegs(play, segs, 1);
}

// Deku Nuts: seg8 OPA scroll (GetItem_DrawDekuNuts).
inline void OOT_DrawForeignDekuNuts(PlayState* play, const ComboForeignDrawInfo* info) {
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    OOT_FOREIGN_PIN_OPA();
    gSPSegment(POLY_OPA_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(play->state.gfxCtx, G_TX_RENDERTILE, play->state.frames * 6,
                                             play->state.frames * 6, 32, 32, 1, play->state.frames * 6,
                                             play->state.frames * 6, 32, 32, 6, 6, 6, 6));
    COMBO_FOREIGN_MTX(POLY_OPA_DISP++);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    CLOSE_DISPS(play->state.gfxCtx);
    int32_t segs[] = { 0x08 };
    OOT_RestoreForeignSegs(play, segs, 1);
}

// Recovery Heart: seg8 XLU scroll (GetItem_DrawRecoveryHeart).
inline void OOT_DrawForeignRecoveryHeart(PlayState* play, const ComboForeignDrawInfo* info) {
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    OOT_FOREIGN_PIN_XLU();
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(play->state.gfxCtx, G_TX_RENDERTILE, play->state.frames * 0,
                                             -(int32_t)(play->state.frames * 3), 32, 32, 1, play->state.frames * 0,
                                             -(int32_t)(play->state.frames * 2), 32, 32, 0, -3, 0, -2));
    COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[0]);
    CLOSE_DISPS(play->state.gfxCtx);
    int32_t segs[] = { 0x08 };
    OOT_RestoreForeignSegs(play, segs, 1);
}

// Fish container: seg8 XLU scroll (GetItem_DrawFish).
inline void OOT_DrawForeignFish(PlayState* play, const ComboForeignDrawInfo* info) {
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    OOT_FOREIGN_PIN_XLU();
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(play->state.gfxCtx, G_TX_RENDERTILE, play->state.frames * 0,
                                             play->state.frames * 1, 32, 32, 1, play->state.frames * 0,
                                             play->state.frames * 1, 32, 32, 0, 1, 0, 1));
    COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[0]);
    CLOSE_DISPS(play->state.gfxCtx);
    int32_t segs[] = { 0x08 };
    OOT_RestoreForeignSegs(play, segs, 1);
}

inline void OOT_DrawForeignBlueFire(PlayState* play, const ComboForeignDrawInfo* info) {
    GraphicsContext* gfxCtx = play->state.gfxCtx;
    OPEN_DISPS(gfxCtx);
    Gfx_SetupDL_25Opa(gfxCtx);
    OOT_FOREIGN_PIN_OPA();
    COMBO_FOREIGN_MTX(POLY_OPA_DISP++);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    Gfx_SetupDL_25Xlu(gfxCtx);
    OOT_FOREIGN_PIN_XLU();
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(gfxCtx, G_TX_RENDERTILE, 0, 0, 16, 32, 1, play->state.frames * 1,
                                             -(play->state.frames * 8), 16, 32, 0, 0, 1, -8));
    Matrix_Push();
    Matrix_Translate(-8.0f, -2.0f, 0.0f, MTXMODE_APPLY);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[1]);
    Matrix_Pop();
    CLOSE_DISPS(gfxCtx);
    int32_t segs[] = { 0x08 };
    OOT_RestoreForeignSegs(play, segs, 1);
}

// Potions: seg8 OPA scroll, OPA dl[1,0,2,3] + XLU dl[4,5] (GetItem_DrawPotion).
inline void OOT_DrawForeignPotion(PlayState* play, const ComboForeignDrawInfo* info) {
    s32 f = (s32)play->state.frames; // soh's frames is unsigned; MM's func negates it as signed
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    OOT_FOREIGN_PIN_OPA();
    gSPSegment(POLY_OPA_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(play->state.gfxCtx, G_TX_RENDERTILE, -f, f, 32, 32, 1, -f, f, 32, 32, -1,
                                             1, -1, 1));
    COMBO_FOREIGN_MTX(POLY_OPA_DISP++);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[1]);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[2]);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[3]);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    OOT_FOREIGN_PIN_XLU();
    COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[4]);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[5]);
    CLOSE_DISPS(play->state.gfxCtx);
    int32_t segs[] = { 0x08 };
    OOT_RestoreForeignSegs(play, segs, 1);
}

// Poe / Big Poe bottle: OPA dl0; XLU dl1; seg8 scroll; billboard dl3,dl2 (GetItem_DrawPoes).
inline void OOT_DrawForeignPoes(PlayState* play, const ComboForeignDrawInfo* info) {
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    OOT_FOREIGN_PIN_OPA();
    COMBO_FOREIGN_MTX(POLY_OPA_DISP++);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    OOT_FOREIGN_PIN_XLU();
    COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[1]);
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(play->state.gfxCtx, G_TX_RENDERTILE, play->state.frames * 0,
                                             play->state.frames * 0, 16, 32, 1, play->state.frames,
                                             -(int32_t)(play->state.frames * 6), 16, 32, 0, 0, 1, -6));
    Matrix_Push();
    Matrix_ReplaceRotation(&play->billboardMtxF);
    COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[3]);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[2]);
    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
    int32_t segs[] = { 0x08 };
    OOT_RestoreForeignSegs(play, segs, 1);
}

// Bottled fairy: OPA dl0; XLU dl1; seg8 scroll; billboard dl2 (GetItem_DrawFairyBottle).
inline void OOT_DrawForeignFairyBottle(PlayState* play, const ComboForeignDrawInfo* info) {
    const ComboFairyBottleMotion motion = ComboFairyBottle_SampleForShell(play->gameplayFrames, info->dls[0]);
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    OOT_FOREIGN_PIN_OPA();
    COMBO_FOREIGN_MTX(POLY_OPA_DISP++);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    OOT_FOREIGN_PIN_XLU();
    COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
    if (strcmp(info->dls[0], info->dls[1]) != 0) {
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[1]);
    }
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(play->state.gfxCtx, G_TX_RENDERTILE, play->state.frames * 0,
                                             play->state.frames * 0, 32, 32, 1, play->state.frames,
                                             -(int32_t)(play->state.frames * 6), 32, 320, 0, 0, 1, -6));
    Matrix_Push();
    if (ComboFairyBottle_IsBlueFireShell(info->dls[0])) {
        Matrix_Translate(-8.0f, -2.0f, 0.0f, MTXMODE_APPLY);
    }
    Matrix_Translate(motion.x, motion.y, motion.z, MTXMODE_APPLY);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    Matrix_Scale(motion.scaleX, motion.scaleY, motion.scaleZ, MTXMODE_APPLY);
    if (!ComboFairyBottle_DrawVfx(play, info->dls[0])) {
        COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[2]);
    }
    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
    int32_t segs[] = { 0x08 };
    OOT_RestoreForeignSegs(play, segs, 1);
    if (ComboFairyBottle_IsBundledShell(info->dls[0]))
        ComboBottleShimmer_DrawMotes(play, CW_SHIMMER_FAIRY);
}

// MM's native bottle keeps its contents-placement Mtx and animated materials.
// Slot 3 is a resource path, never a display list. Load it from the asset owner,
// then leave that scope before submitting commands or using the host fallback.
// False means the complete native fallback already rendered its own shimmer.
inline bool OOT_DrawForeignFairyContainer(PlayState* play, const ComboForeignDrawInfo* info) {
    MtxF contentsMtx;
    bool hasMatrix = false;
    if (info->count >= 4 && info->dls[3] != nullptr && strncmp(info->dls[3], "__OTR__@mm:", 11) == 0) {
        if (auto owner = Ship::CrossRMRegistry::Get("mm")) {
            Ship::ResourceManagerScope ownerScope(owner);
            if (auto matrix = owner->LoadResource(info->dls[3] + 11)) {
                if (matrix->GetRawPointer() != nullptr) {
                    Matrix_MtxToMtxF((Mtx*)matrix->GetRawPointer(), &contentsMtx);
                    hasMatrix = true;
                }
            }
        }
    }
    int32_t matSegs[kMaxMatEntries] = {};
    int32_t matSegCount = 0;
    if (!hasMatrix || !ComboForeignTexAnim_Run(play, "mm", info->matAnimPath, true, matSegs, &matSegCount)) {
        GetItem_Draw(play, GID_FAIRY);
        return false;
    }

    const ComboFairyBottleMotion motion = ComboFairyBottle_SampleForShell(play->gameplayFrames, info->dls[0]);
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    OOT_FOREIGN_PIN_OPA();
    COMBO_FOREIGN_MTX(POLY_OPA_DISP++);
    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[0]);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    OOT_FOREIGN_PIN_XLU();
    COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
    if (strcmp(info->dls[0], info->dls[1]) != 0) {
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[1]);
    }
    Matrix_Push();
    Matrix_Translate(motion.x, motion.y, motion.z, MTXMODE_APPLY);
    if (ComboFairyBottle_IsBlueFireShell(info->dls[0])) {
        Matrix_Translate(-8.0f, -2.0f, 0.0f, MTXMODE_APPLY);
    } else if (!ComboFairyBottle_IsBundledShell(info->dls[0])) {
        // The native billboard matrix sizes a sprite, not an actor skeleton.
        // Keep its anchor for the VFX; apply its full scale only to fallback contents.
        Matrix_Translate(contentsMtx.xw, contentsMtx.yw, contentsMtx.zw, MTXMODE_APPLY);
    }
    Matrix_ReplaceRotation(&play->billboardMtxF);
    Matrix_Scale(motion.scaleX, motion.scaleY, motion.scaleZ, MTXMODE_APPLY);
    if (!ComboFairyBottle_DrawVfx(play, info->dls[0])) {
        Matrix_Pop();
        Matrix_Push();
        Matrix_Translate(motion.x, motion.y, motion.z, MTXMODE_APPLY);
        if (ComboFairyBottle_IsBlueFireShell(info->dls[0])) {
            Matrix_Translate(-8.0f, -2.0f, 0.0f, MTXMODE_APPLY);
        } else {
            Matrix_Mult(&contentsMtx, MTXMODE_APPLY);
        }
        Matrix_ReplaceRotation(&play->billboardMtxF);
        Matrix_Scale(motion.scaleX, motion.scaleY, motion.scaleZ, MTXMODE_APPLY);
        COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[2]);
    }
    Matrix_Pop();
    CLOSE_DISPS(play->state.gfxCtx);
    ComboForeignTexAnim_Restore(play, matSegs, matSegCount, true);
    if (ComboFairyBottle_IsBundledShell(info->dls[0]))
        ComboBottleShimmer_DrawMotes(play, CW_SHIMMER_FAIRY);
    return true;
}

// MM enemy soul: the billboarded soul flame in the soul's own color (DrawFuncs.cpp DrawEnLight).
// The enemy skeleton the real func draws around it isn't expressible cross-game, so this is the
// flame alone; the 0.1 scale is MM's 0.01 model scale x its flameSize 10.
inline void OOT_DrawForeignSoulFlame(PlayState* play, const ComboForeignDrawInfo* info) {
    int32_t t = (int32_t)play->state.frames;
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    OOT_FOREIGN_PIN_XLU();
    Matrix_ReplaceRotation(&play->billboardMtxF);
    gSPSegment(POLY_XLU_DISP++, 0x08,
               (uintptr_t)Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, 0, 0x10, 0x20, 1, (t * 2) & 0x3F,
                                             (t * -6) & 0x7F, 0x10, 0x20, 0, 0, 2, -6));
    gDPSetPrimColor(POLY_XLU_DISP++, 0xC0, 0xC0, info->primColorXlu[0], info->primColorXlu[1], info->primColorXlu[2],
                    0);
    gDPSetEnvColor(POLY_XLU_DISP++, info->primColorXlu[0], info->primColorXlu[1], info->primColorXlu[2], 0);
    Matrix_Scale(0.1f, 0.1f, 0.1f, MTXMODE_APPLY);
    COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[0]);
    CLOSE_DISPS(play->state.gfxCtx);
    int32_t segs[] = { 0x08 };
    OOT_RestoreForeignSegs(play, segs, 1);
}

// Same independent weapon flame as MM's DrawOotSlateRuneFlame. Its resource
// belongs to OoT, and its scroll/billboard use the active host frame and pose.
inline void OOT_DrawForeignWeaponFlame(PlayState* play, const uint8_t color[4]) {
    if (!color[3])
        return;
    const char* path = "__OTR__objects/object_gi_fire/gGiBlueFireFlameDL";
    if (!ResourceMgr_FileExists(path) && !(ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(path)))
        return;
    Matrix_Push();
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Xlu(play->state.gfxCtx);
    OOT_FOREIGN_PIN_XLU();
    gSPComboRMPush(POLY_XLU_DISP++, "oot");
    gSPSegment(POLY_XLU_DISP++, 8,
               (uintptr_t)Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, 0, 0, 16, 32, 1, play->state.frames,
                                             -(play->state.frames * 8), 16, 32, 0, 0, 1, -8));
    Matrix_Translate(0.f, -70.f, 0.f, MTXMODE_APPLY);
    Matrix_Scale(5.f, 5.f, 5.f, MTXMODE_APPLY);
    Matrix_ReplaceRotation(&play->billboardMtxF);
    COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
    gDPSetGrayscaleColor(POLY_XLU_DISP++, color[0], color[1], color[2], 255);
    gSPGrayscale(POLY_XLU_DISP++, true);
    gDma1p(POLY_XLU_DISP++, G_DL_OTR_FILEPATH, path, 0, G_DL_PUSH);
    gSPGrayscale(POLY_XLU_DISP++, false);
    Matrix_Pop();
    OOT_FOREIGN_PIN_XLU();
    COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
    gSPComboRMPop(POLY_XLU_DISP++);
    CLOSE_DISPS(play->state.gfxCtx);
    const int32_t segs[] = { 8 };
    OOT_RestoreForeignSegs(play, segs, 1);
}

// Concrete imported Master/True Master legacy drawer: native scroll, .05
// scale, 2.1-radian tilt and the True tier's separate gold blade/blue flame.
inline void OOT_DrawForeignMasterSword(PlayState* play, const ComboForeignDrawInfo* info, bool drawFlame = true) {
    if (drawFlame)
        OOT_DrawForeignWeaponFlame(play, info->primColorXlu);
    Matrix_Push();
    Matrix_Scale(.05f, .05f, .05f, MTXMODE_APPLY);
    Matrix_RotateZ(2.1f, MTXMODE_APPLY);
    OPEN_DISPS(play->state.gfxCtx);
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    OOT_FOREIGN_PIN_OPA();
    gSPGrayscale(POLY_OPA_DISP++, false);
    gSPComboRMPush(POLY_OPA_DISP++, "oot");
    gSPSegment(POLY_OPA_DISP++, 8,
               (uintptr_t)Gfx_TwoTexScrollEx(play->state.gfxCtx, 0, play->state.frames, 0, 32, 32, 1, 0, 0, 32, 32, 1,
                                             0, 0, 0));
    COMBO_FOREIGN_MTX(POLY_OPA_DISP++);
    if (info->primColorOpa[3]) {
        gDPSetGrayscaleColor(POLY_OPA_DISP++, info->primColorOpa[0], info->primColorOpa[1], info->primColorOpa[2],
                             info->primColorOpa[3]);
        gSPGrayscale(POLY_OPA_DISP++, true);
    }
    gDma1p(POLY_OPA_DISP++, G_DL_OTR_FILEPATH, info->dls[0], 0, G_DL_PUSH);
    gSPGrayscale(POLY_OPA_DISP++, false);
    Matrix_Pop();
    OOT_FOREIGN_PIN_OPA();
    COMBO_FOREIGN_MTX(POLY_OPA_DISP++);
    gSPComboRMPop(POLY_OPA_DISP++);
    CLOSE_DISPS(play->state.gfxCtx);
    const int32_t segs[] = { 8 };
    OOT_RestoreForeignSegs(play, segs, 1);
}

// Selected standalone/Din GI recipe. Match MM_DrawForeignCustomGi's signed
// spin, hand-local transforms, split passes and independent optional flame.
inline void OOT_DrawForeignCustomGi(PlayState* play, const ComboForeignDrawInfo* info, bool shop = false,
                                    bool drawFlame = true) {
    if (drawFlame)
        OOT_DrawForeignWeaponFlame(play, info->primColorXlu);
    Matrix_Push();
    const uint32_t bits = (uint32_t(play->gameplayFrames) * 2u) & 0xFFFFu;
    Matrix_RotateY((bits >= 0x8000u ? int32_t(bits) - 0x10000 : int32_t(bits)) * .01f, MTXMODE_APPLY);
    const bool sword = info->neiShimmer > 0 && info->neiShimmer <= static_cast<int32_t>(NeiGi::Kind::Last) + 1 &&
                       NeiGi::IsSword(static_cast<NeiGi::Kind>(info->neiShimmer - 1));
    int selectedOpaque = -1;
    bool noCull = false;
    for (int i = 0; i < info->opCount; ++i) {
        const auto& op = info->ops[i];
        switch (op.op) {
            case CW_OP_ROTATE_X:
                Matrix_RotateX(op.a * (3.14159265358979323846f / 32768.f), MTXMODE_APPLY);
                break;
            case CW_OP_ROTATE_Z:
                Matrix_RotateZ(sword ? ComboSwordGi_SelectedTilt(op.a * (3.14159265358979323846f / 32768.f), shop)
                                     : op.a * (3.14159265358979323846f / 32768.f),
                               MTXMODE_APPLY);
                break;
            case CW_OP_SCALE:
                Matrix_Scale(op.a, op.b, op.c, MTXMODE_APPLY);
                break;
            case CW_OP_TRANSLATE:
                Matrix_Translate(op.a, op.b, op.c, MTXMODE_APPLY);
                break;
            case CW_OP_FRAME_PAIR:
                if (op.a >= 0 && op.a <= 30)
                    selectedOpaque = (uint32_t(play->gameplayFrames) >> int(op.a)) & 1u;
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
    OPEN_DISPS(play->state.gfxCtx);
    gSPComboRMPush(POLY_OPA_DISP++, "oot");
    gSPComboRMPush(POLY_XLU_DISP++, "oot");
    const int split = info->xluStart < 0 || info->xluStart > info->count ? info->count : info->xluStart;
    for (int stream = 0; stream < 2; ++stream) {
        const int begin = stream ? split : 0, end = stream ? info->count : split;
        if (begin >= end)
            continue;
        if (stream) {
            Gfx_SetupDL_25Xlu(play->state.gfxCtx);
            OOT_FOREIGN_PIN_XLU();
            gSPGrayscale(POLY_XLU_DISP++, false);
            COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
            for (int i = begin; i < end; ++i)
                gDma1p(POLY_XLU_DISP++, G_DL_OTR_FILEPATH, info->dls[i], 0, G_DL_PUSH);
        } else {
            Gfx_SetupDL_25Opa(play->state.gfxCtx);
            OOT_FOREIGN_PIN_OPA();
            gSPGrayscale(POLY_OPA_DISP++, false);
            COMBO_FOREIGN_MTX(POLY_OPA_DISP++);
            if (noCull)
                gSPClearGeometryMode(POLY_OPA_DISP++, G_CULL_BOTH);
            if (info->primColorOpa[3]) {
                gDPSetGrayscaleColor(POLY_OPA_DISP++, info->primColorOpa[0], info->primColorOpa[1],
                                     info->primColorOpa[2], info->primColorOpa[3]);
                gSPGrayscale(POLY_OPA_DISP++, true);
            }
            for (int i = begin; i < end; ++i)
                if (selectedOpaque < 0 || i == selectedOpaque)
                    gDma1p(POLY_OPA_DISP++, G_DL_OTR_FILEPATH, info->dls[i], 0, G_DL_PUSH);
            gSPGrayscale(POLY_OPA_DISP++, false);
            if (noCull)
                gSPSetGeometryMode(POLY_OPA_DISP++, G_CULL_BACK);
        }
    }
    CLOSE_DISPS(play->state.gfxCtx);
    if (info->opCount == 1 && info->ops[0].op == CW_OP_ROTATE_Z)
        ComboDinSwordGi_DrawLayers(play, "oot", info->dls[0]);
    Matrix_Pop();
    OPEN_DISPS(play->state.gfxCtx);
    gSPGrayscale(POLY_OPA_DISP++, false);
    gSPGrayscale(POLY_XLU_DISP++, false);
    OOT_FOREIGN_PIN_OPA();
    OOT_FOREIGN_PIN_XLU();
    COMBO_FOREIGN_MTX(POLY_OPA_DISP++);
    COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
    gSPComboRMPop(POLY_OPA_DISP++);
    gSPComboRMPop(POLY_XLU_DISP++);
    CLOSE_DISPS(play->state.gfxCtx);
}

// CW_DRAW_KIND_OPS: replay the producer's transform/color/DL bytecode (MM's DrawClock,
// DrawOwlStatue, DrawSkeletonKey, DrawDoubleDefense, DrawTycoonWallet). Matrix pushes are balanced
// by the producer; a stray POP without a PUSH is guarded here so we never underflow OOT's stack.
inline void OOT_DrawForeignOps(PlayState* play, const ComboForeignDrawInfo* info) {
    int32_t depth = 0;
    bool xlu = false;
    OPEN_DISPS(play->state.gfxCtx);
    // Default stream + colour pin; every SETUP op below re-pins the stream it switches to, so a
    // recipe that opens with CW_OP_SETUP_XLU (MM's DrawDoubleDefense) is covered too.
    Gfx_SetupDL_25Opa(play->state.gfxCtx);
    OOT_FOREIGN_PIN_OPA();
    for (int32_t i = 0; i < info->opCount; i++) {
        const CwDrawOp* o = &info->ops[i];
        switch (o->op) {
            case CW_OP_SETUP_OPA:
                Gfx_SetupDL_25Opa(play->state.gfxCtx);
                if (info->setupDlOpa != nullptr) {
                    gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->setupDlOpa);
                }
                OOT_FOREIGN_PIN_OPA();
                xlu = false;
                break;
            case CW_OP_SETUP_XLU:
                Gfx_SetupDL_25Xlu(play->state.gfxCtx);
                if (info->setupDlXlu != nullptr) {
                    gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->setupDlXlu);
                }
                OOT_FOREIGN_PIN_XLU();
                xlu = true;
                break;
            case CW_OP_LOAD_MATRIX:
                if (xlu) {
                    COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
                } else {
                    COMBO_FOREIGN_MTX(POLY_OPA_DISP++);
                }
                break;
            case CW_OP_PUSH:
                Matrix_Push();
                depth++;
                break;
            case CW_OP_POP:
                if (depth > 0) {
                    Matrix_Pop();
                    depth--;
                }
                break;
            case CW_OP_TRANSLATE:
                Matrix_Translate(o->a, o->b, o->c, MTXMODE_APPLY);
                break;
            case CW_OP_SCALE:
                Matrix_Scale(o->a, o->b, o->c, MTXMODE_APPLY);
                break;
            case CW_OP_ROTATE_X:
                Matrix_RotateZYX((s16)o->a, 0, 0, MTXMODE_APPLY);
                break;
            case CW_OP_ROTATE_Y:
                Matrix_RotateZYX(0, (s16)o->a, 0, MTXMODE_APPLY);
                break;
            case CW_OP_ROTATE_Z:
                Matrix_RotateZYX(0, 0, (s16)o->a, MTXMODE_APPLY);
                break;
            case CW_OP_BILLBOARD:
                Matrix_ReplaceRotation(&play->billboardMtxF);
                break;
            case CW_OP_PRIM_COLOR:
                if (xlu) {
                    gDPSetPrimColor(POLY_XLU_DISP++, 0, (u8)o->a, o->rgba[0], o->rgba[1], o->rgba[2], o->rgba[3]);
                } else {
                    gDPSetPrimColor(POLY_OPA_DISP++, 0, (u8)o->a, o->rgba[0], o->rgba[1], o->rgba[2], o->rgba[3]);
                }
                break;
            case CW_OP_ENV_COLOR:
                if (xlu) {
                    gDPSetEnvColor(POLY_XLU_DISP++, o->rgba[0], o->rgba[1], o->rgba[2], o->rgba[3]);
                } else {
                    gDPSetEnvColor(POLY_OPA_DISP++, o->rgba[0], o->rgba[1], o->rgba[2], o->rgba[3]);
                }
                break;
            case CW_OP_GRAYSCALE_COLOR:
                if (xlu) {
                    gDPSetGrayscaleColor(POLY_XLU_DISP++, o->rgba[0], o->rgba[1], o->rgba[2], o->rgba[3]);
                } else {
                    gDPSetGrayscaleColor(POLY_OPA_DISP++, o->rgba[0], o->rgba[1], o->rgba[2], o->rgba[3]);
                }
                break;
            case CW_OP_GRAYSCALE_ON:
            case CW_OP_GRAYSCALE_OFF: {
                bool on = (o->op == CW_OP_GRAYSCALE_ON);
                if (xlu) {
                    gSPGrayscale(POLY_XLU_DISP++, on);
                } else {
                    gSPGrayscale(POLY_OPA_DISP++, on);
                }
                break;
            }
            case CW_OP_DLIST: {
                int32_t idx = (int32_t)o->a;
                if (idx >= 0 && idx < info->count && info->dls[idx] != nullptr) {
                    if (xlu) {
                        gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[idx]);
                    } else {
                        gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[idx]);
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    while (depth-- > 0) { // never leave the shared matrix stack unbalanced
        Matrix_Pop();
    }
    CLOSE_DISPS(play->state.gfxCtx);
}

// The original flat path: OPA layers then XLU layers, plus the animated-material / skull-token
// segment replication. Used for every SIMPLE recipe (the bulk of MM's table).
inline void OOT_DrawForeignSimple(PlayState* play, const ComboForeignDrawInfo* info) {
    int32_t n = info->count;
    int32_t xs = (info->xluStart < 0 || info->xluStart > n) ? n : info->xluStart;

    OPEN_DISPS(play->state.gfxCtx);
    if (info->drawKind == CW_DRAW_KIND_SONG_GI)
        gSPGrayscale(POLY_XLU_DISP++, false);

    // ComboShip: replicate MM's AnimatedMat_Draw segment bind before the DLs (Moon's Tear).
    int32_t matSegs[kMaxMatEntries];
    int32_t matSegCount = 0;
    if (info->matAnimPath != nullptr) {
        ComboForeignTexAnim_Run(play, "mm", info->matAnimPath, info->matAnimBindOpa, matSegs, &matSegCount);
    }

    // ComboShip: extra model scale carried from MM's draw func (e.g. boss remains: 0.02).
    if (info->scale > 0.0f) {
        Matrix_Scale(info->scale, info->scale, info->scale, MTXMODE_APPLY);
    }

    // Mirror MM's GetItem_DrawOpa*/Xlu* structure: one 25Opa setup + matrix for the OPA layers,
    // then one 25Xlu setup + matrix for the XLU layers.
    if (xs > 0) {
        // MM's own setup when the row uses one other than 25 (bombchu = 23, which is 1-CYCLE without
        // fog). Under OOT's 2-cycle 25 the list's duplicated second cycle wins and samples TEXEL1 —
        // whatever tile OOT last bound — instead of the item's own texture.
        if (info->setupDlOpa != nullptr) {
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->setupDlOpa);
        } else {
            Gfx_SetupDL_25Opa(play->state.gfxCtx);
        }
        OOT_FOREIGN_PIN_OPA();
        COMBO_FOREIGN_MTX(POLY_OPA_DISP++);
        if (info->hasEnvColor) {
            gDPSetEnvColor(POLY_OPA_DISP++, info->envColor[0], info->envColor[1], info->envColor[2], info->envColor[3]);
        }
        for (int32_t i = 0; i < xs; i++) {
            gSPDisplayList(POLY_OPA_DISP++, (Gfx*)info->dls[i]);
        }
    }
    if (xs < n) {
        if (info->setupDlXlu != nullptr) { // compass glass: setup 5, not 25
            gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->setupDlXlu);
        } else {
            Gfx_SetupDL_25Xlu(play->state.gfxCtx);
        }
        OOT_FOREIGN_PIN_XLU();
        // ComboShip: MM's GetItem_DrawSkullToken inlines this seg-8 flame scroll (no texanim
        // resource to carry), so it is hardcoded here. See docs/deviations/rando.md.
        if (info->xluSeg8TexScroll) {
            gSPSegment(POLY_XLU_DISP++, 0x08,
                       (uintptr_t)Gfx_TwoTexScrollEx(play->state.gfxCtx, G_TX_RENDERTILE, 0, play->state.frames * -5,
                                                     32, 32, 1, 0, 0, 32, 64, 0, -5, 0, 0));
        }
        // ComboShip: billboard the XLU layer toward the camera (Moon's Tear glow), mirroring MM's
        // Matrix_ReplaceRotation before the glow DL. Must precede the matrix capture below.
        if (info->matAnimBillboard) {
            Matrix_ReplaceRotation(&play->billboardMtxF);
        }
        COMBO_FOREIGN_MTX(POLY_XLU_DISP++);
        if (info->hasEnvColor) {
            gDPSetEnvColor(POLY_XLU_DISP++, info->envColor[0], info->envColor[1], info->envColor[2], info->envColor[3]);
        }
        for (int32_t i = xs; i < n; i++) {
            gSPDisplayList(POLY_XLU_DISP++, (Gfx*)info->dls[i]);
        }
    }

    // ComboShip: segment hygiene — later same-frame draws must not sample this item's scroll DL.
    if (matSegCount > 0) {
        ComboForeignTexAnim_Restore(play, matSegs, matSegCount, info->matAnimBindOpa);
    }

    CLOSE_DISPS(play->state.gfxCtx);
}

// Draw a foreign (MM-bound) item's real MM model at the current model matrix. Any resolution
// failure falls back to the sentinel (the RG_COMBO_FOREIGN entry's blue rupee), so we never draw
// blank. Mirror of MM_DrawComboForeign (combo/menu/ComboForeignDrawMM.h).
inline void OOT_DrawComboForeign(PlayState* play, GetItemEntry* getItemEntry, bool shop = false) {
    RandomizerCheck rc = (RandomizerCheck)getItemEntry->comboForeignCheck;
    if (rc == RC_UNKNOWN_CHECK) {
        // Defensive: entries not built via GetFinalGIEntry carry no check; the queued get-item
        // check is the only other identity source.
        rc = OOT_GetQueuedDrawCheck();
    }

    const ComboForeignDrawInfo* info = (rc != RC_UNKNOWN_CHECK) ? ComboResolveForeignDrawInfo(rc) : nullptr;
    if (info == nullptr) {
        GetItem_Draw(play, getItemEntry->gid);
        return;
    }

    if (ComboDrawMagicMirrorForeign(play, info->drawKind))
        return;

    // ComboShip: animated class — combo-owned skeletal draw (any failure -> sentinel, never blank).
    if (info->animOk) {
        if (!ComboForeignAnim_Draw(&info->anim, "mm", play)) {
            GetItem_Draw(play, getItemEntry->gid);
        }
        return;
    }

    const bool swordIdentity = info->neiShimmer > 0 &&
                               info->neiShimmer <= static_cast<int32_t>(NeiGi::Kind::Last) + 1 &&
                               NeiGi::IsSword(static_cast<NeiGi::Kind>(info->neiShimmer - 1));
    const bool fitSword = info->count > 0 && swordIdentity &&
                          (info->drawKind == CW_DRAW_KIND_CUSTOM_GI || info->drawKind == CW_DRAW_KIND_SIMPLE ||
                           info->drawKind == CW_DRAW_KIND_MASTER_SWORD || info->drawKind == CW_DRAW_KIND_GORON_SWORD);
    const bool swordFlame = swordIdentity && info->primColorXlu[3] &&
                            (info->drawKind == CW_DRAW_KIND_CUSTOM_GI || info->drawKind == CW_DRAW_KIND_MASTER_SWORD);
    if (fitSword) {
        Matrix_Push();
        ComboSwordGi_ApplyPresentationSize(shop);
    }
    if (swordFlame) {
        Matrix_Push();
        ComboSwordGi_ApplyEffectFit(static_cast<NeiGi::Kind>(info->neiShimmer - 1), shop);
        OOT_DrawForeignWeaponFlame(play, info->primColorXlu);
        Matrix_Pop();
    }
    if (fitSword) {
        Matrix_Push();
        const float scale = info->drawKind == CW_DRAW_KIND_MASTER_SWORD  ? .05f
                            : info->drawKind == CW_DRAW_KIND_GORON_SWORD ? 1.f
                            : info->scale > 0                            ? info->scale
                                                                         : 1.f;
        const float tilt =
            info->drawKind == CW_DRAW_KIND_MASTER_SWORD ? 2.1f
            : info->drawKind == CW_DRAW_KIND_CUSTOM_GI && info->opCount == 1 && info->ops[0].op == CW_OP_ROTATE_Z
                ? ComboSwordGi_SelectedTilt(info->ops[0].a * (3.14159265358979323846f / 32768.f), shop && swordIdentity)
                : 0.f;
        ComboSwordGi_ApplyModelsFit("oot", info->dls, info->count, scale, tilt, shop);
    }
    // Authored NEI recipes carry their palette in neiEffect and render their
    // shimmer inside the shared presentation. Other recipes use the overlay.
    const bool overlayShimmer = info->itemShimmer && info->drawKind != CW_DRAW_KIND_NEI_GI &&
                                info->drawKind != CW_DRAW_KIND_ELEMENTAL_ARROW &&
                                info->drawKind != CW_DRAW_KIND_MAGIC_SPELL;
    if (overlayShimmer) {
        Matrix_Push();
    }
    switch (info->drawKind) {
        case CW_DRAW_KIND_ELEMENTAL_ARROW: {
            auto core = *info;
            core.count = 1;
            core.xluStart = -1;
            OOT_DrawForeignSimple(play, &core);
            NeiGi_DrawElementalArrowForOwner(play, info->neiEffect, 1, shop);
            break;
        }
        case CW_DRAW_KIND_NEI_GI:
            NeiGi_DrawPresentation(play, info->dls[0], info->xluStart == 1 && info->count > 1 ? info->dls[1] : nullptr,
                                   info->scale, info->neiEffect, info->neiEffectCenter,
                                   info->itemShimmer || CVarGetInteger("gEnhancements.SkijerNEI.ItemEffects", 0), "oot",
                                   shop, 0, info->neiShimmer - 1);
            break;
        case CW_DRAW_KIND_MAGIC_SPELL:
            if (!NeiGi_DrawElementalSpell(play, info->neiEffect, 1, shop))
                OOT_DrawForeignMagicSpell(play, info);
            break;
        case CW_DRAW_KIND_MM_SPIN_ATTACK:
            ComboDrawSpinAttackGi(play, info->dls[0], info->dls[1], info->scale, info->primColorXlu, "mm");
            break;
        case CW_DRAW_KIND_GORON_SWORD:
            OOT_DrawForeignGoronSword(play, info);
            break;
        case CW_DRAW_KIND_MASTER_SWORD:
            OOT_DrawForeignMasterSword(play, info, !swordFlame);
            break;
        case CW_DRAW_KIND_CUSTOM_GI:
            OOT_DrawForeignCustomGi(play, info, shop, !swordFlame);
            break;
        case CW_DRAW_KIND_DEKU_NUTS:
            OOT_DrawForeignDekuNuts(play, info);
            break;
        case CW_DRAW_KIND_RECOVERY_HEART:
            OOT_DrawForeignRecoveryHeart(play, info);
            break;
        case CW_DRAW_KIND_FISH:
            OOT_DrawForeignFish(play, info);
            break;
        case CW_DRAW_KIND_POTION:
            OOT_DrawForeignPotion(play, info);
            break;
        case CW_DRAW_KIND_BLUE_FIRE:
            OOT_DrawForeignBlueFire(play, info);
            break;
        case CW_DRAW_KIND_POES:
            OOT_DrawForeignPoes(play, info);
            break;
        case CW_DRAW_KIND_MM_FAIRY_BOTTLE:
            OOT_DrawForeignFairyBottle(play, info);
            break;
        case CW_DRAW_KIND_MM_FAIRY_CONTAINER:
            if (!OOT_DrawForeignFairyContainer(play, info)) {
                if (overlayShimmer) {
                    Matrix_Pop();
                }
                return; // the native fallback includes its pink hex overlay
            }
            break;
        case CW_DRAW_KIND_MM_SOUL_FLAME:
            OOT_DrawForeignSoulFlame(play, info);
            break;
        case CW_DRAW_KIND_OPS:
            OOT_DrawForeignOps(play, info);
            break;
        case CW_DRAW_KIND_BOTTLE_CONTENTS:
            if (!ComboBottleContents_Draw(play, ComboBottleContents_Profile(info->dls[0]))) {
                OOT_DrawForeignSimple(play, info);
            }
            break;
        case CW_DRAW_KIND_SEASON_GI:
            if (info->neiEffect == 5) {
                Matrix_Push();
                OOT_DrawForeignSimple(play, info);
                Matrix_Pop();
            }
            NeiGi_DrawSeasonOverlay(play, info->neiEffect, "oot");
            break;
        case CW_DRAW_KIND_SONG_GI:
        case CW_DRAW_KIND_SIMPLE:
        default:
            OOT_DrawForeignSimple(play, info);
            break;
    }
    if (overlayShimmer)
        Matrix_Pop();
    if (fitSword)
        Matrix_Pop();
    if (overlayShimmer) {
        if (swordIdentity) {
            Matrix_Push();
            ComboSwordGi_ApplyEffectFit(static_cast<NeiGi::Kind>(info->neiShimmer - 1), shop);
        }
        if (info->neiShimmer > 0 && info->neiShimmer <= static_cast<int32_t>(NeiGi::Kind::Last) + 1 &&
            NeiGi::IsSword(static_cast<NeiGi::Kind>(info->neiShimmer - 1)))
            NeiGi_DrawMesh(
                play,
                NeiGi::SampleSpecial(
                    static_cast<NeiGi::Kind>(info->neiShimmer - 1), play->gameplayFrames, NeiGi_CameraBasis(play),
                    ComboSwordGi_ParticleScale(static_cast<NeiGi::Kind>(info->neiShimmer - 1), shop, 0, fitSword)));
        if (info->drawKind == CW_DRAW_KIND_SONG_GI)
            NeiGi_DrawSongOverlay(play, info->neiEffect, "mm");
        else if (info->neiShimmer > 0 && info->neiShimmer <= static_cast<int32_t>(NeiGi::Kind::Last) + 1)
            NeiGi_DrawMesh(play, NeiGi::SampleShimmer(play->gameplayFrames, true, NeiGi_CameraBasis(play),
                                                      static_cast<NeiGi::Kind>(info->neiShimmer - 1)));
        else if (info->neiEffect == static_cast<int32_t>(NeiGi::Kind::Pokeball))
            NeiGi_DrawMesh(
                play, NeiGi::SampleShimmer(play->gameplayFrames, true, NeiGi_CameraBasis(play), NeiGi::Kind::Pokeball));
        else if (info->itemShimmer == CW_ITEM_SHIMMER_TINTED)
            NeiGi_DrawTintedShimmerOverlay(play, info->itemShimmerColor, "mm");
        else
            ComboDrawMaskShimmer(play, nullptr, info->itemShimmerColor, "mm");
    }
    if (overlayShimmer && swordIdentity)
        Matrix_Pop();
    if (fitSword)
        Matrix_Pop();
}

// EnGirlA's sentinel callback has no local Presentation binding. Fit only
// complete authored catalog recipes, then reuse the foreign renderer
// so the model and every effect inherit the same shelf pose.
extern "C" bool OOT_DrawComboForeignShop(PlayState* play, GetItemEntry* entry) {
    if (!play || !entry || entry->drawFunc != Randomizer_DrawComboForeign)
        return false;
    RandomizerCheck rc = static_cast<RandomizerCheck>(entry->comboForeignCheck);
    if (rc == RC_UNKNOWN_CHECK)
        rc = OOT_GetQueuedDrawCheck();
    const auto* info = rc != RC_UNKNOWN_CHECK ? ComboResolveForeignDrawInfo(rc) : nullptr;
    if (info && info->ok && !info->animOk && info->neiEffect >= 1 && info->neiEffect <= 3 &&
        ((info->drawKind == CW_DRAW_KIND_ELEMENTAL_ARROW && info->count >= 1 && info->dls[0]) ||
         (info->drawKind == CW_DRAW_KIND_MAGIC_SPELL && info->count >= 3 && info->dls[0] && info->dls[1] &&
          info->dls[2]))) {
        OOT_DrawComboForeign(play, entry, true);
        return true;
    }
    if (!info || !info->ok || info->animOk || info->drawKind != CW_DRAW_KIND_NEI_GI || info->count < 1 ||
        !info->dls[0] || !(info->scale > 0.f) || (info->xluStart == 1 && (info->count < 2 || !info->dls[1])))
        return false;
    const auto* bounds = NeiGi::FindFrameBounds(info->dls[0]);
    if (!bounds || static_cast<int>(bounds->effect) != info->neiEffect)
        return false;
    OOT_DrawComboForeign(play, entry, true);
    return true;
}

#undef COMBO_FOREIGN_MTX
#undef OOT_FOREIGN_PIN_OPA
#undef OOT_FOREIGN_PIN_XLU

#endif /* COMBO_FOREIGN_DRAW_OOT_H */
