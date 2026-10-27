/* Rod of Seasons: shared ownership, native MM input, weather and held presentation. */
#include "z64.h"
#include "mods/extended_inventory.h"
#include "mods/ext_buttons/ext_buttons.h"
#include "mods/items/helpers/equip_helper.h"
#include "2s2h/Enhancements/Audio/MMWeather.h"
#include "mods/items/helpers/season_particles.h"

static u8 sRodDrawn;
static u8 sRodPendingDraw;
static u8 sRodPrevRightHand;
static s8 sRodPrevInvinc;
static PlayState* sRodPlay;
static s32 sRodScene = -1;
static u32 sRodFrame;

u8 MasterCycle_IsRiding(void);
u8 Pacci_UltrahandModeActive(void);
void Player_UseItem(PlayState* play, Player* player, ItemId item);
s32 Player_UpperAction_ChangeHeldItem(Player* player, PlayState* play);
void Seasons_UpdateWeather(PlayState* play);

u8 Seasons_IsDrawn(void) {
    return sRodDrawn;
}

u8 Seasons_WalksOnWater(void) {
    return MMWeather_Season() == SEASON_WINTER;
}

static void Seasons_Stow(PlayState* play, Player* player) {
    if (sRodDrawn) {
        sRodDrawn = 0;
        player->rightHandType = sRodPrevRightHand;
        ItemEquip_PlayUnequipSFX(play, player);
    }
}

static void Seasons_TryFinishDraw(PlayState* play, Player* player) {
    if ((player->stateFlags3 & PLAYER_STATE3_START_CHANGING_HELD_ITEM) ||
        player->upperActionFunc == Player_UpperAction_ChangeHeldItem || player->heldItemAction != PLAYER_IA_NONE ||
        player->itemAction != PLAYER_IA_NONE) {
        return;
    }
    // Native FinishItemChange/InitItemAction can put custom items away again.
    // Draw only after that transition releases the final empty-hand model.
    sRodPendingDraw = 0;
    sRodPrevRightHand = player->rightHandType;
    sRodDrawn = 1;
    player->rightHandType = PLAYER_MODELTYPE_RH_CLOSED;
    ItemEquip_PlayEquipSFX(play, player);
}

static u16 Seasons_EquippedButtonMask(void) {
    static const u16 cButtons[] = { BTN_CLEFT, BTN_CDOWN, BTN_CRIGHT };
    static const u16 dButtons[] = { BTN_DRIGHT, BTN_DLEFT, BTN_DDOWN, BTN_DUP };
    u16 mask = 0;
    for (s32 i = 0; i < 3; ++i) {
        if (ExtButton_GetItem(0, i + EQUIP_SLOT_C_LEFT) == EXT_ITEM_ROD_OF_SEASONS &&
            gSaveContext.buttonStatus[i + EQUIP_SLOT_C_LEFT] != BTN_DISABLED) {
            mask |= cButtons[i];
        }
    }
    if (CVarGetInteger("gEnhancements.Dpad.DpadEquips", 0) && !Pacci_UltrahandModeActive()) {
        for (s32 i = 0; i < 4; ++i) {
            if (ExtButton_GetDpadItem(0, i) == EXT_ITEM_ROD_OF_SEASONS &&
                gSaveContext.shipSaveContext.dpad.status[i] != BTN_DISABLED) {
                mask |= dButtons[i];
            }
        }
    }
    return mask;
}

static void Seasons_FilterPlayerInput(PlayState* play, Player* player, Input* input) {
    if (!sRodDrawn || Seasons_EquippedButtonMask() == 0 || ItemInput_IsBlocked(player, play) ||
        play->pauseCtx.state != PAUSE_STATE_OFF || play->csCtx.state != CS_STATE_IDLE ||
        play->msgCtx.msgMode != MSGMODE_NONE || player->csAction != 0 ||
        play->transitionTrigger == TRANS_TRIGGER_START || player->heldItemAction != PLAYER_IA_NONE ||
        player->itemAction != PLAYER_IA_NONE ||
        (player->stateFlags1 & (PLAYER_STATE1_FIRST_PERSON | PLAYER_STATE1_CARRYING_ACTOR)) ||
        player->doorType != PLAYER_DOORTYPE_NONE || player->interactRangeActor != NULL ||
        (player->stateFlags2 & PLAYER_STATE2_CAN_ACCEPT_TALK_OFFER) || play->bButtonAmmoPlusOne != 0 ||
        player->currentMask == PLAYER_MASK_BLAST || player->currentMask == PLAYER_MASK_BREMEN ||
        player->currentMask == PLAYER_MASK_KAMARO) {
        return;
    }
    // Native mask/minigame/combat B actions retain their existing delegation.
    // Empty hands holding only the Rod use A to put it away.
    input->cur.button &= ~BTN_B;
    input->press.button &= ~BTN_B;
}

