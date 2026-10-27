#include <stdbool.h>
#include <stdint.h>

#include "test_require.h"
#include "../src/overlays/actors/ovl_En_Viewer/static_story_talk.h"

typedef struct {
    StaticStoryTalkMessageState messageState;
    bool advance;
    bool process;
    bool offer;
    unsigned getStateCalls;
    unsigned advanceCalls;
    unsigned closeCalls;
    unsigned processCalls;
    unsigned offerCalls;
    float offeredDistance;
} TalkFixture;

static StaticStoryTalkMessageState FixtureGetState(void* context) {
    TalkFixture* fixture = context;
    fixture->getStateCalls++;
    return fixture->messageState;
}

static bool FixtureShouldAdvance(void* context) {
    TalkFixture* fixture = context;
    fixture->advanceCalls++;
    return fixture->advance;
}

static void FixtureClose(void* context) {
    ((TalkFixture*)context)->closeCalls++;
}

static bool FixtureProcess(void* context) {
    TalkFixture* fixture = context;
    fixture->processCalls++;
    return fixture->process;
}

static bool FixtureOffer(float distance, void* context) {
    TalkFixture* fixture = context;
    fixture->offerCalls++;
    fixture->offeredDistance = distance;
    return fixture->offer;
}

static const StaticStoryTalkOperations sOperations = {
    FixtureGetState, FixtureShouldAdvance, FixtureClose, FixtureProcess, FixtureOffer,
};

static void TestPoseTextSelection(void) {
    static const struct {
        StaticStoryActorType type;
        uint8_t pose;
        uint16_t before;
        uint16_t after;
    } cases[] = {
        { STATIC_STORY_ACTOR_CHILD_ZELDA, 0, 0x8F2D, 0x8F2D },   { STATIC_STORY_ACTOR_CHILD_ZELDA, 1, 0x8F2D, 0x8F2D },
        { STATIC_STORY_ACTOR_CHILD_ZELDA, 2, 0x8F2B, 0x8F2B },   { STATIC_STORY_ACTOR_CHILD_ZELDA, 3, 0x8F2D, 0x8F2D },
        { STATIC_STORY_ACTOR_CHILD_ZELDA, 255, 0x8F2D, 0x8F2D }, { STATIC_STORY_ACTOR_ADULT_ZELDA, 0, 0x70FF, 0x70FE },
        { STATIC_STORY_ACTOR_ADULT_ZELDA, 1, 0x70FF, 0x70FE },   { STATIC_STORY_ACTOR_ADULT_ZELDA, 2, 0x70FF, 0x70FE },
        { STATIC_STORY_ACTOR_ADULT_ZELDA, 3, 0x8F2C, 0x8F2C },   { STATIC_STORY_ACTOR_ADULT_ZELDA, 4, 0x70FF, 0x70FE },
        { STATIC_STORY_ACTOR_ADULT_ZELDA, 255, 0x70FF, 0x70FE }, { STATIC_STORY_ACTOR_SARIA, 3, 0x1001, 0x10AD },
    };

    for (unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        REQUIRE(StaticStoryTalk_CanTalkPose(cases[i].type, cases[i].pose));
        for (unsigned completed = 0; completed < 2; ++completed) {
            StaticStoryProgression progression = { .metZelda = completed, .forestComplete = completed };
            StaticStoryTalkSession session = { 0 };
            TalkFixture fixture = { .offer = true };

            StaticStoryTalk_UpdateForPose(cases[i].type, cases[i].pose, &progression, 90.0f, false, &session,
                                          &sOperations, &fixture);
            REQUIRE(session.textId == (completed ? cases[i].after : cases[i].before));
            REQUIRE(!session.talking && session.tracking);
            REQUIRE(fixture.processCalls == 1 && fixture.offerCalls == 1);
            REQUIRE(fixture.getStateCalls == 0 && fixture.advanceCalls == 0 && fixture.closeCalls == 0);
        }
    }

    static const StaticStoryActorType rejected[] = {
        STATIC_STORY_ACTOR_NONE,  STATIC_STORY_ACTOR_PHANTOM_GANON, STATIC_STORY_ACTOR_STALCHILD,
        STATIC_STORY_ACTOR_MIKAU, STATIC_STORY_ACTOR_DARMANI,       STATIC_STORY_ACTOR_MAX,
    };
    for (unsigned i = 0; i < sizeof(rejected) / sizeof(rejected[0]); ++i) {
        REQUIRE(!StaticStoryTalk_CanTalkPose(rejected[i], 2));
        REQUIRE(!StaticStoryTalk_CanTalkPose(rejected[i], 3));
    }
}

