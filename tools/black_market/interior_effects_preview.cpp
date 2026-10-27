// Export production effect poses; no ROM pixels or renderer implementation.
#include "global.h"
#include "combo/BlackMarketInteriorEffects.h"
#include <cstdio>
#include <cstdlib>
int main(int argc, char** argv) {
    using namespace BlackMarketInteriorEffects;
    const uint32_t frame = argc > 1 ? std::strtoul(argv[1], nullptr, 10) : 0;
    std::printf("{\"torches\":[");
    for (size_t i = 0; i < kTorches.size(); ++i) {
        const auto p = kTorches[i];
        std::printf("%s[%g,%g,%g,%g]", i ? "," : "", p.x,p.y,p.z,kFlameScale);
    }
    std::printf("],\"prim\":[%u,%u,%u,%u],\"env\":[%u,%u,%u,%u],\"puffs\":[",
                kFlamePrim[0],kFlamePrim[1],kFlamePrim[2],kFlamePrim[3],kFlameEnv[0],kFlameEnv[1],kFlameEnv[2],kFlameEnv[3]);
    for (int pot = 0; pot < int(kPotMouths.size()); ++pot)
        for (int puff = 0; puff < kPuffsPerPot; ++puff) {
            const auto p = Vapor(pot,puff,frame);
            std::printf("%s[%g,%g,%g,%g,%g,%u]", pot || puff ? "," : "",p.center.x,p.center.y,p.center.z,p.halfWidth,p.halfHeight,p.alpha);
        }
    std::puts("]}");
}
