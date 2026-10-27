/*
 * File: z_kaleido_equipment.c
 * Overlay: ovl_kaleido_scope
 * Description: Pause Menu - Equipment Page (Skijer's NEI port, 1:1 with soh's
 *              z_kaleido_equipment.c structure adapted to MM's kaleido).
 *
 * REPLACES the vanilla Mask page (the 24 MM masks live on page 2 of the extended item kaleido).
 *
 * OoT layout on MM's mask-page grid (maskVtx, 6 cols x 4 rows). The equipment grid takes the
 * RIGHT FOUR columns (2..5) — exactly OoT's 4x4 equipment cursor space:
 *   col 0 = passive column (Magic Cape / Pendant of Memories; lower two sockets empty)
 *   cols 1..3 = equipment (row 0 swords, row 1 shields, row 2 tunics, row 3 boots)
 * The Link doll fills the LEFT area (player's LIVE skeleton rendered into an isolated
 * framebuffer — gfx_create_framebuffer + gsSPSetFB/gsSPResetFB, the SoH pause-Link technique).
 *
 * Three sub-pages cycled with L (SoH freed-shoulder-button logic; the NES.c page-toggle
 * handler is patched to leave L to us on this page):
 *   0 "vanilla":   MM swords (Kokiri/Razor/Gilded) + MM shields (Hero/Mirror) functional;
 *                  OoT tunics/boots display-only (behavior per-item later).
 *   1 "extended":  the full NEI ExtEquip grid (3 swords / 3 shields / 3 tunics / 3 boots).
 *                  A equips on the body, C-left/down/right assigns to that C button.
 *   2 "transform": Crossover-Items form selector (Link / Mario / Pikachu).
 */

#include "z_kaleido_scope.h"
#include "sword_element.h"
#include "sword_element_kaleido.h"
#include "interface/parameter_static/parameter_static.h"
#include "archives/icon_item_static/icon_item_static_yar.h"
#include "2s2h/GameInteractor/GameInteractor.h"
#include "2s2h/CustomMessage/PauseItemDescriptions.h" // NEI: C-Up descriptions for ext equipment
#include "2s2h/BenGui/CosmeticEditor.h"               // PlayerTunic_BindLocalColor (per-player tunic tint)
#include "2s2h/FleetShipCombo/FleetComboIds.h"        // FC_SHIELD_* / FC_OOT_TUNIC/BOOTS ownership bits
#include "2s2h/FleetShipCombo/FleetComboItems.h"      // FCI_KOKIRI_SWORD acquisition receipt
#include "2s2h/FleetShipCombo/FleetShipCombo.h"       // FleetShipCombo_GetActiveGame (combo ownership gate)
#include "2s2h/Rando/NeiResourceRouting.h"
#include "../../../../../combo/menu/ComboItemIconOwnership.h"
#include <libultraship/bridge/consolevariablebridge.h>

#include "mods/extended_equipment.h"        // ExtEquip_* (Skijer's NEI)
#include "mods/broken_items/broken_items.h" // BrokenItems_* form selector (Skijer's NEI)
#include "mods/nei_save.h"
#include "expansions/sm64/sm64_mario.h"

// MM natively has SWORD=0/SHIELD=1/TUNIC=2; BOOTS is the NEI accessory tag (3) — same value
// as mods/nei_oot_compat.h (that header is z_player-TU-only, so define it here too).
#ifndef EQUIP_TYPE_BOOTS
#define EQUIP_TYPE_BOOTS 3
#endif

void KaleidoScope_SetCursorVtxPos(PauseContext* pauseCtx, u16 vtxIndex, Vtx* vtx);
void KaleidoScope_MoveCursorToSpecialPos(PlayState* play, s16 cursorSpecialPos);
void KaleidoScope_MoveCursorFromSpecialPos(PlayState* play);
void KaleidoScope_DrawTexQuadRGBA32(GraphicsContext* gfxCtx, TexturePtr texture, u16 width, u16 height, u16 point);
Gfx* Gfx_DrawTexQuadIA8(Gfx* gfx, TexturePtr texture, s16 textureWidth, s16 textureHeight, u16 point);
void Interface_LoadItemIconImpl(PlayState* play, u8 btn);
void* ExtInv_GetItemIcon(u16 itemId);
void Player_DrawImpl(PlayState* play, void** skeleton, Vec3s* jointTable, s32 dListCount, s32 lod,
                     PlayerTransformation playerForm, s32 boots, s32 face, OverrideLimbDrawFlex overrideLimbDraw,
                     PostLimbDrawFlex postLimbDraw, Actor* actor);
s32 Player_OverrideLimbDrawGameplayDefault(PlayState* play, s32 limbIndex, Gfx** dList, Vec3f* pos, Vec3s* rot,
                                           Actor* actor);
extern Gfx gCullBackDList[]; // segment 0x0C cull DL the player limb DLs reference (Player_DrawGameplay)
int gfx_create_framebuffer(uint32_t width, uint32_t height, uint32_t native_width, uint32_t native_height,
                           unsigned char upscale);
// NEI weapon-upgrade bits (mods/items/logic/weapon_upgrades.c): Kokiri->Razor->Gilded chain +
// the Biggoron->Great-Fairy longsword upgrade (drives the col-3 line icon/equipped state).
u8 WeaponUpgrade_KokiriLevel(void);
u8 WeaponUpgrade_HasRazor(void);
u8 WeaponUpgrade_HasGilded(void);
u8 WeaponUpgrade_HasGreatFairy(void);

// ---------------------------------------------------------------------------
// Doll framebuffer (OoT pause-Link dimensions)
// ---------------------------------------------------------------------------
#define EQUIP_DOLL_WIDTH 64 // on-screen size (OoT pause-Link dimensions)
#define EQUIP_DOLL_HEIGHT 112
#define EQUIP_DOLL_RES 3 // framebuffer supersampling factor (renders at 3x, shown at 1x)
#define EQUIP_DOLL_FBW (EQUIP_DOLL_WIDTH * EQUIP_DOLL_RES)
#define EQUIP_DOLL_FBH (EQUIP_DOLL_HEIGHT * EQUIP_DOLL_RES)

int gPauseLinkFrameBuffer = -1;

// ---------------------------------------------------------------------------
// Sub-page + cursor state (SoH: vanilla / ExtEquip page / Crossover-Items transform)
// ---------------------------------------------------------------------------
#define EQUIP_SUBPAGE_VANILLA 0
#define EQUIP_SUBPAGE_EXT 1
#define EQUIP_SUBPAGE_TRANSFORM 2

static s16 sEquipSubPage = EQUIP_SUBPAGE_VANILLA;
static s16 sEquipCursorX = 1; // 0 = upgrades column, 1..3 = equipment — OoT cursorX space
static s16 sEquipCursorY = 0; // 0..3 — OoT cursorY space
static s16 sTransformCursor = 0;

// The 4x4 grid lives on the mask page's own vertices: equipment cols 1..3 on maskVtx columns 3..5,
// and the PASSIVE column (col 0: Cape/Pendant toggles) on maskVtx column 0 — the
// free carved blocks at the far LEFT of the page (columns 1-2 sit under the Link doll panel).
// Keeping stock grid cells means the shared cursor tables (sItemMaskCursorsX/Y, indexed by
// cursorXIndex/cursorYIndex) position the drawn cursor for free.
#define EQUIP_GRID_COL_BASE 2
#define EQUIP_MASK_COL(col) ((col) == 0 ? 0 : ((col) + EQUIP_GRID_COL_BASE))
#define EQUIP_CELL(row, col) (((row)*MASK_GRID_COLS) + EQUIP_MASK_COL(col))

// Upgrade column (col 0) OWN vertices — OoT places its upgrade column at page x = -114 (the carved
// sockets on the page's left edge, z_kaleido_scope_PAL.c D_8082B12C), further left than maskVtx
// column 0 (-96). maskVtx can't be moved (the real mask page shares it), so the upgrade cells get
// their own 4 quads, built each draw with the mask-grid row formula. The drawn cursor on this page
// comes from sItemMaskCursorsX[cursorXIndex] (shared table, col-0 entry = -62 which pairs with
// maskVtx -96); for our shifted column the cursor X is overridden in the draw with the value below
// (cursor-space ≈ 0.8125 * cellCenterX + 3, from the col-0/col-3 table anchor points).
#define EQUIP_UPGRADE_COL_X (-112)      // quad x0, page space (OoT -114 + 2 margin)
#define EQUIP_UPGRADE_CURSOR_X (-76.5f) // cursor-space X centered on that column
static Vtx sUpgradeColVtx[4 * 4];

// Called from KaleidoScope_UpdateCursorSize (z_kaleido_scope_NES.c) AFTER it reads the shared
// sItemMaskCursorsX table and right before KaleidoScope_DrawCursor: when the equipment cursor sits
// on the shifted upgrade column, hand back its custom cursor X. Returns true when the override
// applies. (The table can't hold this column — it's shared with the real mask/item pages.)
s32 KaleidoEquip_GetUpgradeCursorX(f32* outX) {
    if ((sEquipSubPage != EQUIP_SUBPAGE_TRANSFORM) && (sEquipCursorX == 0)) {
        *outX = EQUIP_UPGRADE_CURSOR_X;
        return true;
    }
    return false;
}

