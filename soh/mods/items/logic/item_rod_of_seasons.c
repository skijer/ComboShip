/**
 * item_rod_of_seasons.c — Rod of Seasons (Skijer's NEI)
 *
 * Four seasons share ONE page-2 cell (SLOT_ROD_OF_SEASONS) behind ONE ext item id
 * (EXT_ITEM_ROD_OF_SEASONS), the Sheikah Slate idiom: ownership is the NeiSaveData.seasonsOwned
 * bitmask, one sibling pickup per season, and NeiSaveData.season is the live one.
 *
 * CONTROLS
 * --------
 *   C (the button holding the rod) : FIRST press draws the staff into Link's hand (the Cane of
 *                                    Somaria idiom); every press after that opens the change-season
 *                                    prompt — A confirms, B cancels, unowned seasons draw grey and
 *                                    the cursor skips them
 *
 * Confirming a DIFFERENT season reloads the scene in place, the Time Gate's trick, so actors and
 * scene behaviour re-init under the new season instead of waiting for the next room change.
 *
 * The season is saved world state. The separate presentation controller restores its weather
 * after scene loads while this file retains the Rod input and gameplay behavior.
 */

#include "global.h"
#include "mods/extended_inventory.h"
#include "mods/items/helpers/equip_helper.h"
#include "soh/Enhancements/audio/SeasonWeather.h"
#include "soh/Enhancements/Graphics/ZorasDomain.h"
// box_menu.c is unity-included just before this file in custom_items.c, so its BoxMenu_*
// declarations are already in scope — it has no header (see the note at its top).

// Ext-button store: which u16 item a button really holds when it shows ITEM_EXT_BUTTON.
extern u16 ExtButton_GetItem(s32 btn);

// Drains the scene's water in Summer and hands it back otherwise (season_scene.cpp). Called every
// frame, season included in its own decision, so the restore has one place to happen too.
extern void SeasonScene_UpdateWater(PlayState* play);
extern void SeasonScene_UpdateWasteland(PlayState* play);

static u8 sRodDrawn = 0; // the staff is out, in Link's hand
static s8 sRodPrevInvinc = 0;

// Is the rod currently in Link's hand? Read by the in-hand draw.
u8 Seasons_IsDrawn(void) {
    return sRodDrawn;
}

// Winter freezes every water surface, so Link walks it. Answered here and OR'd into
// RocBoots_WalksOnWater, which is the one gate for that ability — the Garo form joins the same way.
// Its own conditions keep the water swimmable underneath: the pin only takes while Link is NOT
// already in the water and is not moving up.
u8 Seasons_WalksOnWater(void) {
    if (ZorasDomain_IsAlwaysThawed(gPlayState)) {
        return false;
    }
    return (Seasons_SeasonCount() != 0) && (Seasons_GetSeason() == SEASON_WINTER);
}

// ============================================================================
// WEATHER
// ============================================================================

// Retained for the native snow draw ABI. Seasonal petals/leaves/snow now use
// their own native-engine display lists; native fairy and snow actors keep the
// original neutral color and their own precipitation count.
void Seasons_PrecipTint(u8* r, u8* g, u8* b) {
    *r = *g = *b = 200;
}

static void Seasons_ApplyWeather(PlayState* play) {
    SeasonWeather_Update(play);
    // Seasonal weather must not assert the native Song of Storms puzzle flag.
    // Its ocarina actor retains ownership of genuine song activations.
}

// ============================================================================
// INPUT TICK — C draws the rod, the next press opens the change-season prompt
// ============================================================================

/**
 * Reloads the scene around Link without moving him, so actors and scene behaviour re-init under the
 * new season. The age-switch idiom: respawnFlag + RESPAWN_MODE_DOWN is what preserves the position;
 * a plain entrance reload would drop him back at the scene's spawn point.
 */
static void Seasons_ReloadSceneInPlace(PlayState* play, Player* player) {
    // RESPAWN_MODE_DOWN is the pit-fall path, so vanilla would charge Link the void-out damage for
    // turning the season. season_scene.cpp arms a one-shot VB to waive it.
    extern void SeasonScene_SuppressVoidDamage(void);

    SeasonScene_SuppressVoidDamage();
    gSaveContext.respawnFlag = 1;
    play->nextEntranceIndex = gSaveContext.entranceIndex;

    gSaveContext.respawn[RESPAWN_MODE_DOWN].entranceIndex = play->nextEntranceIndex;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].roomIndex = play->roomCtx.curRoom.num;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].pos = player->actor.world.pos;
    gSaveContext.respawn[RESPAWN_MODE_DOWN].yaw = player->actor.shape.rot.y;

    if (play->roomCtx.curRoom.behaviorType2 < 4) {
        gSaveContext.respawn[RESPAWN_MODE_DOWN].playerParams = 0x0DFF;
    } else {
        // Static-background rooms ride a fixed camera that has to be restored with them.
        gSaveContext.respawn[RESPAWN_MODE_DOWN].playerParams = 0x0D00 | GET_ACTIVE_CAM(play)->camDataIdx;
    }

    play->transitionTrigger = TRANS_TRIGGER_START;
    play->transitionType = TRANS_TYPE_INSTANT;
    gSaveContext.nextTransitionType = TRANS_TYPE_FADE_BLACK_FAST;
}

// Set by the confirm callback, acted on in the tick: the menu closes and unpauses BEFORE its
// callback runs, and the reload must be requested from a normal frame.
static u8 sSeasonReloadPending = 0;

