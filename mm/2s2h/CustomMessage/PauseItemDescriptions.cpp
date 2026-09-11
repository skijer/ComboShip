/**
 * PauseItemDescriptions.cpp - C-Up item descriptions in the pause menu
 *
 * Ported from soh's PauseItemDescriptions.cpp (Skijer's NEI). MM builds an item description text id
 * as `0x1700 + itemId`, which only resolves for vanilla inventory items — NEI's custom items live at
 * 0xB6+, so their id lands outside the message table. Worse, Message_FindMessageNES leaves
 * `font->messageStart` untouched on a miss and func_801514B0 dereferences it anyway, so hovering a
 * custom item and pressing C-Up read a stale pointer instead of showing anything.
 *
 * This file supplies the missing text (built through 2ship's CustomMessage font path, no message
 * table entry needed) plus the existence check the kaleido pages gate on.
 */

#include "PauseItemDescriptions.h"
#include "CustomMessage.h"

extern "C" {
#include "z64item.h"
#include "macros.h"
#include "functions.h"
#include "variables.h"
#include "message_data_static.h"
#include "mods/extended_inventory.h" // Sw97_* / Wand_* (Skijer's NEI)
#include "mods/extended_equipment.h" // ITEM_EXT_* ids for the equipment-page table
}

// ---------------------------------------------------------------------------
// Description table
// ---------------------------------------------------------------------------

struct ItemDescEntry {
    u16 itemId;
    const char* desc;
};