static void KaleidoEquip_BuildUpgradeColVtx(PauseContext* pauseCtx) {
    s16 row;

    for (row = 0; row < 4; row++) {
        Vtx* v = &sUpgradeColVtx[row * 4];
        s16 y0 = (MASK_GRID_ROWS * MASK_GRID_CELL_HEIGHT) / 2 - 6 - (row * MASK_GRID_CELL_HEIGHT) + pauseCtx->offsetY -
                 MASK_GRID_QUAD_MARGIN;

        v[0].v.ob[0] = v[2].v.ob[0] = EQUIP_UPGRADE_COL_X;
        v[1].v.ob[0] = v[3].v.ob[0] = EQUIP_UPGRADE_COL_X + MASK_GRID_QUAD_WIDTH;
        v[0].v.ob[1] = v[1].v.ob[1] = y0;
        v[2].v.ob[1] = v[3].v.ob[1] = y0 - MASK_GRID_QUAD_HEIGHT;
        v[0].v.ob[2] = v[1].v.ob[2] = v[2].v.ob[2] = v[3].v.ob[2] = 0;
        v[0].v.flag = v[1].v.flag = v[2].v.flag = v[3].v.flag = 0;
        v[0].v.tc[0] = v[0].v.tc[1] = v[1].v.tc[1] = v[2].v.tc[0] = 0;
        v[1].v.tc[0] = v[2].v.tc[1] = v[3].v.tc[0] = v[3].v.tc[1] = MASK_GRID_QUAD_TEX_SIZE * (1 << 5);
        v[0].v.cn[0] = v[1].v.cn[0] = v[2].v.cn[0] = v[3].v.cn[0] = 255;
        v[0].v.cn[1] = v[1].v.cn[1] = v[2].v.cn[1] = v[3].v.cn[1] = 255;
        v[0].v.cn[2] = v[1].v.cn[2] = v[2].v.cn[2] = v[3].v.cn[2] = 255;
        v[0].v.cn[3] = v[1].v.cn[3] = v[2].v.cn[3] = v[3].v.cn[3] = 255;
    }
}

// entry: itemId shown for icon/name, or -1 = empty cell.
typedef struct {
    s16 item;            // icon/name item id (-1 = empty; extended names use ExtEquip_GetNameTex)
    s16 equipType;       // >=0 ExtEquip type; -1 empty; -2 upgrade; -3 sword; -4 shield; -5 display-only
    s16 index;           // equip value / ext index (1-based) / upgrade row
    const char* ootIcon; // OoT icon OTR path (used when the OoT archive is present in mods/)
} EquipCell;

#define CELL_EMPTY (-1)
#define CELL_UPGRADE (-2)
#define CELL_SWORD (-3) // Kokiri progressive line (Kokiri -> Razor -> Gilded, MM native)
#define CELL_SHIELD (-4)
#define CELL_DISPLAY (-5)
#define CELL_SWORD_MASTER (-6)       // Master line (Master -> True Master; own IA, OoT blade, full-HP beam)
#define CELL_SWORD_BGS (-7)          // Longsword line (Biggoron -> Great Fairy's Sword; two-handed IA)
#define CELL_TUNIC (-8)              // OoT tunic (Kokiri/Goron/Zora): Nei_Save()->vanillaTunic, index 0..2
#define CELL_BOOTS (-9)              // OoT boots (Kokiri/Iron/Hover): Nei_Save()->vanillaBoots, index 0..2
#define CELL_SHIELD_DEKU (-10)       // Deku shield: equips AS Hero (index 1) but sets the Deku skin flag
#define CELL_SHIELD_OOT_MIRROR (-11) // OoT Mirror: equips AS Mirror (2) + skin flag 2 (draws OoT mirror)

// OoT icon paths for the sword lines (fall back to the MM item icon when oot.o2r is absent).
#define OOT_ICON_SWORD_MASTER "__OTR__textures/icon_item_static/gItemIconSwordMasterTex"
#define OOT_ICON_SWORD_BIGGORON "__OTR__textures/icon_item_static/gItemIconSwordBiggoronTex"
#define OOT_ICON_SHIELD_DEKU "__OTR__textures/icon_item_static/gItemIconShieldDekuTex"
#define OOT_ICON_SHIELD_HYLIAN "__OTR__textures/icon_item_static/gItemIconShieldHylianTex"
#define OOT_ICON_SHIELD_MIRROR "__OTR__textures/icon_item_static/gItemIconShieldMirrorTex"

// Prefer MM-local replacements, then the registered OoT owner. Deferred texture
// paths retain HD metadata and the selected owner's independent Alt mode.
static void* KaleidoEquip_OotTex(const char* path) {
    extern u8 ResourceMgr_FileExists(const char* resName);
    extern u8 ResourceMgr_FileAltExists(const char* resName);
    extern bool ResourceMgr_IsAltAssetsEnabled(void);
    extern u8 MM_IsRgba32TextureAvailable(const char* filePath);
    if (path == NULL) {
        return NULL;
    }
    // This native MM icon must not consult the OoT donor or the cold
    // filename cache used for optional imported equipment textures.
    if (ComboIconUsesMmOwnership(path)) {
        const char* icon = COMBO_IKANA_SHIELD_ICON;
        // Missing textures retain the previous cell's TMEM. Keep the private
        // route only when its selected owner supplies a usable RGBA32 tile.
        return MM_IsRgba32TextureAvailable(icon) ? (void*)icon : NULL;
    }
    if (ResourceMgr_FileExists(path) || (ResourceMgr_IsAltAssetsEnabled() && ResourceMgr_FileAltExists(path))) {
        return (void*)path;
    }
    return NeiResource_Available(path) ? (void*)NeiResource_Route(path) : NULL;
}

// ---------------------------------------------------------------------------
// Left column (col 0) — Skijer 2026-07-29 (kaleido re-layout):
//   row 0 = MAGIC CAPE          (A opens Show/Hide cloth choices; its half-cost passive is always on)
//   row 1 = PENDANT OF MEMORIES (A toggles its whole moveset)
//   rows 2/3 = empty. Strength/swim upgrades live on the quest page; these sockets have no items.
// "Value" here = the cell is populated (drives landability + draw).
// ---------------------------------------------------------------------------
// Page-2 grid rows. The BOOTS row is NOT C-assignable: all three are real boots (equipped with A),
// and its middle id (0xEA) belongs to the Pendant of Memories in the inventory/C-button id space.
#define EQUIP_EXT_ROW_BOOTS 3

static s16 KaleidoEquip_UpgradeValue(s16 row) {
    switch (row) {
        case 0:
            return ExtEquip_CapeOwned();
        case 1:
            return ExtEquip_PendantOwned();
    }
    return 0;
}

static void* KaleidoEquip_UpgradeIcon(s16 row, s16 value) {
    if (value <= 0) {
        return NULL;
    }
    switch (row) {
        // Rows 0/1 = Magic Cape / Pendant of Memories (cape from the NEI icon set, pendant from mm.o2r).
        case 0:
            return ExtEquip_GetCapeIcon();
        case 1:
            return ExtEquip_GetPendantIcon();
    }
    return NULL;
}

// Item id whose name labels the hovered passive. The name resolver uses the non-grid context.
static s16 KaleidoEquip_UpgradeNameItem(s16 row, s16 value) {
    if (value <= 0) {
        return -1;
    }
    if (row == 0) {
        return ITEM_EXT_TUNIC_1;
    }
    if (row == 1) {
        return ITEM_EXT_BOOTS_2;
    }
    return -1;
}

// Vanilla OoT equipment reuses MM/ext ids for fallback icons. Select its title by cell so a
// Kokiri Tunic cannot inherit the Cape/Champion label, nor a Deku Shield the Hero's Shield label.
void* KaleidoEquip_GetNameTex(void) {
    static const char* sOotNames[4][3] = {
        { NULL, "__OTR__textures/item_name_static/gMasterSwordItemNameENGTex",
          "__OTR__textures/item_name_static/gBiggoronsSwordItemNameENGTex" },
        { "__OTR__textures/item_name_static/gDekuShieldItemNameENGTex",
          "__OTR__textures/item_name_static/gHylianShieldItemNameENGTex",
          "__OTR__textures/item_name_static/gMirrorShieldItemNameENGTex" },
        { "__OTR__textures/item_name_static/gKokiriTunicItemNameENGTex",
          "__OTR__textures/item_name_static/gGoronTunicItemNameENGTex",
          "__OTR__textures/item_name_static/gZoraTunicItemNameENGTex" },
        { "__OTR__textures/item_name_static/gKokiriBootsItemNameENGTex",
          "__OTR__textures/item_name_static/gIronBootsItemNameENGTex",
          "__OTR__textures/item_name_static/gHoverBootsItemNameENGTex" },
    };
    if (sEquipSubPage != EQUIP_SUBPAGE_VANILLA || sEquipCursorX < 1 || sEquipCursorX > 3 || sEquipCursorY < 0 ||
        sEquipCursorY > 3) {
        return NULL;
    }
    if (sEquipCursorY == 0 && sEquipCursorX == 3 && WeaponUpgrade_HasGreatFairy()) {
        return NULL; // Great Fairy's Sword uses the native MM name, like the Kokiri line.
    }
    const char* path = sOotNames[sEquipCursorY][sEquipCursorX - 1];
    if (path == NULL) {
        return NULL;
    }
    void* texture = KaleidoEquip_OotTex(path);
    return texture != NULL ? texture : (void*)"__OTR__textures/virtual/gEmptyTexture";
}

