// Native OoT backend for the standalone shuffled stump. Included by MagicMirror.cpp.
#include "soh/Enhancements/randomizer/static_data.h"
#include "soh/Enhancements/randomizer/item_location.h"
extern "C" bool Randomizer_QueueMagicMirrorStump(void);
extern "C" void Randomizer_CancelMagicMirrorStump(void);
extern "C" bool Randomizer_MagicMirrorStumpQueued(void);
namespace MirrorShuffle {
static int sRewardToken = 0;
void CancelStumpReward() {
    sRewardToken = 0;
    Randomizer_CancelMagicMirrorStump();
}
bool Enabled() {
    return IS_RANDO && Rando::Context::GetInstance()->GetOption(RSK_SHUFFLE_MAGIC_MIRROR).Is(RO_GENERIC_ON);
}
bool StumpAvailable() {
    return Enabled() && !Combo_IsCombinedGame() && !Flags_GetRandomizerInf(RAND_INF_MAGIC_MIRROR_STUMP) &&
           !Randomizer_MagicMirrorStumpQueued();
}
bool QueueStumpReward(int token) {
    if (!token || !StumpAvailable())
        return false;
    if (!Randomizer_QueueMagicMirrorStump())
        return false;
    sRewardToken = token;
    return true;
}
void DrawStumpReward(PlayState* play) {
    if (!StumpAvailable())
        return;
    auto entry = Rando::Context::GetInstance()->GetFinalGIEntry(RC_LOST_WOODS_MAGIC_MIRROR_STUMP, false, GI_NONE, true);
    // Native GI scale; Mirror's receipt callback contributes its normal 2.25 factor.
    Matrix_Scale(0.45f, 0.45f, 0.45f, MTXMODE_APPLY);
    GetItemEntry_Draw(play, entry);
}
} // namespace MirrorShuffle
extern "C" bool MagicMirror_CommitStumpRewardToken(void) {
    const int token = MirrorShuffle::sRewardToken;
    MirrorShuffle::sRewardToken = 0;
    return MirrorShuffle::Enabled() && MirrorRuntime::FinishPickup(token);
}