// Skijer's NEI custom items (z64item.h, 0xB6-0xCF). Text mirrors mods/items/CONTROLS.md.
// "\n" is a hard line break; CustomMessage word-wraps whatever is left over.
static const ItemDescEntry sCustomItemDescs[] = {
    { ITEM_ROCS_FEATHER_SKIJER, "Jump in ground and small jump\nfrom water." },
    { ITEM_ROCS_CAPE, "Jump from ground or water. Press\nagain in the air for a double jump." },
    // (No ITEM_DESIRE_SENSOR row: the item is retired and its hint is the slate's Sensor rune, which
    // costs a Heart Container rather than the 3 hearts this row used to advertise.)
    { ITEM_HYLIAS_GRACE,
      "Fairy flight for 10s. Ignores walls.\nA=up, B=down, L=sprint. 24 MP." }, // RETIRED item; row kept for old saves
    // 2026-08-06 page-2 additions. Shadow Crystal and Rod of Seasons are model-only on this side.
    { EXT_ITEM_SHEIKAH_SLATE, "C draws the slate, then casts the\nactive rune. Hold L for the rune wheel." },
    { EXT_ITEM_PHANTOM_HOURGLASS,
      "C stops time and aims. C again rewinds\nwhat the reticle holds along its own\npath. C or B lets go." },
    { EXT_ITEM_SHADOW_CRYSTAL, "Cursed twilight crystal. Turns Link\ninto Wolf Link. OoT only for now." },
    { EXT_ITEM_ROD_OF_SEASONS, "Rod bearing the four seasons.\nOoT only for now." },
    { ITEM_ZONAI_PERMAFROST,
      "Toggle the time stop. 4 MP to start,\nthen 1 MP every 10 frames. Ends on\na second press or an empty meter." },
    { ITEM_DEMISE_DESTRUCTION, "Massive AoE explosion. Damages all\nenemies in range. Ground only. 12 MP." },
    { ITEM_DEKU_LEAF, "Ground: blow wind gust. Air: hold\nto glide. Drains magic while gliding." },
    { ITEM_SWITCH_HOOK, "Aim and fire to swap positions\nwith objects and enemies." },
    { ITEM_MOGMA_MITTS, "Toggle to climb any wall.\nDrains magic over time." },
    { ITEM_GUST_JAR,
      "Hold C to suck things in, release to\nfire them back. Hold C 20 frames while\nidle for the element wheel." },
    { ITEM_BALL_AND_CHAIN,
      "Heavy thrown weapon. Breaks ice walls\nand heavy objects. Hold C to charge.\nC-Up to aim." },
    { ITEM_WHIP, "Grapple from any bar surface. Swing\nwith joystick. Release for momentum\nlaunch." },
    { ITEM_SPINNER, "Hold C to charge, release to ride.\nRelease while Z-targeting for a\nhoming dash. Breaks rocks." },
    { ITEM_CANE_OF_SOMARIA,
      "Four canes on one cell. A here cycles\nthe cane; C draws it, then casts.\nL and R step the summon." },
    { ITEM_DOMINION_ROD, "Fire orb to possess Beamos, Armos\nor Anubis. Control them with analog+C." },
    { ITEM_TIME_GATE, "Travel through time. Swap between\nyoung and adult Link. Costs 48 magic." },
    { ITEM_BOMB_ARROWS,
      "Hold C to aim, release to fire. Costs\n1 arrow and 1 bomb. Holding past 70\nframes drops a live bomb instead." },
    { ITEM_ROD_FIRE, "Slash=3 fireballs. Stab=long shot.\nJump=flamethrower. Spin=fire AoE.\nC-Up to aim." },
    { ITEM_ROD_ICE, "Slash=3 iceballs. Stab=long shot.\nJump=ice wave. Spin=ice AoE.\nC-Up to aim." },
    { ITEM_ROD_LIGHT, "Slash=3 orbs. Stab=long shot.\nJump=beam. Spin=light AoE.\nC-Up to aim." },
    { ITEM_BEETLE,
      "Hold C to aim, release to launch.\nStick steers, A boosts, Z locks on,\nB lets it fly home on its own." },
    { ITEM_SHOVEL, "Dig to uncover grottos, Gold\nSkulltulas and buried rewards." },
    { ITEM_MINISH_CAP,
      "C by a pod soil: fast travel map.\nC away from one: shrink or grow back.\nGold Skulltulas unlock the soils." },
    { ITEM_LANTERN, "Swing near fire to catch it. 4 types.\nBlue=melts red ice. Green=HP regen.\nPoe/Green=free Lens. "
                    "Swing=fire dmg." },
    { ITEM_CHATEAU_ROMANI, "Drink for infinite magic.\nOne-time consumable." },
    { ITEM_POKEBALL, "Transform into Pikachu.\nPress again to revert." },

    // (The twelve SW97 arrow/bullet rows are gone — the elemental shot has no item id any more.
    // sSw97ElemDescs below is keyed by SW97_ELEM_* instead, and the magic costs those rows
    // advertised are gone with them: medallion shots are free now.)

    // Bottle-side custom items
    { ITEM_NET, "Catch bugs, fish and fairies.\nSwing it like a sword." },
    { ITEM_BOTTOMLESS_BOTTLE, "Refills itself for a set number of\nuses per fill." },
    { ITEM_MAGIC_MUSHROOM, "A strange mushroom.\nBottle it before it spoils." },
    { ITEM_BOTTLE_WITH_MAGIC_MUSHROOM, "A bottled magic mushroom." },
};

// ---------------------------------------------------------------------------
// Lookup
// ---------------------------------------------------------------------------

// Keyed by SW97_ELEM_*, NOT by item id — the elemental shot is a flag on the bow/slingshot now.
static const ItemDescEntry sSw97ElemDescs[] = {
    { SW97_ELEM_FIRE, "Fire elemental shot. Costs no magic." },
    { SW97_ELEM_ICE, "Ice elemental shot. Costs no magic." },
    { SW97_ELEM_LIGHT, "Light elemental shot. Costs no magic." },
    { SW97_ELEM_DARK, "Dark elemental shot. Costs no magic." },
    { SW97_ELEM_SOUL, "Soul elemental shot. Costs no magic." },
    { SW97_ELEM_WIND, "Wind elemental shot. Costs no magic." },
    // BOMB is the one element whose wording depends on the weapon, so it is handled separately in
    // PauseItemDesc_Get rather than living in this element-keyed table.
};

static const char* kBombArrowsDesc =
    "Hold C to aim, release to fire. Costs\n1 arrow and 1 bomb. Holding past 70\nframes drops a live bomb instead.";
