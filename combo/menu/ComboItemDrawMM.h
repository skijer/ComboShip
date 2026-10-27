#include "ItemGrantAuditBridge.h"
/* combo/menu/ComboItemDrawMM.h — ComboShip: MM-side bodies of the cross-game item-draw exports
 * (combo/menu/ComboItemDrawABI.h). Combo-OWNED source compiled INTO 2ship.dll (the menu-extraction
 * pattern) so the vendored BenPort.cpp keeps only a single include — the data here mirrors MM's
 * Rando/DrawItem.cpp draw cases (song colors, stray-fairy parameters) and must track them.
 *
 * TU-GLUE HEADER: include ONCE from mm/2s2h/BenPort.cpp, inside its #ifdef COMBO_BUILD include
 * block, AFTER Rando StaticData / engine headers are in scope. Not standalone.
 */
#ifndef COMBO_ITEM_DRAW_MM_H
#define COMBO_ITEM_DRAW_MM_H

#include <cstring>
#include "ComboExport.h"
#include "ComboResolve.h"
#include "ComboItemDrawABI.h"
#define COMBO_COLLECTIBLE_SHIMMER_HOST_MM
#include "ComboCollectibleShimmerHost.h"
#undef COMBO_COLLECTIBLE_SHIMMER_HOST_MM

#include <ship/resource/CrossRMRegistry.h>
#include <ship/resource/ResourceManagerScope.h>
#include "ComboSongDrawMM.h"
#include "ComboBottleContents.h"
#include "ComboOotBottleShimmerMM.h"
#include "2s2h/Rando/NeiGiPresentation.h"
#include "2s2h/Rando/NeiResourceRouting.h"
#include "2s2h/Enhancements/ItemVisuals.h"
#include "2s2h/Rando/DungeonItemVisuals.h"
#include "2s2h/Rando/SpinAttackGi.h"
#include "2s2h_assets.h"                                     // custom rando models (triforce, ocarina buttons, ...)
#include "objects/gameplay_keep/gameplay_keep.h"             // stray-fairy skel/anim + soul flame DL
#include "objects/object_gi_melody/object_gi_melody.h"       // gGiSongNoteDL
#include "objects/object_sek/object_sek.h"                   // gOwlStatueOpenedDL
#include "objects/object_gi_reserve00/object_gi_reserve00.h" // Moon's Tear item DL + texanim path
#include "objects/object_gi_bottle_04/object_gi_bottle_04.h" // gGiFairyBottleTexAnim
#include "objects/object_gi_hearts/object_gi_hearts.h"       // Double Defense heart border/container
#include "objects/object_gi_purse/object_gi_purse.h"         // Tycoon Wallet layers
#include "objects/object_obj_tokeidai/object_obj_tokeidai.h" // clock tower DLs
// Enemy-soul + minifrog skeletons for the animated cross-game class (mirrors the include block in
// mm/2s2h/Rando/DrawFuncs.cpp; only the models we can express cross-game are pulled in).
#include "assets/objects/object_uch/object_uch.h"                 // Alien
#include "assets/objects/object_am/object_am.h"                   // Armos
#include "assets/objects/object_vm/object_vm.h"                   // Beamos
#include "assets/objects/object_bb/object_bb.h"                   // Bubble
#include "assets/objects/object_bsb/object_bsb.h"                 // Captain Keeta
#include "assets/objects/object_famos/object_famos.h"             // Death Armos
#include "assets/objects/object_utubo/object_utubo.h"             // Deep Python
#include "assets/objects/object_dekubaba/object_dekubaba.h"       // Deku Baba
#include "assets/objects/object_dinofos/object_dinofos.h"         // Dinolfos
#include "assets/objects/object_dodongo/object_dodongo.h"         // Dodongo
#include "assets/objects/object_grasshopper/object_grasshopper.h" // Dragonfly
#include "assets/objects/object_snowman/object_snowman.h"         // Eeno
#include "assets/objects/object_eg/object_eg.h"                   // Eyegore
#include "assets/objects/object_jso/object_jso.h"                 // Garo
#include "overlays/actors/ovl_En_Pametfrog/z_en_pametfrog.h"      // Gekko
#include "assets/objects/object_bee/object_bee.h"                 // Giant Bee
#include "assets/objects/object_crow/object_crow.h"               // Guay
#include "assets/objects/object_pp/object_pp.h"                   // Hiploop
#include "assets/objects/object_knight/object_knight.h"           // Igos du Ikana
#include "assets/objects/object_firefly/object_firefly.h"         // Keese
#include "assets/objects/object_rb/object_rb.h"                   // Leever
#include "assets/objects/object_dekunuts/object_dekunuts.h"       // Mad Scrub
#include "assets/objects/object_gmo/object_gmo.h"                 // Nejiron
#include "assets/objects/object_okuta/object_okuta.h"             // Octorok
#include "assets/objects/object_ph/object_ph.h"                   // Peahat
#include "assets/objects/object_kz/object_kz.h"                   // Pirate
#include "assets/objects/object_po/object_po.h"                   // Poe
#include "assets/objects/object_rd/object_rd.h"                   // Redead
#include "assets/objects/object_sb/object_sb.h"                   // Shellblade
#include "assets/objects/object_pr/object_pr.h"                   // Skullfish
#include "assets/objects/object_st/object_st.h"                   // Skulltula
#include "assets/objects/object_tl/object_tl.h"                   // Snapper
#include "assets/objects/object_skb/object_skb.h"                 // Stalchild
#include "assets/objects/object_thiefbird/object_thiefbird.h"     // Takkuri
#include "assets/objects/object_tite/object_tite.h"               // Tektite
#include "assets/objects/object_wallmaster/object_wallmaster.h"   // Wallmaster
#include "assets/objects/object_boss04/object_boss04.h"           // Wart
#include "assets/objects/object_wiz/object_wiz.h"                 // Wizrobe
#include "assets/objects/object_wf/object_wf.h"                   // Wolfos
#include "objects/object_fr/object_fr.h"                          // Minifrog
#include "objects/object_boss_hakugin/object_boss_hakugin.h"      // Goht soul
#include "objects/object_boss03/object_boss03.h"                  // Gyorg soul
#include "objects/object_boss02/object_boss02.h"                  // Twinmold soul
#include "objects/object_boss01/object_boss01.h"                  // Odolwa soul

// Portable slice of one sDrawItemTable row (defined in mm/src/code/z_draw.c). outDrawKind is a
// CwDrawKind: 0 = plain OPA/XLU submission, else a non-portable func the consumer replicates.
extern "C" s32 GetItem_GetDrawTableEntry(s32 drawId, void** outDlists, s32 maxDlists, s32* outXluStart, f32* outScale,
                                         s32* outXluSeg8TexScroll, s32* outDrawKind);

// Which setup DL the row's func emits (NULL = plain 25). The consumer must submit the same one.
extern "C" void GetItem_GetDrawSetupDLs(s32 drawId, void** outOpa, void** outXlu);
extern "C" int ResourceMgr_GetIkanaShieldGiTiltXForGame(const char* game, const char* path, float* tilt);
extern "C" int ResourceMgr_GetIkanaShieldGiPoseForGame(const char* game, const char* path, float* tilt, float* roll);

// --- CW_DRAW_KIND_OPS emitters. Bounds-checked; an overflowing recipe is truncated, never written
// out of range (the consumer just draws fewer layers).
static CwDrawOp* MM_Op(CwItemDrawInfo* out, int32_t op) {
    if (out->opCount >= CW_DRAW_MAX_OPS) {
        return NULL;
    }
    CwDrawOp* o = &out->ops[out->opCount++];
    *o = CwDrawOp{};
    o->op = op;
    return o;
}
static void MM_OpV(CwItemDrawInfo* out, int32_t op, float a, float b, float c) {
    CwDrawOp* o = MM_Op(out, op);
    if (o != NULL) {
        o->a = a;
        o->b = b;
        o->c = c;
    }
}
static void MM_OpColor(CwItemDrawInfo* out, int32_t op, uint8_t r, uint8_t g, uint8_t b, uint8_t a, float lodFrac) {
    CwDrawOp* o = MM_Op(out, op);
    if (o != NULL) {
        o->rgba[0] = r;
        o->rgba[1] = g;
        o->rgba[2] = b;
        o->rgba[3] = a;
        o->a = lodFrac;
    }
}
// Append a display list and the op that submits it.
static void MM_OpDL(CwItemDrawInfo* out, const char* dl) {
    if (out->dlistCount >= CW_DRAW_MAX_DLISTS) {
        return;
    }
    int32_t i = out->dlistCount++;
    out->dlists[i] = dl;
    MM_OpV(out, CW_OP_DLIST, (float)i, 0.0f, 0.0f);
}