static void Seasons_OnWheelConfirm(s32 index) {
    const u8 previous = Seasons_GetSeason();
    Seasons_SetSeason((u8)index);
    if (gPlayState != NULL) {
        if (Seasons_GetSeason() != previous) {
            MM_BoxMenu_StartSeasonFlash(gPlayState);
        }
        ExtInv_RefreshButtonIconsForItem(gPlayState, EXT_ITEM_ROD_OF_SEASONS);
    }
}

static s32 Seasons_BuildWheel(BoxMenuEntry* out) {
    // These are the existing OoT season icons, loaded through ComboShip's game namespace.
    static const char* icons[] = {
        "__OTR__@oot:textures/icon_item_custom/gItemIconSeasonSpringTex",
        "__OTR__@oot:textures/icon_item_custom/gItemIconSeasonSummerTex",
        "__OTR__@oot:textures/icon_item_custom/gItemIconSeasonAutumnTex",
        "__OTR__@oot:textures/icon_item_custom/gItemIconSeasonWinterTex",
        "__OTR__@oot:textures/icon_item_custom/gItemIconSeasonOffTex",
    };
    for (s32 s = 0; s <= SEASON_OFF; ++s) {
        out[s].iconPath = icons[s];
        out[s].iconSize = 32;
        out[s].enabled = Seasons_SeasonOwned(s);
    }
    return SEASON_OFF + 1;
}

void Seasons_TickInput(PlayState* play, Player* player, Input* input) {
    BoxMenuEntry entries[SEASON_OFF + 1];
    if (sRodPlay != play || sRodScene != play->sceneId || play->gameplayFrames < sRodFrame) {
        sRodDrawn = 0;
        sRodPendingDraw = 0;
        sRodPrevInvinc = 0;
        sRodPlay = play;
        sRodScene = play->sceneId;
    }
    sRodFrame = play->gameplayFrames;
    Seasons_UpdateGates();
    if (!Seasons_HasRod() || player->transformation != PLAYER_FORM_HUMAN || MasterCycle_IsRiding()) {
        sRodPendingDraw = 0;
        Seasons_Stow(play, player);
        return;
    }
    if (sRodDrawn) {
        player->rightHandType = PLAYER_MODELTYPE_RH_CLOSED;
    }
    if (BoxMenu_IsOpen()) {
        sRodPendingDraw = 0;
        return;
    }
    u16 buttons = Seasons_EquippedButtonMask();
    u16 replacement = BTN_B | BTN_CLEFT | BTN_CDOWN | BTN_CRIGHT;
    if (CVarGetInteger("gEnhancements.Dpad.DpadEquips", 0)) {
        replacement |= BTN_DPAD;
    }
    // Waiting for our own native unequip may set START_CHANGING_HELD_ITEM; retain
    // the remaining input blockers while allowing that one transition to finish.
    u8 blocked = sRodPendingDraw ? (player->stateFlags1 & (ITEM_BLOCK_STATE1 | PLAYER_STATE1_SHIELDING)) != 0
                                 : ItemInput_IsBlocked(player, play);
    Seasons_FilterPlayerInput(play, player, input);
    if (!buttons || blocked || (player->stateFlags1 & PLAYER_STATE1_8000000) || player->meleeWeaponState != 0 ||
        ItemInput_CheckDamage(player, &sRodPrevInvinc) || play->pauseCtx.state != PAUSE_STATE_OFF ||
        play->csCtx.state != CS_STATE_IDLE || play->msgCtx.msgMode != MSGMODE_NONE ||
        (input->press.button & (replacement & ~buttons))) {
        sRodPendingDraw = 0;
        Seasons_Stow(play, player);
        return; // replacement remains available to native Player_UpdateItems
    }
    if (sRodPendingDraw) {
        if (player->heldItemAction != PLAYER_IA_NONE && player->heldItemId != ITEM_NONE) {
            sRodPendingDraw = 0; // the request was rejected or replaced by another native item
            return;
        }
        input->press.button &= ~buttons;
        play->state.input[0].press.button &= ~buttons;
        Seasons_TryFinishDraw(play, player);
        return;
    }
    if (input->press.button & buttons) {
        // Player_Update already copied/suppressed the pad and ran input hooks.
        // Consume that effective edge too, so native dispatch cannot replay
        // ITEM_EXT_BUTTON as an empty-hand request and stow the Rod this frame.
        input->press.button &= ~buttons;
        play->state.input[0].press.button &= ~buttons;
        if (!sRodDrawn) {
            Player_UseItem(play, player, ITEM_NONE);
            if (player->heldItemAction == PLAYER_IA_NONE || player->heldItemId == ITEM_NONE) {
                sRodPendingDraw = 1;
                Seasons_TryFinishDraw(play, player);
            }
        } else {
            if (BoxMenu_Open(play, entries, Seasons_BuildWheel(entries), Seasons_GetSeason(), 0,
                             Seasons_OnWheelConfirm)) {
                MM_BoxMenu_UseSeasonStyle();
            }
        }
    }
}