// ---------------------------------------------------------------------------
// Cell tables (cols 1..3 of the grid)
// ---------------------------------------------------------------------------
static void KaleidoEquip_GetCell(s16 subPage, s16 row, s16 col, EquipCell* out) {
    // OoT tunic/boot icon set for the display-only rows (loads from the player's OoT archive)
    static const char* sOotIcons[2][3] = {
        { "__OTR__textures/icon_item_static/gItemIconTunicKokiriTex",
          "__OTR__textures/icon_item_static/gItemIconTunicGoronTex",
          "__OTR__textures/icon_item_static/gItemIconTunicZoraTex" },
        { "__OTR__textures/icon_item_static/gItemIconBootsKokiriTex",
          "__OTR__textures/icon_item_static/gItemIconBootsIronTex",
          "__OTR__textures/icon_item_static/gItemIconBootsHoverTex" },
    };

    out->item = -1;
    out->equipType = CELL_EMPTY;
    out->index = col;
    out->ootIcon = NULL;

    if (col < 1 || col > 3 || row < 0 || row > 3) {
        return;
    }

    if (subPage == EQUIP_SUBPAGE_VANILLA) {
        if (row == 0) {
            // OoT-style 3 progressive sword LINES (one column each):
            //   col1 = Kokiri line   (Kokiri -> Razor -> Gilded; MM's native progressive sword)
            //   col2 = Master line   (Master -> True Master; own IA, OoT blade, full-HP beam)
            //   col3 = Longsword line (Biggoron -> Great Fairy's Sword; MM two-handed IA)
            out->index = col;
            if (col == 1) {
                out->equipType = CELL_SWORD;
                out->item = ITEM_SWORD_KOKIRI + WeaponUpgrade_KokiriLevel(); // icon = current tier
            } else if (col == 2) {
                out->equipType = CELL_SWORD_MASTER;
                out->item = ITEM_SWORD_MASTER;
                out->ootIcon = OOT_ICON_SWORD_MASTER;
            } else { // col == 3
                out->equipType = CELL_SWORD_BGS;
                // GFS icon once the Great Fairy upgrade is owned, else the Biggoron icon.
                out->item = WeaponUpgrade_HasGreatFairy() ? ITEM_SWORD_GREAT_FAIRY : ITEM_SWORD_BGS;
                out->ootIcon = WeaponUpgrade_HasGreatFairy() ? NULL : OOT_ICON_SWORD_BIGGORON;
            }
        } else if (row == 1) {
            // OoT shields: Deku (OoT DL, smaller, Hero IA) / Hero=Hylian / Mirror (MM mirror DL)
            if (col == 1) {
                out->equipType = CELL_SHIELD_DEKU;
                out->index = EQUIP_VALUE_SHIELD_HERO; // Deku equips AS Hero + the Deku skin flag
                out->item = ITEM_SHIELD_HERO;
                out->ootIcon = OOT_ICON_SHIELD_DEKU;
            } else if (col == 2) {
                out->equipType = CELL_SHIELD;
                out->index = EQUIP_VALUE_SHIELD_HERO;
                out->item = ITEM_SHIELD_HERO;
                out->ootIcon = OOT_ICON_SHIELD_HYLIAN;
            } else { // col == 3 — OoT Mirror shield (distinct from the MM Mirror = Shield of Ikana)
                out->equipType = CELL_SHIELD_OOT_MIRROR;
                out->index = EQUIP_VALUE_SHIELD_MIRROR;
                out->item = ITEM_SHIELD_MIRROR;
                out->ootIcon = OOT_ICON_SHIELD_MIRROR;
            }
        } else if (row == 2) {
            // OoT tunics (functional): Kokiri / Goron (fireproof) / Zora (gas-immune).
            out->equipType = CELL_TUNIC;
            out->index = col - 1;                     // 0=Kokiri, 1=Goron, 2=Zora
            out->item = ITEM_EXT_TUNIC_1 + (col - 1); // fallback icon when the OoT archive is absent
            out->ootIcon = sOotIcons[0][col - 1];
        } else if (row == 3) {
            // OoT boots (functional): Kokiri / Iron (anti-knockback) / Hover (air-float).
            out->equipType = CELL_BOOTS;
            out->index = col - 1; // 0=Kokiri, 1=Iron, 2=Hover
            out->item = ITEM_EXT_BOOTS_1 + (col - 1);
            out->ootIcon = sOotIcons[1][col - 1];
        }
    } else if (subPage == EQUIP_SUBPAGE_EXT) {
        // The FULL extended-equipment set, 1:1 with the OoT build:
        // rows = sword/shield/tunic/boots, cols = ext index 1..3.
        static const s16 sExtRowType[4] = { EQUIP_TYPE_SWORD, EQUIP_TYPE_SHIELD, EQUIP_TYPE_TUNIC, EQUIP_TYPE_BOOTS };
        // Skijer 2026-07-16: retired slots stay EMPTY (Cape/Pendant live on the upgrade column;
        // Water Dragon Scale deleted — Zora swim = Zora Tunic effect). Reserved for new boots.
        if (ExtEquip_SlotRetired(sExtRowType[row], (u8)col)) {
            return;
        }
        out->equipType = sExtRowType[row];
        out->index = col;
        out->item = (s16)ExtEquip_GetItemId(out->equipType, (u8)col);
    }
}

static u8 KaleidoEquip_UsesLegacyFreeEquipment(void) {
#ifdef COMBO_BUILD
    return false; // Solo must not unlock unearned gear when the cross-game bridge is inactive.
#else
    return FleetShipCombo_GetActiveGame() < 0;
#endif
}

static u8 KaleidoEquip_CellOwned(EquipCell* cell) {
    switch (cell->equipType) {
        case CELL_EMPTY:
            return false;
        // Native solo and combined sessions both use the saved ownership records.
        // Only the legacy standalone build keeps its free-equipment policy.
        case CELL_SWORD: // Kokiri line: owns at least the base Kokiri sword
            if (KaleidoEquip_UsesLegacyFreeEquipment()) {
                return true;
            }
            return Nei_Save()->comboObtainedFc[FCI_KOKIRI_SWORD] != 0 || WeaponUpgrade_KokiriLevel() > 0 ||
                   (ExtEquip_GetCurrent(EQUIP_TYPE_SWORD) == 0 &&
                    GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) >= EQUIP_VALUE_SWORD_KOKIRI &&
                    GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) <= EQUIP_VALUE_SWORD_GILDED &&
                    BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) ==
                        ITEM_SWORD_KOKIRI + GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) - EQUIP_VALUE_SWORD_KOKIRI);
        case CELL_SWORD_MASTER:
            if (KaleidoEquip_UsesLegacyFreeEquipment()) {
                return true;
            }
            return Nei_Save()->comboObtained[FC_OOT_SWORD_MASTER] != 0;
        case CELL_SWORD_BGS: // Biggoron line OR the Great Fairy's Sword upgrade
            if (KaleidoEquip_UsesLegacyFreeEquipment()) {
                return true;
            }
            return (Nei_Save()->comboObtained[FC_OOT_SWORD_BIGGORON] != 0) || WeaponUpgrade_HasGreatFairy();
        case CELL_SHIELD_DEKU:
            if (KaleidoEquip_UsesLegacyFreeEquipment()) {
                return true;
            }
            return (Nei_Save()->shieldOwned & FC_SHIELD_DEKU) != 0;
        case CELL_SHIELD_OOT_MIRROR:
            if (KaleidoEquip_UsesLegacyFreeEquipment()) {
                return true;
            }
            return (Nei_Save()->shieldOwned & FC_SHIELD_MIRROR_OOT) != 0;
        case CELL_SHIELD: // Hero (index 1) / MM Mirror = Ikana (index 2)
            if (KaleidoEquip_UsesLegacyFreeEquipment()) {
                return true;
            }
            return (Nei_Save()->shieldOwned & (cell->index == 1 ? FC_SHIELD_HYLIAN : FC_SHIELD_IKANA)) != 0 ||
                   (ExtEquip_GetCurrent(EQUIP_TYPE_SHIELD) == 0 && Nei_Save()->vanillaShieldSkin == 0 &&
                    GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) == cell->index);
        case CELL_TUNIC: // Kokiri (idx 0) always; in COMBO, Goron/Zora require synced ownership
                         // (comboObtained via FleetSync). Standalone keeps "always available".
            if (cell->index == 0 || KaleidoEquip_UsesLegacyFreeEquipment()) {
                return true;
            }
            return Nei_Save()->comboObtained[cell->index == 1 ? FC_OOT_TUNIC_GORON : FC_OOT_TUNIC_ZORA] != 0;
        case CELL_BOOTS: // Kokiri (idx 0) always; in COMBO, Iron/Hover require synced ownership.
            if (cell->index == 0 || KaleidoEquip_UsesLegacyFreeEquipment()) {
                return true;
            }
            return Nei_Save()->comboObtained[cell->index == 1 ? FC_OOT_BOOTS_IRON : FC_OOT_BOOTS_HOVER] != 0;
        case CELL_DISPLAY:
            return false; // shown greyed; not selectable-equippable yet
        default:
            return ExtEquip_HasItem(cell->equipType, (u8)cell->index);
    }
}

static u8 KaleidoEquip_CellEquipped(EquipCell* cell) {
    // Which sword line is on the B button right now (the B item is the reliable discriminator,
    // since the Master line reuses the Gilded equip value).
    u8 bItem = BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B);
    switch (cell->equipType) {
        case CELL_SWORD: // Kokiri line lit when a Kokiri/Razor/Gilded sword is on B (not Master)
            return (bItem == ITEM_SWORD_KOKIRI || bItem == ITEM_SWORD_RAZOR || bItem == ITEM_SWORD_GILDED) &&
                   ExtEquip_GetCurrent(EQUIP_TYPE_SWORD) == 0;
        case CELL_SWORD_MASTER:
            return (bItem == ITEM_SWORD_MASTER) && ExtEquip_GetCurrent(EQUIP_TYPE_SWORD) == 0;
        case CELL_SWORD_BGS:
            return (bItem == ITEM_SWORD_BGS || bItem == ITEM_SWORD_GREAT_FAIRY) &&
                   ExtEquip_GetCurrent(EQUIP_TYPE_SWORD) == 0;
        case CELL_SHIELD: // native Hero/Mirror lit only when NOT wearing the Deku skin
            return GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) == cell->index &&
                   ExtEquip_GetCurrent(EQUIP_TYPE_SHIELD) == 0 && Nei_Save()->vanillaShieldSkin == 0;
        case CELL_SHIELD_DEKU: // Deku = Hero equip value + the Deku skin flag
            return GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) == EQUIP_VALUE_SHIELD_HERO &&
                   ExtEquip_GetCurrent(EQUIP_TYPE_SHIELD) == 0 && Nei_Save()->vanillaShieldSkin == 1;
        case CELL_SHIELD_OOT_MIRROR: // OoT Mirror = Mirror equip value + skin flag 2
            return GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SHIELD) == EQUIP_VALUE_SHIELD_MIRROR &&
                   ExtEquip_GetCurrent(EQUIP_TYPE_SHIELD) == 0 && Nei_Save()->vanillaShieldSkin == 2;
        case CELL_TUNIC:
            return Nei_Save()->vanillaTunic == cell->index;
        case CELL_BOOTS:
            return Nei_Save()->vanillaBoots == cell->index;
        case CELL_EMPTY:
        case CELL_DISPLAY:
            return false;
        default:
            return ExtEquip_GetCurrent(cell->equipType) == cell->index;
    }
}