// Same two color channels in both hosts. The crest's metal is inside MetalDL,
// so Stone Tower's gold/silver ornament and key body cannot diverge.
static int32_t MM_FillDungeonKeyModelInfo(RandoItemId id, s16 drawId, CwItemDrawInfo* out) {
    const char *metalPath, *emblemPath;
    Color_RGBA8 metal, emblem;
    if (!GetItem_GetDungeonKeyModel(drawId, DungeonItem_GetOwner(id), &metalPath, &emblemPath, &metal, &emblem)) {
        return 0;
    }
    out->drawKind = CW_DRAW_KIND_OPS;
    out->xluStartIndex = -1;
    out->scale = 0.0f;       // OPS carries its transform explicitly; SIMPLE's scale field is not replayed here.
    out->stateDependent = 2; // Live appearance, distinct from a grant-latched progressive tier.
    MM_Op(out, CW_OP_SETUP_OPA);
    MM_Op(out, CW_OP_GRAYSCALE_OFF);
    MM_OpColor(out, CW_OP_PRIM_COLOR, 255, 255, 255, 255, 128.0f);
    MM_Op(out, CW_OP_PUSH);
    MM_OpV(out, CW_OP_SCALE, 0.25f, 0.25f, 0.25f);
    MM_Op(out, CW_OP_LOAD_MATRIX);
    MM_OpColor(out, CW_OP_ENV_COLOR, metal.r, metal.g, metal.b, 255, 0.0f);
    MM_OpDL(out, metalPath);
    MM_OpColor(out, CW_OP_ENV_COLOR, emblem.r, emblem.g, emblem.b, 255, 0.0f);
    MM_OpDL(out, emblemPath);
    MM_OpColor(out, CW_OP_ENV_COLOR, 255, 255, 255, 255, 0.0f);
    MM_Op(out, CW_OP_POP);
    return 1;
}

// All four vanilla GI families and retained replacements use the same palette-only
// helpers as native MM. Compass glass remains untinted; an edited boss gem is separate.
static int32_t MM_FillDungeonTintInfo(RandoItemId id, s16 drawId, CwItemDrawInfo* out) {
    const int owner = DungeonItem_GetOwner(id);
    Color_RGBA8 c, emblem;
    u8 strength = 0;
    const bool tintBody = GetItem_GetDungeonItemTint(drawId, owner, &c, &strength);
    const bool tintGem = drawId == GID_KEY_BOSS && GetItem_GetDungeonKeyEmblemTint(owner, &emblem);
    if (!tintBody && !tintGem) {
        return 0;
    }
    void* dls[CW_DRAW_MAX_DLISTS] = {};
    s32 xluStart = -1, scroll = 0, drawKind = CW_DRAW_KIND_SIMPLE;
    f32 scale = 0.0f;
    const int n = GetItem_GetDrawTableEntry(drawId, dls, CW_DRAW_MAX_DLISTS, &xluStart, &scale, &scroll, &drawKind);
    const int opaEnd = xluStart >= 0 ? xluStart : n;
    if (n <= 0 || opaEnd <= 0 || n > CW_DRAW_MAX_DLISTS || xluStart >= n) {
        return 0;
    }
    out->drawKind = CW_DRAW_KIND_OPS;
    out->xluStartIndex = -1;
    out->scale = scale;
    out->stateDependent = 2;
    void *setupOpa = nullptr, *setupXlu = nullptr;
    GetItem_GetDrawSetupDLs(drawId, &setupOpa, &setupXlu);
    out->setupDlOpa = setupOpa;
    out->setupDlXlu = setupXlu;
    MM_Op(out, CW_OP_SETUP_OPA);
    if (tintBody) {
        MM_OpColor(out, CW_OP_GRAYSCALE_COLOR, c.r, c.g, c.b, strength, 0.0f);
        MM_Op(out, CW_OP_GRAYSCALE_ON);
    } else {
        MM_Op(out, CW_OP_GRAYSCALE_OFF);
    }
    MM_Op(out, CW_OP_LOAD_MATRIX);
    for (int i = 0; i < opaEnd; i++) {
        MM_OpDL(out, (const char*)dls[i]);
    }
    MM_Op(out, CW_OP_GRAYSCALE_OFF);
    if (xluStart >= 0) {
        MM_Op(out, CW_OP_SETUP_XLU);
        if (tintGem) {
            MM_OpColor(out, CW_OP_GRAYSCALE_COLOR, emblem.r, emblem.g, emblem.b, 192, 0.0f);
            MM_Op(out, CW_OP_GRAYSCALE_ON);
        } else {
            MM_Op(out, CW_OP_GRAYSCALE_OFF);
        }
        MM_Op(out, CW_OP_LOAD_MATRIX);
        for (int i = xluStart; i < n; i++) {
            MM_OpDL(out, (const char*)dls[i]);
        }
        MM_Op(out, CW_OP_GRAYSCALE_OFF);
    }
    return 1;
}

// Songs have no sDrawItemTable row — MM draws them as one tinted note DL (Rando/DrawItem.cpp
// DrawSong: 25Xlu + per-song gDPSetEnvColor + gGiSongNoteDL). Fully portable as a static
// description. Returns 1 and fills env color if the item is a song. Color table mirrors DrawSong.
static int32_t MM_FillSongDrawInfo(RandoItemId id, CwItemDrawInfo* out) {
    uint8_t rgb[4];
    if (!ComboSongShimmerColor(ComboSongForMmItem(id), rgb))
        return 0;
    out->drawKind = CW_DRAW_KIND_SONG_GI;
    out->neiEffect = ComboSongForMmItem(id);
    out->itemShimmer = ComboSongHasOverlay(out->neiEffect);
    std::memcpy(out->itemShimmerColor, rgb, 4);
    out->dlists[0] = gGiSongNoteDL;
    out->dlistCount = 1;
    out->xluStartIndex = 0; // XLU layer, like MM's DrawSong
    out->scale = 0.0f;
    out->hasEnvColor = 1;
    out->envColor[0] = rgb[0];
    out->envColor[1] = rgb[1];
    out->envColor[2] = rgb[2];
    out->envColor[3] = 255;
    return 1;
}

// Items MM draws with a bespoke Rando::DrawItem func whose shape is per-DL transforms/colors rather
// than one flat OPA/XLU submission. Each branch is a 1:1 description of the func in
// mm/2s2h/Rando/DrawItem.cpp (or DrawFuncs.cpp for the clock). Returns 1 when id is handled.
static int32_t MM_FillOpsDrawInfo(RandoItemId id, CwItemDrawInfo* out) {
    switch (id) {
        case RI_OWL_CLOCK_TOWN_SOUTH: // DrawOwlStatue: opened-owl model, scale 0.01, -3000 y-translate
        case RI_OWL_GREAT_BAY_COAST:
        case RI_OWL_IKANA_CANYON:
        case RI_OWL_MILK_ROAD:
        case RI_OWL_MOUNTAIN_VILLAGE:
        case RI_OWL_SNOWHEAD:
        case RI_OWL_SOUTHERN_SWAMP:
        case RI_OWL_STONE_TOWER:
        case RI_OWL_WOODFALL:
        case RI_OWL_ZORA_CAPE:
            MM_Op(out, CW_OP_SETUP_OPA);
            MM_OpV(out, CW_OP_SCALE, 0.01f, 0.01f, 0.01f);
            MM_OpV(out, CW_OP_TRANSLATE, 0.0f, -3000.0f, 0.0f);
            MM_Op(out, CW_OP_LOAD_MATRIX);
            MM_OpDL(out, gOwlStatueOpenedDL);
            break;

        case RI_TIME_DAY_1: // DrawClock (DrawFuncs.cpp)
        case RI_TIME_DAY_2:
        case RI_TIME_DAY_3:
        case RI_TIME_NIGHT_1:
        case RI_TIME_NIGHT_2:
        case RI_TIME_NIGHT_3:
        case RI_TIME_PROGRESSIVE: {
            bool night = (id == RI_TIME_NIGHT_1) || (id == RI_TIME_NIGHT_2) || (id == RI_TIME_NIGHT_3) ||
                         ((id == RI_TIME_PROGRESSIVE) && gSaveContext.save.isNight);
            // No ObjTokeidai actor cross-game, so DrawClock's live-actor fields stay 0 and its
            // yTranslation / +-1791 / clockFaceZTranslation steps fold to identity — dropped here.
            int16_t clockFaceRotation = night ? 0 : (int16_t)0xC000;
            int16_t sunMoonPanelRotation = night ? (int16_t)0x8000 : 0;
            MM_Op(out, CW_OP_SETUP_OPA);
            MM_OpV(out, CW_OP_SCALE, 0.015f, 0.015f, 0.015f);
            MM_Op(out, CW_OP_PUSH);
            MM_Op(out, CW_OP_LOAD_MATRIX);
            MM_OpDL(out, gClockTowerMinuteRingDL);
            MM_Op(out, CW_OP_POP);
            MM_Op(out, CW_OP_LOAD_MATRIX);
            MM_OpDL(out, gClockTowerClockCenterAndHandDL);
            MM_OpV(out, CW_OP_ROTATE_Z, (float)(int16_t)(-clockFaceRotation * 2), 0.0f, 0.0f);
            MM_Op(out, CW_OP_LOAD_MATRIX);
            MM_OpDL(out, gClockTowerClockFaceDL);
            MM_OpV(out, CW_OP_TRANSLATE, 0.0f, -1112.0f, -19.6f);
            MM_OpV(out, CW_OP_ROTATE_Y, (float)sunMoonPanelRotation, 0.0f, 0.0f);
            MM_Op(out, CW_OP_LOAD_MATRIX);
            MM_OpDL(out, gClockTowerSunAndMoonPanelDL);
            break;
        }

        case RI_SKELETON_KEY: // DrawSkeletonKey
            MM_Op(out, CW_OP_SETUP_OPA);
            MM_OpV(out, CW_OP_SCALE, 0.8f, 0.8f, 0.8f);
            MM_Op(out, CW_OP_LOAD_MATRIX);
            MM_OpColor(out, CW_OP_ENV_COLOR, 255, 255, 170, 255, 0.0f);
            MM_OpDL(out, gSkeletonKeyDL);
            break;

        case RI_DOUBLE_DEFENSE: // DrawDoubleDefense: white-grayscale border, then red-grayscale container
            MM_Op(out, CW_OP_SETUP_XLU);
            MM_Op(out, CW_OP_LOAD_MATRIX);
            MM_OpColor(out, CW_OP_GRAYSCALE_COLOR, 255, 255, 255, 255, 0.0f);
            MM_Op(out, CW_OP_GRAYSCALE_ON);
            MM_OpDL(out, gGiHeartBorderDL);
            MM_OpColor(out, CW_OP_GRAYSCALE_COLOR, 255, 0, 0, 100, 0.0f);
            MM_OpDL(out, gGiHeartContainerDL);
            MM_Op(out, CW_OP_GRAYSCALE_OFF);
            break;

        case RI_WALLET_TYCOON: // DrawTycoonWallet: Giant's Wallet layers, body overridden purple
            MM_Op(out, CW_OP_SETUP_OPA);
            MM_Op(out, CW_OP_LOAD_MATRIX);
            MM_OpDL(out, gGiGiantsWalletColorDL);
            MM_OpColor(out, CW_OP_PRIM_COLOR, 150, 0, 200, 255, 128.0f);
            MM_OpColor(out, CW_OP_ENV_COLOR, 80, 0, 120, 255, 0.0f);
            MM_OpDL(out, gGiWalletDL);
            MM_OpDL(out, gGiGiantsWalletRupeeOuterColorDL);
            MM_OpDL(out, gGiWalletRupeeOuterDL);
            MM_OpDL(out, gGiGiantsWalletStringColorDL);
            MM_OpDL(out, gGiWalletStringDL);
            MM_OpDL(out, gGiGiantsWalletRupeeInnerColorDL);
            MM_OpDL(out, gGiWalletRupeeInnerDL);
            break;

        default:
            return 0;
    }
    out->drawKind = CW_DRAW_KIND_OPS;
    out->xluStartIndex = -1; // unused by the ops path
    return 1;
}

