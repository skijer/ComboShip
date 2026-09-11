/**
 * item_elemental_wand.c — Elemental Wand (Skijer's NEI)
 *
 * Six rods share ONE page-2 cell, ONE item id (ITEM_ELEMENTAL_WAND) and ONE item action. Which rod
 * is live is NeiSaveData.wandMode, cycled by the kaleido wheel; this file is where that mode turns
 * into behavior.
 *
 * WHY ONE ACTION FOR SIX RODS
 * ---------------------------
 * Not a shortcut — a hard constraint. The NEI custom PlayerItemAction band (0x63-0x7F) is full, and
 * `heldItemAction` is s8, so 0x80+ is negative and unusable. 2ship's wand sits at 0x5C, in the gap
 * between MM's PLAYER_IA_MAX (0x53) and the NEI band, where the ExtPlayer_* getters route by exact
 * match and never index the native tables.
 *
 * WHY THE CAST IS NOT IN THE UPPER ACTION
 * ---------------------------------------
 * An upper action never sees the pad. So the press is read by Wand_TickInput, called from
 * Player_Update next to the slate's and the hourglass's — the shape every extended item ends up in.
 */

#include "global.h"
#include "mods/extended_inventory.h"         // Wand_GetMode / WAND_MODE_*
#include "mods/extended_player.h"            // PLAYER_IA_ELEMENTAL_WAND
#include "../helpers/equip_helper.h"         // ItemInput_*, ItemMagic_*, equip SFX
#include "../helpers/target_select_helper.h" // the rods that need a target
// box_menu.c is unity-included just before this file in custom_items.c, so its BoxMenu_* symbols
// are already visible — it has no header of its own.

extern s32 func_8083485C(Player* this, PlayState* play); // generic "held item" upper action
extern PlayState* gPlayState;

/**
 * Is the wand in Link's hand? Unlike the slate and the rod, it owns a real PlayerItemAction, so the
 * engine already tracks this — a second equip state machine could only drift out of step with it.
 */
u8 Wand_IsDrawn(void) {
    Player* player = (gPlayState != NULL) ? GET_PLAYER(gPlayState) : NULL;

    return (player != NULL) && (player->heldItemAction == PLAYER_IA_ELEMENTAL_WAND);
}

// One rod per file, unity-included so none of them reach the build files. They come BEFORE the
// dispatch because that is what makes their Cast/Tick/Draw visible to it.
#include "wand/wand_sand.c"
#include "wand/wand_wind.c"
#include "wand/wand_water.c"
#include "wand/wand_meteor.c"
#include "wand/wand_storm.c"
#include "wand/wand_shadow.c"

// Magic per cast, indexed by WAND_MODE_*. Sand and Water undercut the elemental rods' 3 because they
// are movement and get spammed; Storm costs the most because it fires world flags. TORNADO is 0 here
// on purpose: it is a toggle that drains, see wand_wind.c.
static const s16 sWandMagicCost[WAND_MODE_COUNT] = {
    2, // SAND
    0, // TORNADO
    2, // WATER
    3, // METEOR
    6, // STORM
    3, // SCEPTER
};

// Hold L long enough and the rod wheel opens in game, so switching element never needs the pause
// menu. Same length as the slate's rune row.
#define WAND_WHEEL_HOLD_FRAMES 8

static void Wand_OnWheelConfirm(s32 index) {
    Wand_SetMode(Wand_ModeAt((u8)index));
}

// Owned rods only, in the order the kaleido wheel cycles them, so every entry is selectable. The
// icon is the MEDALLION: the six rods share one staff sprite, so the element is the only thing that
// tells them apart anywhere else either.
static s32 Wand_BuildWheel(BoxMenuEntry* out) {
    s32 count = Wand_ModeCount();

    for (s32 i = 0; i < count; i++) {
        out[i].iconPath = (const char*)ExtInv_GetItemIcon(Wand_ModeMedallion(Wand_ModeAt((u8)i)));
        out[i].iconSize = 24; // quest icons, unlike the slate's 32x32 runes
        out[i].enabled = 1;
    }
    return count;
}

// A WAND_MODE_* value stops matching its row position as soon as one rod is missing.
static s32 Wand_ActiveWheelIndex(void) {
    u8 active = Wand_GetMode();
    s32 count = Wand_ModeCount();

    for (s32 i = 0; i < count; i++) {
        if (Wand_ModeAt((u8)i) == active) {
            return i;
        }
    }
    return 0;
}

/**
 * Fire the active rod. Magic is checked first and spent only on success, so a rod that declines
 * (no room for the actor, a geyser already up) costs nothing and reports the error itself.
 */