// Equipment cells represent acquired inventory, like the item and OoT equipment pages.
// Keep cursor eligibility consistent with the draw and action ownership checks.
static u8 KaleidoEquip_CursorCanSit(s16 row, s16 col) {
    EquipCell cell;

    if (col == 0) {
        return KaleidoEquip_UpgradeValue(row) != 0;
    }
    KaleidoEquip_GetCell(sEquipSubPage, row, col, &cell);
    if (cell.equipType == CELL_EMPTY) {
        return false;
    }
    return KaleidoEquip_CellOwned(&cell);
}

// ---------------------------------------------------------------------------
// Equip actions (A button)
// ---------------------------------------------------------------------------
static void KaleidoEquip_EquipCell(PlayState* play, s16 row, s16 col) {
    EquipCell cell;

    KaleidoEquip_GetCell(sEquipSubPage, row, col, &cell);

    if (cell.equipType == CELL_EMPTY || cell.equipType == CELL_DISPLAY || !KaleidoEquip_CellOwned(&cell)) {
        Audio_PlaySfx(NA_SE_SY_ERROR);
        return;
    }

    // Vanilla cells: the ext piece of that type comes off FIRST (ExtEquip_SetSlot cleans it up and
    // leaves the slot bare/Kokiri), then the vanilla value goes on top. The old order — vanilla
    // write, then a deferred cleanup restoring the ext piece's snapshot — is what overwrote a freshly
    // equipped Master Sword with the Kokiri Sword.
    if (cell.equipType == CELL_SWORD || cell.equipType == CELL_SWORD_MASTER || cell.equipType == CELL_SWORD_BGS) {
        ExtEquip_RecordNativeSwordOwnership();
    }
    if (cell.equipType == CELL_SWORD) {
        // Kokiri line: equip the highest owned progressive tier (Kokiri/Razor/Gilded).
        u8 level = WeaponUpgrade_KokiriLevel(); // 0/1/2
        ExtEquip_Unequip(EQUIP_TYPE_SWORD);
        SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_KOKIRI + level);
        BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) = ITEM_SWORD_KOKIRI + level;
        Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
        ExtEquip_RefreshPlayer();
        Audio_PlaySfx(NA_SE_SY_DECIDE);
        return;
    }

    if (cell.equipType == CELL_SWORD_MASTER) {
        // Master line: reuse the Gilded equip value (one-handed mechanics) but put ITEM_SWORD_MASTER
        // on B so heldItemAction resolves to PLAYER_IA_SWORD_MASTER (own IA -> OoT blade + full-HP
        // beam once True Master is owned). z_player_lib.c overrides the blade DL by heldItemId.
        ExtEquip_Unequip(EQUIP_TYPE_SWORD);
        SET_EQUIP_VALUE(EQUIP_TYPE_SWORD, EQUIP_VALUE_SWORD_GILDED);
        BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) = ITEM_SWORD_MASTER;
        Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
        ExtEquip_RefreshPlayer();
        Audio_PlaySfx(NA_SE_SY_DECIDE);
        return;
    }

    if (cell.equipType == CELL_SWORD_BGS) {
        // Longsword line: two-handed IA (ITEM_SWORD_BGS -> PLAYER_IA_SWORD_BIGGORON). The two-hand
        // model group draws gPlayerLeftHandTwoHandSwordDLs (Great Fairy's Sword); a distinct
        // Biggoron blade DL (pre-Great-Fairy) is a follow-up. GFS HP/MP recover is already wired
        // (GreatFairySword_* via ExtEquip_OnMeleeHit) once WeaponUpgrade_HasGreatFairy().
        ExtEquip_Unequip(EQUIP_TYPE_SWORD);
        BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) = ITEM_SWORD_BGS;
        Interface_LoadItemIconImpl(play, EQUIP_SLOT_B);
        ExtEquip_RefreshPlayer();
        Audio_PlaySfx(NA_SE_SY_DECIDE);
        return;
    }

    // The Trident tolerates only the Divine Shield or a Mirror.
    if (cell.equipType == CELL_SHIELD || cell.equipType == CELL_SHIELD_DEKU ||
        cell.equipType == CELL_SHIELD_OOT_MIRROR) {
        u16 wantValue = (cell.equipType == CELL_SHIELD)        ? (u16)cell.index
                        : (cell.equipType == CELL_SHIELD_DEKU) ? EQUIP_VALUE_SHIELD_HERO
                                                               : EQUIP_VALUE_SHIELD_MIRROR;
        if ((ExtEquip_GetCurrent(EQUIP_TYPE_SWORD) == 3) && !ExtEquip_TridentAllowsShield(0, wantValue)) {
            Audio_PlaySfx(NA_SE_SY_ERROR);
            return;
        }
        ExtEquip_RecordNativeShieldOwnership();
    }

    if (cell.equipType == CELL_SHIELD) {
        Nei_Save()->shieldOwned |= (cell.index == 1) ? FC_SHIELD_HYLIAN : FC_SHIELD_IKANA;
        ExtEquip_Unequip(EQUIP_TYPE_SHIELD);
        SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, cell.index);
        Nei_Save()->vanillaShieldSkin = 0; // native Hero/Mirror model, not the Deku skin
        ExtEquip_RefreshPlayer();
        Audio_PlaySfx(NA_SE_SY_DECIDE);
        return;
    }

    if (cell.equipType == CELL_SHIELD_DEKU) {
        // Deku shield: equip AS Hero (same IA / raise gate / collider), then flag the Deku skin so
        // the draw shows OoT's smaller Deku model instead of the Hylian/Hero one.
        Nei_Save()->shieldOwned |= FC_SHIELD_DEKU;
        ExtEquip_Unequip(EQUIP_TYPE_SHIELD);
        SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_HERO);
        Nei_Save()->vanillaShieldSkin = 1;
        ExtEquip_RefreshPlayer();
        Audio_PlaySfx(NA_SE_SY_DECIDE);
        return;
    }

    if (cell.equipType == CELL_SHIELD_OOT_MIRROR) {
        // OoT Mirror shield: equip AS the Mirror value (reflects light / blocks like Mirror), flag
        // skin 2 so the draw shows OoT's mirror model (child-hand-patched). Distinct from the MM
        // Mirror (= Shield of Ikana on the ext page).
        Nei_Save()->shieldOwned |= FC_SHIELD_MIRROR_OOT;
        ExtEquip_Unequip(EQUIP_TYPE_SHIELD);
        SET_EQUIP_VALUE(EQUIP_TYPE_SHIELD, EQUIP_VALUE_SHIELD_MIRROR);
        Nei_Save()->vanillaShieldSkin = 2;
        ExtEquip_RefreshPlayer();
        Audio_PlaySfx(NA_SE_SY_DECIDE);
        return;
    }

    if (cell.equipType == CELL_TUNIC) {
        ExtEquip_Unequip(EQUIP_TYPE_TUNIC);        // Goron/Zora tunic and an ext tunic are exclusive
        Nei_Save()->vanillaTunic = (u8)cell.index; // 0=Kokiri, 1=Goron (fireproof), 2=Zora (gas-immune)
        Audio_PlaySfx(NA_SE_SY_DECIDE);
        return;
    }

    if (cell.equipType == CELL_BOOTS) {
        ExtEquip_Unequip(EQUIP_TYPE_BOOTS);        // Iron/Hover boots and ext boots are exclusive
        Nei_Save()->vanillaBoots = (u8)cell.index; // 0=Kokiri, 1=Iron (anti-KB/wind), 2=Hover (float)
        Audio_PlaySfx(NA_SE_SY_DECIDE);
        return;
    }

    // Extended equipment (toggle: equipping the current one unequips it — SoH behavior). Refusals
    // that ExtEquip_Equip would make silently get their error beep here.
    if (ExtEquip_GetCurrent(cell.equipType) == cell.index) {
        ExtEquip_Unequip(cell.equipType);
    } else if (!ExtEquip_CheckAgeReq(cell.equipType, (u8)cell.index) ||
               ((cell.equipType == EQUIP_TYPE_SHIELD) && (ExtEquip_GetCurrent(EQUIP_TYPE_SWORD) == 3) &&
                !ExtEquip_TridentAllowsShield((u8)cell.index, 0))) {
        Audio_PlaySfx(NA_SE_SY_ERROR);
        return;
    } else {
        ExtEquip_Equip(cell.equipType, (u8)cell.index);
    }
    Audio_PlaySfx(NA_SE_SY_DECIDE);
}

static SwordElementMenu sSwordElementMenu;
static s16 sSwordElementPage;
static s16 sSwordElementRow;
static s16 sSwordElementCol;

static u8 KaleidoEquip_ElementUsable(EquipCell* cell) {
    u8 owned;
    if (sEquipSubPage == EQUIP_SUBPAGE_TRANSFORM || sEquipCursorY != EQUIP_TYPE_SWORD || sEquipCursorX < 1 ||
        sEquipCursorX > 3) {
        return false;
    }
    /* Standalone's native free-equipment display policy is not acquisition. Enchant only
     * saved-owned sword lines; ordinary A still uses the untouched legacy equip policy. */
    switch (cell->equipType) {
        case CELL_SWORD:
            owned = Nei_Save()->comboObtainedFc[FCI_KOKIRI_SWORD] != 0 || WeaponUpgrade_KokiriLevel() > 0 ||
                    (ExtEquip_GetCurrent(EQUIP_TYPE_SWORD) == 0 &&
                     GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) >= EQUIP_VALUE_SWORD_KOKIRI &&
                     GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) <= EQUIP_VALUE_SWORD_GILDED &&
                     BUTTON_ITEM_EQUIP(0, EQUIP_SLOT_B) ==
                         ITEM_SWORD_KOKIRI + GET_CUR_EQUIP_VALUE(EQUIP_TYPE_SWORD) - EQUIP_VALUE_SWORD_KOKIRI);
            break;
        case CELL_SWORD_MASTER:
            owned = Nei_Save()->comboObtained[FC_OOT_SWORD_MASTER] != 0;
            break;
        case CELL_SWORD_BGS:
            owned = Nei_Save()->comboObtained[FC_OOT_SWORD_BIGGORON] != 0 || WeaponUpgrade_HasGreatFairy();
            break;
        default:
            owned = KaleidoEquip_CellOwned(cell);
            break;
    }
    return owned && (cell->equipType < 0 || ExtEquip_CheckAgeReq(cell->equipType, cell->index));
}