// Seasonal particles use MM's native snow actor; the weather bridge owns rain, sky and light.
// Native targets/counts remain live so Off always exposes the current scene weather.
static PlayState* sSeasonWeatherPlay;
static s32 sSeasonWeatherScene = -1;
static s32 sSeasonWeatherRoom = -1;
static u32 sSeasonWeatherFrame;
static Actor* sSeasonSnowActor;

static void Seasons_RemoveSnowActor(PlayState* play) {
    if (sSeasonSnowActor == NULL) {
        return;
    }
    // Actor_Kill defers removal. Compare against the live list before touching
    // our pointer, since a room unload may already have freed this actor.
    for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].first; actor != NULL; actor = actor->next) {
        if (actor == sSeasonSnowActor && actor->id == ACTOR_OBJECT_KANKYO && actor->params == 1 &&
            actor->room == sSeasonWeatherRoom) {
            Actor_Kill(actor);
            break;
        }
    }
    sSeasonSnowActor = NULL;
}

void Seasons_UpdateWeather(PlayState* play) {
    // Only our supplemental actor belongs to this room. Native snow and its
    // evolving precipitation/fog stay under the scene actors' control.
    if (sSeasonWeatherPlay == play && sSeasonWeatherScene == play->sceneId &&
        play->gameplayFrames >= sSeasonWeatherFrame && sSeasonWeatherRoom != play->roomCtx.curRoom.num) {
        Seasons_RemoveSnowActor(play);
    }
    if (sSeasonWeatherPlay != play || sSeasonWeatherScene != play->sceneId ||
        sSeasonWeatherRoom != play->roomCtx.curRoom.num || play->gameplayFrames < sSeasonWeatherFrame) {
        sSeasonSnowActor = NULL;
        sSeasonWeatherPlay = play;
        sSeasonWeatherScene = play->sceneId;
        sSeasonWeatherRoom = play->roomCtx.curRoom.num;
    }
    sSeasonWeatherFrame = play->gameplayFrames;
    const int season = MMWeather_SeasonForPlay(play);
    if (!MMSeasonParticles_IsEnabled(season)) {
        Seasons_RemoveSnowActor(play);
        return;
    }
    for (Actor* actor = play->actorCtx.actorLists[ACTORCAT_ITEMACTION].first; actor != NULL; actor = actor->next) {
        if (actor->id == ACTOR_OBJECT_KANKYO && actor->params >= 1 && actor->params <= 3 && actor->update != NULL) {
            return; // reuse native snow rather than draw or update it twice
        }
    }
    Actor* snow = Actor_Spawn(&play->actorCtx, play, ACTOR_OBJECT_KANKYO, 0, 0, 0, 0, 0, 0, 1);
    if (snow != NULL) {
        // Native snow actors persist across rooms. Our supplemental actor should
        // leave with this room, so a later room's native snow remains the sole owner.
        snow->room = play->roomCtx.curRoom.num;
        sSeasonSnowActor = snow;
    }
}

#include "../objects/object_rod_of_seasons.c"
