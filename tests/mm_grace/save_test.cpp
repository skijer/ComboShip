#include "2s2h/BenJsonConversions.hpp"
#include "tests/test_require.h"
#include <cstdio>

int main() {
    RandoSaveInfo original{};
    for (unsigned i = 0; i < RO_MAX; ++i) original.randoSaveOptions[i] = i * 17;
    original.randoSaveOptions[RO_HYLIAS_GRACE] = RO_GRACE_GATED;
    original.randoSaveOptions[RO_HYLIAS_GRACE_REWARDS] = 7;
    json current = original;
    RandoSaveInfo restored{};
    current.get_to(restored);
    REQUIRE(restored.randoSaveOptions[RO_HYLIAS_GRACE] == RO_GRACE_GATED);
    REQUIRE(restored.randoSaveOptions[RO_HYLIAS_GRACE_REWARDS] == 7);
    json legacy = current;
    auto& options = legacy["randoSaveOptions"];
    options.erase(options.begin() + RO_HYLIAS_GRACE, options.end());
    legacy.get_to(restored); // Reuse a nonzero destination to catch stale tail values.
    for (unsigned i = 0; i < RO_HYLIAS_GRACE; ++i)
        REQUIRE(restored.randoSaveOptions[i] == original.randoSaveOptions[i]);
    REQUIRE(restored.randoSaveOptions[RO_HYLIAS_GRACE] == RO_GRACE_ON);
    REQUIRE(restored.randoSaveOptions[RO_HYLIAS_GRACE_REWARDS] == 0);
    current["randoSaveOptions"].push_back(99); // Forward-compatible unknown tail.
    current.get_to(restored);
    REQUIRE(restored.randoSaveOptions[RO_HYLIAS_GRACE_REWARDS] == 7);
    puts("PASS real MM save JSON: legacy option arrays load, new policy roundtrips and unknown tail is ignored");
}