// Items MM draws as one plain scaled OPA or XLU display list (no table row, no extra GPU state).
static int32_t MM_FillSimpleDrawInfo(RandoItemId id, CwItemDrawInfo* out) {
    const char* dl;
    bool xlu;
    float scale = 0.0f;
    switch (id) {
        case RI_ABILITY_SWIM: // DrawAbilityItem
            dl = gGiFlippersDL;
            xlu = true;
            break;
        case RI_MAX_TRAP: // DrawTrapModel (ice cube)
            dl = gTrapDL;
            xlu = true;
            scale = 0.03f;
            break;
        case RI_OCARINA_BUTTON_A: // DrawOcarinaButtonItem
        case RI_OCARINA_BUTTON_C_DOWN:
        case RI_OCARINA_BUTTON_C_RIGHT:
        case RI_OCARINA_BUTTON_C_LEFT:
        case RI_OCARINA_BUTTON_C_UP: {
            static const char* buttons[5] = {
                gOcarinaAButtonDL,     gOcarinaCDownButtonDL, gOcarinaCRightButtonDL,
                gOcarinaCLeftButtonDL, gOcarinaCUpButtonDL,
            };
            dl = buttons[id - RI_OCARINA_BUTTON_A];
            xlu = false;
            break;
        }
        case RI_TRIFORCE_PIECE: // DrawTriforcePiece: shard cycles with the collected count
        case RI_TRIFORCE_PIECE_PREVIOUS: {
            static const char* shards[3] = { gTriforcePiece0DL, gTriforcePiece1DL, gTriforcePiece2DL };
            uint16_t found = gSaveContext.save.shipSaveInfo.rando.foundTriforcePieces;
            if (found >= RANDO_SAVE_OPTIONS[RO_TRIFORCE_PIECES_REQUIRED]) {
                dl = gTriforcePieceCompletedDL;
            } else if (id == RI_TRIFORCE_PIECE_PREVIOUS) {
                dl = shards[(found > 0 ? found - 1 : 0) % 3]; // guard the 0-shard underflow MM's func has
            } else {
                dl = shards[found % 3];
            }
            xlu = true;
            scale = 0.03f;
            break;
        }
        default:
            return 0;
    }
    out->drawKind = CW_DRAW_KIND_SIMPLE;
    out->dlistCount = 1;
    out->dlists[0] = dl;
    out->xluStartIndex = xlu ? 0 : -1;
    out->scale = scale;
    return 1;
}

// Enemy souls (GID_NONE) are drawn by bespoke SkelAnime funcs — the enemy model plus DrawEnLight's
// billboarded flame. The skeletons aren't expressible cross-game, so we emit the FLAME ONLY, in the
// soul's own color (DrawFuncs.cpp DrawEnLight + the per-enemy color at each Draw* tail). Reads as a
// soul rather than a stand-in item; the enemy body is the part we drop.
static int32_t MM_FillEnemySoulDrawInfo(RandoItemId id, CwItemDrawInfo* out) {
    uint8_t rgb[3] = { 155, 155, 155 }; // the color the large majority of souls use
    switch (id) {
        case RI_SOUL_ENEMY_ALIEN:
            rgb[0] = 10;
            rgb[1] = 138;
            rgb[2] = 46;
            break;
        case RI_SOUL_ENEMY_CAPTAIN_KEETA:
            rgb[0] = 255;
            rgb[1] = 192;
            rgb[2] = 0;
            break;
        case RI_SOUL_ENEMY_DEXIHAND:
            rgb[0] = 155;
            rgb[1] = 155;
            rgb[2] = 70;
            break;
        case RI_SOUL_ENEMY_EENO:
            rgb[0] = 155;
            rgb[1] = 155;
            rgb[2] = 35;
            break;
        case RI_SOUL_ENEMY_EYEGORE:
            rgb[0] = 192;
            rgb[1] = 192;
            rgb[2] = 64;
            break;
        case RI_SOUL_ENEMY_GARO:
            rgb[0] = 150;
            rgb[1] = 255;
            rgb[2] = 150;
            break;
        case RI_SOUL_ENEMY_GEKKO:
            rgb[0] = 150;
            rgb[1] = 100;
            rgb[2] = 255;
            break;
        case RI_SOUL_ENEMY_GOMESS:
            rgb[0] = 155;
            rgb[1] = 0;
            rgb[2] = 0;
            break;
        case RI_SOUL_ENEMY_IGOS_DU_IKANA:
            rgb[0] = 0;
            rgb[1] = 0;
            rgb[2] = 0;
            break;
        // Every remaining enemy soul uses DrawEnLight's default gray flame.
        case RI_SOUL_ENEMY_ARMOS:
        case RI_SOUL_ENEMY_BAD_BAT:
        case RI_SOUL_ENEMY_BEAMOS:
        case RI_SOUL_ENEMY_BOE:
        case RI_SOUL_ENEMY_BUBBLE:
        case RI_SOUL_ENEMY_CHUCHU:
        case RI_SOUL_ENEMY_DEATH_ARMOS:
        case RI_SOUL_ENEMY_DEEP_PYTHON:
        case RI_SOUL_ENEMY_DEKU_BABA:
        case RI_SOUL_ENEMY_DINOLFOS:
        case RI_SOUL_ENEMY_DODONGO:
        case RI_SOUL_ENEMY_DRAGONFLY:
        case RI_SOUL_ENEMY_FREEZARD:
        case RI_SOUL_ENEMY_GIANT_BEE:
        case RI_SOUL_ENEMY_GUAY:
        case RI_SOUL_ENEMY_HIPLOOP:
        case RI_SOUL_ENEMY_IRON_KNUCKLE:
        case RI_SOUL_ENEMY_KEESE:
        case RI_SOUL_ENEMY_LEEVER:
        case RI_SOUL_ENEMY_LIKE_LIKE:
        case RI_SOUL_ENEMY_MAD_SCRUB:
        case RI_SOUL_ENEMY_NEJIRON:
        case RI_SOUL_ENEMY_OCTOROK:
        case RI_SOUL_ENEMY_PEAHAT:
        case RI_SOUL_ENEMY_PIRATE:
        case RI_SOUL_ENEMY_POE:
        case RI_SOUL_ENEMY_REDEAD:
        case RI_SOUL_ENEMY_SHELLBLADE:
        case RI_SOUL_ENEMY_SKULLFISH:
        case RI_SOUL_ENEMY_SKULLTULA:
        case RI_SOUL_ENEMY_SNAPPER:
        case RI_SOUL_ENEMY_STALCHILD:
        case RI_SOUL_ENEMY_TAKKURI:
        case RI_SOUL_ENEMY_TEKTITE:
        case RI_SOUL_ENEMY_WALLMASTER:
        case RI_SOUL_ENEMY_WART:
        case RI_SOUL_ENEMY_WIZROBE:
        case RI_SOUL_ENEMY_WOLFOS:
            break;
        default:
            return 0;
    }
    out->drawKind = CW_DRAW_KIND_MM_SOUL_FLAME;
    out->dlistCount = 1;
    out->dlists[0] = gameplay_keep_DL_01ACF0;
    out->xluStartIndex = 0;
    out->primColorXlu[0] = rgb[0];
    out->primColorXlu[1] = rgb[1];
    out->primColorXlu[2] = rgb[2];
    out->primColorXlu[3] = 255;
    return 1;
}