static const char* kBombBulletsDesc =
    "Hold C to aim, release to fire. Costs\n1 seed and 1 bomb. Holding past 70\nframes drops a live bomb instead.";

// The six rods share one item id, so their descriptions key off the active mode.
static const ItemDescEntry sWandModeDescs[] = {
    { WAND_MODE_SAND, "Sand Rod. Unlocked by the Spirit\nMedallion." },
    { WAND_MODE_TORNADO, "Tornado Rod. Unlocked by the Forest\nMedallion." },
    { WAND_MODE_WATER, "Water Rod. Unlocked by the Water\nMedallion." },
    { WAND_MODE_METEOR, "Meteor Rod. Unlocked by the Fire\nMedallion." },
    { WAND_MODE_STORM, "Storm Rod. Unlocked by the Light\nMedallion." },
    { WAND_MODE_SCEPTER, "Shadow Scepter. Unlocked by the\nShadow Medallion." },
};

// The eight OoT page-0 items, which live on MM sentinel ids. Without these the fallback would hand
// `0x1700 + id` to the message table, where those ids belong to MM items entirely — Milk, Gold Dust,
// the Hylian Loach, the map points — so the cursor showed somebody else's description.
static const ItemDescEntry sOotPageZeroDescs[] = {
    { ITEM_DINS_FIRE, "Ring of flame around you. Burns\nfoes and lights torches. 6 MP." },
    { ITEM_FARORES_WIND, "Set a warp point, then teleport\nback to it later. 6 MP." },
    { ITEM_NAYRUS_LOVE, "Protective barrier that blocks\nall damage for a time. 12 MP." },
    { ITEM_FAIRY_SLINGSHOT, "Child ranged weapon. Fires Deku\nSeeds. Hold C to aim." },
    { ITEM_HOOKSHOT_OOT, "Fire to grab targets and pull\nyourself in, or items to you." },
    { ITEM_LONGSHOT_OOT, "Like the Hookshot but with\ntwice the reach." },
    { ITEM_BOOMERANG, "Throw to stun foes and grab\ndistant items. Returns to you." },
    { ITEM_HAMMER, "Megaton Hammer. Smash rusty\nswitches, posts and armor." },
};

// Page-2 equipment, read on the EQUIP page. Slot contents differ from SoH's: sword 3 is the Trident,
// shield 2 the Kite Shield, boots 2 the Climb Boots and boots 3 the Roc Boots.
static const ItemDescEntry sExtEquipDescs[] = {
    // The reach and the HP/MP-on-hit belong to the Great Fairy's Sword here; this slot is a dummy.
    { ITEM_EXT_SWORD_1, "Cane of Byrna. Cosmetic for now: its\ncombat perks moved to the Great\nFairy's Sword." },
    { ITEM_EXT_SWORD_2,
      "Four Sword. Hold R+B for 3 clones that\nmirror your attacks, 12 MP each.\nHold L for the formation wheel." },
    { ITEM_EXT_SWORD_3, "Trident. B chains, hold B charges,\nR+B guard dashes, hold R+A flies." },
    { ITEM_EXT_SHIELD_1, "Goddess Shield. Fireproof; an early\nblock stuns every enemy nearby.\nOoT only for now." },
    { ITEM_EXT_SHIELD_2, "Kite Shield. R in mid-air to surf.\nDownhill builds speed. A hops, B spins." },
    { ITEM_EXT_SHIELD_3,
      "Shield of Ikana. Perfect guards drain\nlife; revives you once per scene.\nOoT only for now." },
    { ITEM_EXT_TUNIC_1, "Champion's Tunic. Dodge past an attack\nfor a Flurry Rush, aim in mid-air for\nBullet Time. "
                        "Both slow time to 33%." },
    { ITEM_EXT_TUNIC_2,
      "Spirit Tunic. Rupees absorb damage,\n1 HP each, and the fire and water\ntimers stop. At zero you are slow." },
    { ITEM_EXT_TUNIC_3, "Sage's Tunic. Each medallion you own\nadds a passive resistance while worn." },
    { ITEM_EXT_BOOTS_1, "Pegasus Boots. Keep holding B after a\nswing to charge forward, sword first." },
    { ITEM_EXT_BOOTS_2, "Climb Boots. Full traction: ice stops\nbeing slippery and steep slopes stop\nsliding you." },
    { ITEM_EXT_BOOTS_3, "Roc Boots. Water and lava become solid\nground. You fall at half speed." },
};

