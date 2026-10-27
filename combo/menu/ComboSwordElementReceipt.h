#ifndef COMBO_SWORD_ELEMENT_RECEIPT_H
#define COMBO_SWORD_ELEMENT_RECEIPT_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

// These are each engine's native red, light-blue and yellow text controls.
// Keep the tutorial outside auto-formatting, with an actual A-button glyph.
#define CW_SWORD_RECEIPT_OOT(color, element) \
    "\004\010You can now enchant with "      \
    "\005" color element "\005\100!"         \
    "\001Equipment: select a sword, then"    \
    "\001press \237 to choose an element."   \
    "\001Enchanted strikes cost no magic.\011\002"
#define CW_SWORD_RECEIPT_MM(color, element)           \
    "\020\027In Equipment, select a sword."           \
    "\021Press \260 to choose " color element "\000." \
    "\021Enchanted strikes cost no magic.\030\277"

// A no-op returns the original length and does not write any byte. Accept only
// ordinary receipts: choices, scripted endings and chained text keep their
// complete native flow. Control arguments must never be mistaken for endings.
static inline size_t CwSwordElementReceipt_Append(uint8_t* buffer, size_t length, size_t capacity, int mm,
                                                  uint8_t element) {
    static const struct {
        const char* bytes;
        size_t length;
    } suffixes[2][3] = {
        { { CW_SWORD_RECEIPT_OOT("\101", "Fire"), sizeof(CW_SWORD_RECEIPT_OOT("\101", "Fire")) - 1 },
          { CW_SWORD_RECEIPT_OOT("\104", "Ice"), sizeof(CW_SWORD_RECEIPT_OOT("\104", "Ice")) - 1 },
          { CW_SWORD_RECEIPT_OOT("\106", "Light"), sizeof(CW_SWORD_RECEIPT_OOT("\106", "Light")) - 1 } },
        { { CW_SWORD_RECEIPT_MM("\001", "Fire"), sizeof(CW_SWORD_RECEIPT_MM("\001", "Fire")) - 1 },
          { CW_SWORD_RECEIPT_MM("\005", "Ice"), sizeof(CW_SWORD_RECEIPT_MM("\005", "Ice")) - 1 },
          { CW_SWORD_RECEIPT_MM("\004", "Light"), sizeof(CW_SWORD_RECEIPT_MM("\004", "Light")) - 1 } }
    };
    size_t pos = mm ? 11 : 0;
    const uint8_t end = mm ? 0xBF : 0x02;
    if (!buffer || element < 1 || element > 3 || length > capacity || pos >= length)
        return length;
    if (mm && (buffer[3] != 0xFF || buffer[4] != 0xFF))
        return length;
    while (pos < length) {
        const uint8_t code = buffer[pos];
        size_t arguments = 0;
        // CheckQueue's ordinary overhead receipt uses precisely this fade
        // trailer. Keep its timer and ending on the new final page; all other
        // scripted fades remain untouched.
        const int overheadFade = mm && code == 0x1C && length - pos == 4 && buffer[pos + 1] == 2 &&
                                 buffer[pos + 2] == 0x10 && buffer[pos + 3] == end;
        if (code == end || overheadFade) {
            for (size_t tail = overheadFade ? length : pos + 1; tail < length; ++tail)
                if (buffer[tail] != 0)
                    return length;
            const size_t added = suffixes[mm != 0][element - 1].length;
            const size_t total = added + (overheadFade ? 3 : 0);
            if (total > capacity - pos)
                return length;
            const uint8_t fade[] = { 0x1C, 2, 0x10, 0xBF };
            memcpy(buffer + pos, suffixes[mm != 0][element - 1].bytes, added - (overheadFade ? 1 : 0));
            if (overheadFade)
                memcpy(buffer + pos + added - 1, fade, sizeof(fade));
            return pos + total;
        }
        if (mm) {
            if (code == 0x15 || code == 0x19 || code == 0x1A || code == 0x1C || code == 0x1D || code >= 0xC1)
                return length;
            if (code == 0x14)
                arguments = 1;
            else if (code == 0x1B || code == 0x1E || code == 0x1F)
                arguments = 2;
        } else {
            if (code == 0 || code == 0x07 || code == 0x0A || code == 0x0B || code == 0x0E || code == 0x10 ||
                code == 0x11 || code == 0x1B || code == 0x1C)
                return length;
            if (code == 0x05 || code == 0x06 || code == 0x0C || code == 0x13 || code == 0x14 || code == 0x1E)
                arguments = 1;
            else if (code == 0x12)
                arguments = 2;
            else if (code == 0x15)
                arguments = 3;
        }
        if (arguments >= length - pos)
            return length;
        pos += arguments + 1;
    }
    return length;
}

#undef CW_SWORD_RECEIPT_OOT
#undef CW_SWORD_RECEIPT_MM
#endif
