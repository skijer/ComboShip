/**
 * wand_wind.c — Tornado Rod (Skijer's NEI).
 *
 * A toggle, not a cast: silver wind surrounds Link and every launch off the ground
 * comes out harder. It burns magic for as long as it is lit, and dies on damage, on an empty meter,
 * or on another press.
 *
 * The boost is applied where the engine writes the launch velocity, not where the jump is decided —
 * that is one place per launch kind instead of one per input.
 */

#include "2s2h/Rando/NeiAirMagicPresentation.h"

#define WIND_BOOST 1.55f
#define WIND_HOVER_RISE 4.0f   // held: the rise is pinned here instead of decaying into gravity
#define WIND_DRAIN_INTERVAL 20 // frames between each point of magic
#define WIND_DRAIN_COST 1

static u8 sWindOn = 0;
static s16 sWindDrainTimer = 0;
static s8 sWindPrevInvinc = 0;

static void WandWind_Stop(PlayState* play, Player* player) {
    if (!sWindOn) {
        return;
    }
    sWindOn = 0;
    sWindDrainTimer = 0;
    ItemEquip_PlayUnequipSFX(play, player);
}

// Cast = toggle. Lighting it costs nothing up front; the meter is spent while it burns.
u8 WandWind_Cast(Player* player, PlayState* play) {
    if (sWindOn) {
        WandWind_Stop(play, player);
        return 1;
    }
    if (!ItemMagic_HasEnough(play, WIND_DRAIN_COST)) {
        return 0;
    }
    sWindOn = 1;
    sWindDrainTimer = WIND_DRAIN_INTERVAL;
    ItemEquip_PlayEquipSFX(play, player);
    return 1;
}

void WandWind_Tick(PlayState* play, Player* player) {
    if (!sWindOn) {
        sWindPrevInvinc = player->invincibilityTimer;
        return;
    }

    if (ItemInput_CheckDamage(player, &sWindPrevInvinc)) {
        WandWind_Stop(play, player);
        return;
    }
    if (--sWindDrainTimer > 0) {
        return;
    }
    sWindDrainTimer = WIND_DRAIN_INTERVAL;
    if (!ItemMagic_HasEnough(play, WIND_DRAIN_COST)) {
        WandWind_Stop(play, player);
        return;
    }
    ItemMagic_Consume(play, WIND_DRAIN_COST);
}

/**
 * Multiply whatever vertical launch just happened. Called from z_player.c the instant the engine
 * writes velocity.y, so it covers the plain jump, the side hops, the backflip and the jump slash
 * without knowing which of them ran.
 *
 * SoH also clears PLAYER_STATE2_HOPPING here so a boosted hop can still grab a ledge on the way up.
 * That bit has no counterpart in MM — the same bit index is a lock-on flag, not a jump flag — so a
 * boosted hop keeps whatever ledge behaviour the launch it came from had.
 */
void WandWind_Boost(Player* player) {
    if (!sWindOn || (player->actor.velocity.y <= 0.0f)) {
        return;
    }
    player->actor.velocity.y *= WIND_BOOST;
}

/**
 * Holding the button with the wind lit keeps Link rising instead of letting gravity win, so he
 * floats. Used out of a boosted hop or a jump slash it turns that launch into flight.
 *
 * Only ever raises: a dive is still a dive, and the ground clamp stays untouched.
 */
void WandWind_TickHover(Player* player, u8 held) {
    if (!sWindOn || !held) {
        return;
    }
    if (player->actor.velocity.y < WIND_HOVER_RISE) {
        player->actor.velocity.y = WIND_HOVER_RISE;
    }
}

// Drawn from the wand's draw hook, never from the tick: the tornado emits into POLY_XLU and the
// update pass has no display list open. The envelope follows Link's body height.
void WandWind_Draw(Player* player, PlayState* play) {
    if (!sWindOn) {
        return;
    }
    NeiAirMagic_DrawEnvelope(play, player);
}