// GID aliasing: items with no table row of their own (GID_NONE) whose real draw func is a bespoke
// SkelAnime routine, mapped to a stand-in table row so they get a recognizable model instead of the
// sentinel. Animated boss souls and minifrogs are intercepted before this legacy fallback;
// Majora still aliases Twinmold's remains because its distinct material route is not described.
static int32_t MM_FillGidAliasDrawInfo(RandoItemId id, CwItemDrawInfo* out) {
    s32 gid;
    switch (id) {
        case RI_SOUL_BOSS_GOHT:
            gid = GID_REMAINS_GOHT;
            break;
        case RI_SOUL_BOSS_GYORG:
            gid = GID_REMAINS_GYORG;
            break;
        case RI_SOUL_BOSS_ODOLWA:
            gid = GID_REMAINS_ODOLWA;
            break;
        case RI_SOUL_BOSS_TWINMOLD:
        case RI_SOUL_BOSS_MAJORA: // no Majora remains model; Twinmold's stands in
            gid = GID_REMAINS_TWINMOLD;
            break;
        case RI_FROG_BLUE:
        case RI_FROG_CYAN:
        case RI_FROG_PINK:
        case RI_FROG_WHITE:
            gid = GID_MASK_DON_GERO;
            break;
        default:
            return 0;
    }
    void* dls[CW_DRAW_MAX_DLISTS] = {};
    int32_t xluStart = -1;
    f32 scale = 0.0f;
    s32 xluSeg8TexScroll = 0;
    s32 drawKind = CW_DRAW_KIND_SIMPLE;
    int32_t n =
        GetItem_GetDrawTableEntry(gid, dls, CW_DRAW_MAX_DLISTS, &xluStart, &scale, &xluSeg8TexScroll, &drawKind);
    if (n <= 0) {
        return 0;
    }
    out->dlistCount = n;
    out->xluStartIndex = xluStart;
    out->scale = scale;
    out->xluSeg8TexScroll = xluSeg8TexScroll;
    out->drawKind = drawKind;
    out->hasEnvColor = 0;
    for (int32_t k = 0; k < n; k++) {
        out->dlists[k] = (const char*)dls[k];
    }
    return 1;
}

// ---- ComboShip: the animated (skeletal) cross-game class. MM's enemy souls and minifrogs are drawn
// by bespoke SkelAnime funcs (Rando/DrawFuncs.cpp); described here, OOT renders the REAL model via
// combo/menu/ComboForeignAnim.h instead of the flame-only / Don-Gero-mask stand-ins above.

// One enemy soul's common shape. The per-soul extras (segment binds, colours, texanims) live in the
// switch in MM_FillEnemySoulAnim; everything here is 1:1 with that enemy's Draw* in DrawFuncs.cpp.
struct MmSoulModel {
    int32_t id;
    const char* skel;
    const char* anim;
    int32_t limbCount;
    int32_t nonFlex;
    float scale;
    float transPostY; // Matrix_Translate AFTER the scale, as the real funcs do (model units)
    uint8_t flame[3];
    float flameSize;
};

// clang-format off
static const MmSoulModel kMmSoulModels[] = {
    { RI_SOUL_ENEMY_ALIEN,         gAlienSkel,                  gAlienFloatAnim,                ALIEN_LIMB_MAX,          0, 0.007f,     0.0f, {  10, 138,  46 }, 30.0f },
    { RI_SOUL_ENEMY_ARMOS,         object_am_Skel_005948,       gArmosHopAnim,                  OBJECT_AM_LIMB_MAX,      1, 0.01f,  -3100.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_BEAMOS,        gBeamosSkel,                 gBeamosAnim,                    BEAMOS_LIMB_MAX,         1, 0.01f,  -3200.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_BUBBLE,        gBubbleSkel,                 gBubbleFlyingAnim,              BUBBLE_LIMB_MAX,         1, 0.02f,      0.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_CAPTAIN_KEETA, object_bsb_Skel_00C3E0,      object_bsb_Anim_004894,         OBJECT_BSB_LIMB_MAX,     1, 0.01f,  -3500.0f, { 255, 192,   0 },  5.0f },
    { RI_SOUL_ENEMY_DEATH_ARMOS,   gFamosSkel,                  gFamosIdleAnim,                 FAMOS_LIMB_MAX,          1, 0.008f, -4100.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_DEEP_PYTHON,   gDeepPythonSkel,             gDeepPythonUnusedSideSwayAnim,  DEEP_PYTHON_LIMB_MAX,    0, 0.02f,      0.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_DEKU_BABA,     gDekuBabaSkel,               gDekuBabaFastChompAnim,         DEKUBABA_LIMB_MAX,       1, 0.02f,      0.0f, { 155, 155, 155 },  6.0f },
    { RI_SOUL_ENEMY_DINOLFOS,      gDinolfosSkel,               gDinolfosIdleAnim,              DINOLFOS_LIMB_MAX,       0, 0.014f, -2200.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_DODONGO,       object_dodongo_Skel_008318,  object_dodongo_Anim_004C20,     OBJECT_DODONGO_LIMB_MAX, 1, 0.015f, -1500.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_DRAGONFLY,     gDragonflySkel,              gDragonflyFlyAnim,              DRAGONFLY_LIMB_MAX,      1, 0.01f,   -700.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_EENO,          gEenoSkel,                   gEenoIdleAnim,                  EENO_LIMB_MAX,           0, 0.01f,  -3000.0f, { 155, 155,  35 }, 10.0f },
    { RI_SOUL_ENEMY_EYEGORE,       gEyegoreSkel,                gEyegoreUnusedWalkAnim,         EYEGORE_LIMB_MAX,        0, 0.006f, -4000.0f, { 192, 192,  64 }, 20.0f },
    { RI_SOUL_ENEMY_GARO,          gGaroSkel,                   gGaroIdleAnim,                  GARO_LIMB_MAX,           0, 0.03f,      0.0f, { 150, 255, 150 },  8.0f },
    { RI_SOUL_ENEMY_GEKKO,         gGekkoSkel,                  gGekkoBoxingStanceAnim,         GEKKO_LIMB_MAX,          0, 0.006f, -4100.0f, { 150, 100, 255 }, 20.0f },
    { RI_SOUL_ENEMY_GIANT_BEE,     gBeeSkel,                    gBeeFlyingAnim,                 OBJECT_BEE_LIMB_MAX,     1, 0.01f,      0.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_GUAY,          gGuaySkel,                   gGuayFlyAnim,                   OBJECT_CROW_LIMB_MAX,    0, 0.02f,      0.0f, { 155, 155, 155 },  6.0f },
    { RI_SOUL_ENEMY_HIPLOOP,       gHiploopSkel,                gHiploopChargeAnim,             HIPLOOP_LIMB_MAX,        0, 0.02f,  -1400.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_IGOS_DU_IKANA, gIgosSkel,                   gKnightIdleAnim,                IGOS_LIMB_MAX,           0, 0.01f,  -2000.0f, {   0,   0,   0 }, 10.0f },
    { RI_SOUL_ENEMY_KEESE,         gFireKeeseSkel,              gFireKeeseFlyAnim,              FIRE_KEESE_LIMB_MAX,     1, 0.01f,   -700.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_LEEVER,        gLeeverSkel,                 gLeeverSpinAnim,                LEEVER_LIMB_MAX,         1, 0.05f,   -700.0f, { 155, 155, 155 },  3.0f },
    { RI_SOUL_ENEMY_MAD_SCRUB,     gDekuScrubSkel,              gDekuScrubLookAroundAnim,       DEKU_SCRUB_LIMB_MAX,     1, 0.01f,  -2300.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_NEJIRON,       gNejironSkel,                gNejironIdleAnim,               NEJIRON_LIMB_MAX,        1, 0.015f,     0.0f, { 155, 155, 155 }, 13.0f },
    { RI_SOUL_ENEMY_OCTOROK,       gOctorokSkel,                gOctorokFloatAnim,              OCTOROK_LIMB_MAX,        1, 0.007f,  -700.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_PEAHAT,        object_ph_Skel_001C80,       object_ph_Anim_0009C4,          OBJECT_PH_LIMB_MAX,      1, 0.01f,      0.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_PIRATE,        gFighterPirateSkel,          gFighterPirateFightingIdleAnim, KAIZOKU_LIMB_MAX,        0, 0.01f,  -2000.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_POE,           gPoeSkel,                    gPoeFloatAnim,                  POE_LIMB_MAX,            1, 0.0075f,-5000.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_REDEAD,        gRedeadSkel,                 gGibdoRedeadPirouetteAnim,      REDEAD_LIMB_MAX,         0, 0.01f,  -2900.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_SHELLBLADE,    object_sb_Skel_002BF0,       object_sb_Anim_000194,          OBJECT_SB_LIMB_MAX,      0, 0.007f, -3500.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_SKULLFISH,     object_pr_Skel_004188,       object_pr_Anim_004340,          OBJECT_PR_2_LIMB_MAX,    0, 0.02f,      0.0f, { 155, 155, 155 },  5.0f },
    { RI_SOUL_ENEMY_SKULLTULA,     object_st_Skel_005298,       object_st_Anim_000304,          OBJECT_ST_LIMB_MAX,      1, 0.03f,      0.0f, { 155, 155, 155 },  5.0f },
    { RI_SOUL_ENEMY_SNAPPER,       gSnapperSkel,                gSnapperIdleAnim,               SNAPPER_LIMB_MAX,        0, 0.01f,  -3100.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_STALCHILD,     gStalchildSkel,              gStalchildIdleAnim,             STALCHILD_LIMB_MAX,      1, 0.01f,  -3200.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_TAKKURI,       gTakkuriSkel,                gTakkuriFlyAnim,                TAKKURI_LIMB_MAX,        0, 0.01f,      0.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_TEKTITE,       object_tite_Skel_003A20,     object_tite_Anim_0012E4,        OBJECT_TITE_LIMB_MAX,    1, 0.01f,  -2900.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_WALLMASTER,    gWallmasterSkel,             gWallmasterIdleAnim,            WALLMASTER_LIMB_MAX,     0, 0.01f,  -3500.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_WART,          gWartSkel,                   gWartIdleAnim,                  WART_LIMB_MAX,           0, 0.02f,      0.0f, { 155, 155, 155 }, 10.0f },
    { RI_SOUL_ENEMY_WIZROBE,       gWizrobeSkel,                gWizrobeIdleAnim,               WIZROBE_LIMB_MAX,        0, 0.006f,     0.0f, { 155, 155, 155 }, 15.0f },
    { RI_SOUL_ENEMY_WOLFOS,        gWolfosNormalSkel,           gWolfosWaitAnim,                WOLFOS_NORMAL_LIMB_MAX,  0, 0.01f,  -3000.0f, { 155, 155, 155 }, 10.0f },
};
// clang-format on

