/**
 * This file is for handling the Randomize Rupee Names enhancement
 */
#include <soh/OTRGlobals.h>
#include "soh/Enhancements/randomizer/randomizer.h"
#include "ComboRupeeNames.h"

#include <libultraship/bridge/consolevariablebridge.h>

#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "soh/Enhancements/custom-message/CustomMessageTypes.h"
#include "soh/ShipInit.hpp"
#include "soh/ShipUtils.h"

extern "C" {
#include "variables.h"
}

using ComboRupeeNames::englishRupeeNames;
using ComboRupeeNames::frenchRupeeNames;
using ComboRupeeNames::germanRupeeNames;

void BuildRupeeMessage(uint16_t* textId, bool* loadFromMessageTable) {
    CustomMessage msg =
        CustomMessage("You found [[color]][[amount]] [[rupee]]%w!", "Du hast [[color]][[amount]] [[rupee]]%w gefunden!",
                      "Vous obtenez [[color]][[amount]] [[rupee]]%w!");
    std::string color;
    std::string amount;
    CustomMessage rupee =
        CustomMessage(ShipUtils::RandomElement(englishRupeeNames), ShipUtils::RandomElement(germanRupeeNames),
                      ShipUtils::RandomElement(frenchRupeeNames));
    switch (*textId) {
        case TEXT_BLUE_RUPEE:
            color = "%b";
            amount = "5";
            break;
        case TEXT_RED_RUPEE:
            color = "%r";
            amount = "20";
            break;
        case TEXT_PURPLE_RUPEE:
            color = "%p";
            amount = "50";
            break;
        case TEXT_HUGE_RUPEE:
            color = "%y";
            amount = "200";
            break;
        default:
            assert(false);
            return;
    }
    msg.Replace("[[color]]", color);
    msg.Replace("[[amount]]", amount);
    msg.Replace("[[rupee]]", rupee);
    msg.AutoFormat();
    msg.LoadIntoFont();
    *loadFromMessageTable = false;
}

void RegisterRandomRupeeNames() {
    COND_ID_HOOK(OnOpenText, TEXT_BLUE_RUPEE,
                 IS_RANDO && CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("RandomizeRupeeNames"), 1), BuildRupeeMessage);
    COND_ID_HOOK(OnOpenText, TEXT_RED_RUPEE,
                 IS_RANDO && CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("RandomizeRupeeNames"), 1), BuildRupeeMessage);
    COND_ID_HOOK(OnOpenText, TEXT_PURPLE_RUPEE,
                 IS_RANDO && CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("RandomizeRupeeNames"), 1), BuildRupeeMessage);
    COND_ID_HOOK(OnOpenText, TEXT_HUGE_RUPEE,
                 IS_RANDO && CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("RandomizeRupeeNames"), 1), BuildRupeeMessage);
}

static RegisterShipInitFunc initFunc(RegisterRandomRupeeNames,
                                     { CVAR_RANDOMIZER_ENHANCEMENT("RandomizeRupeeNames"), "IS_RANDO" });