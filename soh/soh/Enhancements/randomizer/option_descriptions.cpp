#include "settings.h"

namespace Rando {
// Native descriptions live in 9.3 language resources; retain authored integration help.
void Settings::CreateOptionDescriptions() {
    mOptionDescriptions[RSK_MAPS_COMPASSES_GIVE_INFORMATION] =
        "Compasses reveal the actual reward at the dungeon's boss, including shuffled items. "
        "Boss entrance shuffle also names the boss reached from that dungeon. "
        "Maps reveal the physical entrance location when dungeon entrances are shuffled. "
        "Collecting the item shows its information; press C-Up on an owned Map or Compass on the dungeon map "
        "page to read it again, including with Start With Maps/Compasses. "
        "The Temple of Time altar stops revealing reward locations; progression requirements remain. "
        "Bottom of the Well and Ice Cavern have no boss reward hints. Ordinary/masterful hints are unchanged.";
    mOptionDescriptions[RSK_SW97_SPELLS] =
        "Sage Spells: the elemental medallions grant elemental damage with magic cost.\n"
        "  - Spell: Medallion alone acts as a passive elemental source (Fire Medallion = Din's Fire equivalent for "
        "lighting torches and burning webs, Water Medallion = melt red ice, etc.).\n"
        "  - Projectile: Medallion + Bow (adult) or Slingshot (child) imbues the shot with the element.\n"
        "Item placement logic will respect this option.";
    mOptionDescriptions[RSK_SHUFFLE_MAGIC_MIRROR] =
        "Adds one Magic Mirror to the randomized item pool and shuffles the reward on the Lost Woods stump. Disabled "
        "keeps the fixed Mirror there. Independent of the other NEI items.";
    mOptionDescriptions[RSK_SKIJER_CUSTOM_ITEMS] =
        "Adds Skijer's 24 custom items to the item pool (Second Inventory Page). \n"
        "These include: Whip, Spinner, Bomb Arrows, Fire/Ice/Light Rods, Deku Leaf, \n"
        "Time Gate, Beetle, Switch Hook, Mogma Mitts, Gust Jar, Ball and Chain, \n"
        "Cane of Somaria, Dominion Rod, and more. \n"
        "These items are not considered by logic. \n"
        "2/24 | Logic Supported for : Shovel, Demise Destruction";
    mOptionDescriptions[RSK_SHUFFLE_BOMB_ARROWS] =
        "How Bomb Arrows are obtained. They are no longer an inventory item — they are the\n"
        "last entry of the bow's element wheel, next to the medallion arrows.\n\n"
        "Off: never granted on their own (the Twilight Upgrade still unlocks them).\n"
        "Bomb Bag: granted the moment you own any bomb bag.\n"
        "Shuffled: a real randomizer item, placed like any other.";
    mOptionDescriptions[RSK_ELEMENTAL_WAND_SHUFFLE] =
        "How the Elemental Wand is obtained. Six rods — Sand, Tornado, Water, Meteor,\n"
        "Storm and the Shadow Scepter — share one inventory cell and one wheel.\n\n"
        "Medallions: one wand in the pool; each rod works once you own its medallion.\n"
        "Single item: one wand in the pool; finding it unlocks all six rods.\n"
        "Elemental shuffle: the six rods are separate items; the first one found also\n"
        "grants the wand itself.";
    mOptionDescriptions[RSK_MM_SONGS] = "Adds Majora's Mask's songs to a solo-OoT item pool: Sonata of Awakening,\n"
                                        "Goron Lullaby, New Wave Bossa Nova, Elegy of Emptiness, Oath to Order,\n"
                                        "the Song of Healing and the Song of Soaring.\n\n"
                                        "They land as collectibles (the MM quest page shows them); no OoT location\n"
                                        "requires them, so seeds stay beatable. In combo they cross on their own.";
    mOptionDescriptions[RSK_MM_MASKS_ALL] = "Adds all 24 MM masks to the randomizer item pool.\n"
                                            "Masks can be found at random locations like custom items.\n"
                                            "Removes OOT Goron/Zora masks from pool.\n\n"
                                            "REQUIRES: 'Include MM Masks Inventory' enabled and mm.o2r loaded.";
    mOptionDescriptions[RSK_MM_MASKS_TRANSFORM] =
        "Adds only the 4 transformation masks (Deku, Goron, Zora, Fierce Deity) to the randomizer item pool.\n"
        "Removes OOT Goron/Zora masks from pool.\n\n"
        "REQUIRES: 'Include MM Masks Inventory' enabled and mm.o2r loaded.";
    mOptionDescriptions[RSK_EXT_EQUIPMENT] =
        "Adds 12 extended equipment pieces (3 swords, 3 shields, 3 tunics, 3 boots) to the item pool.\n"
        "Press L on the equipment page to toggle between vanilla and extended equipment.";
    mOptionDescriptions[RSK_CROSSOVER_POKEBALL] =
        "Adds the Pikachu Pokeball to the item pool.\n"
        "Finding it unlocks PIKACHU MODE on the equipment page's Crossover Items sub-page.";
    mOptionDescriptions[RSK_CROSSOVER_MARIO_MASK] =
        "Adds the Mario Mask to the item pool.\n"
        "Finding it unlocks MARIO MODE on the equipment page's Crossover Items sub-page\n"
        "(the same unlock the Peach's Castle set piece grants).";
    mOptionDescriptions[RSK_NEI_WEAPON_UPGRADES] =
        "Adds NEI weapon upgrades to the item pool. Each upgrade requires the base weapon to be owned:\n"
        "  - Hammer Upgrade (Iron Knuckle's Axe): double damage/reach + tomahawk throw\n"
        "  - Kokiri Sword Upgrade (x2): Razor Sword, then Gilded Sword\n"
        "  - True Master Sword (Master Sword)\n"
        "  - Great Fairy's Sword (Biggoron Sword)\n"
        "Only the Hammer upgrade has gameplay behavior for now.";
}
} // namespace Rando
