static void CheckButtons(const std::string& text, bool customized) {
    unsigned color = 0, a = 0, b = 0;
    for (size_t i = 0; i < text.size();) {
        const uint8_t c = text[i];
        const size_t command = ComboReceipt_CommandSize(c, false);
        assert(command <= text.size() - i);
        if (c == 0x04) color = 0;
        if (c == 0x05) color = uint8_t(text[i + 1]) & 0xF;
        if (c == 0x9F || c == 0xA0) {
            const uint32_t wanted = c == 0x9F ? (customized ? 0x112233 : 0x505AFF)
                                             : (customized ? 0x445566 : 0x46FF50);
            const uint32_t actual = MarketGlyph_PrimColor(color, customized);
            if (actual != wanted) {
                std::fprintf(stderr, "FAIL OoT %c glyph prim RGB=%06x expected=%06x (custom=%d)\n",
                             c == 0x9F ? 'A' : 'B', actual, wanted, customized);
                std::abort();
            }
            // Labels and following text use the normal native message color.
            assert(i + 3 < text.size() && uint8_t(text[i + 1]) == 0x05 &&
                   (uint8_t(text[i + 2]) & 0xF) == 0);
            c == 0x9F ? ++a : ++b;
        }
        i += command;
    }
    assert(a == 1 && b == 1);
}

int main() {
    // Native I4 text glyphs inherit current RGB: raw A/B are white; they do
    // not acquire the automatic button tint that MM's renderer supplies.
    assert(MarketGlyph_PrimColor(0, false) == 0xFFFFFF);
    CustomMessage formatter("");
    BlackMarketText::Offer offer;
    offer.item = ComboRando::BlackMarket::Find("nei.season.spring");
    assert(offer.item);
    offer.name = "Season: Spring";
    offer.color = "%r";
    offer.price = 173;
    for (int language = 0; language < 3; ++language) {
        for (auto page : {BlackMarketText::Page::Welcome, BlackMarketText::Page::Browse}) {
            auto text = BlackMarketText::Oot(page, offer, {}, language);
            formatter.AutoFormatString(text);
            text = BlackMarketText::OotPages(text);
            CheckButtons(text, false);
            CheckButtons(text, true);
            assert(text.ends_with("\x0B\x02"));
            if (page == BlackMarketText::Page::Browse) {
                const auto name = text.find("Season: Spring");
                assert(name >= 2 && name != std::string::npos);
                assert(uint8_t(text[name - 2]) == 0x05 && uint8_t(text[name - 1]) == 0x41);
                assert(uint8_t(text[name + 14]) == 0x05 && uint8_t(text[name + 15]) == 0);
                assert(text.find("\x05\x46" "173 ") != std::string::npos);
            }
        }
        auto choices = BlackMarketText::Oot(BlackMarketText::Page::Confirm, offer, {}, language);
        formatter.AutoFormatString(choices);
        choices = BlackMarketText::OotPages(choices);
        assert(std::count(choices.begin(), choices.end(), '\x1B') == 1);
        assert(choices.find("\x05\x42") != std::string::npos);
        assert(choices.ends_with(std::string_view("\x05\x00\x09\x02", 4)));
    }
    std::puts("PASS OoT welcome/browse A/B native primitive RGB, text cosmetics, item/price colors, resets, EVENT and choice controls (ASan/UBSan)");
}
