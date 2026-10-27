#ifndef RUPEE_COUNTER_DIGITS_H
#define RUPEE_COUNTER_DIGITS_H

#include <stdint.h>

#define RUPEE_COUNTER_MAX_DIGITS 5

// Format the signed16 save field for drawing only. Negative/corrupt balances display
// zero; every nonnegative balance is shown in full, without changing currency or capacity.
static inline int16_t RupeeCounter_BuildDigits(int16_t balance, uint8_t minimumDigits,
                                               int16_t digits[RUPEE_COUNTER_MAX_DIGITS]) {
    int16_t remaining = balance < 0 ? 0 : balance;
    int16_t count = 1;
    for (int16_t value = remaining; value >= 10; value /= 10) {
        ++count;
    }
    if (minimumDigits > RUPEE_COUNTER_MAX_DIGITS) {
        minimumDigits = RUPEE_COUNTER_MAX_DIGITS;
    }
    if (count < minimumDigits) {
        count = minimumDigits;
    }
    for (int16_t i = count - 1; i >= 0; --i) {
        digits[i] = remaining % 10;
        remaining /= 10;
    }
    return count;
}

#endif
