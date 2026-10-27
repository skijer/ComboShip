// Native MM backend. Reuse CheckQueue for native/foreign draw, receipts and persistent gives.
#include "2s2h/Rando/Rando.h"
extern "C" void Rando_CancelMagicMirrorStump(void);
namespace MirrorShuffle {
static int sRewardToken = 0, sRewardSlot = -1;
static uint32_t sRewardSeed = 0;
void CancelStumpReward() {
    sRewardToken = 0;
    // SaveContext is live storage even at boot/teardown; never touch an old actor
    // or clear an eligibility bit belonging to a newly bound save.
    if (sRewardSlot == gSaveContext.fileNum && sRewardSeed == gSaveContext.save.shipSaveInfo.rando.finalSeed)
        RANDO_SAVE_CHECKS[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].eligible = false;
    sRewardSlot = -1;
    sRewardSeed = 0;
    Rando_CancelMagicMirrorStump();
}
bool Enabled() {
    return IS_RANDO && RANDO_SAVE_OPTIONS[RO_SHUFFLE_MAGIC_MIRROR] == RO_GENERIC_ON;
}
bool StumpAvailable() {
    return Enabled() && !RANDO_SAVE_CHECKS[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].obtained &&
           !RANDO_SAVE_CHECKS[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].eligible;
}
bool QueueStumpReward(int token) {
    if (!token || !StumpAvailable())
        return false;
    sRewardToken = token;
    sRewardSlot = gSaveContext.fileNum;
    sRewardSeed = gSaveContext.save.shipSaveInfo.rando.finalSeed;
    RANDO_SAVE_CHECKS[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].eligible = true;
    return true;
}
void DrawStumpReward(PlayState* play) {
    if (!StumpAvailable())
        return;
    const auto raw = RANDO_SAVE_CHECKS[RC_LOST_WOODS_MAGIC_MIRROR_STUMP].randoItemId;
    // CheckQueue applies 30 over an item actor's .015 matrix for ordinary GI models.
    // Mirror's custom model is already world-sized; the same callback adds its leaves.
    const float scale = raw == RI_OOT_NEI_MAGIC_MIRROR ? 1.0f : 0.45f;
    Matrix_Scale(scale, scale, scale, MTXMODE_APPLY);
    Rando::DrawItem(raw, RC_LOST_WOODS_MAGIC_MIRROR_STUMP);
}
} // namespace MirrorShuffle
extern "C" bool MagicMirror_CommitStumpRewardToken(void) {
    const int token = MirrorShuffle::sRewardToken;
    MirrorShuffle::sRewardToken = 0;
    return MirrorShuffle::Enabled() && MirrorRuntime::FinishPickup(token);
}