static u8 KaleidoEquip_OpenSwordElement(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;
    EquipCell cell;
    s16 vtxIndex = EQUIP_CELL(sEquipCursorY, sEquipCursorX) * 4;
    KaleidoEquip_GetCell(sEquipSubPage, sEquipCursorY, sEquipCursorX, &cell);
    if (!SwordElementKaleido_Open(&sSwordElementMenu, cell.item, KaleidoEquip_ElementUsable(&cell),
                                  pauseCtx->maskVtx[vtxIndex].v.ob[0] * 10, pauseCtx->maskVtx[vtxIndex].v.ob[1] * 10)) {
        return false;
    }
    sSwordElementPage = sEquipSubPage;
    sSwordElementRow = sEquipCursorY;
    sSwordElementCol = sEquipCursorX;
    Audio_PlaySfx(NA_SE_SY_DECIDE);
    return true;
}

void KaleidoEquip_UpdateSwordElement(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;
    Input* input = CONTROLLER1(&play->state);
    EquipCell cell;
    s8 direction = 0;
    u8 events;
    u8 element;
    if (!sSwordElementMenu.phase) {
        return;
    }
    KaleidoEquip_GetCell(sEquipSubPage, sEquipCursorY, sEquipCursorX, &cell);
    /* Keep native shoulders / Start available and reset on every departure from this menu. */
    if (pauseCtx->state != PAUSE_STATE_MAIN || pauseCtx->mainState != PAUSE_MAIN_STATE_IDLE ||
        pauseCtx->pageIndex != PAUSE_MASK || pauseCtx->itemDescriptionOn || pauseCtx->cursorSpecialPos ||
        sEquipSubPage != sSwordElementPage || sEquipCursorY != sSwordElementRow || sEquipCursorX != sSwordElementCol ||
        cell.item != sSwordElementMenu.item || !KaleidoEquip_ElementUsable(&cell) ||
        CHECK_BTN_ANY(input->press.button, BTN_START | BTN_L | BTN_R | BTN_Z)) {
        SwordElementKaleido_Reset(&sSwordElementMenu);
        return;
    }
    if (input->rel.stick_x < -30 || input->rel.stick_y > 30 ||
        CHECK_BTN_ANY(input->press.button, BTN_DLEFT | BTN_DUP)) {
        direction = -1;
    } else if (input->rel.stick_x > 30 || input->rel.stick_y < -30 ||
               CHECK_BTN_ANY(input->press.button, BTN_DRIGHT | BTN_DDOWN)) {
        direction = 1;
    }
    element = sSwordElementMenu.selection;
    events = SwordElementKaleido_Update(&sSwordElementMenu, true, direction, CHECK_BTN_ALL(input->press.button, BTN_A),
                                        CHECK_BTN_ALL(input->press.button, BTN_B));
    if (events & SWORD_ELEMENT_MENU_EVENT_ARROW) {
        element = sSwordElementMenu.selection;
    }
    if (events & SWORD_ELEMENT_MENU_EVENT_APPLY) {
        KaleidoEquip_EquipCell(play, sSwordElementRow, sSwordElementCol);
    }
    if (events & SWORD_ELEMENT_MENU_EVENT_CURSOR) {
        Audio_PlaySfx(NA_SE_SY_CURSOR);
    }
    if (events & (SWORD_ELEMENT_MENU_EVENT_ERROR | SWORD_ELEMENT_MENU_EVENT_CANCEL | SWORD_ELEMENT_MENU_EVENT_ARROW |
                  SWORD_ELEMENT_MENU_EVENT_FUSE)) {
        u16 sfx = (events & SWORD_ELEMENT_MENU_EVENT_ERROR)    ? NA_SE_SY_ERROR
                  : (events & SWORD_ELEMENT_MENU_EVENT_CANCEL) ? NA_SE_SY_CANCEL
                  : (events & SWORD_ELEMENT_MENU_EVENT_ARROW)  ? NA_SE_SY_SET_FIRE_ARROW + element - SWORD_ELEMENT_FIRE
                                                               : NA_SE_SY_SYNTH_MAGIC_ARROW;
        Audio_PlaySfx(sfx);
    }
    input->press.button &=
        ~(BTN_A | BTN_B | BTN_CUP | BTN_CLEFT | BTN_CDOWN | BTN_CRIGHT | BTN_DUP | BTN_DDOWN | BTN_DLEFT | BTN_DRIGHT);
    input->cur.button &= ~(BTN_DUP | BTN_DDOWN | BTN_DLEFT | BTN_DRIGHT);
    input->rel.stick_x = input->rel.stick_y = 0;
    pauseCtx->stickAdjX = pauseCtx->stickAdjY = 0;
}

// Ext page: C-left/down/right assigns the ext item to that C button for gameplay
// toggling (SoH's extended-page C-assign, on MM's button tables).
static void KaleidoEquip_AssignCButton(PlayState* play, s16 row, s16 col, u16 pressed) {
    EquipCell cell;
    s16 slot;

    KaleidoEquip_GetCell(sEquipSubPage, row, col, &cell);
    if (cell.equipType < 0 || !KaleidoEquip_CellOwned(&cell)) {
        Audio_PlaySfx(NA_SE_SY_ERROR);
        return;
    }

    if (pressed & BTN_CLEFT) {
        slot = EQUIP_SLOT_C_LEFT;
    } else if (pressed & BTN_CDOWN) {
        slot = EQUIP_SLOT_C_DOWN;
    } else {
        slot = EQUIP_SLOT_C_RIGHT;
    }

    // Toggle: if the same ext item is already on this button, clear it back to empty.
    if (BUTTON_ITEM_EQUIP(0, slot) == (u8)cell.item) {
        BUTTON_ITEM_EQUIP(0, slot) = ITEM_NONE;
    } else {
        BUTTON_ITEM_EQUIP(0, slot) = (u8)cell.item;
    }
    Interface_LoadItemIconImpl(play, slot);
    Audio_PlaySfx(NA_SE_SY_DECIDE);
}

// ---------------------------------------------------------------------------
// Sub-page cycling (SoH freed-button logic, MM's free shoulder button is L):
// vanilla -> ext (if ExtEquip on) -> transform (if Crossover Items on) -> vanilla.
// NES.c's KaleidoScope_HandlePageToggles is patched to leave L to us here.
// ---------------------------------------------------------------------------
static void KaleidoEquip_CycleSubPage(void) {
    s16 next = sEquipSubPage;

    do {
        next = (next + 1) % 3;
        // The extended-equipment page (vanilla-OoT <-> extended) is ALWAYS reachable with L — the
        // equipment menu is a core feature now, not gated behind the ExtEquip cheat. (The ext items'
        // ownership / gameplay behaviors are still governed by ExtEquip_IsEnabled elsewhere.)
        if (next == EQUIP_SUBPAGE_TRANSFORM && !BrokenItems_Enabled()) {
            continue; // transform (Crossover Items) page only when that feature is on
        }
        break;
    } while (next != sEquipSubPage);

    if (next != sEquipSubPage) {
        sEquipSubPage = next;
        // Keep the ExtEquip module's page state in sync (gameplay code reads it)
        if (ExtEquip_IsEnabled()) {
            int wantExt = (sEquipSubPage == EQUIP_SUBPAGE_EXT);
            if (ExtEquip_GetPage() != wantExt) {
                ExtEquip_SwitchPage();
            }
        }
        if (sEquipSubPage == EQUIP_SUBPAGE_TRANSFORM) {
            sTransformCursor = BrokenItems_CurrentForm();
        } else {
            // Land on a valid cell of the new page
            if (!KaleidoEquip_CursorCanSit(sEquipCursorY, sEquipCursorX)) {
                sEquipCursorY = 0;
                sEquipCursorX = 1;
            }
        }
        Audio_PlaySfx(NA_SE_SY_DECIDE);
    }
}