static u8 Wand_Cast(Player* player, PlayState* play, u8 mode) {
    u8 cast;

    if (mode >= WAND_MODE_COUNT) {
        return 0;
    }

    s16 cost = sWandMagicCost[mode];
    if ((cost > 0) && !ItemMagic_HasEnough(play, cost)) {
        return 0;
    }

    switch (mode) {
        case WAND_MODE_SAND: // Spirit Medallion
            cast = WandSand_Cast(player, play);
            break;
        case WAND_MODE_TORNADO: // Forest Medallion
            cast = WandWind_Cast(player, play);
            break;
        case WAND_MODE_WATER: // Water Medallion
            cast = WandWater_Cast(player, play);
            break;
        case WAND_MODE_METEOR: // Fire Medallion
            cast = WandMeteor_Cast(player, play);
            break;
        case WAND_MODE_STORM: // Light Medallion
            cast = WandStorm_Cast(player, play);
            break;
        case WAND_MODE_SCEPTER: // Shadow Medallion
            cast = WandShadow_Cast(player, play);
            break;
        default:
            return 0;
    }

    if (cast && (cost > 0)) {
        ItemMagic_Consume(play, cost);
    }
    return cast;
}

/**
 * Per-frame wand input, called from Player_Update beside the slate's and the hourglass's.
 *
 * "First press draws it, the rest cast" is the same feel as the cane and the slate, but it needs a
 * different trick here: the wand owns a real item action, so by the time this runs the engine has
 * ALREADY put it in Link's hand on that first press. What marks the press as the equip one is that
 * the wand was not drawn on the previous frame.
 */
void Wand_TickInput(PlayState* play, Player* player) {
    static s16 sLastScene = -1;
    static u8 sWasDrawn = 0;
    static s16 sHoldTimer = 0;
    BoxMenuEntry entries[WAND_MODE_COUNT];
    ItemInputState in;

    // Everything a rod leaves in the world is an actor the new scene has already thrown away.
    // Pointers are dropped, never written through: that memory may belong to somebody else now.
    if (sLastScene != play->sceneId) {
        sLastScene = play->sceneId;
        WandSand_Forget();
        WandWater_Forget();
        WandShadow_Forget();
        WandStorm_Forget();
    }

    // The rods that own something outside the wand keep running whatever the wand is doing, and
    // must keep running with it stowed — the bolt is mid-flight and the wind is still burning magic.
    WandShadow_Tick(play);
    WandStorm_Tick(play, player);
    WandWind_Tick(play, player);

    u8 drawn = Wand_IsDrawn();

    // No guard clause anywhere below on purpose: an early return would skip the latch at the end,
    // and a frame the wand spent stowed HAS to be recorded or the next draw reads as a continuation.
    if (BoxMenu_IsOpen()) {
        sHoldTimer = 0;
    } else if (drawn && (Wand_ModeCount() > 0)) {
        u8 mode = Wand_GetMode();
        ItemInput_Update(&in, ITEM_ELEMENTAL_WAND, player, play);

        // ---- HOLD L: the rod wheel, without opening the pause menu ----
        // Read from cur.button, never press: L is Z-target and the player actor consumes its press
        // bit long before item code runs (the trap the slate and the cane both document).
        if ((Wand_ModeCount() > 1) && (play->state.input[0].cur.button & BTN_L)) {
            if (sHoldTimer < (WAND_WHEEL_HOLD_FRAMES + 1)) {
                sHoldTimer++;
            }
            if (sHoldTimer == WAND_WHEEL_HOLD_FRAMES) {
                BoxMenu_Open(play, entries, Wand_BuildWheel(entries), Wand_ActiveWheelIndex(), BTN_L,
                             Wand_OnWheelConfirm);
            }
        } else {
            sHoldTimer = 0;

            // ---- HOLD C: the two rods that do something while the button is down ----
            if (mode == WAND_MODE_TORNADO) {
                WandWind_TickHover(player, in.isHeld);
            } else if ((mode == WAND_MODE_SAND) && WandSand_HoldElapsed(player, in.isHeld)) {
                Wand_Cast(player, play, WAND_MODE_SAND); // billed like any other slab
            }

            // ---- PRESS C: cast ----
            if (in.wasEquipped && in.isPressed && sWasDrawn && !ItemInput_IsBlocked(player, play) &&
                !Wand_Cast(player, play, mode)) {
                Audio_PlaySoundGeneral(NA_SE_SY_ERROR, &player->actor.world.pos, 4, &gSfxDefaultFreqAndVolScale,
                                       &gSfxDefaultFreqAndVolScale, &gSfxDefaultReverb);
            }
        }
    } else {
        sHoldTimer = 0;
    }
    sWasDrawn = drawn;
}

// The world-space half of the rods, run from the player's draw pass: these emit into POLY_XLU and
// the update pass has no display list open.
void Wand_Draw(Player* player, PlayState* play) {
    WandShadow_Draw(play);
    WandStorm_Draw(play);
    WandWind_Draw(player, play);
}

/**
 * Per-rod upper action. Runs every frame while the wand is the held item.
 *
 * Returning func_8083485C keeps the vanilla hold/aim handling, which is what every rod wants as its
 * base. Nothing per-rod belongs here — the casting lives in Wand_TickInput, which can see the pad.
 */
s32 Player_UpperAction_ElementalWand(Player* player, PlayState* play) {
    return func_8083485C(player, play);
}

/**
 * Runs once when the wand becomes the held item. Per-rod setup (charge timers, spawned helper
 * actors, aim reticles) belongs here.
 */
void Player_InitElementalWandIA(PlayState* play, Player* player) {
    (void)play;
    (void)player;
}