static const MmSoulModel* MM_FindSoulModel(RandoItemId id) {
    for (const MmSoulModel& m : kMmSoulModels) {
        if (m.id == (int32_t)id) {
            return &m;
        }
    }
    return nullptr;
}

// Overflow writes to the last slot but still bumps the count, so the consumer's validation rejects
// the whole recipe (sentinel) instead of us writing out of range or silently dropping a bind.
static CwAnimSegBind* MM_AnimSeg(CwItemAnimDrawInfo* out, int32_t kind, int32_t segment) {
    int32_t i = out->segCount++;
    CwAnimSegBind* s = &out->segs[i < CW_ANIM_MAX_SEGS ? i : CW_ANIM_MAX_SEGS - 1];
    s->kind = kind;
    s->segment = segment;
    s->onOpa = 1;
    return s;
}
static void MM_AnimTexSeg(CwItemAnimDrawInfo* out, int32_t segment, const char* path) {
    MM_AnimSeg(out, CW_ANIM_SEG_PATH, segment)->path = path;
}
static void MM_AnimEnv(CwItemAnimDrawInfo* out, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    out->hasModelEnvColor = 1;
    out->modelEnvColor[0] = r;
    out->modelEnvColor[1] = g;
    out->modelEnvColor[2] = b;
    out->modelEnvColor[3] = a;
}
static void MM_AnimPrim(CwItemAnimDrawInfo* out, int32_t lodFrac, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    out->hasPrimColor = 1;
    out->primLodFrac = lodFrac;
    out->primColor[0] = r;
    out->primColor[1] = g;
    out->primColor[2] = b;
    out->primColor[3] = a;
}

// DrawEnLight: the billboarded soul flame, drawn AFTER the model so it inherits its scale/translate.
static void MM_AnimSoulFlame(CwItemAnimDrawInfo* out, const uint8_t rgb[3], float sx, float sy, float sz) {
    out->flameDlPath = gameplay_keep_DL_01ACF0;
    out->flameAfter = 1;
    out->flameBillboardFirst = 1;
    out->flameColor[0] = rgb[0];
    out->flameColor[1] = rgb[1];
    out->flameColor[2] = rgb[2];
    out->flameColor[3] = 255;
    out->flameScale[0] = sx;
    out->flameScale[1] = sy;
    out->flameScale[2] = sz;
    out->flameHasSeg = 1;
    out->flameSeg.kind = CW_ANIM_SEG_TEXSCROLL;
    out->flameSeg.segment = 8;
    out->flameSeg.onXlu = 1;
    out->flameSeg.width1 = out->flameSeg.width2 = 0x10;
    out->flameSeg.height1 = out->flameSeg.height2 = 0x20;
    out->flameSeg.xStep2 = 2;
    out->flameSeg.xMask2 = 0x3F;
    out->flameSeg.yStep2 = -6;
    out->flameSeg.yMask2 = 0x7F;
}

// Native MM boss models whose draw routines fit the existing skeletal ABI. Canonical
// paths are loaded through MM's RM, including its own Alt selection and vanilla fallback.
// Native Goht/Gyorg/Odolwa use this same recipe to avoid a rig cached across Alt changes.
// Twinmold uses its native head rig/clip and a dedicated initialized matrix-13 profile.
static int32_t MM_FillBossSoulAnim(RandoItemId id, CwItemAnimDrawInfo* out) {
    uint8_t flame[3];
    float flameSize;
    switch (id) {
        case RI_SOUL_BOSS_GOHT:
            out->skelPath = gGohtSkel;
            out->animPath = gGohtRunAnim;
            out->limbCount = GOHT_LIMB_MAX;
            out->scale = 0.005f;
            flame[0] = 10;
            flame[1] = 138;
            flame[2] = 46;
            flameSize = 30.0f;
            MM_AnimTexSeg(out, 8, gGohtMetalPlateWithCirclePatternTex);
            break;
        case RI_SOUL_BOSS_GYORG:
            out->skelPath = gGyorgSkel;
            out->animPath = gGyorgGentleSwimmingAnim;
            out->limbCount = GYORG_LIMB_MAX;
            out->scale = 0.05f;
            flame[0] = 19;
            flame[1] = 99;
            flame[2] = 165;
            flameSize = 3.0f;
            break;
        case RI_SOUL_BOSS_ODOLWA:
            out->skelPath = gOdolwaSkel;
            out->animPath = gOdolwaReadyAnim;
            out->limbCount = ODOLWA_LIMB_MAX;
            out->scale = 0.005f;
            flame[0] = 145;
            flame[1] = 20;
            flame[2] = 133;
            flameSize = 25.0f;
            break;
        case RI_SOUL_BOSS_TWINMOLD:
            out->skelPath = gTwinmoldHeadSkel;
            out->animPath = gTwinmoldHeadFlyAnim;
            out->limbCount = TWINMOLD_HEAD_LIMB_MAX;
            out->nonFlexSkeleton = 1;
            out->scale = 0.06f;
            out->proceduralProfile = CW_ANIM_PROFILE_MM_TWINMOLD;
            flame[0] = 168;
            flame[1] = 180;
            flame[2] = 20;
            flameSize = 3.0f;
            MM_AnimTexSeg(out, 8, gTwinmoldBlueSkinTex);
            break;
        default:
            return 0;
    }
    out->opa = 1;
    out->hiddenLimb = -1;
    out->translatePre[1] = id == RI_SOUL_BOSS_TWINMOLD ? 0.0f : -20.0f;
    MM_AnimSoulFlame(out, flame, flameSize, flameSize, flameSize);
    return 1;
}