// The prompt's confirm: the highlighted season becomes the active one.
static void Seasons_OnWheelConfirm(s32 index) {
    if (Seasons_GetSeason() == (u8)index) {
        return; // picked the season already running; nothing to re-init
    }
    Seasons_SetSeason((u8)index); // no-op if that season is not owned
    sSeasonReloadPending = (Seasons_GetSeason() == (u8)index);
}

// Fills the row with ALL slots — locked seasons included, drawn grayed and unselectable, so the row
// doubles as a reminder of what is still missing (the slate's rune row does the same). The blank
// coin is last and always selectable.
static s32 Seasons_BuildWheel(BoxMenuEntry* out) {
    for (s32 s = 0; s < SEASON_SLOTS; s++) {
        out[s].iconPath = (const char*)Seasons_SeasonIcon((u8)s);
        out[s].iconSize = 32;
        out[s].enabled = Seasons_SeasonOwned((u8)s);
    }
    return SEASON_SLOTS;
}

// Every C button currently holding the rod. C items live in the flat button array at 1..3.
static u16 Seasons_EquippedButtonMask(void) {
    u16 mask = 0;

    if (ExtButton_GetItem(1) == EXT_ITEM_ROD_OF_SEASONS) {
        mask |= BTN_CLEFT;
    }
    if (ExtButton_GetItem(2) == EXT_ITEM_ROD_OF_SEASONS) {
        mask |= BTN_CDOWN;
    }
    if (ExtButton_GetItem(3) == EXT_ITEM_ROD_OF_SEASONS) {
        mask |= BTN_CRIGHT;
    }
    return mask;
}

// Put it away. Idempotent, so every blocking path can call it unconditionally.
static void Seasons_Stow(PlayState* play, Player* player) {
    if (!sRodDrawn) {
        return;
    }
    sRodDrawn = 0;
    ItemEquip_PlayUnequipSFX(play, player);
}

// Runs on the effective native input before item dispatch. The late Rod tick
// keeps the raw C edge, so a second press opens its chooser instead of sending
// ITEM_EXT_BUTTON through native ITEM_NONE and putting the Rod away first.
static void Seasons_FilterPlayerInput(PlayState* play, Player* player, Input* input) {
    if (!sRodDrawn || Seasons_EquippedButtonMask() == 0 || BoxMenu_IsOpen() || ItemInput_IsBlocked(player, play) ||
        play->pauseCtx.state != 0 || play->csCtx.state != CS_STATE_IDLE || play->msgCtx.msgMode != MSGMODE_NONE ||
        player->csAction != 0 || play->transitionTrigger == TRANS_TRIGGER_START) {
        return;
    }
    input->press.button &= ~Seasons_EquippedButtonMask();

    // A is the put-away action. Do not turn an idle B press into a same-frame
    // Rod stow plus sword draw; native items and interaction states keep B.
    if (player->heldItemAction == PLAYER_IA_NONE && player->itemAction == PLAYER_IA_NONE &&
        !(player->stateFlags1 & (PLAYER_STATE1_FIRST_PERSON | PLAYER_STATE1_CARRYING_ACTOR)) &&
        player->doorType == PLAYER_DOORTYPE_NONE && player->interactRangeActor == NULL &&
        !(player->stateFlags2 & PLAYER_STATE2_CAN_ACCEPT_TALK_OFFER)) {
        input->cur.button &= ~BTN_B;
        input->press.button &= ~BTN_B;
    }
}

/**
 * Per-frame rod input, called from Player_UpdateCommon. The weather runs off ownership alone: once a
 * season is owned the world is in it, whether or not the rod sits on a button.
 */
void Seasons_TickInput(PlayState* play, Player* player) {
    BoxMenuEntry entries[SEASON_SLOTS];
    u16 btnMask;

    Seasons_UpdateGates();
    if (!Seasons_HasRod()) {
        return;
    }

    Seasons_ApplyWeather(play);
    // Frozen water is saved Rod gameplay rather than sky weather, so the
    // Water Temple and every other indoor pool freeze too.
    SeasonScene_UpdateWater(play);
    SeasonScene_UpdateWasteland(play);

    if (sSeasonReloadPending) {
        sSeasonReloadPending = 0;
        Seasons_Stow(play, player);
        Seasons_ReloadSceneInPlace(play, player);
        return;
    }

    if (BoxMenu_IsOpen()) {
        return; // the menu owns the frame (and the game is paused anyway)
    }

    btnMask = Seasons_EquippedButtonMask();
    if (btnMask == 0) {
        Seasons_Stow(play, player); // taken off the button while it was out
        return;
    }

    // Anything that would look wrong with a staff in hand puts it away.
    if (ItemInput_IsBlocked(player, play) || (player->stateFlags1 & PLAYER_STATE1_IN_WATER) ||
        (player->meleeWeaponState != 0)) {
        Seasons_Stow(play, player);
        return;
    }
    if (ItemInput_CheckDamage(player, &sRodPrevInvinc)) {
        Seasons_Stow(play, player);
        return;
    }

    if (play->state.input[0].press.button & btnMask) {
        if (!sRodDrawn) {
            // Equip only — the press that draws the rod never also opens the prompt, same as the
            // cane and the slate.
            sRodDrawn = 1;
            ItemEquip_PlayEquipSFX(play, player);
            return;
        }
        // Latched (holdButton 0): opened on a press, so A confirms and B cancels.
        BoxMenu_Open(play, entries, Seasons_BuildWheel(entries), Seasons_GetSeason(), 0, Seasons_OnWheelConfirm);
    }
}

// Unity include, like the cane and the slate: keeps the in-hand draw out of the build files.
#include "../objects/object_rod_of_seasons.c"
