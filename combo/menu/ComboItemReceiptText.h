#pragma once

#include <cstdint>
#include <string>
#include <string_view>

// Receipt bodies cross the module boundary in MM's encoding, without a header or
// terminator. The finder owns the icon, bank attribution and normal textbox end.
using Fn_OOT_GetItemReceiptText = int32_t (*)(const char*, char*, uint32_t);

namespace ComboItemReceiptText {
inline bool WithoutOotItemIcons(std::string_view src, std::string& out) {
    out.clear();
    for (size_t offset = 0; offset < src.size();) {
        const uint8_t code = src[offset];
        const size_t parameters =
            code == 0x05 || code == 0x06 || code == 0x0C || code == 0x0E || code == 0x13 || code == 0x14 || code == 0x1E
                ? 1
            : code == 0x07 || code == 0x11 || code == 0x12 ? 2
            : code == 0x15                                 ? 3
                                                           : 0;
        if (src.size() - offset <= parameters)
            return false;
        if (code != 0x13)
            out.append(src.substr(offset, parameters + 1));
        offset += parameters + 1;
    }
    return true;
}

inline uint8_t OotButtonToMM(uint8_t c) {
    return c >= 0x9F && c <= 0xAA ? c + (0xB0 - 0x9F) : c;
}

inline uint8_t OotGlyphToMM(uint8_t c) {
    // The glyph textures in each engine's z_kanfont.c have different Latin
    // ordering. MM inserts additional accented I/N/O/U glyphs before buttons.
    constexpr uint8_t latin[] = { 0x80, 0x8B, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x8C, 0x90,
                                  0x91, 0x92, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C,
                                  0x9D, 0x9E, 0x9F, 0xA3, 0xA7, 0xA8, 0xA9, 0xAB, 0xAC };
    if (c == 0xAB)
        return '+'; // MM has no control-pad glyph; converters expand it to D-Pad
    return c >= 0x80 && c <= 0x9E ? latin[c - 0x80] : OotButtonToMM(c);
}

inline std::string FromNeiMarkup(std::string_view src) {
    std::string out;
    for (size_t i = 0; i < src.size(); ++i) {
        const uint8_t c = src[i];
        if (c >= 0xC0) {
            // Raw UTF-8 lead bytes are MM commands (C2/C3 are choices). Encode
            // the shared Latin font glyphs before they reach the MM decoder.
            constexpr std::string_view glyphs[] = { "À", "Î", "Â", "Ä", "Ç", "È", "É", "Ê", "Ë", "Ï", "Ô",
                                                    "Ö", "Ù", "Û", "Ü", "ß", "à", "á", "â", "ä", "ç", "è",
                                                    "é", "ê", "ë", "ï", "ô", "ö", "ù", "û", "ü" };
            bool found = false;
            if (src.substr(i, 2) == "î") {
                out += static_cast<char>(0xA2);
                ++i;
                continue;
            }
            for (size_t g = 0; g < sizeof(glyphs) / sizeof(glyphs[0]); ++g) {
                if (src.substr(i, glyphs[g].size()) == glyphs[g]) {
                    out += static_cast<char>(OotGlyphToMM(0x80 + g));
                    i += glyphs[g].size() - 1;
                    found = true;
                    break;
                }
            }
            if (!found) {
                out += '?';
                while (i + 1 < src.size() && (static_cast<uint8_t>(src[i + 1]) & 0xC0) == 0x80)
                    ++i;
            }
        } else if (c == '&' || c == '\n') {
            out += '\x11';
        } else if (c == '^') {
            out += '\x10';
        } else if (c == 0xAB) {
            out += "D-Pad";
        } else if (c == '%' && i + 1 < src.size()) {
            uint8_t color;
            switch (src[i + 1]) {
                case 'w':
                    color = 0;
                    break;
                case 'r':
                    color = 1;
                    break;
                case 'g':
                    color = 2;
                    break;
                case 'b':
                    color = 3;
                    break;
                case 'y':
                    color = 4;
                    break;
                case 'c':
                    color = 5;
                    break;
                case 'p':
                    color = 6;
                    break;
                case 'B':
                    color = 7;
                    break;
                default:
                    out += c;
                    continue;
            }
            out += static_cast<char>(color);
            ++i;
        } else {
            out += static_cast<char>(OotGlyphToMM(c));
        }
    }
    return out;
}

inline bool FromOotMessage(std::string_view src, std::string& out) {
    out.clear();
    for (size_t i = 0; i < src.size(); ++i) {
        const uint8_t c = src[i];
        switch (c) {
            case 0x01:
                out += '\x11';
                break;
            case 0x02:
                return !out.empty();
            case 0x04:
                out += '\x10';
                break;
            case 0x05: {
                if (++i == src.size())
                    return false;
                // OoT: default/red/adjustable/blue/cyan/purple/yellow/black.
                constexpr uint8_t colors[] = { 0, 1, 2, 3, 5, 6, 4, 7 };
                const uint8_t color = src[i];
                if (color == 0) {
                    out += '\x00';
                    break;
                } // CustomMessage's QM_WHITE
                if (color < 0x40 || color > 0x47)
                    return false;
                out += static_cast<char>(colors[color - 0x40]);
                break;
            }
            case 0x06: // indentation is layout-only
            case 0x0C: // delayed page -> normal page, with no timed dismissal
            case 0x0E: // fade
            case 0x13: // donor item icon is replaced by the finder's staged icon
            case 0x14: // text speed
                if (++i == src.size())
                    return false;
                if (c == 0x0C)
                    out += '\x10';
                break;
            case 0x08:
                out += '\x17';
                break;
            case 0x09:
                out += '\x18';
                break;
            case 0x0A: // persistent/event are inappropriate for a queued receipt
            case 0x0B:
            case 0x0D:
            case 0x1A:
                break;
            case 0x0F:
                out += '\x16';
                break; // player name is expanded by MM
            case 0x11: // fade2 and SFX each have two argument bytes
            case 0x12:
                if (src.size() - i <= 2)
                    return false;
                i += 2;
                break;
            // Chained dialogue, ocarina, choices and game-specific counters need
            // their own receipt builder; fail closed instead of executing them.
            default:
                if (c < 0x20)
                    return false;
                if (c == 0xAB)
                    out += "D-Pad";
                else
                    out += static_cast<char>(OotGlyphToMM(c));
                break;
        }
    }
    return !out.empty();
}

// Keep all body text, but strip story-only ending behavior. Parse arguments so
// an SFX's byte 0xBF cannot be mistaken for END. No vanilla nextMessageID survives.
inline bool FromMMMessage(std::string_view src, std::string& out) {
    out.clear();
    for (size_t i = 0; i < src.size(); ++i) {
        const uint8_t c = src[i];
        if (c == 0xBF)
            return !out.empty();
        if (c == 0x19 || c == 0x1A || c == 0xE0)
            continue;
        if (c == 0x1B || c == 0x1C || c == 0x1D || c == 0x1E || c == 0x1F) {
            if (src.size() - i <= 2)
                return false;
            i += 2;
            if (c == 0x1B)
                out += '\x10';
            continue;
        }
        if (c == 0x14) {
            if (++i == src.size())
                return false;
            continue;
        }
        if (c == 0x15 || c == 0xC1 || c == 0xC2 || c == 0xC3 || (c >= 0xCC && c <= 0xD6))
            return false;
        out += static_cast<char>(c);
    }
    return !out.empty();
}

// NEI pages are authored for OoT. Reflow for MM's narrower icon-bearing receipt
// box and enforce MM's three line-offset entries without dropping any text.
inline void Wrap(std::string& body, const float* widths, size_t count, float maxWidth) {
    float width = 0;
    unsigned line = 0;
    size_t space = std::string::npos;
    auto glyphWidth = [&](uint8_t c) {
        return c >= 0x20 && static_cast<size_t>(c - 0x20) < count ? widths[c - 0x20] : 0.0f;
    };
    for (size_t i = 0; i < body.size(); ++i) {
        const uint8_t c = body[i];
        if (c == 0x10 || c == 0x12) {
            width = 0;
            line = 0;
            space = std::string::npos;
            continue;
        }
        if (c == 0x11) {
            if (++line == 3) {
                body[i] = '\x10';
                line = 0;
            }
            width = 0;
            space = std::string::npos;
            continue;
        }
        if (c == ' ')
            space = i;
        width += glyphWidth(c);
        if (width <= maxWidth)
            continue;
        const char br = ++line == 3 ? (line = 0, '\x10') : '\x11';
        size_t start;
        if (space != std::string::npos) {
            body[space] = br;
            start = space + 1;
        } else {
            body.insert(i, 1, br);
            ++i;
            start = i;
        }
        width = 0;
        for (size_t j = start; j <= i; ++j)
            width += glyphWidth(body[j]);
        space = std::string::npos;
    }
}
} // namespace ComboItemReceiptText