// The 8 enemy souls with no entry in the table above keep the flame-only stand-in: Bad Bat (9 wing
// frame DLs, no skeleton), Boe / Chuchu / Freezard / Like Like (non-skeletal or matrix-array driven),
// Dexihand (hand-built arm segment chain), Gomess (two texanims, one stepped) and Iron Knuckle
// (segments bound to material DLs). Each needs GPU state this ABI deliberately cannot express.
static int32_t MM_FillEnemySoulAnim(RandoItemId id, CwItemAnimDrawInfo* out) {
    const MmSoulModel* m = MM_FindSoulModel(id);
    if (m == nullptr) {
        return 0;
    }
    out->opa = 1;
    out->hiddenLimb = -1;
    out->skelPath = m->skel;
    out->animPath = m->anim;
    out->limbCount = m->limbCount;
    out->nonFlexSkeleton = m->nonFlex;
    out->scale = m->scale;
    out->translatePost[1] = m->transPostY;
    MM_AnimSoulFlame(out, m->flame, m->flameSize, m->flameSize, m->flameSize);

    switch (id) {
        case RI_SOUL_ENEMY_ALIEN:
            MM_AnimTexSeg(out, 8, gAlienEyeTex);
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 0x0C); // Scene_SetRenderModeXlu(play, 0, 1)
            MM_AnimEnv(out, 255, 255, 255, 255);
            // gAlienEmptyTexAnim is a type-6 Empty no-op even in MM; describing it only trips the
            // consumer's texanim validation (sentinel rupee), so it is deliberately not carried.
            break;
        case RI_SOUL_ENEMY_ARMOS:
            MM_AnimEnv(out, 0, 0, 0, 255);
            break;
        case RI_SOUL_ENEMY_CAPTAIN_KEETA:
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 0x0C);
            MM_AnimEnv(out, 0, 0, 0, 255);
            out->flameScale[1] = 10.0f; // DrawEnLight({5, 10, 5}) — the one non-uniform flame
            break;
        case RI_SOUL_ENEMY_DEATH_ARMOS:
            out->texAnimPath = gFamosNormalGlowingEmblemTexAnim;
            break;
        case RI_SOUL_ENEMY_DINOLFOS:
            MM_AnimTexSeg(out, 8, gDinolfosEyeOpenTex);
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 0x0C);
            MM_AnimEnv(out, 20, 40, 40, 255);
            break;
        case RI_SOUL_ENEMY_EYEGORE:
            MM_AnimPrim(out, 0xFF, 175, 255, 255, 255);
            MM_AnimEnv(out, 255, 115, 155, 255);
            out->texAnimPath = gEyegoreEyeLaserTexAnim;
            break;
        case RI_SOUL_ENEMY_GARO:
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 0x0C);
            break;
        case RI_SOUL_ENEMY_HIPLOOP:
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 0x0C);
            break;
        case RI_SOUL_ENEMY_IGOS_DU_IKANA:
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 0x0A)->onXlu = 1;
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 0x09);
            break;
        case RI_SOUL_ENEMY_KEESE:
            MM_AnimEnv(out, 0, 0, 0, 0);
            break;
        case RI_SOUL_ENEMY_LEEVER:
            out->playSpeed = -1.0f; // the spin already matches the get-item rotation; reverse it
            MM_AnimPrim(out, 0x01, 255, 255, 255, 255);
            break;
        case RI_SOUL_ENEMY_NEJIRON:
            MM_AnimTexSeg(out, 8, gNejironEyeOpenTex);
            break;
        case RI_SOUL_ENEMY_OCTOROK:
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 8);
            break;
        case RI_SOUL_ENEMY_PIRATE:
            MM_AnimTexSeg(out, 8, gFighterPirateEyeOpenTex);
            break;
        case RI_SOUL_ENEMY_POE:
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 8);
            MM_AnimEnv(out, 255, 255, 255, 255);
            break;
        case RI_SOUL_ENEMY_REDEAD:
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 8);
            break;
        case RI_SOUL_ENEMY_SKULLFISH:
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 0x0C);
            MM_AnimPrim(out, 0, 255, 255, 255, 255);
            MM_AnimEnv(out, 0, 0, 0, 255);
            break;
        case RI_SOUL_ENEMY_SNAPPER:
            MM_AnimTexSeg(out, 8, gSnapperEyeOpenTex);
            break;
        case RI_SOUL_ENEMY_TEKTITE:
            MM_AnimTexSeg(out, 8, object_tite_Tex_001300);
            MM_AnimTexSeg(out, 9, object_tite_Tex_001700);
            MM_AnimTexSeg(out, 0x0A, object_tite_Tex_001900);
            break;
        case RI_SOUL_ENEMY_WIZROBE:
            out->translatePre[1] = -20.0f; // Wizrobe translates in world units, before the scale
            MM_AnimTexSeg(out, 8, gWizrobeEyeTex);
            MM_AnimSeg(out, CW_ANIM_SEG_EMPTY_DL, 0x0C);
            MM_AnimEnv(out, 255, 255, 255, 255);
            break;
        case RI_SOUL_ENEMY_WOLFOS:
            MM_AnimTexSeg(out, 8, gWolfosNormalEyeOpenTex);
            break;
        default:
            break;
    }
    return 1;
}

// DrawMinifrog: per-frog env colour, iris textures on segments 8/9, and the eye limbs suppressed
// then re-submitted billboarded in the post-limb pass (EnMinifrog_OverrideLimbDraw + PostLimbDraw).
static int32_t MM_FillMinifrogAnim(RandoItemId id, CwItemAnimDrawInfo* out) {
    uint8_t env[4] = { 200, 170, 0, 255 }; // FROG_YELLOW
    switch (id) {
        case RI_FROG_BLUE:
            env[0] = 120;
            env[1] = 130;
            env[2] = 230;
            break;
        case RI_FROG_CYAN:
            env[0] = 0;
            env[1] = 170;
            env[2] = 200;
            break;
        case RI_FROG_PINK:
            env[0] = 210;
            env[1] = 120;
            env[2] = 100;
            break;
        case RI_FROG_WHITE:
            env[0] = env[1] = env[2] = 190;
            break;
        default:
            return 0;
    }
    out->opa = 1;
    out->hiddenLimb = -1;
    out->skelPath = gFrogSkel;
    out->animPath = gFrogIdleAnim;
    out->limbCount = FROG_LIMB_MAX;
    out->translatePre[1] = -20.0f;
    out->scale = 0.03f;
    MM_AnimEnv(out, env[0], env[1], env[2], env[3]);
    MM_AnimTexSeg(out, 8, gFrogIrisOpenTex);
    MM_AnimTexSeg(out, 9, gFrogIrisOpenTex);

    CwAnimLimbDL* body = &out->limbDLs[out->limbDLCount++]; // 3 of CW_ANIM_MAX_LIMB_DLS
    body->limbIndex = FROG_LIMB_LOWER_BODY;
    body->posDz = -500.0f;
    for (int32_t limb : { (int32_t)FROG_LIMB_RIGHT_EYE, (int32_t)FROG_LIMB_LEFT_EYE }) {
        CwAnimLimbDL* eye = &out->limbDLs[out->limbDLCount++];
        eye->limbIndex = limb;
        eye->hide = 1;
        eye->postSelf = 1;
        eye->postBillboard = 1;
    }
    return 1;
}

static int32_t MM_FillAnimDrawInfo(RandoItemId id, CwItemAnimDrawInfo* out) {
    if (MM_FillBossSoulAnim(id, out) || MM_FillEnemySoulAnim(id, out) || MM_FillMinifrogAnim(id, out)) {
        return 1;
    }
    return 0;
}

// True for items served by the animated ABI, so the static export bails and OOT falls through to it.
static bool MM_HasAnimDraw(RandoItemId id) {
    CwItemAnimDrawInfo probe{};
    return MM_FillAnimDrawInfo(id, &probe) != 0;
}

// Cross-game item draw info. OOT resolves this via Combo_ResolveSym to learn which MM display lists
// render a foreign item, then submits them through "__OTR__@mm:"-routed paths resolved against
// MM's ResourceManager (CrossRMRegistry). itemName is the friendly combo-spoiler name the foreign
// map carries (resolve via GetItemIdFromDisplayName; fall back to the RI_ spoilerName for the
// sentinel / any raw id). The returned dlists point at MM's static OTR asset-path string literals,
// valid for process lifetime. Returns 0 for unknown items / non-portable draw funcs; the caller
// falls back to its sentinel.
// Items whose concrete model depends on how far the player has progressed.
static bool MM_IsProgressiveItem(RandoItemId id) {
    switch (id) {
        case RI_PROGRESSIVE_SWORD:
        case RI_OOT_PROGRESSIVE_HAMMER:
        case RI_OOT_PROGRESSIVE_MASTER_SWORD:
        case RI_OOT_PROGRESSIVE_BGS:
        case RI_PROGRESSIVE_BOW:
        case RI_PROGRESSIVE_BOMB_BAG:
        case RI_PROGRESSIVE_WALLET:
        case RI_PROGRESSIVE_MAGIC:
        case RI_PROGRESSIVE_LULLABY:
        case RI_TIME_PROGRESSIVE:
            return true;
        default:
            return false;
    }
}

// Items whose concrete model is picked at draw time from live state, so the consumer must
// re-resolve them every frame: junk/trap indirection and the Triforce shard cycle (progressive
// tiers are flagged separately below).
static bool MM_IsStateDependentDraw(RandoItemId id) {
    if (DungeonItem_GetOwner(id) >= 0) {
        return true; // Picker/Tab changes remain live through the fallback too.
    }
    if (id == RI_GREAT_SPIN_ATTACK) {
        return true;
    }
    switch (id) {
        case RI_JUNK:
        case RI_TRAP:
        case RI_TRIFORCE_PIECE:
        case RI_TRIFORCE_PIECE_PREVIOUS:
            return true;
        default:
            return false;
    }
}

// These concrete aliases have GID_NONE in MM. The owner already describes its
// selected standalone/Din weapon and native fallback; do not fall through
// to the empty MM row when the authored NEI mesh is unavailable or declined.
// This is export-only: MM's native NEI hook still returns false for legacy draws.
static int32_t MM_FillImportedSwordFallback(RandoItemId id, CwItemDrawInfo* out) {
    const char* name;
    switch (id) {
        case RI_OOT_MASTER_SWORD:
            name = "Master Sword";
            break;
        case RI_OOT_TRUE_MASTER_SWORD:
            name = "True Master Sword";
            break;
        case RI_OOT_BIGGORON_SWORD:
            name = "Biggoron's Sword";
            break;
        case RI_OOT_NEI_LANTERN:
            name = "Lantern";
            break;
        case RI_OOT_NEI_POKE_BALL:
            name = "Poké Ball";
            break;
        case RI_OOT_NEI_MARIO_MASK:
            name = "Mario Mask";
            break;
        default:
            return 0;
    }
    static Fn_GetItemDrawInfo describe = nullptr;
    if (!describe)
        describe = reinterpret_cast<Fn_GetItemDrawInfo>(Combo_ResolveSym("soh", "OOT_GetItemDrawInfo"));
    if (!describe)
        return CW_DRAW_NOT_READY;
    CwItemDrawInfo info{};
    const int32_t result = describe(name, &info);
    if (result != 1)
        return result == CW_DRAW_NOT_READY ? result : 0;
    if (info.dlistCount < 1 || info.dlistCount > CW_DRAW_MAX_DLISTS || info.opCount < 0 ||
        info.opCount > CW_DRAW_MAX_OPS)
        return 0;
    for (int i = 0; i < info.dlistCount; ++i) {
        const char* path = info.dlists[i];
        if (!path || std::strncmp(path, "__OTR__", 7))
            return 0;
        if (path[7] != '@')
            info.dlists[i] = NeiResource_Route(path);
        else if (std::strncmp(path, "__OTR__@oot:", 12) && std::strncmp(path, "__OTR__@mm:", 11))
            return 0;
        if (!info.dlists[i])
            return 0;
    }
    // The native MM true-tier drawer retains its gold blade independently of
    // the sacred-blue flame. A selected CUSTOM_GI carries its own palette.
    if (id == RI_OOT_TRUE_MASTER_SWORD && info.drawKind == CW_DRAW_KIND_MASTER_SWORD) {
        const uint8_t gold[4] = { 255, 215, 110, 255 };
        std::memcpy(info.primColorOpa, gold, 4);
    }
    const char* resolvedName = out->resolvedName;
    *out = info;
    out->resolvedName = resolvedName;
    return 1;
}

