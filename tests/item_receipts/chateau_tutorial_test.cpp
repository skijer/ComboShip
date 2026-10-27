#include "combo/menu/ComboVanillaItemReceiptText.h"
#include <cassert>
#include <cstdio>
#include <string>

int main() {
    namespace Receipt = ComboVanillaItemReceiptText;
    const char* names[] = { "Chateau Romani", "Bottle of Chateau Romani", "Bottle with Chateau Romani",
                            "Chateau Romani Refill" };
    const Receipt::Language languages[] = { Receipt::Language::English, Receipt::Language::German,
                                            Receipt::Language::French };
    const char* lessons[] = {
        "Drink this special milk to restore&hearts and gain unlimited magic&until its effect expires.",
        "Diese Milch heilt deine Herzen&und gibt dir unbegrenzte Magie,&bis ihre Wirkung endet.",
        "Ce lait restaure votre vie&et donne une magie illimitée&jusqu'à la fin de son effet."
    };
    for (const char* name : names) {
        const auto* entry = Receipt::Find(name);
        assert(entry && entry->kind == Receipt::Kind::Chateau);
        for (unsigned locale = 0; locale < 3; ++locale) {
            const std::string body = Receipt::Body(*entry, languages[locale]);
            assert(body.find(lessons[locale]) != std::string::npos);
            // The actual use-button operand and both instructional pages survive.
            assert(body.find(std::string("%y") + char(0xA1) + "%w") != std::string::npos);
            const auto titleEnd = body.find('^');
            assert(titleEnd != std::string::npos);
            const auto useEnd = body.find('^', titleEnd + 1);
            assert(useEnd != std::string::npos && body.find(lessons[locale]) > useEnd);
        }
    }
    puts("PASS Chateau tutorial preserved: bottle/refill aliases, three languages, use controls and magic lesson");
}
