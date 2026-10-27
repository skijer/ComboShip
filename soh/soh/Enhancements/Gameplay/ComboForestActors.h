#pragma once
#include <stdint.h>
#define COMBO_SOLO_FOREST_EPONA_PARAMS 0x7E40
#ifdef COMBO_BUILD
struct Actor;
struct PlayState;
#ifdef __cplusplus
extern "C" {
#endif
void ComboForestActors_DrawSariaParticles(struct Actor* actor, struct PlayState* play, int seated);
#ifdef __cplusplus
}
#endif
#endif
#ifdef __cplusplus

namespace ComboForestActors {
constexpr int kViewer = 0x002A;
constexpr int kSaria = 0x7F13;
constexpr int kSkullKid = 0x7E08;
constexpr int kHorse = COMBO_SOLO_FOREST_EPONA_PARAMS;
// Readiness is tested before looking at authored actors or consuming a visit.
struct Visit {
    int room = -1;
    const void* segment = nullptr;
    unsigned complete = 0;
    void Reset() {
        room = -1;
        segment = nullptr;
        complete = 0;
    }
    bool Ready(int nextRoom, const void* nextSegment, int status) {
        if (status != 0 || !nextSegment)
            return false;
        if (room != nextRoom || segment != nextSegment) {
            room = nextRoom;
            segment = nextSegment;
            complete = 0;
        }
        return true;
    }
    template <class Present, class Spawn> void Ensure(unsigned bit, Present present, Spawn spawn) {
        if (!(complete & bit) && (present() || spawn()))
            complete |= bit;
    }
};
} // namespace ComboForestActors

#endif
