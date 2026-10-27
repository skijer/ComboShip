#include "2s2h/BenJsonConversions.hpp"
#include <cstdlib>
#define CHECK(x)                                    \
    do {                                            \
        if (!(x)) {                                 \
            std::fprintf(stderr, "FAIL: %s\n", #x); \
            std::exit(1);                           \
        }                                           \
    } while (0)

int main() {
    NeiSaveData source{};
    source.comboSariaSongClaimed = 1;
    source.seasonsGates = 7;
    json encoded = source;
    CHECK(encoded.at("comboSariaSongClaimed") == 1);
    auto restored = encoded.get<NeiSaveData>();
    CHECK(restored.comboSariaSongClaimed == 1 && restored.seasonsGates == 7);
    encoded.erase("comboSariaSongClaimed");
    from_json(encoded, restored);
    CHECK(restored.comboSariaSongClaimed == 0 && restored.seasonsGates == 7);
    puts("PASS MM Saria claim save: actual JSON roundtrip, independent completion flag, old-save default");
}