static void TestZeldaTalkLifecycle(StaticStoryActorType type, uint8_t pose, uint16_t expectedTextId) {
    StaticStoryProgression progression = {
        .metZelda = true, .forestComplete = true, .waterComplete = true, .eponaComplete = true
    };
    StaticStoryTalkSession session = { 0 };
    TalkFixture fixture = { .offer = true };

    /* A nearby actor may offer A repeatedly, but only an accepted request begins talking. */
    for (unsigned frame = 0; frame < 3; ++frame) {
        StaticStoryTalk_UpdateForPose(type, pose, &progression, 95.0f, true, &session, &sOperations, &fixture);
        REQUIRE(session.textId == expectedTextId);
        REQUIRE(!session.talking && session.tracking);
    }
    REQUIRE(fixture.processCalls == 3 && fixture.offerCalls == 3);
    REQUIRE(fixture.offeredDistance == 95.0f);
    REQUIRE(fixture.getStateCalls == 0 && fixture.advanceCalls == 0 && fixture.closeCalls == 0);

    for (unsigned conversation = 0; conversation < 3; ++conversation) {
        fixture = (TalkFixture){ .process = true, .offer = true };
        StaticStoryTalk_UpdateForPose(type, pose, &progression, 95.0f, true, &session, &sOperations, &fixture);
        REQUIRE(session.textId == expectedTextId && session.talking && session.tracking);
        REQUIRE(fixture.processCalls == 1 && fixture.offerCalls == 0);

        fixture = (TalkFixture){ .messageState = STATIC_STORY_TALK_MESSAGE_OTHER, .advance = true };
        StaticStoryTalk_UpdateForPose(type, pose, &progression, 95.0f, false, &session, &sOperations, &fixture);
        REQUIRE(session.talking && session.tracking);
        REQUIRE(fixture.getStateCalls == 1 && fixture.advanceCalls == 0 && fixture.closeCalls == 0);
        REQUIRE(fixture.processCalls == 0 && fixture.offerCalls == 0);

        fixture = (TalkFixture){ .messageState = STATIC_STORY_TALK_MESSAGE_EVENT };
        StaticStoryTalk_UpdateForPose(type, pose, &progression, 95.0f, false, &session, &sOperations, &fixture);
        REQUIRE(session.talking && session.tracking);
        REQUIRE(fixture.getStateCalls == 1 && fixture.advanceCalls == 1 && fixture.closeCalls == 0);
        REQUIRE(fixture.processCalls == 0 && fixture.offerCalls == 0);

        fixture = (TalkFixture){ .messageState = STATIC_STORY_TALK_MESSAGE_EVENT, .advance = true };
        StaticStoryTalk_UpdateForPose(type, pose, &progression, 95.0f, false, &session, &sOperations, &fixture);
        REQUIRE(!session.talking && !session.tracking && session.textId == expectedTextId);
        REQUIRE(fixture.getStateCalls == 1 && fixture.advanceCalls == 1 && fixture.closeCalls == 1);
        REQUIRE(fixture.processCalls == 0 && fixture.offerCalls == 0);

        fixture = (TalkFixture){ .process = true };
        StaticStoryTalk_UpdateForPose(type, pose, &progression, 95.0f, false, &session, &sOperations, &fixture);
        REQUIRE(session.talking && session.tracking && session.textId == expectedTextId);
        fixture = (TalkFixture){ .messageState = STATIC_STORY_TALK_MESSAGE_CLOSING };
        StaticStoryTalk_UpdateForPose(type, pose, &progression, 95.0f, false, &session, &sOperations, &fixture);
        REQUIRE(!session.talking && !session.tracking && session.textId == expectedTextId);
        REQUIRE(fixture.getStateCalls == 1 && fixture.advanceCalls == 0 && fixture.closeCalls == 0);
        REQUIRE(fixture.processCalls == 0 && fixture.offerCalls == 0);
        REQUIRE(progression.metZelda && progression.forestComplete && progression.waterComplete &&
                progression.eponaComplete);
    }
}

