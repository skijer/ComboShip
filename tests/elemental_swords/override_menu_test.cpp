#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
static int flags[3];
static int writes;
static const char* keys[] = { "gEnhancements.DinFireSwordDamage", "gEnhancements.IceSwordDamage",
                              "gEnhancements.LightSwordDamage" };
extern "C" int32_t CVarGetInteger(const char* key, int32_t fallback) {
    for (int i = 0; i < 3; ++i) if (!strcmp(keys[i], key)) return flags[i];
    return fallback;
}
extern "C" void CVarSetInteger(const char* key, int32_t value) {
    for (int i = 0; i < 3; ++i) if (!strcmp(keys[i], key)) { flags[i] = value; ++writes; }
}
#if __has_include("gameplay/SwordElementOverride.h")
#include "gameplay/SwordElementOverride.h"
#else
static void CwSwordElementOverride_Changed(const char*) {}
#endif
int main() {
    for (int selected = 0; selected < 3; ++selected) {
        flags[0] = flags[1] = flags[2] = 1;
        writes = 0;
        CwSwordElementOverride_Changed(keys[selected]);
        for (int i = 0; i < 3; ++i) assert(flags[i] == (i == selected));
        assert(writes == 2);
        flags[selected] = 0;
        // Historical preferences can contain several enabled flags. Turning
        // the effective override off must not reveal a different hidden cheat.
        for (int i = 0; i < 3; ++i) if (i != selected) flags[i] = 1;
        writes = 0;
        CwSwordElementOverride_Changed(keys[selected]);
        assert(writes == 2);
        for (int i = 0; i < 3; ++i) assert(flags[i] == 0);
    }
    writes = 0;
    CwSwordElementOverride_Changed(nullptr);
    CwSwordElementOverride_Changed("unrelated.option");
    assert(writes == 0);
    puts("PASS override menu: last enabled element wins, off restores saved path, unrelated options untouched");
}