// ---------------------------------------------------------------------------
// Cursor / input (runs from the kaleido update, replacing UpdateMaskCursor)
// ---------------------------------------------------------------------------
void KaleidoScope_UpdateEquipmentCursor(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;
    Input* input = CONTROLLER1(&play->state);
    EquipCell cell;
    s16 col;
    s16 row;

    pauseCtx->cursorColorSet = PAUSE_CURSOR_COLOR_SET_WHITE;

    if (pauseCtx->pageIndex != PAUSE_MASK || pauseCtx->itemDescriptionOn || sSwordElementMenu.phase) {
        return;
    }

    ExtEquip_Update(); // tick the page-switch cooldown (Player_Update doesn't run while paused)

    // L cycles the sub-pages (only while actually interacting with the page)
    if ((pauseCtx->state == PAUSE_STATE_MAIN) && (pauseCtx->mainState == PAUSE_MAIN_STATE_IDLE) &&
        CHECK_BTN_ALL(input->press.button, BTN_L) && ExtEquip_CanSwitch()) {
        KaleidoEquip_CycleSubPage();
    }

    // --- Transform sub-page: forms on row 0, cols 1..count ---
    if (sEquipSubPage == EQUIP_SUBPAGE_TRANSFORM) {
        s32 count = BrokenItems_FormCount();

        if (sTransformCursor >= count) {
            sTransformCursor = count - 1;
        }
        if (pauseCtx->cursorSpecialPos != 0) {
            if (pauseCtx->cursorSpecialPos == PAUSE_CURSOR_PAGE_LEFT && pauseCtx->stickAdjX > 30) {
                KaleidoScope_MoveCursorFromSpecialPos(play);
                sTransformCursor = 0;
            } else if (pauseCtx->cursorSpecialPos == PAUSE_CURSOR_PAGE_RIGHT && pauseCtx->stickAdjX < -30) {
                KaleidoScope_MoveCursorFromSpecialPos(play);
                sTransformCursor = count - 1;
            } else {
                return;
            }
        } else if (pauseCtx->stickAdjX < -30) {
            if (sTransformCursor > 0) {
                sTransformCursor--;
                Audio_PlaySfx(NA_SE_SY_CURSOR);
            } else {
                KaleidoScope_MoveCursorToSpecialPos(play, PAUSE_CURSOR_PAGE_LEFT);
            }
        } else if (pauseCtx->stickAdjX > 30) {
            if (sTransformCursor < count - 1) {
                sTransformCursor++;
                Audio_PlaySfx(NA_SE_SY_CURSOR);
            } else {
                KaleidoScope_MoveCursorToSpecialPos(play, PAUSE_CURSOR_PAGE_RIGHT);
            }
        }

        if ((pauseCtx->state == PAUSE_STATE_MAIN) && (pauseCtx->mainState == PAUSE_MAIN_STATE_IDLE) &&
            CHECK_BTN_ALL(input->press.button, BTN_A)) {
            BrokenItems_EquipForm(play, sTransformCursor);
            Audio_PlaySfx(NA_SE_SY_DECIDE);
        } else if ((pauseCtx->state == PAUSE_STATE_MAIN) && (pauseCtx->mainState == PAUSE_MAIN_STATE_IDLE) &&
                   pauseCtx->cursorSpecialPos == 0 && CHECK_BTN_ALL(input->press.button, BTN_CUP)) {
            if (PauseItemDesc_ShowForm(play, sTransformCursor, 3)) {
                pauseCtx->itemDescriptionOn = true;
            }
        }

        // Name-panel guard: custom ids (>= 0xE0) would index past the name-texture tables.
        {
            u16 formItem = BrokenItems_FormItem(sTransformCursor);
            pauseCtx->cursorItem[PAUSE_MASK] = (formItem < 0xE0) ? formItem : PAUSE_ITEM_NONE;
        }
        pauseCtx->cursorSlot[PAUSE_MASK] = EQUIP_CELL(0, sTransformCursor + 1);
        pauseCtx->cursorPoint[PAUSE_MASK] = pauseCtx->cursorSlot[PAUSE_MASK];
        pauseCtx->cursorXIndex[PAUSE_MASK] = sTransformCursor + 1 + EQUIP_GRID_COL_BASE;
        pauseCtx->cursorYIndex[PAUSE_MASK] = 0;
        KaleidoScope_SetCursorVtxPos(pauseCtx, pauseCtx->cursorSlot[PAUSE_MASK] * 4, pauseCtx->maskVtx);
        return;
    }

    // --- Grid sub-pages (vanilla / ext): OoT 4x4 cursor space ---

    // A sub-page or save-file change can leave the persistent cursor on an unowned cell.
    // Find a populated cell before accepting input or publishing its title.
    if (pauseCtx->cursorSpecialPos == 0 && !KaleidoEquip_CursorCanSit(sEquipCursorY, sEquipCursorX)) {
        for (col = 0; col <= 3; col++) {
            for (row = 0; row < 4; row++) {
                if (KaleidoEquip_CursorCanSit(row, col)) {
                    sEquipCursorX = col;
                    sEquipCursorY = row;
                    goto EQUIPMENT_CURSOR_VALID;
                }
            }
        }
        pauseCtx->cursorItem[PAUSE_MASK] = PAUSE_ITEM_NONE;
        gExtEquipGridNameContext = false;
        KaleidoScope_MoveCursorToSpecialPos(play, PAUSE_CURSOR_PAGE_LEFT);
        return;
    EQUIPMENT_CURSOR_VALID:;
    }

    // Entering the grid from the page-switch special position (vanilla-cursor behavior):
    // scan for the first cell the cursor can sit on, nearest the entered side.
    if (pauseCtx->cursorSpecialPos != 0) {
        s16 fromLeft;

        if (pauseCtx->cursorSpecialPos == PAUSE_CURSOR_PAGE_LEFT && pauseCtx->stickAdjX > 30) {
            fromLeft = true;
        } else if (pauseCtx->cursorSpecialPos == PAUSE_CURSOR_PAGE_RIGHT && pauseCtx->stickAdjX < -30) {
            fromLeft = false;
        } else {
            return;
        }
        KaleidoScope_MoveCursorFromSpecialPos(play);
        for (col = fromLeft ? 0 : 3; col >= 0 && col <= 3; col += fromLeft ? 1 : -1) {
            for (row = 0; row < 4; row++) {
                if (KaleidoEquip_CursorCanSit(row, col)) {
                    sEquipCursorX = col;
                    sEquipCursorY = row;
                    goto CURSOR_PLACED;
                }
            }
        }
        // Nothing selectable at all — bounce to the opposite special position (OoT behavior)
        KaleidoScope_MoveCursorToSpecialPos(play, fromLeft ? PAUSE_CURSOR_PAGE_RIGHT : PAUSE_CURSOR_PAGE_LEFT);
        return;
    CURSOR_PLACED:;
    } else {
        // Horizontal: step, skipping cells the cursor can't sit on; off the edge -> page arrows.
        if (pauseCtx->stickAdjX < -30) {
            for (col = sEquipCursorX - 1; col >= 0; col--) {
                // Same-row first, then any row in that column (OoT walks the column)
                for (row = 0; row < 4; row++) {
                    s16 tryRow = (sEquipCursorY + row) % 4;
                    if (KaleidoEquip_CursorCanSit(tryRow, col)) {
                        sEquipCursorX = col;
                        sEquipCursorY = tryRow;
                        Audio_PlaySfx(NA_SE_SY_CURSOR);
                        goto MOVED_H;
                    }
                }
            }
            KaleidoScope_MoveCursorToSpecialPos(play, PAUSE_CURSOR_PAGE_LEFT);
            return;
        } else if (pauseCtx->stickAdjX > 30) {
            for (col = sEquipCursorX + 1; col <= 3; col++) {
                for (row = 0; row < 4; row++) {
                    s16 tryRow = (sEquipCursorY + row) % 4;
                    if (KaleidoEquip_CursorCanSit(tryRow, col)) {
                        sEquipCursorX = col;
                        sEquipCursorY = tryRow;
                        Audio_PlaySfx(NA_SE_SY_CURSOR);
                        goto MOVED_H;
                    }
                }
            }
            KaleidoScope_MoveCursorToSpecialPos(play, PAUSE_CURSOR_PAGE_RIGHT);
            return;
        }
    MOVED_H:
        // Vertical: step within the column, skipping unusable cells; stop at the edges.
        if (pauseCtx->stickAdjY > 30) {
            for (row = sEquipCursorY - 1; row >= 0; row--) {
                if (KaleidoEquip_CursorCanSit(row, sEquipCursorX)) {
                    sEquipCursorY = row;
                    Audio_PlaySfx(NA_SE_SY_CURSOR);
                    break;
                }
            }
        } else if (pauseCtx->stickAdjY < -30) {
            for (row = sEquipCursorY + 1; row <= 3; row++) {
                if (KaleidoEquip_CursorCanSit(row, sEquipCursorX)) {
                    sEquipCursorY = row;
                    Audio_PlaySfx(NA_SE_SY_CURSOR);
                    break;
                }
            }
        }
    }

    // --- A / C buttons ---
    if ((pauseCtx->state == PAUSE_STATE_MAIN) && (pauseCtx->mainState == PAUSE_MAIN_STATE_IDLE)) {
        if (CHECK_BTN_ALL(input->press.button, BTN_A)) {
            if (sEquipCursorX == 0) {
                // Cape A opens an explicit appearance choice; its half-cost passive stays active.
                // Pendant A toggles its moveset. Solid icon = ON, half-transparent = OFF.
                if ((sEquipCursorY == 0) && ExtEquip_CapeOwned()) {
                    if (PauseItemDesc_ShowCapeVisibility(play, 3)) {
                        pauseCtx->itemDescriptionOn = true;
                        Audio_PlaySfx(NA_SE_SY_DECIDE);
                    } else {
                        Audio_PlaySfx(NA_SE_SY_ERROR);
                    }
                } else if ((sEquipCursorY == 1) && ExtEquip_PendantOwned()) {
                    ExtEquip_TogglePendantEffect();
                    Audio_PlaySfx(NA_SE_SY_DECIDE);
                } else {
                    Audio_PlaySfx(NA_SE_SY_ERROR);
                }
            } else {
                if (!KaleidoEquip_OpenSwordElement(play)) {
                    KaleidoEquip_EquipCell(play, sEquipCursorY, sEquipCursorX);
                }
            }
        } else if ((sEquipSubPage == EQUIP_SUBPAGE_EXT) && (sEquipCursorX != 0) &&
                   (sEquipCursorY != EQUIP_EXT_ROW_BOOTS) &&
                   CHECK_BTN_ANY(input->press.button, BTN_CLEFT | BTN_CDOWN | BTN_CRIGHT)) {
            KaleidoEquip_AssignCButton(play, sEquipCursorY, sEquipCursorX,
                                       input->press.button & (BTN_CLEFT | BTN_CDOWN | BTN_CRIGHT));
        } else if (CHECK_BTN_ALL(input->press.button, BTN_CUP)) {
            // Cell coordinates distinguish shields, tunics and boots that share
            // fallback icons, and separate passive Cape/Pendant from grid items.
            if (PauseItemDesc_ShowEquipment(play, sEquipSubPage, sEquipCursorY, sEquipCursorX,
                                            (sEquipCursorY < 2) ? 3 : 1)) {
                pauseCtx->itemDescriptionOn = true;
            } else {
                Audio_PlaySfx(NA_SE_SY_ERROR);
            }
        }
    }

    // --- Publish cursor state (name panel + drawn cursor) ---
    // Cape/Champion and Pendant/Climb Boots share ids. Refresh even when only the context or
    // grid position changed (the two vanilla shields also share their fallback icon id).
    u8 gridNameContext = (sEquipSubPage == EQUIP_SUBPAGE_EXT) && (sEquipCursorX != 0);
    if (gExtEquipGridNameContext != gridNameContext ||
        pauseCtx->cursorSlot[PAUSE_MASK] != EQUIP_CELL(sEquipCursorY, sEquipCursorX)) {
        pauseCtx->namedItem = PAUSE_ITEM_NONE;
    }
    gExtEquipGridNameContext = gridNameContext;

    if (sEquipCursorX == 0) {
        s16 nameItem = KaleidoEquip_UpgradeNameItem(sEquipCursorY, KaleidoEquip_UpgradeValue(sEquipCursorY));
        pauseCtx->cursorItem[PAUSE_MASK] = (nameItem >= 0) ? (u16)nameItem : PAUSE_ITEM_NONE;
    } else {
        KaleidoEquip_GetCell(sEquipSubPage, sEquipCursorY, sEquipCursorX, &cell);
        pauseCtx->cursorItem[PAUSE_MASK] =
            (cell.item >= 0 && KaleidoEquip_CellOwned(&cell)) ? (u16)cell.item : PAUSE_ITEM_NONE;
    }
    pauseCtx->cursorSlot[PAUSE_MASK] = EQUIP_CELL(sEquipCursorY, sEquipCursorX);
    // Keep the shared cursor machinery in sync — the kaleido re-derives the drawn cursor from
    // cursorXIndex/cursorYIndex each frame (sItemMaskCursorsX/Y), so those must track our grid.
    pauseCtx->cursorPoint[PAUSE_MASK] = pauseCtx->cursorSlot[PAUSE_MASK];
    pauseCtx->cursorXIndex[PAUSE_MASK] = EQUIP_MASK_COL(sEquipCursorX); // col 0 = far-left sockets
    pauseCtx->cursorYIndex[PAUSE_MASK] = sEquipCursorY;
    KaleidoScope_SetCursorVtxPos(pauseCtx, pauseCtx->cursorSlot[PAUSE_MASK] * 4, pauseCtx->maskVtx);
}

