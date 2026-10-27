#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
using namespace std::string_literals;
#include "sword_element_font_widths.inc"

#if __has_include("ComboSwordElementReceipt.h")
#include "ComboSwordElementReceipt.h"
#else
// Baseline negative control: the current renderer leaves receipts unchanged.
static size_t CwSwordElementReceipt_Append(uint8_t*, size_t length, size_t, int, uint8_t) { return length; }
#endif

static std::string Receipt(bool mm, std::string body) {
    if (!mm)
        return body + '\002';
    const char header[] = { 2, 0, 7, '\377', '\377', '\377', '\377', '\377', '\377', '\377', '\377' };
    return std::string(header, sizeof(header)) + body + '\277';
}

static std::string Append(bool mm, uint8_t element, const std::string& original, size_t capacity = 1280) {
    std::array<uint8_t, 1280> bytes{};
    assert(original.size() <= bytes.size());
    std::memcpy(bytes.data(), original.data(), original.size());
    const auto result = CwSwordElementReceipt_Append(bytes.data(), original.size(), capacity, mm, element);
    assert(result <= capacity);
    return std::string(reinterpret_cast<const char*>(bytes.data()), result);
}

static void TestNativePagesAndColors() {
    const char* names[] = { "", "Fire", "Ice", "Light" };
    const uint8_t ootColors[] = { 0, 0x41, 0x44, 0x46 };
    const uint8_t mmColors[] = { 0, 1, 5, 4 };
    for (bool mm : { false, true }) {
        // Item-icon arguments and SFX arguments can contain end/control bytes.
        const std::string native = mm ? "\027You found \001Fire Arrows\000!\030\021Custom receipt preserved."s
                                      : "\023\007\010You found \005\101Fire Arrows\005\100!\011\001Custom receipt preserved."s;
        const auto original = Receipt(mm, native);
        for (uint8_t element = 1; element <= 3; ++element) {
            const auto result = Append(mm, element, original);
            assert(result.size() > original.size() && "first enabled arrow pickup needs a tutorial page");
            assert(result.compare(0, original.size() - 1, original, 0, original.size() - 1) == 0);
            assert(uint8_t(result[original.size() - 1]) == (mm ? 0x10 : 4));
            assert(uint8_t(result.back()) == (mm ? 0xBF : 2));
            const std::string color = mm ? std::string(1, char(mmColors[element]))
                                         : std::string("\005") + char(ootColors[element]);
            const std::string reset = mm ? std::string(1, '\000') : std::string("\005\100");
            assert(result.find(color + names[element] + reset, original.size()) != std::string::npos);
            assert(result.find(mm ? '\260' : '\237', original.size()) != std::string::npos);
            assert(result.find("cost no magic.", original.size()) != std::string::npos);
            assert(result.find('#', original.size()) == std::string::npos);
            assert(result.find("0x", original.size()) == std::string::npos);
            float width = 0;
            unsigned lines = 1, glyphs = 0;
            for (size_t i = original.size(); i + 1 < result.size(); ++i) {
                const uint8_t c = result[i];
                if (!mm && c == 5) { ++i; continue; }
                if (c == (mm ? 0x11 : 1)) {
                    assert(width <= 215.0f && "tutorial must fit beside the native item icon");
                    width = 0;
                    ++lines;
                } else if (c >= 0x20) {
                    const auto* widths = mm ? mmFontWidths : ootFontWidths;
                    width += c == ' ' ? 6.0f : widths[c - 0x20] * 0.75f;
                    glyphs += c != ' ';
                }
            }
            assert(lines == (mm ? 3 : 4) && width <= 215.0f);
            assert(glyphs <= 120 && "MM allocates 120 character textures per page");
            assert(Append(mm, element, original, original.size() + 4) == original);
            assert(Append(mm, 0, original) == original);
            assert(Append(mm, 4, original) == original);
        }
    }
}

static void TestSpecialReceiptsRemainUntouched() {
    for (bool mm : { false, true }) {
        const std::string controls = mm ? std::string("\025\031\032\034\035\302\303\340", 8)
                                        : std::string("\007\012\013\016\021\033\034", 7);
        for (unsigned char control : controls) {
            const auto original = Receipt(mm, std::string("Original ") + char(control) + " flow");
            assert(Append(mm, 1, original) == original);
        }
        auto noEnd = Receipt(mm, "Unterminated");
        noEnd.pop_back();
        assert(Append(mm, 1, noEnd) == noEnd);
        auto hiddenTail = Receipt(mm, "Original");
        hiddenTail += "hidden";
        assert(Append(mm, 1, hiddenTail) == hiddenTail);
        auto padded = Receipt(mm, "Padded receipt");
        padded.append(3, '\000');
        assert(Append(mm, 1, padded).find("cost no magic.") != std::string::npos);
        auto full = Receipt(mm, std::string(1268 - (mm ? 11 : 0), 'x'));
        assert(Append(mm, 1, full) == full);
    }
    auto nextMessage = Receipt(true, "Continued receipt");
    nextMessage[3] = 0;
    nextMessage[4] = 0x70;
    assert(Append(true, 1, nextMessage) == nextMessage);
    assert(Append(false, 1, std::string("\023", 1)) == std::string("\023", 1));
    assert(Append(true, 1, Receipt(true, std::string("\036\277\000", 3))) !=
           Receipt(true, std::string("\036\277\000", 3)));
    const auto timed = Receipt(true, "Original overhead receipt"s + "\034\002\020"s);
    const auto appended = Append(true, 2, timed);
    assert(appended.size() > timed.size());
    assert(appended.compare(appended.size() - 4, 4, "\034\002\020\277", 4) == 0);
    assert(appended.compare(0, timed.size() - 4, timed, 0, timed.size() - 4) == 0);
}

int main() {
    TestNativePagesAndColors();
    TestSpecialReceiptsRemainUntouched();
    std::cout << "PASS native arrow receipt pages, palette bytes, A glyph, terminal and capacity guards\n";
}