static const char* kMagicCapeDesc =
    "Magic Cape. Halves every magic cost\nwhile owned; 1 MP items become free.\nA toggles "
    "whether it is drawn.";
static const char* kPendantDesc =
    "Pendant of Memories. Three extra B\nmoves: Mortal Draw, Ground Pound and\nParry Leap. "
    "A toggles the moveset.";

extern "C" const char* PauseItemDesc_GetEquipUpgrade(s16 row) {
    switch (row) {
        case 0:
            return kMagicCapeDesc;
        case 1:
            return kPendantDesc;
        default:
            return NULL;
    }
}

// SW97 medallions, read on the OoT quest page. Each one arms a spell on a C button.
static const ItemDescEntry sMedallionDescs[] = {
    { ITEM_MEDALLION_FOREST,
      "Wind spell, 12 MP. A tornado that\ndrags enemies in and grinds them.\nC here equips the spell." },
    { ITEM_MEDALLION_FIRE,
      "Fire spell, 12 MP. A column of flame\nthat burns harder the longer it\nstands. C here equips the spell." },
    { ITEM_MEDALLION_WATER,
      "Ice spell, 24 MP. Freezes every enemy\nit touches for 6 seconds.\nC here equips the spell." },
    { ITEM_MEDALLION_SPIRIT,
      "Soul spell, 24 MP. Turns you into a\nfairy until you cast it again.\nC here equips the spell." },
    { ITEM_MEDALLION_SHADOW,
      "Dark spell, 12 MP. A shield that blocks\nall damage for a minute while the\nworld dims. C here equips it." },
    { ITEM_MEDALLION_LIGHT,
      "Light spell, 24 MP. Undead freeze for\n30 seconds and you heal 6 hearts.\nC here equips the spell." },
};

// Boss remains, read on the MM quest page. Unlike the medallions these are real MM item ids, so the
// PAUSE_QUEST lookup below finds them by id.
static const ItemDescEntry sBossRemainsDescs[] = {
    { ITEM_REMAINS_ODOLWA, "Press its button to wear it.\nHold A to sprint, trailing fire.\nR+B calls 6 beetles "
                           "(6 MP).\nA by soft soil takes off on moths." },
    { ITEM_REMAINS_GOHT, "Press its button to wear it.\nHold A to charge like a bull, R+A to\nground-pound. Hold B "
                         "for a thunder\nbolt (4 MP). R+B throws a bombchu." },
    { ITEM_REMAINS_GYORG, "Press its button to wear it.\nSwim like a Zora. In water R calls a\nfish school and B "
                          "holds a whirlpool;\non land R+B calls the fish." },
    { ITEM_REMAINS_TWINMOLD, "Press its button to wear it.\nIts Dark Link companion is not\nimplemented yet." },
};

static const char* PauseItemDesc_Find(const ItemDescEntry* table, size_t count, u16 itemId) {
    for (size_t i = 0; i < count; i++) {
        if (table[i].itemId == itemId) {
            return table[i].desc;
        }
    }
    return NULL;
}