int main(void) {
    StaticStoryProgression progression = { 0 };
    StaticStoryTalkSession session = { .talking = true, .tracking = true, .textId = 0x7777 };
    TalkFixture fixture = {
        .messageState = STATIC_STORY_TALK_MESSAGE_EVENT,
        .advance = true,
        .process = true,
        .offer = true,
    };

    /* Phantom is rejected before both the active-conversation and offer/request branches. */
    StaticStoryTalk_Update(STATIC_STORY_ACTOR_PHANTOM_GANON, &progression, 120.0f, false, &session, &sOperations,
                           &fixture);
    REQUIRE(!session.talking);
    REQUIRE(!session.tracking);
    REQUIRE(session.textId == 0);
    REQUIRE(fixture.getStateCalls == 0);
    REQUIRE(fixture.advanceCalls == 0);
    REQUIRE(fixture.closeCalls == 0);
    REQUIRE(fixture.processCalls == 0);
    REQUIRE(fixture.offerCalls == 0);

    session = (StaticStoryTalkSession){ 0 };
    fixture = (TalkFixture){ .process = false, .offer = true };
    StaticStoryTalk_Update(STATIC_STORY_ACTOR_SKULL_KID, &progression, 90.0f, false, &session, &sOperations, &fixture);
    REQUIRE(session.textId == STATIC_STORY_TEXT_SKULL_KID);
    REQUIRE(!session.talking);
    REQUIRE(session.tracking);
    REQUIRE(fixture.processCalls == 1);
    REQUIRE(fixture.offerCalls == 1);
    REQUIRE(fixture.offeredDistance == 90.0f);

    session = (StaticStoryTalkSession){ 0 };
    fixture = (TalkFixture){ .process = true, .offer = true };
    /* A request accepted before the pedestal offer remains a real conversation. */
    StaticStoryTalk_Update(STATIC_STORY_ACTOR_SKULL_KID, &progression, 90.0f, true, &session, &sOperations, &fixture);
    REQUIRE(session.talking);
    REQUIRE(session.tracking);
    REQUIRE(fixture.processCalls == 1);
    REQUIRE(fixture.offerCalls == 0);

    fixture = (TalkFixture){ .messageState = STATIC_STORY_TALK_MESSAGE_EVENT, .advance = true };
    StaticStoryTalk_Update(STATIC_STORY_ACTOR_SKULL_KID, &progression, 90.0f, true, &session, &sOperations, &fixture);
    REQUIRE(!session.talking);
    REQUIRE(!session.tracking);
    REQUIRE(fixture.getStateCalls == 1);
    REQUIRE(fixture.advanceCalls == 1);
    REQUIRE(fixture.closeCalls == 1);

    session = (StaticStoryTalkSession){ .talking = true };
    fixture = (TalkFixture){ .messageState = STATIC_STORY_TALK_MESSAGE_CLOSING };
    StaticStoryTalk_Update(STATIC_STORY_ACTOR_SKULL_KID, &progression, 90.0f, false, &session, &sOperations, &fixture);
    REQUIRE(!session.talking);
    REQUIRE(!session.tracking);
    REQUIRE(fixture.closeCalls == 0);

    /* Room-10 Saria and Skull Kid must leave A to the offered time-pedestal interaction. */
    session = (StaticStoryTalkSession){ 0 };
    fixture = (TalkFixture){ .offer = true };
    StaticStoryTalk_Update(STATIC_STORY_ACTOR_SARIA, &progression, 70.0f, true, &session, &sOperations, &fixture);
    REQUIRE(session.textId == 0x1001);
    REQUIRE(!session.talking);
    REQUIRE(!session.tracking);
    REQUIRE(fixture.processCalls == 1);
    REQUIRE(fixture.offerCalls == 0);

    session = (StaticStoryTalkSession){ 0 };
    fixture = (TalkFixture){ .offer = true };
    StaticStoryTalk_Update(STATIC_STORY_ACTOR_SKULL_KID, &progression, 90.0f, true, &session, &sOperations, &fixture);
    REQUIRE(session.textId == STATIC_STORY_TEXT_SKULL_KID);
    REQUIRE(!session.talking);
    REQUIRE(!session.tracking);
    REQUIRE(fixture.processCalls == 1);
    REQUIRE(fixture.offerCalls == 0);

    /* Other catalogue actors and these actors away from a pedestal keep their ordinary talk offers. */
    session = (StaticStoryTalkSession){ 0 };
    fixture = (TalkFixture){ .offer = true };
    StaticStoryTalk_Update(STATIC_STORY_ACTOR_IMPA, &progression, 80.0f, true, &session, &sOperations, &fixture);
    REQUIRE(session.tracking);
    REQUIRE(fixture.offerCalls == 1);

    session = (StaticStoryTalkSession){ 0 };
    fixture = (TalkFixture){ .offer = true };
    StaticStoryTalk_Update(STATIC_STORY_ACTOR_SARIA, &progression, 70.0f, false, &session, &sOperations, &fixture);
    REQUIRE(session.tracking);
    REQUIRE(fixture.offerCalls == 1);

    TestPoseTextSelection();
    TestZeldaTalkLifecycle(STATIC_STORY_ACTOR_CHILD_ZELDA, 0, 0x8F2D);
    TestZeldaTalkLifecycle(STATIC_STORY_ACTOR_CHILD_ZELDA, 1, 0x8F2D);
    TestZeldaTalkLifecycle(STATIC_STORY_ACTOR_CHILD_ZELDA, 2, 0x8F2B);
    TestZeldaTalkLifecycle(STATIC_STORY_ACTOR_ADULT_ZELDA, 2, 0x70FE);
    TestZeldaTalkLifecycle(STATIC_STORY_ACTOR_ADULT_ZELDA, 3, 0x8F2C);

    session = (StaticStoryTalkSession){ 0 };
    fixture = (TalkFixture){ .offer = true };
    StaticStoryTalk_Update(STATIC_STORY_ACTOR_CHILD_ZELDA, &progression, 95.0f, false, &session, &sOperations,
                           &fixture);
    REQUIRE(session.textId == 0x8F2D && !session.talking && session.tracking);
    REQUIRE(fixture.processCalls == 1 && fixture.offerCalls == 1);
    return 0;
}
