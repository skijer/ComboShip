#pragma once

#include "rando/BlackMarket.h"
#include "menu/ComboItemReceiptText.h"
#include "menu/ComboItemReceiptPresentation.h"

// Presentation only: the saved offer and fixed item ID remain the authority for
// price, tier, eligibility and delivery. No randomized-check conversion belongs
// in this path, including when a native metadata name is unavailable.
namespace BlackMarketText {
using ComboRando::BlackMarket::Item;
using ComboRando::BlackMarket::PurchaseResult;
enum class Page { Unavailable, Welcome, Browse, Confirm, Result, KeeperTalk, KeeperHelpChoice, KeeperHelp };
struct Offer {
    const Item* item = nullptr;
    std::string name;
    std::string color = "%g";
    int price = 0;
    int clears = 0;
    int slot = 0;
    PurchaseResult status = PurchaseResult::Ready;
};

inline std::string Status(const Offer& offer) {
    std::string text = "%yTier " + std::to_string(offer.item->tier) + "%w";
    switch (offer.status) {
        case PurchaseResult::SoldOut:
            return text + " - Sold out";
        case PurchaseResult::Owned:
            return text + " - Already owned";
        case PurchaseResult::Locked:
            return text + " - Sealed: " + std::to_string(offer.clears) + " dungeon clears";
        case PurchaseResult::Unavailable:
            return text + (offer.item->renewable ? " - Empty bottle required" : " - Prerequisite required");
        case PurchaseResult::NeedRupees:
            return text + " - Not enough Rupees";
        default:
            return text;
    }
}

inline std::string Result(const Offer& offer, const std::string& status) {
    // Purchase owns the result/state. Only substitute the matching successful
    // catalogue label with the same fixed item's native display name.
    if (offer.item && status == std::string("Yours: ") + offer.item->label + ".")
        return "Yours: %r" + offer.name + "%w.";
    return status;
}

// Explicit three-line pages fit both native message windows. The host codec
// supplies its own page, choice, button and EVENT controls.
inline std::string KeeperCopy(Page page) {
    switch (page) {
        case Page::KeeperTalk:
            return "No guessing games here.&Pick the treasure you want!";
        case Page::KeeperHelpChoice:
            return "Want a tip?";
        case Page::KeeperHelp:
            return "Each treasure shows its price.&Choose it, then confirm to buy.&No Rupees spent until you buy.^"
                   "Seals need unique dungeon clears.&Each dungeon counts only once.&Clear more to open sealed offers.^"
                   "Most offers can be bought once.&Bottle refills can be bought again,&but bring an empty bottle.";
        default:
            return {};
    }
}

inline std::string Mm(Page page, const Offer& offer, const std::string& result, const float* widths) {
    std::string text;
    if (page == Page::KeeperHelpChoice) {
        text = ComboItemReceiptText::FromNeiMarkup(KeeperCopy(page));
        ComboItemReceiptText::Wrap(text, widths, 160, 300.f);
        return text + "\x11\x02\xC2How does this shop work?\x11"
                      "Back\xBF";
    }
    if (page == Page::Confirm) {
        text = ComboItemReceiptText::FromNeiMarkup("%r" + offer.name + ": " + std::to_string(offer.price) + " Rupees");
        ComboItemReceiptText::Wrap(text, widths, 160, 300.f);
        // Keep the two native choice lines together on MM's three-line pages.
        const auto pageBreak = text.find_last_of('\x10');
        const auto lineBreak = text.find_last_of('\x11');
        const bool titleWrapped =
            lineBreak != std::string::npos && (pageBreak == std::string::npos || lineBreak > pageBreak);
        text += titleWrapped ? "\x10\x11" : "\x11"; // Native arrows target rows two and three.
        text += '\x02'; // Native green; append raw choices after UTF-8 conversion/page break.
        text += "\xC2I'll buy it\x11No thanks\xBF";
        return text;
    }
    switch (page) {
        case Page::Unavailable:
            text = "My wares aren't ready yet.&Come back soon!";
            break;
        case Page::Welcome:
            text = "Welcome to the Black Market!&\x9F Continue   \xA0 Leave";
            break;
        case Page::Browse:
            text = "%r" + offer.name + ": " + std::to_string(offer.price) + " Rupees%w&" + Status(offer) + "&(" +
                   std::to_string(offer.slot + 1) + "/8) \x9F Select   \xA0 Leave";
            break;
        case Page::KeeperTalk:
        case Page::KeeperHelp:
            text = KeeperCopy(page);
            break;
        case Page::Result:
            text = Result(offer, result);
            break;
        default:
            break;
    }
    text = ComboItemReceiptText::FromNeiMarkup(text);
    ComboItemReceiptText::Wrap(text, widths, 160, 300.f);
    if (page != Page::Unavailable)
        text += '\x19'; // Actor-owned EVENT, matching the browsing/result state machine.
    text += '\xBF';
    return text;
}

// OoT keeps its own formatter and font encoding. Its native shop formatter also
// places the choices on lines three/four and wraps localized item metadata.
inline std::string Oot(Page page, const Offer& offer, const std::string& result, int language) {
    const char* currency = language == 1 ? "Rubine" : language == 2 ? "Rubis" : "Rupees";
    const std::string title =
        "\x08" + offer.color + offer.name + "%w  %y" + std::to_string(offer.price) + "_" + currency + "%w";
    switch (page) {
        case Page::Unavailable:
            return "My wares aren't ready yet.&Come back soon!";
        case Page::Welcome:
            // OoT's I4 button glyphs inherit the text color; unlike MM they
            // have no automatic button tint. Keep the native message palette
            // (and its cosmetics), then restore normal text after each glyph.
            return "Left / Right: browse items&%b\x9F%w Talk to shopkeeper&%g\xA0%w Leave\x0B";
        case Page::KeeperTalk:
        case Page::KeeperHelp:
            return KeeperCopy(page) + "\x0B";
        case Page::KeeperHelpChoice:
            return KeeperCopy(page) + "\x1B%gHow does this shop work?&Back%w\x09";
        case Page::Confirm:
            return title + "\x09\x1B%g" +
                   (language == 1   ? "Kaufen&Nicht kaufen"
                    : language == 2 ? "Acheter&Ne pas acheter"
                                    : "Buy&Don't buy") +
                   "%w\x09";
        case Page::Result:
            return Result(offer, result) + "\x0B";
        case Page::Browse:
            return title + "&" + Status(offer) + "&(" + std::to_string(offer.slot + 1) +
                   "/8) %b\x9F%w Select   %g\xA0%w Back\x0B";
    }
    return {};
}

inline std::string OotPages(std::string_view formatted) {
    // Native AutoFormat may stop before a short trailing section even when its
    // explicit newline creates a fifth line. Retain its width/choice layout,
    // but enforce native page boundaries after formatting these market bodies.
    std::string out;
    unsigned line = 1;
    char color = '\x00';
    auto page = [&] {
        out += '\x04';
        out += '\x05';
        out += color;
        line = 1;
    };
    for (size_t i = 0; i < formatted.size();) {
        const uint8_t c = formatted[i];
        const size_t command = c == 0x15 ? 4 : ComboReceipt_CommandSize(c, false);
        if (command > formatted.size() - i)
            return std::string(formatted);
        if (c == 0x05)
            color = formatted[i + 1];
        if (c == 0x04)
            line = 1;
        if (c == 0x01) {
            if (line == 4) {
                page();
                ++i;
                continue;
            }
            ++line;
        }
        if (c == 0x1B) {
            if (line > 3)
                page();
            // OoT's native two-choice arrows target lines three and four.
            while (line < 3) {
                out += '\x01';
                ++line;
            }
        }
        out.append(formatted.substr(i, command));
        i += command;
    }
    return out;
}
} // namespace BlackMarketText