static bool MM_IsSwordAppearanceDependent(RandoItemId id) {
    switch (id) {
        case RI_SWORD_KOKIRI:
        case RI_SWORD_RAZOR:
        case RI_SWORD_GILDED:
        case RI_GREAT_FAIRY_SWORD:
        case RI_OOT_MASTER_SWORD:
        case RI_OOT_TRUE_MASTER_SWORD:
        case RI_OOT_BIGGORON_SWORD:
            return true;
        default:
            return false;
    }
}

static int32_t MM_FillElementalSpellDrawInfo(RandoItemId id, CwItemDrawInfo* out) {
    const int profile = NeiArrowGi_ProfileForDrawId(id, RI_OOT_DINS_FIRE, RI_OOT_FARORES_WIND, RI_OOT_NAYRUS_LOVE);
    if (!out || !profile)
        return 0;
    out->drawKind = CW_DRAW_KIND_MAGIC_SPELL;
    out->neiEffect = profile;
    out->opCount = 0;
    out->dlistCount = 3;
    out->xluStartIndex = 0;
    out->scale = 1.f;
    // Keep the established three-list recipe contract for both resolvers.
    // The procedural GI uses MM's editor palette; these assets belong to OoT.
    out->dlists[0] = "__OTR__@oot:objects/object_gi_goddess/gGiMagicSpellDiamondDL";
    out->dlists[1] = profile == 1   ? "__OTR__@oot:objects/object_gi_goddess/gGiDinsFireColorDL"
                     : profile == 2 ? "__OTR__@oot:objects/object_gi_goddess/gGiFaroresWindColorDL"
                                    : "__OTR__@oot:objects/object_gi_goddess/gGiNayrusLoveColorDL";
    out->dlists[2] = "__OTR__@oot:objects/object_gi_goddess/gGiMagicSpellOrbDL";
    return 1;
}

static int32_t MM_FillItemDrawInfo(RandoItemId id, CwItemDrawInfo* out) {
    if (id == RI_OOT_NEI_MAGIC_MIRROR)
        return CwDescribeMagicMirror(out);
    bool progressiveConverted = false;
    // ComboShip (#88): a progressive item's model is the tier the player is owed, not the static base
    // drawId (which is always tier 1 — every Progressive Sword drew a Kokiri Sword). Resolve it the way
    // MM's own drawer does. Runs before the helpers so Progressive Lullaby, which resolves to a song,
    // still reaches MM_FillSongDrawInfo.
    if (MM_IsProgressiveItem(id)) {
        RandoItemId resolved = Rando::ConvertItem(id);
        if (resolved != RI_UNKNOWN && resolved != id) {
            id = resolved;
            progressiveConverted = true;
        }
    }
    // ComboShip: junk/trap are indirections MM resolves at draw time (Rando::DrawItem), with no check
    // id here to resolve against. Generation now bakes cross-placed junk, so the junk arm is only
    // reached by an older seed or a plando row that still names the placeholder.
    if (id == RI_JUNK) {
        id = Rando::CurrentJunkItem();
    } else if (id == RI_TRAP) {
        id = Rando::CurrentTrapItem();
    }
    // Name follows the same id the model used (so a maxed progressive that fell to junk names the junk item).
    if (progressiveConverted) {
        auto nameIt = Rando::StaticData::Items.find(id);
        if (nameIt != Rando::StaticData::Items.end()) {
            out->resolvedName = nameIt->second.name;
        }
    }
    auto it = Rando::StaticData::Items.find(id);
    if (it == Rando::StaticData::Items.end()) {
        return 0;
    }
    CwItemDrawInfo nei{};
    if (MM_FillElementalSpellDrawInfo(id, out))
        return 1;
    if (MM_DescribeNeiGi(id, &nei)) {
        const char* resolvedName = out->resolvedName;
        *out = nei;
        out->resolvedName = resolvedName;
        return 1;
    }
    out->neiShimmer = nei.neiShimmer;
    out->itemShimmer = nei.itemShimmer;
    if (nei.neiShimmer)
        out->stateDependent = 2;
    const int32_t swordFallback = MM_FillImportedSwordFallback(id, out);
    if (swordFallback != 0)
        return swordFallback;
    // These two imported bottles have GID_NONE in MM: describe their OoT
    // resources before consulting MM's native draw table.
    if (id == RI_OOT_BOTTLE_BLUE_FIRE || id == RI_OOT_RUTOS_LETTER) {
        out->dlistCount = 2;
        out->xluStartIndex = 1;
        if (id == RI_OOT_BOTTLE_BLUE_FIRE) {
            out->drawKind = CW_DRAW_KIND_BLUE_FIRE;
            out->dlists[0] = "__OTR__@oot:objects/object_gi_fire/gGiBlueFireChamberstickDL";
            out->dlists[1] = "__OTR__@oot:objects/object_gi_fire/gGiBlueFireFlameDL";
        } else {
            out->drawKind = CW_DRAW_KIND_SIMPLE;
            out->dlists[0] = "__OTR__@oot:objects/object_gi_bottle_letter/gGiLetterBottleContentsDL";
            out->dlists[1] = "__OTR__@oot:objects/object_gi_bottle_letter/gGiLetterBottleDL";
        }
        out->itemShimmer = MM_OotBottleShimmerColor(id, out->itemShimmerColor);
        return 1;
    }
    if (MM_FillDungeonKeyModelInfo(id, (s16)it->second.drawId, out) ||
        MM_FillDungeonTintInfo(id, (s16)it->second.drawId, out)) {
        return 1;
    }
    if (id == RI_OOT_NEI_ROD_OF_SEASONS || (id >= RI_OOT_NEI_SEASON_SPRING && id <= RI_OOT_NEI_SEASON_WINTER)) {
        out->drawKind = CW_DRAW_KIND_SEASON_GI;
        out->xluStartIndex = -1;
        out->neiEffect = id == RI_OOT_NEI_ROD_OF_SEASONS ? 5 : 1 + id - RI_OOT_NEI_SEASON_SPRING;
        if (id == RI_OOT_NEI_ROD_OF_SEASONS) {
            out->dlistCount = 1;
            out->scale = .35f;
            out->dlists[0] = "__OTR__@oot:objects/object_nei_rod_of_seasons/gNeiRodOfSeasonsDL";
        } else {
            out->dlistCount = 0;
        }
        return 1;
    }
    if (id == RI_GREAT_SPIN_ATTACK) {
        return MM_FillSpinAttackGi(out);
    }
    if (MM_FillSongDrawInfo(id, out)) {
        return 1; // songs: tinted note, no table row
    }
    if (MM_FillOpsDrawInfo(id, out)) {
        return 1; // clock / owl / skeleton key / double defense / tycoon wallet
    }
    if (MM_FillSimpleDrawInfo(id, out)) {
        return 1; // flippers / ice trap / ocarina buttons / triforce shards
    }
    if (MM_FillEnemySoulDrawInfo(id, out)) {
        return 1; // enemy souls: colored soul flame, no table row
    }
    if (MM_FillGidAliasDrawInfo(id, out)) {
        return 1; // boss souls / minifrogs: stand-in table row
    }
    void* dls[CW_DRAW_MAX_DLISTS] = {};
    int32_t xluStart = -1;
    f32 scale = 0.0f;
    s32 xluSeg8TexScroll = 0;
    s32 drawKind = CW_DRAW_KIND_SIMPLE;
    // Recipe classification follows MM's registered resources and Alt state
    // while this export is invoked from OoT's graph thread.
    const auto drawOwner = Ship::CrossRMRegistry::Get("mm");
    if (!drawOwner)
        return CW_DRAW_NOT_READY;
    Ship::ResourceManagerScope drawOwnerScope(drawOwner);
    int32_t n = GetItem_GetDrawTableEntry((s32)it->second.drawId, dls, CW_DRAW_MAX_DLISTS, &xluStart, &scale,
                                          &xluSeg8TexScroll, &drawKind);
    if (n <= 0) {
        return 0;
    }
    out->dlistCount = n;
    out->xluStartIndex = xluStart;
    out->scale = scale;
    out->hasEnvColor = 0;
    out->xluSeg8TexScroll = xluSeg8TexScroll;
    out->drawKind = drawKind;
    if (drawKind == CW_DRAW_KIND_ELEMENTAL_ARROW)
        out->neiEffect = NeiArrowGi_ProfileForDrawId(id, RI_ARROW_FIRE, RI_ARROW_ICE, RI_ARROW_LIGHT);
    if (drawKind == CW_DRAW_KIND_MM_FAIRY_BOTTLE || drawKind == CW_DRAW_KIND_MM_FAIRY_CONTAINER ||
        id == RI_BOTTLE_EMPTY)
        out->stateDependent = 2; // selected shell follows live MM owner Alt/mod state
    for (int32_t i = 0; i < n; i++) {
        out->dlists[i] = (const char*)dls[i];
    }
    // Rows drawn under a setup other than 25 (bombchu = 23 Opa, compass glass = 5 Xlu): carry it so
    // the consumer submits the same GPU state, not its own 25.
    void* setupOpa = nullptr;
    void* setupXlu = nullptr;
    GetItem_GetDrawSetupDLs((s32)it->second.drawId, &setupOpa, &setupXlu);
    out->setupDlOpa = setupOpa;
    out->setupDlXlu = setupXlu;
    const int32_t nativeShimmer = GetItem_GetShimmerColor((s16)it->second.drawId, out->itemShimmerColor);
    if (nativeShimmer)
        out->itemShimmer = nativeShimmer;
    if (ComboCollectibleShimmer_ProfileForDrawId((s16)it->second.drawId) >= 0)
        out->stateDependent = 2; // Keep opt-in and owning-game color/rainbow live, including while disabled.
    if (it->second.drawId == GID_CHATEAU) {
        out->neiShimmer = static_cast<int32_t>(NeiGi::Kind::ChateauRomani) + 1;
    }
    if (MM_OotBottleShimmerColor(id, out->itemShimmerColor)) {
        out->itemShimmer = 1;
    }
    // ComboShip: some MM item bodies sample an animated segment-8 material their draw func binds via
    // AnimatedMat_Draw (Moon's Tear, fairy bottle). z_draw.c can't carry that across, so report the
    // texanim resource for the consumer to replicate (ComboForeignTexAnim_Run). The fairy kind is
    // stable even when its shell is supplied by the selected generic-bottle mod.
    if (n >= 1 && dls[0] != NULL && strcmp((const char*)dls[0], gGiMoonsTearItemDL) == 0) {
        out->matAnimPath = gGiMoonsTearTexAnim; // MM's own path; consumer loads via CrossRMRegistry("mm")
        out->matAnimBindOpa = 1;                // the tear body (OPA) samples the animated segment
        out->matAnimBillboard = 1;              // the glow (XLU) billboards toward the camera
    } else if (drawKind == CW_DRAW_KIND_MM_FAIRY_CONTAINER) {
        out->matAnimPath = gGiFairyBottleTexAnim; // GetItem_DrawFairyContainer's AnimatedMat_Draw
        out->matAnimBindOpa = 1;
    }
    if (id == RI_SHIELD_MIRROR) {
        out->stateDependent = 2; // The selected owner's Alt geometry can change its pose.
        float tilt = 0.f, roll = 0.f;
        if (drawKind == CW_DRAW_KIND_SIMPLE && n == 2 && xluStart == 1 &&
            ResourceMgr_GetIkanaShieldGiPoseForGame("mm", "objects/object_gi_shield_3/gGiMirrorShieldDL", &tilt,
                                                    &roll) &&
            (tilt != 0.f || roll != 0.f)) {
            // Preserve the native split passes and caller spin. CUSTOM_GI
            // would add another spin; OPS carries only this pose correction.
            out->drawKind = CW_DRAW_KIND_OPS;
            out->opCount = 0;
            MM_Op(out, CW_OP_PUSH);
            if (scale > 0.f)
                MM_OpV(out, CW_OP_SCALE, scale, scale, scale);
            if (roll != 0.f)
                MM_OpV(out, CW_OP_ROTATE_Z, roll * (32768.f / 3.14159265358979323846f), 0.f, 0.f);
            if (tilt != 0.f)
                MM_OpV(out, CW_OP_ROTATE_X, tilt * (32768.f / 3.14159265358979323846f), 0.f, 0.f);
            MM_Op(out, CW_OP_SETUP_OPA);
            MM_Op(out, CW_OP_LOAD_MATRIX);
            MM_OpV(out, CW_OP_DLIST, 0.f, 0.f, 0.f);
            MM_Op(out, CW_OP_SETUP_XLU);
            MM_Op(out, CW_OP_LOAD_MATRIX);
            MM_OpV(out, CW_OP_DLIST, 1.f, 0.f, 0.f);
            MM_Op(out, CW_OP_POP);
        }
    }
    return 1;
}

