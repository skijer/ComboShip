// Export the production camera path for offline preview; no engine types or assets.
#include "combo/BlackMarketBrowsing.h"
#include <cstdio>
using namespace BlackMarketBrowsing;
void Print(const View& v) {
    std::printf("{\"eye\":[%g,%g,%g],\"at\":[%g,%g,%g],\"fov\":%g}",
        v.eye.x,v.eye.y,v.eye.z,v.at.x,v.at.y,v.at.z,v.fov);
}
int main() {
    std::printf("{\"center\":"); Print(Target(-1));
    std::printf(",\"right\":"); Print(Target(0));
    std::printf(",\"left\":"); Print(Target(4));
    std::printf(",\"slots\":[");
    for (int slot = 0; slot < 8; ++slot) {
      if (slot)
        std::printf(",");
      Print(Target(slot));
    }
    std::printf("]");
    std::printf(",\"frames\":[");
    State state;
    for (int i=0;i<60;++i) {
        if (i==8) state.Move(1,0);
        if (i==24) state.Move(-1,0);
        if (i==38) state.Move(-1,0);
        state.TickCamera();state.TickCamera(); // exported clip is 10fps; native updates at 20fps
        if (i) std::printf(","); Print(state.view);
    }
    std::puts("]}");
}