extern "C" const char* PauseItemDesc_Get(u16 itemId, s32 pageIndex) {
    // The equipment and quest pages carry their own tables and share nothing with the item page.
    // NEI's equipment page rides PAUSE_MASK — MM has no PAUSE_EQUIP of its own.
    if (pageIndex == PAUSE_MASK) {
        return PauseItemDesc_Find(sExtEquipDescs, ARRAY_COUNT(sExtEquipDescs), itemId);
    }
    if (pageIndex == PAUSE_QUEST) {
        const char* remains = PauseItemDesc_Find(sBossRemainsDescs, ARRAY_COUNT(sBossRemainsDescs), itemId);
        if (remains != NULL) {
            return remains;
        }
        return PauseItemDesc_Find(sMedallionDescs, ARRAY_COUNT(sMedallionDescs), itemId);
    }
    if (pageIndex != PAUSE_ITEM) {
        return NULL;
    }

    // SW97 elemental shot: the cursor is on a plain bow/slingshot and the element rides a flag, so
    // describe whatever is primed on THAT weapon rather than looking the cursor item up.
    if (Sw97_IsBowItem(itemId) || Sw97_IsSlingItem(itemId)) {
        u8 isSling = Sw97_IsSlingItem(itemId);
        u8 elem = Sw97_EffectiveElement(isSling);
        if (elem == SW97_ELEM_BOMB) {
            return isSling ? kBombBulletsDesc : kBombArrowsDesc;
        }
        if (elem != SW97_ELEM_NONE) {
            for (size_t i = 0; i < ARRAY_COUNT(sSw97ElemDescs); i++) {
                if (sSw97ElemDescs[i].itemId == elem) {
                    return sSw97ElemDescs[i].desc;
                }
            }
        }
    }

    // Elemental Wand: one id, six descriptions — follow the active mode.
    if (itemId == ITEM_ELEMENTAL_WAND) {
        u8 mode = Wand_GetMode();
        for (size_t i = 0; i < ARRAY_COUNT(sWandModeDescs); i++) {
            if (sWandModeDescs[i].itemId == mode) {
                return sWandModeDescs[i].desc;
            }
        }
    }

    const char* custom = PauseItemDesc_Find(sCustomItemDescs, ARRAY_COUNT(sCustomItemDescs), itemId);
    if (custom != NULL) {
        return custom;
    }

    return PauseItemDesc_Find(sOotPageZeroDescs, ARRAY_COUNT(sOotPageZeroDescs), itemId);
}

extern "C" u8 PauseItemDesc_VanillaTextExists(u16 textId) {
    if (gPlayState == NULL) {
        return false;
    }

    MessageTableEntry* entry = gPlayState->msgCtx.messageTableNES;
    if (entry == NULL) {
        return false;
    }

    while (entry->textId != 0xFFFF) {
        if (entry->textId == textId) {
            return true;
        }
        entry++;
    }

    return false;
}

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------

/**
 * A description text id that is known to be in the table, used as the template for custom text.
 * func_801514B0 has no OnOpenText hook of its own (unlike Message_OpenText), so the only way to run
 * it with custom text is to let it set up all the pause-message state from a real entry and then
 * swap the font buffer underneath. Resolved once, since the item range differs per ROM revision.
 */
static u16 PauseItemDesc_GetTemplateTextId() {
    static u16 sTemplateTextId = 0;

    if (sTemplateTextId == 0) {
        for (u16 textId = 0x1700; textId < 0x1740; textId++) {
            if (PauseItemDesc_VanillaTextExists(textId)) {
                sTemplateTextId = textId;
                break;
            }
        }
    }

    return sTemplateTextId;
}

extern "C" void PauseItemDesc_Show(PlayState* play, const char* desc, u8 textBoxPos) {
    u16 templateTextId = PauseItemDesc_GetTemplateTextId();
    if (templateTextId == 0) {
        return;
    }

    func_801514B0(play, templateTextId, textBoxPos);

    MessageContext* msgCtx = &play->msgCtx;
    Font* font = &msgCtx->font;

    // Keep the template's header bytes: func_801514B0 already derived unk11F08/unk11F18/unk11F0C
    // from the first word of the buffer, so only the body may change.
    CustomMessage::Entry entry;
    entry.textboxType = font->msgBuf.schar[0];
    entry.textboxYPos = font->msgBuf.schar[1];
    entry.icon = font->msgBuf.schar[2];
    entry.nextMessageID = 0xFFFF;
    entry.firstItemCost = 0xFFFF;
    entry.secondItemCost = 0xFFFF;
    entry.msg = desc;

    CustomMessage::LoadCustomMessageIntoFont(entry);

    msgCtx->msgBufPos = 0;
    msgCtx->textDrawPos = 0;
    msgCtx->decodedTextLen = 0;
}
