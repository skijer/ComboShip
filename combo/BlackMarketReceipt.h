#pragma once
#include <cstdint>

// One outstanding shop receipt. A cancelled queued actor cannot acknowledge a
// later purchase: each offer carries its own nonzero native s16 ticket.
namespace BlackMarketReceipt {
struct State {
    int slot = -1, file = -1, price = 0;
    uint32_t seed = 0;
    uint16_t ticket = 0;
    bool pending = false, delivered = false, offered = false;
    void Cancel() {
        slot = file = -1;
        pending = delivered = offered = false;
    }
    bool Begin(int shelf, int save, uint32_t generation, int cost) {
        if (pending)
            return false;
        ticket = ticket == 32767 ? 1 : ticket + 1;
        slot = shelf;
        file = save;
        seed = generation;
        price = cost;
        pending = true;
        delivered = offered = false;
        return true;
    }
    bool Matches(int save, uint32_t generation, int receipt) const {
        return pending && file == save && seed == generation && ticket == receipt;
    }
    bool Acknowledge(int save, uint32_t generation, int receipt) {
        if (!Matches(save, generation, receipt) || delivered)
            return false;
        delivered = true;
        return true;
    }
};
} // namespace BlackMarketReceipt
