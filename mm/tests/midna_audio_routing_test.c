#include <string.h>
#include "tests/test_require.h"
#include "src/overlays/actors/ovl_En_Elf/z_en_elf.h"
#include "2s2h/Enhancements/Companion/MidnaAudio.h"
#include "2s2h/GameInteractor/GameInteractor.h"

Vec3f gSfxDefaultPos;
f32 gSfxDefaultFreqAndVolScale = 1;
s8 gSfxDefaultReverb;
static u16 sCUpInvisible, sCUpTimer;
static int nativeCalls, customCalls, clipPresent, disableCalls;
static u16 nativeId;
static MMMidnaAudioEvent customEvent;
static int idleUpdates, idleEligible, playerCutscene, enabled = 1, resets, lightRemovals;

void MMMidnaAudio_UpdateIdle(bool eligible) {
    ++idleUpdates;
    idleEligible = eligible;
}
bool Play_InCsMode(PlayState* play) {
    return play->csCtx.state != CS_STATE_IDLE || playerCutscene;
}
u8 Message_GetState(MessageContext* ctx) {
    return ctx->msgMode == MSGMODE_NONE ? TEXT_STATE_NONE : TEXT_STATE_EVENT;
}

int32_t CVarGetInteger(const char* name, int32_t fallback) {
    return enabled;
}
bool MMMidnaAudio_TryPlay(MMMidnaAudioEvent event) {
    ++customCalls;
    customEvent = event;
    return enabled && clipPresent;
}
void Audio_PlaySfx(u16 id) {
    ++nativeCalls;
    nativeId = id;
}
void Audio_PlaySfx_AtPosWithReverb(Vec3f* pos, u16 id, s8 duration) {
    REQUIRE(pos == &gSfxDefaultPos && duration == 32);
    ++nativeCalls;
    nativeId = id;
}
void Actor_PlaySfx(Actor* actor, u16 id) {
    ++nativeCalls;
    nativeId = id;
}

bool GameInteractor_Should(GIVanillaBehavior flag, uint32_t result, ...) {
    REQUIRE(flag == VB_PLAY_TATL_CALL_AUDIO && result);
    return !disableCalls;
}
void MMMidnaAudio_Reset(void) {
    ++resets;
}
void LightContext_RemoveLight(PlayState* play, LightContext* ctx, LightNode* node) {
    ++lightRemovals;
}
f32 Math_Vec3f_DistXYZ(Vec3f* a, Vec3f* b) {
    return 0;
}
s32 Math_StepToF(f32* value, f32 target, f32 step) {
    *value = target;
    return true;
}
/* PRODUCTION_MIDNA_AUDIO_ROUTING */

