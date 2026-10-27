#include <string.h>
#include "tests/test_require.h"
#include "src/overlays/actors/ovl_En_Viewer/z_en_viewer.h"
#include "src/overlays/actors/ovl_En_Viewer/static_story_actor.h"
#include "variables.h"

void EnViewerStatic_OfferTalk(EnViewer* viewer, PlayState* play);
SaveContext gSaveContext;
u32 gBitFlags[32];
#ifdef COMBO_BUILD
/* The cross-boss service is outside the dialogue test; keep native progression. */
int Combo_SOH_CrossBossLogicalClear(int id, int nativeValue) {
    (void)id;
    return nativeValue;
}
#endif
static unsigned processCalls, offerCalls, stateCalls, advanceCalls, closeCalls, flagCalls;
static bool acceptTalk;
static bool advanceTalk;
static u8 messageState = TEXT_STATE_NONE;
static f32 offeredDistance;
u32 Actor_ProcessTalkRequest(Actor* actor, PlayState* play) {
    (void)actor;
    (void)play;
    processCalls++;
    return acceptTalk;
}
s32 Actor_OfferTalk(Actor* actor, PlayState* play, f32 distance) {
    (void)actor;
    (void)play;
    offeredDistance = distance;
    offerCalls++;
    return 0;
}
u8 Message_GetState(MessageContext* msgCtx) {
    (void)msgCtx;
    stateCalls++;
    return messageState;
}
u8 Message_ShouldAdvance(PlayState* play) {
    (void)play;
    advanceCalls++;
    return advanceTalk;
}
void Message_CloseTextbox(PlayState* play) {
    (void)play;
    closeCalls++;
}
s32 Flags_GetEventChkInf(s32 flag) {
    (void)flag;
    flagCalls++;
    return 0;
}
int main(void) {
    EnViewer viewer;
    PlayState play;
    memset(&play, 0, sizeof(play));
    const StaticStoryActorType ambient[] = { STATIC_STORY_ACTOR_PHANTOM_GANON };
    for (unsigned i = 0; i < sizeof(ambient) / sizeof(ambient[0]); ++i) {
        memset(&viewer, 0, sizeof(viewer));
        viewer.staticState.type = ambient[i];
        viewer.staticState.talking = true;
        viewer.staticState.tracking = true;
        viewer.staticState.interactInfo.talkState = NPC_TALK_STATE_TALKING;
        viewer.actor.textId = 0x7777;
        EnViewerStatic_OfferTalk(&viewer, &play);
        REQUIRE(viewer.actor.textId == 0);
        REQUIRE(!viewer.staticState.talking);
        REQUIRE(!viewer.staticState.tracking);
        REQUIRE(viewer.staticState.interactInfo.talkState == NPC_TALK_STATE_IDLE);
        REQUIRE(processCalls == 0 && offerCalls == 0 && stateCalls == 0 && advanceCalls == 0 && closeCalls == 0);
        REQUIRE(flagCalls == 0);
    }

    memset(&viewer, 0, sizeof(viewer));
    viewer.staticState.type = STATIC_STORY_ACTOR_ANJU;
    viewer.actor.world.pos = (Vec3f){ 10, 123, 30 };
    viewer.actor.shape.rot.y = 12000;
    AnimationHeader seatedAnimation = { 0 };
    viewer.skin.skelAnime.animation = &seatedAnimation;
    EnViewerStatic_OfferTalk(&viewer, &play);
    REQUIRE(viewer.actor.textId == 0x8F26 && offeredDistance == 90.0f);
    REQUIRE(!viewer.staticState.talking);
    acceptTalk = true;
    EnViewerStatic_OfferTalk(&viewer, &play);
    REQUIRE(viewer.staticState.talking && viewer.staticState.interactInfo.talkState == NPC_TALK_STATE_TALKING);
    messageState = TEXT_STATE_CLOSING;
    EnViewerStatic_OfferTalk(&viewer, &play);
    REQUIRE(!viewer.staticState.talking && viewer.staticState.interactInfo.talkState == NPC_TALK_STATE_IDLE);
    REQUIRE(viewer.skin.skelAnime.animation == &seatedAnimation && viewer.staticState.pose == 0);
    REQUIRE(viewer.actor.world.pos.x == 10 && viewer.actor.world.pos.y == 123 && viewer.actor.world.pos.z == 30);
    REQUIRE(viewer.actor.shape.rot.y == 12000);

    /* Every talkable cutscene-actor entry must acknowledge EVENT and preserve
     * its authored pose. Ordinary dialogue must not enter an item grant path. */
    unsigned checked = 0;
    for (int type = 0; type < STATIC_STORY_ACTOR_MAX; ++type) {
        if (!StaticStoryActor_CanTalk(type)) {
            continue;
        }
        memset(&viewer, 0, sizeof(viewer));
        viewer.staticState.type = type;
        viewer.staticState.pose = 1;
        viewer.skin.skelAnime.animation = &seatedAnimation;
        viewer.actor.world.pos = (Vec3f){ 10, 123, 30 };
        viewer.actor.shape.rot.y = 12000;
        acceptTalk = false;
        advanceTalk = false;
        messageState = TEXT_STATE_NONE;
        const unsigned offered = offerCalls;
        EnViewerStatic_OfferTalk(&viewer, &play);
        REQUIRE(viewer.actor.textId != 0 && offerCalls == offered + 1);
        REQUIRE(!viewer.staticState.talking);
        acceptTalk = true;
        EnViewerStatic_OfferTalk(&viewer, &play);
        REQUIRE(viewer.staticState.talking);
        messageState = TEXT_STATE_EVENT;
        const unsigned closed = closeCalls;
        EnViewerStatic_OfferTalk(&viewer, &play);
        REQUIRE(viewer.staticState.talking && closeCalls == closed);
        advanceTalk = true;
        EnViewerStatic_OfferTalk(&viewer, &play);
        REQUIRE(closeCalls == closed + 1 && !viewer.staticState.talking && !viewer.staticState.tracking);
        REQUIRE(viewer.staticState.interactInfo.talkState == NPC_TALK_STATE_IDLE);
        REQUIRE(viewer.skin.skelAnime.animation == &seatedAnimation && viewer.staticState.pose == 1);
        REQUIRE(viewer.actor.world.pos.x == 10 && viewer.actor.world.pos.y == 123 && viewer.actor.world.pos.z == 30);
        REQUIRE(viewer.actor.shape.rot.y == 12000);
        ++checked;
    }
    REQUIRE(checked > 0);
    /* Plain child/adult poses and the blue Chamber variants offer only A-talk.
     * Native proximity, story actions and automatic talk are never involved. */
    const struct {
        StaticStoryActorType type;
        u8 pose;
        u16 text;
    } zelda[] = {
        { STATIC_STORY_ACTOR_CHILD_ZELDA, 0, 0x8F2D }, { STATIC_STORY_ACTOR_CHILD_ZELDA, 1, 0x8F2D },
        { STATIC_STORY_ACTOR_CHILD_ZELDA, 2, 0x8F2B }, { STATIC_STORY_ACTOR_ADULT_ZELDA, 0, 0x70FF },
        { STATIC_STORY_ACTOR_ADULT_ZELDA, 1, 0x70FF }, { STATIC_STORY_ACTOR_ADULT_ZELDA, 2, 0x70FF },
        { STATIC_STORY_ACTOR_ADULT_ZELDA, 3, 0x8F2C },
    };
    for (unsigned i = 0; i < sizeof(zelda) / sizeof(zelda[0]); ++i) {
        memset(&viewer, 0, sizeof(viewer));
        viewer.staticState.type = zelda[i].type;
        viewer.staticState.pose = zelda[i].pose;
        viewer.skin.skelAnime.animation = &seatedAnimation;
        viewer.actor.world.pos = (Vec3f){ 10, 123, 30 };
        viewer.actor.shape.rot.y = 12000;
        acceptTalk = advanceTalk = false;
        messageState = TEXT_STATE_NONE;
        SaveContext before = gSaveContext;
        EnViewerStatic_OfferTalk(&viewer, &play);
        EnViewerStatic_OfferTalk(&viewer, &play);
        REQUIRE(viewer.actor.textId == zelda[i].text && !viewer.staticState.talking);
        REQUIRE(!(viewer.actor.flags & ACTOR_FLAG_TALK_OFFER_AUTO_ACCEPTED));
        acceptTalk = true;
        EnViewerStatic_OfferTalk(&viewer, &play);
        REQUIRE(viewer.staticState.talking);
        messageState = TEXT_STATE_EVENT;
        const unsigned closed = closeCalls;
        EnViewerStatic_OfferTalk(&viewer, &play);
        REQUIRE(closeCalls == closed && viewer.staticState.talking);
        advanceTalk = true;
        EnViewerStatic_OfferTalk(&viewer, &play);
        REQUIRE(closeCalls == closed + 1 && !viewer.staticState.talking);
        REQUIRE(viewer.staticState.pose == zelda[i].pose && viewer.skin.skelAnime.animation == &seatedAnimation);
        REQUIRE(viewer.actor.world.pos.x == 10 && viewer.actor.world.pos.y == 123 && viewer.actor.world.pos.z == 30);
        REQUIRE(viewer.actor.shape.rot.y == 12000 && memcmp(&before, &gSaveContext, sizeof(before)) == 0);
    }
    printf("PASS cutscene-actor talk: %u catalog entries offer, wait, acknowledge EVENT and preserve pose\n", checked);
    return 0;
}
