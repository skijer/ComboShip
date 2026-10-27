#pragma once

#include "ComboMagicItemReceiptText.h"
#include "ComboMaskReceiptText.h"
#include "ComboToolReceiptText.h"
#include <string>

// Read-only pause explanations. Hosts resolve the selected cell/mode before
// calling these helpers; neither ownership nor the active selection is changed.
namespace ComboPauseTutorialText {
using Language = ComboMagicItemReceiptText::Language;

inline std::string AsTutorial(std::string receipt) {
    // Shared tool/power receipts lead with grant/learned wording, a colored
    // name and "!&". Keep that name as a heading and retain all controls.
    const auto nameStart = receipt.find('%');
    const auto grantLineEnd = receipt.find('&');
    if (nameStart != std::string::npos && grantLineEnd != std::string::npos && nameStart < grantLineEnd &&
        receipt[grantLineEnd - 1] == '!') {
        receipt.erase(grantLineEnd - 1, 1);
        receipt.erase(0, nameStart);
    }
    return receipt;
}

inline std::string Shared(const char* name, Language language = Language::English) {
    if (!name)
        return {};
    if (const auto* entry = ComboToolReceiptText::Find(name)) {
        const std::string receipt = language == Language::German   ? entry->german
                                    : language == Language::French ? entry->french
                                                                   : entry->english;
        return AsTutorial(receipt);
    }
    if (const auto* entry = ComboMaskReceiptText::Find(name))
        return language == Language::German   ? entry->german
               : language == Language::French ? entry->french
                                              : entry->english;
    return {};
}

inline std::string Wand(int mode, int rule, Language language = Language::English) {
    static const char* names[] = {
        "Sand Rod", "Tornado Rod", "Water Rod", "Meteor Rod", "Storm Rod", "Shadow Scepter"
    };
    const auto* entry = ComboMagicItemReceiptText::Find(mode >= 0 && mode < 6 ? names[mode] : "Elemental Wand", rule);
    return entry ? AsTutorial(ComboMagicItemReceiptText::Body(*entry, language)) : std::string{};
}

inline std::string Slate(int rune, Language language = Language::English) {
    static const char* names[] = { "Rune: Remote Bomb", "Rune: Stasis", "Rune: Cryonis", "Rune: Master Cycle",
                                   "Rune: Sheikah Sensor" };
    const auto* entry = ComboMagicItemReceiptText::Find(rune >= 0 && rune < 5 ? names[rune] : "Sheikah Slate");
    return entry ? AsTutorial(ComboMagicItemReceiptText::Body(*entry, language)) : std::string{};
}

inline std::string Cane(int type, int skill, bool mm) {
    const char* controls = "^In pause, A opens the cane wheel.&C draws the cane, then casts.";
    if (type == 1) {
        // MM returns skill 6 for this entry and has no cast implementation for
        // it. The donor echo scanner/grid is not compiled into MM's cane.
        return std::string(
                   mm ? "Tri Rod.&This entry is selected on the cane wheel.&Echo casting is not implemented in MM."
                      : "Tri Rod: echoes.&Draw it to learn creatures by killing;&aim at props and C to learn them.^"
                        "Hold L for the echo grid; R steps.&C summons the selected echo.&Aim at your echo and C "
                        "dismisses it.") +
               controls;
    }
    if (type == 3)
        return std::string("Ultrahand.&C enters the object-moving mode.&Grab and move distant objects.") + controls;
    static const char* skills[] = {
        "Cane of Somaria: Statue.&Summon a statue at Link's position.&Use it to weigh down a switch.",
        "Cane of Somaria: Block.&Place a solid block in front of you.&Push it, weigh switches, or climb on.",
        "Cane of Somaria: Platform.&Summon a floating platform.&Ride it across a gap.",
        "Cane of Pacci: Flip.&Tap C to flip a target.&Hold C to lift; release to throw.&Z-target to aim the throw.",
        "Cane of Pacci: Stone.&Petrify an enemy and use it&as a stepping stone.",
        "Cane of Pacci: Ultrahand.&Grab and move distant objects.",
    };
    if (skill < 0 || skill >= 6)
        return {};
    return std::string(skills[skill]) + controls + (type == 0 ? "&With Somaria drawn, L/R step summons." : "");
}

inline std::string OotMask(int index, Language language = Language::English) {
    static const char* names[] = { "Keaton Mask", "Skull Mask", "Spooky Mask", "Bunny Hood",
                                   "Goron Mask",  "Zora Mask",  "Gerudo Mask", "Mask of Truth" };
    if (index < 0 || index >= 8)
        return {};
    auto body = Shared(names[index], language);
    if (!body.empty())
        return body;
    // These OoT trading masks are costumes, unlike the MM transformation masks.
    return std::string(names[index]) + ".&Equip to C or D-Pad to wear it.&Show it to people around Hyrule.";
}

inline std::string Trade(int index) {
    static const char* names[] = { "Pocket Egg",
                                   "Pocket Cucco",
                                   "Cojiro",
                                   "Odd Mushroom",
                                   "Odd Potion",
                                   "Poacher's Saw",
                                   "Broken Goron's Sword",
                                   "Prescription",
                                   "Eyeball Frog",
                                   "Eye Drops",
                                   "Claim Check",
                                   "Moon's Tear",
                                   "Land Title Deed",
                                   "Swamp Title Deed",
                                   "Mountain Title Deed",
                                   "Ocean Title Deed",
                                   "Room Key",
                                   "Letter to Kafei",
                                   "Special Delivery",
                                   "Pendant of Memories",
                                   "Weird Egg",
                                   "Chicken",
                                   "Zelda's Letter" };
    if (index < 0 || index >= 23)
        return {};
    return std::string(names[index]) +
           ".&This is the selected trade item.&Show it to its intended recipient.&A in pause opens the trade wheel.";
}

inline std::string Passive(int row) {
    if (row == 0)
        return "Magic Cape.&All magic costs are halved while owned.&A toggles the visible cloth.";
    if (row == 1)
        return "Pendant of Memories.&Adds Mortal Draw, Ground Pound&and Parry Leap to your B moves.&A toggles the "
               "moveset.";
    return {};
}

inline std::string VanillaEquipment(int row, int col, int kokiri, bool trueMaster, bool greatFairy, bool mm = false) {
    if (row < 0 || row > 3 || col < 1 || col > 3)
        return {};
    if (row == 0) {
        if (col == 1)
            return kokiri >= 2   ? "Gilded Sword.&The final Kokiri blade upgrade.&A equips this sword."
                   : kokiri == 1 ? "Razor Sword.&The sharpened Kokiri blade.&A equips this sword."
                                 : "Kokiri Sword.&A equips this sword.";
        if (col == 2)
            return trueMaster ? "True Master Sword.&At full health, a swing fires&a thunder beam. A equips it."
                              : "Master Sword.&The sacred blade. A equips it.";
        return greatFairy ? "Great Fairy's Sword.&Long reach; hits restore HP and magic.&A equips this sword."
                          : "Biggoron's Sword.&A long, two-handed blade.&A equips this sword.";
    }
    if (mm) {
        if (row == 2 && col == 2)
            return "Goron Tunic.&Fireproof: extinguishes body flames.&A equips this tunic.";
        if (row == 2 && col == 3)
            return "Zora Tunic.&Fast Zora swimming and A boost.&No drown timer; electric shock immunity.&A equips this "
                   "tunic.";
        if (row == 3 && col == 2)
            return "Iron Boots.&Sink in water; reduced knockback.&A equips these boots.";
    }
    static const char* rows[3][3] = {
        { "Deku Shield.&A equips this wooden shield.", "Hylian Shield.&A equips this metal shield.",
          "Mirror Shield.&Reflects light. A equips it." },
        { "Kokiri Tunic.&A equips this tunic.", "Goron Tunic.&Protects against extreme heat.&A equips this tunic.",
          "Zora Tunic.&Lets you breathe underwater.&A equips this tunic." },
        { "Kokiri Boots.&A equips these normal boots.",
          "Iron Boots.&Heavy boots for sinking in water.&A equips these boots.",
          "Hover Boots.&Briefly walk over gaps.&A equips these boots." },
    };
    return rows[row - 1][col - 1];
}

inline std::string Form(int index) {
    static const char* forms[] = {
        "Link Mode.&Stick moves; A acts or rolls.&B uses the sword; C uses items.&Z targets; R guards.^A equips this "
        "mode.",
        "Mario Mode.&Stick moves; A jumps.&B punches or uses fire.&Z crouches or ground-pounds.^"
        "D-Down: Wing Cap.&D-Left: Metal Cap.&D-Right: Vanish Cap.^A equips this mode.",
        "Pikachu Mode.&A fights or talks; B uses electricity.&R shields; C-Left jumps.&C-Right: Quick Attack.^"
        "C-Down: Grass dash.&D-Up: GMax/Charge; D-Down: Iron Tail.&D-Right: Dark bomb; D-Left: Sleep.^A equips this "
        "mode.",
    };
    return index >= 0 && index < 3 ? forms[index] : "";
}
} // namespace ComboPauseTutorialText
