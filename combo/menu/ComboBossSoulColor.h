#ifndef COMBO_BOSS_SOUL_COLOR_H
#define COMBO_BOSS_SOUL_COLOR_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// The existing MM/OoT soul-flame RGB values. Receipt names alone use these
// exact values; the engine still owns ordinary palette/body/button colors.
static inline uint32_t ComboBossSoulNameColor(const char* name) {
    static const struct {
        const char* boss;
        uint32_t rgb;
    } souls[] = { { "Goht", 0x0A8A2E },
                  { "Gyorg", 0x1363A5 },
                  { "Majora", 0xE88015 },
                  { "Odolwa", 0x911485 },
                  { "Twinmold", 0xA8B414 } };
    static const char* prefixes[] = { "Soul of ", "Seele von ", "\x82me de " };
    if (!name)
        return 0;
    for (size_t p = 0; p < sizeof(prefixes) / sizeof(prefixes[0]); ++p) {
        const size_t n = strlen(prefixes[p]);
        if (strncmp(name, prefixes[p], n))
            continue;
        for (size_t i = 0; i < sizeof(souls) / sizeof(souls[0]); ++i)
            if (!strcmp(name + n, souls[i].boss))
                return souls[i].rgb;
    }
    return 0;
}

// Called immediately after an existing green control, in the decoded Latin
// buffer. Both engines encode French Â as 0x82. Read through wrapping newlines
// only; require the native white reset and the exact authored localized name.
// There is no persistent latch, no new control byte and no gameplay state.
static inline uint32_t ComboBossSoulSpanColor(const uint8_t* text, size_t available, int mm) {
    char name[48];
    size_t length = 0;
    if (!text)
        return 0;
    for (size_t i = 0; i < available && length + 1 < sizeof(name); ++i) {
        const uint8_t c = text[i];
        if ((mm && c == 0) || (!mm && c == 5 && i + 1 < available && (text[i + 1] == 0 || text[i + 1] == 0x40))) {
            name[length] = '\0';
            return ComboBossSoulNameColor(name);
        }
        if (c == (mm ? 0x11 : 0x01)) {
            if (length && name[length - 1] != ' ')
                name[length++] = ' ';
        } else if ((c >= 0x20 && c < 0x7F) || c == 0x82) {
            if (c != ' ' || !length || name[length - 1] != ' ')
                name[length++] = (char)c;
        } else {
            return 0; // Pages, choices, buttons and every other control stay native.
        }
    }
    return 0;
}
#endif