static void idleGates(void) {
    static PlayState play;
    Player player = { 0 };
    EnElf fairy = { 0 };
    Actor npc = { 0 };
    npc.category = ACTORCAT_NPC;
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    player.actor.bgCheckFlags = 1;
    fairy.actor.params = FAIRY_TYPE_0;
    fairy.actor.scale.x = .008f;
    fairy.innerColor.a = 255;
    EnElf_UpdateMidnaIdleAudio(&fairy, &play);
    REQUIRE(idleUpdates == 1 && idleEligible);
    play.actorCtx.attention.tatlHoverActor = &npc;
    EnElf_UpdateMidnaIdleAudio(&fairy, &play);
    REQUIRE(idleEligible); // passive NPC proximity keeps Tatl out in native mode 0
    player.focusActor = &npc;
    EnElf_UpdateMidnaIdleAudio(&fairy, &play);
    REQUIRE(!idleEligible); // actively locking onto that same NPC pauses idle time
    player.focusActor = NULL;
    play.actorCtx.attention.tatlHoverActor = NULL;

#define BLOCKED(field, value)                      \
    do {                                           \
        __typeof__(field) saved = field;           \
        field = value;                             \
        EnElf_UpdateMidnaIdleAudio(&fairy, &play); \
        REQUIRE(!idleEligible);                    \
        field = saved;                             \
        EnElf_UpdateMidnaIdleAudio(&fairy, &play); \
        REQUIRE(idleEligible);                     \
    } while (0)
    BLOCKED(player.actor.speed, 2.0f);
    BLOCKED(player.actor.speed, -2.0f);
    BLOCKED(player.speedXZ, 2.0f);
    BLOCKED(player.actor.bgCheckFlags, 0);
    BLOCKED(player.rideActor, &npc);
    BLOCKED(player.currentMask, PLAYER_MASK_GIANT);
    BLOCKED(player.stateFlags1, PLAYER_STATE1_TALKING);
    BLOCKED(player.stateFlags1, PLAYER_STATE1_PARALLEL);
    BLOCKED(player.stateFlags1, PLAYER_STATE1_100000);
    BLOCKED(player.focusActor, &fairy.actor);
    BLOCKED(fairy.unk_244, 1); // native enemy/non-NPC attention
    BLOCKED(fairy.unk_244, 5); // recall
    BLOCKED(fairy.unk_244, 6); // hidden
    BLOCKED(fairy.unk_244, 9); // emergence
    BLOCKED(fairy.fairyFlags, 8);
    BLOCKED(fairy.actor.scale.x, .004f);
    BLOCKED(fairy.innerColor.a, 0);
    BLOCKED(fairy.unk_269, 1);
    BLOCKED(play.pauseCtx.state, 6);
    BLOCKED(play.msgCtx.msgMode, MSGMODE_TEXT_DISPLAYING);
    BLOCKED(play.csCtx.state, CS_STATE_RUN);
    BLOCKED(playerCutscene, 1);
    BLOCKED(play.transitionTrigger, TRANS_TRIGGER_START);
    BLOCKED(play.transitionMode, TRANS_MODE_INSTANT);
    BLOCKED(play.gameOverCtx.state, GAMEOVER_DEATH_START);
#undef BLOCKED
    int before = idleUpdates;
    fairy.actor.params = FAIRY_TYPE_3;
    EnElf_UpdateMidnaIdleAudio(&fairy, &play);
    REQUIRE(idleUpdates == before); // ordinary fairies cannot tick/cancel Tatl's idle state
}