// ---------------------------------------------------------------------------
// Link doll: render the player's LIVE skeleton into the isolated framebuffer.
// Replay both player streams into the doll FB, so translucent equipment cannot
// escape into the pause pages. Composite happens later as a
// page-space quad (see KaleidoScope_DrawEquipment).
// ---------------------------------------------------------------------------
static void KaleidoEquip_RenderDollFB(PlayState* play) {
    static Vp sDollViewport = { (EQUIP_DOLL_FBW / 2) << 2, (EQUIP_DOLL_FBH / 2) << 2, G_MAXZ / 2, 0,
                                (EQUIP_DOLL_FBW / 2) << 2, (EQUIP_DOLL_FBH / 2) << 2, G_MAXZ / 2, 0 };
    static Lights1 sDollLights = gdSPDefLights1(80, 80, 80, 255, 255, 255, 84, 84, -84);
    Player* player = GET_PLAYER(play);
    Mtx* perspMtx;
    Mtx* lookAtMtx;
    u16 perspNorm;
    Vec3s dollRot = { 0, 32300, 0 };

    if (player == NULL || player->skelAnime.skeleton == NULL) {
        return;
    }

    if (gPauseLinkFrameBuffer == -1) {
        // Keep the supersampled framebuffer at its exact native dimensions.
        gPauseLinkFrameBuffer =
            gfx_create_framebuffer(EQUIP_DOLL_FBW, EQUIP_DOLL_FBH, EQUIP_DOLL_FBW, EQUIP_DOLL_FBH, false);
    }

    /* Mario mode owns the same isolated MM doll framebuffer. */
    if (Sm64Kaleido_DrawForm(play)) {
        return;
    }

    perspMtx = GRAPH_ALLOC(play->state.gfxCtx, sizeof(Mtx));
    lookAtMtx = GRAPH_ALLOC(play->state.gfxCtx, sizeof(Mtx));

    OPEN_DISPS(play->state.gfxCtx);

    // SoH structure: the doll renders during the WORK pass (before the main scene), inside the
    // isolated FB. Replay OPA then XLU from WORK and branch over both captured
    // lists in their main passes. Player callbacks draw translucent shields and
    // replacement-model effects on XLU as well as the opaque skeleton on OPA.
    Gfx* opaRef = POLY_OPA_DISP;
    POLY_OPA_DISP++;
    Gfx* xluRef = POLY_XLU_DISP;
    POLY_XLU_DISP++;

    gsSPSetFB(WORK_DISP++, gPauseLinkFrameBuffer);
    gSPDisplayList(WORK_DISP++, POLY_OPA_DISP);
    gSPDisplayList(WORK_DISP++, POLY_XLU_DISP);

    // Clear the doll FB to transparent black so the page shows through around Link.
    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetScissor(POLY_OPA_DISP++, G_SC_NON_INTERLACE, 0, 0, EQUIP_DOLL_FBW, EQUIP_DOLL_FBH);
    gDPSetCycleType(POLY_OPA_DISP++, G_CYC_FILL);
    gDPSetRenderMode(POLY_OPA_DISP++, G_RM_NOOP, G_RM_NOOP2);
    gDPSetFillColor(POLY_OPA_DISP++, (GPACK_RGBA5551(0, 0, 0, 0) << 16) | GPACK_RGBA5551(0, 0, 0, 0));
    gDPFillRectangle(POLY_OPA_DISP++, 0, 0, EQUIP_DOLL_FBW - 1, EQUIP_DOLL_FBH - 1);
    gDPPipeSync(POLY_OPA_DISP++);

    // Player_DrawImpl expects the caller's render/combine setup. Restore the
    // native no-fog player state after the clear; cycle/geometry alone leaves
    // NOOP, and the gameplay fog blender would misread vertex alpha as fog.
    POLY_OPA_DISP = Gfx_SetupDL26(POLY_OPA_DISP);
    gDPSetCycleType(POLY_OPA_DISP++, G_CYC_2CYCLE);
    gSPLoadGeometryMode(POLY_OPA_DISP++, G_ZBUFFER | G_SHADE | G_CULL_BACK | G_LIGHTING | G_SHADING_SMOOTH);
    gSPTexture(POLY_OPA_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gSPViewport(POLY_OPA_DISP++, &sDollViewport);
    gDPPipeSync(POLY_XLU_DISP++);
    POLY_XLU_DISP = Gfx_SetupDL26(POLY_XLU_DISP);
    gDPSetScissor(POLY_XLU_DISP++, G_SC_NON_INTERLACE, 0, 0, EQUIP_DOLL_FBW, EQUIP_DOLL_FBH);
    gDPSetCycleType(POLY_XLU_DISP++, G_CYC_2CYCLE);
    gSPLoadGeometryMode(POLY_XLU_DISP++, G_ZBUFFER | G_SHADE | G_CULL_BACK | G_LIGHTING | G_SHADING_SMOOTH);
    gSPTexture(POLY_XLU_DISP++, 0xFFFF, 0xFFFF, 0, G_TX_RENDERTILE, G_ON);
    gSPViewport(POLY_XLU_DISP++, &sDollViewport);

    guPerspective(perspMtx, &perspNorm, 60.0f, (f32)EQUIP_DOLL_FBW / (f32)EQUIP_DOLL_FBH, 10.0f, 4000.0f, 1.0f);
    gSPPerspNormalize(POLY_OPA_DISP++, perspNorm);
    gSPMatrix(POLY_OPA_DISP++, perspMtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    gSPPerspNormalize(POLY_XLU_DISP++, perspNorm);
    gSPMatrix(POLY_XLU_DISP++, perspMtx, G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_PROJECTION);
    guLookAt(lookAtMtx, 0.0f, 0.0f, -100.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    gSPMatrix(POLY_OPA_DISP++, lookAtMtx, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_PROJECTION);
    gSPMatrix(POLY_XLU_DISP++, lookAtMtx, G_MTX_NOPUSH | G_MTX_MUL | G_MTX_PROJECTION);

    gSPSetLights1(POLY_OPA_DISP++, sDollLights);
    gSPSetLights1(POLY_XLU_DISP++, sDollLights);

    // CRITICAL: the player limb DLs reference segment 0x0C for the cull display list (set by
    // Player_DrawGameplay before Player_DrawImpl). Without it, segment 0x0C is garbage and the
    // limb DL is malformed -> gfx_step crash — most visibly on transformation forms (Deku/Goron/
    // Zora/FD), whose skeletons lean on it. Set it on both streams like the real player draw.
    gSPSegment(POLY_OPA_DISP++, 0x0C, gCullBackDList);
    gSPSegment(POLY_XLU_DISP++, 0x0C, gCullBackDList);
    // Same deal for segment 0x07 while the per-player tunic tint owns the tunic display lists.
    PlayerTunic_BindLocalColor(play);

    Matrix_Push();
    // Camera sits at z=-100 looking at the origin (guLookAt above) — Link must be IN FRONT of
    // it (z >= ~-40). Per-form scale/height: Goron & Zora are much bulkier/taller than Link and
    // Deku, so they need a smaller scale (and lower feet) to fit the 64x112 viewport.
    {
        f32 dollScale = 0.016f;
        f32 dollY = -45.0f;
        switch (player->transformation) {
            case PLAYER_FORM_GORON:
                dollScale = 0.0105f; // bulky + tall
                dollY = -52.0f;
                break;
            case PLAYER_FORM_ZORA:
                dollScale = 0.0120f; // tall/slim
                dollY = -50.0f;
                break;
            case PLAYER_FORM_DEKU:
                dollScale = 0.0190f; // small
                dollY = -42.0f;
                break;
            default: // HUMAN / FIERCE_DEITY
                break;
        }
        Matrix_SetTranslateRotateYXZ(0.0f, dollY, 0.0f, &dollRot);
        Matrix_Scale(dollScale, dollScale, dollScale, MTXMODE_APPLY);
    }

    // Match Player_DrawGameplay's call: real face index + the gameplay post-limb-draw (held item
    // / shield / sheath). postLimbDraw NULL previously skipped setup some forms expect.
    Player_DrawImpl(play, (void**)player->skelAnime.skeleton, player->skelAnime.jointTable,
                    player->skelAnime.dListCount, 0, player->transformation, 0, player->actor.shape.face,
                    Player_OverrideLimbDrawGameplayDefault, Player_PostLimbDrawGameplay, &player->actor);

    Matrix_Pop();

    gDPPipeSync(POLY_OPA_DISP++);

    // Terminate both WORK-executed sublists and skip them in the main passes.
    // Restore the main framebuffer only after translucent preview draws finish.
    gSPEndDisplayList(POLY_OPA_DISP++);
    gSPBranchList(opaRef, POLY_OPA_DISP);
    gDPPipeSync(POLY_XLU_DISP++);
    gSPEndDisplayList(POLY_XLU_DISP++);
    gSPBranchList(xluRef, POLY_XLU_DISP);
    gsSPResetFB(WORK_DISP++);

    CLOSE_DISPS(play->state.gfxCtx);
}

// Like OoT's equipment image, the framebuffer quad belongs to the page's modelview.
// Keep the cached image on incoming/off-axis pages so it moves with the cube throughout
// a transition rather than staying in a fixed screen rectangle until pageIndex changes.
static void KaleidoEquip_DrawDollImage(PlayState* play) {
    static u64 sDollTextureDimensions[2] = { 0, 0 };
    PauseContext* pauseCtx = &play->pauseCtx;
    Vtx* vertices;
    s32 i;

    if (gPauseLinkFrameBuffer == -1) {
        return;
    }

    // The settled pause projection maps these page coordinates to the former
    // (86,68)..(150,180) screen rectangle within one pixel, preserving its placement.
    vertices = GRAPH_ALLOC(play->state.gfxCtx, 4 * sizeof(Vtx));
    for (i = 0; i < 4; i++) {
        vertices[i].v.ob[0] = (i & 1) ? -10 : -72;
        vertices[i].v.ob[1] = (i & 2) ? -58 : 50;
        vertices[i].v.ob[2] = 0;
        vertices[i].v.flag = 0;
        vertices[i].v.tc[0] = (i & 1) ? EQUIP_DOLL_FBW << 5 : 0;
        vertices[i].v.tc[1] = (i & 2) ? EQUIP_DOLL_FBH << 5 : 0;
        vertices[i].v.cn[0] = vertices[i].v.cn[1] = vertices[i].v.cn[2] = vertices[i].v.cn[3] = 255;
    }

    OPEN_DISPS(play->state.gfxCtx);

    gDPPipeSync(POLY_OPA_DISP++);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);
    gDPSetTextureFilter(POLY_OPA_DISP++, G_TF_POINT);
    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, pauseCtx->alpha);
    gDPSetTileCustom(POLY_OPA_DISP++, G_IM_FMT_RGBA, G_IM_SIZ_16b, EQUIP_DOLL_FBW, EQUIP_DOLL_FBH, 0,
                     G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMIRROR | G_TX_CLAMP, G_TX_NOMASK, G_TX_NOMASK, G_TX_NOLOD,
                     G_TX_NOLOD);
    // Like OoT's equipment image, seed the loaded dimensions before binding the FB.
    // LoadTile only records metadata here; the immediate FB bind prevents a CPU upload
    // of this aligned, signature-safe placeholder before the quad is submitted.
    gDPSetTextureImage(POLY_OPA_DISP++, G_IM_FMT_RGBA, G_IM_SIZ_16b, EQUIP_DOLL_FBW, sDollTextureDimensions);
    gDPLoadSync(POLY_OPA_DISP++);
    gDPLoadTile(POLY_OPA_DISP++, G_TX_LOADTILE, 0, 0, (EQUIP_DOLL_FBW - 1) << 2, (EQUIP_DOLL_FBH - 1) << 2);
    gSPVertex(POLY_OPA_DISP++, vertices, 4, 0);
    gDPSetTextureImageFB(POLY_OPA_DISP++, G_IM_FMT_RGBA, G_IM_SIZ_16b, EQUIP_DOLL_FBW, gPauseLinkFrameBuffer);
    gSP1Quadrangle(POLY_OPA_DISP++, 0, 2, 3, 1, 0);

    gDPPipeSync(POLY_OPA_DISP++);

    CLOSE_DISPS(play->state.gfxCtx);
}

// ---------------------------------------------------------------------------
// Page draw (replaces KaleidoScope_DrawMaskSelect at the dispatch sites)
// ---------------------------------------------------------------------------
void KaleidoScope_DrawEquipment(PlayState* play) {
    PauseContext* pauseCtx = &play->pauseCtx;
    s16 row;
    s16 col;
    EquipCell cell;

    // SoH parity: suppress ext icon overrides so vanilla sword/shield icons show here
    gExtEquipSuppressIconOverride = 1;

    OPEN_DISPS(play->state.gfxCtx);

    // Render the doll into its isolated FB first (only while this page is active).
    if (pauseCtx->pageIndex == PAUSE_MASK) {
        KaleidoEquip_RenderDollFB(play);
    }

    Gfx_SetupDL42_Opa(play->state.gfxCtx);
    gDPSetCombineMode(POLY_OPA_DISP++, G_CC_MODULATEIA_PRIM, G_CC_MODULATEIA_PRIM);

    if (sEquipSubPage == EQUIP_SUBPAGE_TRANSFORM) {
        // Form selector: form icons on row 0, cols 1..count
        s32 i;
        for (i = 0; i < BrokenItems_FormCount(); i++) {
            void* tex = BrokenItems_FormIconTex(i);
            if (tex == NULL) {
                continue;
            }
            u8 locked = !BrokenItems_FormUnlocked(i);
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, locked ? 90 : 255, locked ? 90 : 255, locked ? 90 : 255,
                            (i == BrokenItems_CurrentForm()) ? pauseCtx->alpha : (pauseCtx->alpha * 2) / 3);
            gSPVertex(POLY_OPA_DISP++, &pauseCtx->maskVtx[EQUIP_CELL(0, i + 1) * 4], 4, 0);
            KaleidoScope_DrawTexQuadRGBA32(play->state.gfxCtx, tex, 32, 32, 0);
        }
    } else {
        // --- Passive column (col 0): owned Cape / Pendant toggles; lower sockets remain empty ---
        // Own vertices at the page's left-edge sockets (OoT x = -114), not maskVtx column 0.
        KaleidoEquip_BuildUpgradeColVtx(pauseCtx);
        for (row = 0; row < 4; row++) {
            s16 value = KaleidoEquip_UpgradeValue(row);
            void* icon = KaleidoEquip_UpgradeIcon(row, value);
            u8 upgAlpha;
            if (icon == NULL) {
                continue;
            }
            // The owned Cape's passive stays active when its cloth is hidden. Only the Pendant
            // dims when its moveset is off.
            if (row == 1) {
                upgAlpha = ExtEquip_PendantActive() ? pauseCtx->alpha : (pauseCtx->alpha >> 1);
            } else {
                upgAlpha = pauseCtx->alpha;
            }
            gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, upgAlpha);
            gSPVertex(POLY_OPA_DISP++, &sUpgradeColVtx[row * 4], 4, 0);
            KaleidoScope_DrawTexQuadRGBA32(play->state.gfxCtx, icon, 32, 32, 0);
        }

        // (Cursor X for this shifted column is overridden inside KaleidoScope_UpdateCursorSize via
        // KaleidoEquip_GetUpgradeCursorX — that runs right before KaleidoScope_DrawCursor, AFTER the
        // shared sItemMaskCursorsX table write, so it can't be stomped.)

        // --- Equipment grid (cols 1..3) ---
        for (row = 0; row < 4; row++) {
            for (col = 1; col <= 3; col++) {
                void* icon;
                u8 owned;
                u8 equipped;

                KaleidoEquip_GetCell(sEquipSubPage, row, col, &cell);
                if (cell.equipType == CELL_EMPTY) {
                    continue;
                }

                owned = KaleidoEquip_CellOwned(&cell);
                if (!owned) {
                    continue;
                }
                equipped = KaleidoEquip_CellEquipped(&cell);

                // OoT icon first (visual 1:1 when the OoT archive is in mods/), then the
                // ext-equipment table, then the MM item icon.
                icon = KaleidoEquip_OotTex(cell.ootIcon);
                if (icon == NULL) {
                    if (cell.equipType >= 0) {
                        // FileExists-gated: ExtEquip_GetIcon hands back an OTR PATH, and a path that
                        // isn't in the archive does NOT draw nothing — gDPLoadTextureBlock leaves
                        // whatever was last in TMEM, so the cell shows the PREVIOUS cell's icon. That
                        // reads as "the whole grid is in the wrong order" (swords repeated, tunics on
                        // the boots row) whenever an icon PNG hasn't been packed yet. Skijer 2026-07-29
                        icon = KaleidoEquip_OotTex(ExtEquip_GetIcon(cell.equipType, (u8)cell.index));
                    } else {
                        icon = ExtInv_GetItemIcon((u16)cell.item);
                    }
                }
                if (icon == NULL) {
                    continue;
                }

                // OoT-style equipped square (white outline) behind the equipped cell
                if (equipped) {
                    gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255, pauseCtx->alpha);
                    gSPVertex(POLY_OPA_DISP++, &pauseCtx->maskVtx[EQUIP_CELL(row, col) * 4], 4, 0);
                    POLY_OPA_DISP = Gfx_DrawTexQuadIA8(POLY_OPA_DISP, gEquippedItemOutlineTex, 32, 32, 0);
                }
                gDPSetPrimColor(POLY_OPA_DISP++, 0, 0, 255, 255, 255,
                                equipped ? pauseCtx->alpha : (pauseCtx->alpha * 3) / 4);
                gSPVertex(POLY_OPA_DISP++, &pauseCtx->maskVtx[EQUIP_CELL(row, col) * 4], 4, 0);
                KaleidoScope_DrawTexQuadRGBA32(play->state.gfxCtx, icon, 32, 32, 0);
            }
        }
    }

    // Composite the Link doll framebuffer on the left (OoT layout).
    KaleidoEquip_DrawDollImage(play);
    if (pauseCtx->pageIndex == PAUSE_MASK && sSwordElementMenu.phase) {
        void* swordIcon;
        KaleidoEquip_GetCell(sSwordElementPage, sSwordElementRow, sSwordElementCol, &cell);
        swordIcon = KaleidoEquip_OotTex(cell.ootIcon);
        if (swordIcon == NULL) {
            swordIcon = ExtInv_GetItemIcon(cell.item);
        }
        SwordElementKaleido_Draw(play, &sSwordElementMenu, swordIcon);
    }

    CLOSE_DISPS(play->state.gfxCtx);

    gExtEquipSuppressIconOverride = 0;
}