// Whole body inside the try: we run on OOT's graph thread while MM is dormant, and an unwind across
// the C ABI into soh.dll is unrecoverable.
extern "C" COMBO_EXPORT int32_t MM_GetItemDrawInfo(const char* itemName, CwItemDrawInfo* out) {
    ItemGrantAudit::Scope itemGrantAuditScope("MM preview-DrawInfo", -1, -1, true);
    try {
        if (itemName == nullptr || out == nullptr) {
            return 0;
        }
        RandoItemId id = Rando::StaticData::GetItemIdFromDisplayName(itemName);
        if (id == RI_UNKNOWN) {
            id = Rando::StaticData::GetItemIdFromName(itemName); // sentinel / raw RI_
        }
        if (id == RI_UNKNOWN || MM_HasAnimDraw(id)) {
            return 0; // the animated ABI serves the skeletal class (enemy souls, minifrogs)
        }
        *out = CwItemDrawInfo{};
        const int32_t result = MM_FillItemDrawInfo(id, out);
        if (result != 1)
            return result;
        if (MM_IsProgressiveItem(id)) {
            out->stateDependent = 1; // Freeze the awarded tier before its concrete appearance refreshes.
        } else if (out->stateDependent == 2 || DungeonItem_GetOwner(id) >= 0 || id == RI_GREAT_SPIN_ATTACK ||
                   id == RI_RED_POTION_REFILL || id == RI_GREEN_POTION_REFILL || id == RI_BLUE_POTION_REFILL ||
                   id == RI_OOT_BOTTLE_GREEN_POTION || id == RI_OOT_BOTTLE_BLUE_POTION ||
                   MM_IsSwordAppearanceDependent(id)) {
            out->stateDependent = 2;
        } else {
            out->stateDependent = MM_IsStateDependentDraw(id) ? 1 : 0;
        }
        return 1;
    } catch (...) { return 0; }
}

// Animated variant. Items in the animated class (currently the stray fairies, drawn by
// Rando/DrawItem.cpp:DrawStrayFairy in MM) have no static DL row, so the static export returns 0
// and OOT falls through to this one. MM only DESCRIBES the item — resource paths + DrawStrayFairy
// parameters — and the host's combo-owned ComboForeignAnim.h does the loading and drawing.
// Returns 0 for items outside the animated class.
// Whole body inside the try: an unwind across the C ABI into soh.dll is unrecoverable.
extern "C" COMBO_EXPORT int32_t MM_GetItemAnimDrawInfo(const char* itemName, CwItemAnimDrawInfo* out) {
    ItemGrantAudit::Scope itemGrantAuditScope("MM preview-AnimDrawInfo", -1, -1, true);
    try {
        if (itemName == nullptr || out == nullptr) {
            return 0;
        }
        const char* texAnim;
        // ComboShip: itemName is the friendly combo-spoiler name; fall back to the RI_ spoilerName.
        RandoItemId animId = Rando::StaticData::GetItemIdFromDisplayName(itemName);
        if (animId == RI_UNKNOWN) {
            animId = Rando::StaticData::GetItemIdFromName(itemName);
        }
        *out = CwItemAnimDrawInfo{};
        if (MM_FillAnimDrawInfo(animId, out)) { // the OPA/skeletal class (enemy souls, minifrogs)
            return 1;
        }
        switch (animId) {
            case RI_WOODFALL_STRAY_FAIRY:
                texAnim = gStrayFairyWoodfallTexAnim;
                break;
            case RI_SNOWHEAD_STRAY_FAIRY:
                texAnim = gStrayFairySnowheadTexAnim;
                break;
            case RI_GREAT_BAY_STRAY_FAIRY:
                texAnim = gStrayFairyGreatBayTexAnim;
                break;
            case RI_STONE_TOWER_STRAY_FAIRY:
                texAnim = gStrayFairyStoneTowerTexAnim;
                break;
            case RI_CLOCK_TOWN_STRAY_FAIRY:
                texAnim = gStrayFairyClockTownTexAnim;
                break;
            default:
                return 0; // not in the animated class
        }
        out->skelPath = gStrayFairySkel;
        out->animPath = gStrayFairyFlyingAnim;
        out->texAnimPath = texAnim;
        out->scale = 0.03f;
        out->billboard = 1;
        out->xlu = 1;
        out->limbCount = STRAY_FAIRY_LIMB_MAX;
        out->hiddenLimb = STRAY_FAIRY_LIMB_RIGHT_FACING_HEAD;
        return 1;
    } catch (...) { return 0; }
}

#endif // COMBO_ITEM_DRAW_MM_H