int main(void) {
    static PlayState play;
    Player player = { 0 };
    EnElf companion = { 0 };
    companion.actor.id = ACTOR_EN_ELF;
    companion.actor.params = FAIRY_TYPE_0;
    player.tatlActor = &companion.actor;
    const u16 calls[] = { TATL_STATE_2B, TATL_STATE_2A };
    const u16 native[] = { NA_SE_VO_NAVY_CALL, NA_SE_VO_NA_HELLO_2 };
    const MMMidnaAudioEvent events[] = { MM_MIDNA_AUDIO_CALL, MM_MIDNA_AUDIO_HINT };
    for (int i = 0; i < 2; ++i) {
        for (clipPresent = 0; clipPresent <= 1; ++clipPresent) {
            memset(&play, 0, sizeof(play));
            play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
            nativeCalls = customCalls = disableCalls = 0;
            sCUpInvisible = 1;
            Interface_SetTatlCall(&play, calls[i]);
            REQUIRE(customCalls == 1 && customEvent == events[i]);
            REQUIRE(nativeCalls == !clipPresent);
            if (!clipPresent)
                REQUIRE(nativeId == native[i]);
            REQUIRE(play.interfaceCtx.tatlCalling && sCUpInvisible == 0 && sCUpTimer == 10);
            Interface_SetTatlCall(&play, calls[i]);
            REQUIRE(customCalls == 1); // repeated HUD polling must not retrigger
            Interface_SetTatlCall(&play, TATL_STATE_2C);
            REQUIRE(!play.interfaceCtx.tatlCalling);
        }
    }
    memset(&play, 0, sizeof(play));
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    nativeCalls = customCalls = 0;
    disableCalls = 1;
    Interface_SetTatlCall(&play, TATL_STATE_2B);
    REQUIRE(customCalls == 0 && nativeCalls == 0 && play.interfaceCtx.tatlCalling);
    disableCalls = 0;
    memset(&play, 0, sizeof(play));
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    play.csCtx.state = CS_STATE_RUN;
    Interface_SetTatlCall(&play, TATL_STATE_2A);
    REQUIRE(customCalls == 0 && nativeCalls == 0 && !play.interfaceCtx.tatlCalling);

    EnElf fairy = { 0 };
    enabled = clipPresent = 1;
    for (int type = FAIRY_TYPE_0; type <= FAIRY_TYPE_10; ++type) {
        fairy.actor.params = type;
        nativeCalls = customCalls = resets = lightRemovals = 0;
        EnElf_PlayTatlSound(&fairy, MM_MIDNA_AUDIO_DASH, NA_SE_EV_BELL_DASH_NORMAL);
        REQUIRE(customCalls == (type == FAIRY_TYPE_0));
        REQUIRE(nativeCalls == (type != FAIRY_TYPE_0));
        if (nativeCalls)
            REQUIRE(nativeId == NA_SE_EV_BELL_DASH_NORMAL);
        EnElf_Destroy(&fairy.actor, &play);
        REQUIRE(resets == (type == FAIRY_TYPE_0) && lightRemovals == 2);
    }
    for (int active = 0; active < 2; ++active) {
        enabled = active;
        for (clipPresent = 0; clipPresent < 2; ++clipPresent) {
            fairy.actor.params = FAIRY_TYPE_0;
            nativeCalls = customCalls = 0;
            EnElf_PlayTatlSound(&fairy, MM_MIDNA_AUDIO_VANISH, NA_SE_EV_NAVY_VANISH);
            REQUIRE(customCalls == 1 && customEvent == MM_MIDNA_AUDIO_VANISH);
            REQUIRE(nativeCalls == !(enabled && clipPresent));
            if (nativeCalls)
                REQUIRE(nativeId == NA_SE_EV_NAVY_VANISH);
        }
    }
    enabled = clipPresent = 1;
    /* MM's silent native target cue is preserved on fallback. */
    memset(&play, 0, sizeof(play));
    play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
    Actor target = { 0 };
    play.actorCtx.attention.tatlHoverActor = player.focusActor = &target;
    const u8 categories[] = { ACTORCAT_NPC, ACTORCAT_ENEMY, ACTORCAT_PROP };
    const MMMidnaAudioEvent eventsTarget[] = { MM_MIDNA_AUDIO_TARGET_NPC, MM_MIDNA_AUDIO_TARGET_ENEMY,
                                               MM_MIDNA_AUDIO_TARGET_OTHER };
    for (int i = 0; i < 3; ++i) {
        target.category = categories[i];
        memset(&fairy, 0, sizeof(fairy));
        fairy.actor.params = FAIRY_TYPE_0;
        fairy.unk_268 = 1;
        nativeCalls = customCalls = 0;
        func_8088EFA4(&fairy, &play);
        REQUIRE(customCalls == 1 && customEvent == eventsTarget[i] && nativeCalls == 0);
        func_8088EFA4(&fairy, &play);
        REQUIRE(customCalls == 1); // held lock-on never retriggers
    }
    enabled = 0;
    fairy.fairyFlags = 0;
    nativeCalls = customCalls = 0;
    func_8088EFA4(&fairy, &play);
    REQUIRE(nativeCalls == 1 && nativeId == NA_SE_NONE);
    enabled = 1;
    player.focusActor = NULL;
    /* HUD polling with a non-companion pointer cannot trigger private audio. */
    companion.actor.params = FAIRY_TYPE_3;
    play.interfaceCtx.tatlCalling = false;
    nativeCalls = customCalls = 0;
    Interface_SetTatlCall(&play, TATL_STATE_2B);
    REQUIRE(customCalls == 0 && nativeCalls == 1 && nativeId == NA_SE_VO_NAVY_CALL);

    idleGates();
    puts("PASS: Tatl actor/HUD routing, native arguments, call mute and cutscene gates");
}
