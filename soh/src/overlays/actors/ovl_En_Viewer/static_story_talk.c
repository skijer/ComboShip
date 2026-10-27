#include "static_story_talk.h"

#include <stddef.h>

static void StaticStoryTalk_RestorePlacementPose(StaticStoryTalkSession* session) {
    session->talking = false;
    session->tracking = false;
}

static uint16_t StaticStoryTalk_GetChamberTextId(StaticStoryActorType type, uint8_t pose) {
    const StaticStoryPoseDescriptor* descriptor;

    if (type != STATIC_STORY_ACTOR_CHILD_ZELDA && type != STATIC_STORY_ACTOR_ADULT_ZELDA) {
        return 0;
    }
    descriptor = StaticStoryActor_ResolvePose(type, pose);
    if (descriptor == NULL || !(descriptor->flags & STATIC_POSE_FLAG_BLUE_AURA)) {
        return 0;
    }
    return type == STATIC_STORY_ACTOR_CHILD_ZELDA ? STATIC_STORY_TEXT_CHAMBER_CHILD_ZELDA
                                                  : STATIC_STORY_TEXT_CHAMBER_ADULT_ZELDA;
}

bool StaticStoryTalk_CanTalkPose(StaticStoryActorType type, uint8_t pose) {
    return StaticStoryTalk_GetChamberTextId(type, pose) != 0 || StaticStoryActor_CanTalk(type);
}

void StaticStoryTalk_UpdateForPose(StaticStoryActorType type, uint8_t pose, const StaticStoryProgression* progression,
                                   float talkDistance, bool timePedestalOffered, StaticStoryTalkSession* session,
                                   const StaticStoryTalkOperations* operations, void* context) {
    StaticStoryTalkMessageState messageState;

    if (session == NULL || operations == NULL) {
        return;
    }
    if (!StaticStoryTalk_CanTalkPose(type, pose)) {
        session->textId = 0;
        StaticStoryTalk_RestorePlacementPose(session);
        return;
    }
    if (session->talking) {
        session->tracking = true;
        messageState = operations->getMessageState != NULL ? operations->getMessageState(context)
                                                           : STATIC_STORY_TALK_MESSAGE_OTHER;
        if (messageState == STATIC_STORY_TALK_MESSAGE_EVENT && operations->shouldAdvance != NULL &&
            StaticStoryActor_ShouldCloseEventMessage(true, operations->shouldAdvance(context))) {
            if (operations->closeTextbox != NULL) {
                operations->closeTextbox(context);
            }
            StaticStoryTalk_RestorePlacementPose(session);
        } else if (messageState == STATIC_STORY_TALK_MESSAGE_CLOSING) {
            StaticStoryTalk_RestorePlacementPose(session);
        }
        return;
    }

    session->textId = StaticStoryTalk_GetChamberTextId(type, pose);
    if (session->textId == 0) {
        session->textId = StaticStoryActor_SelectTextId(type, progression);
    }
    if (operations->processTalkRequest != NULL && operations->processTalkRequest(context)) {
        session->talking = true;
        session->tracking = true;
    } else if (timePedestalOffered && (type == STATIC_STORY_ACTOR_SARIA || type == STATIC_STORY_ACTOR_SKULL_KID)) {
        session->tracking = false;
    } else {
        session->tracking = operations->offerTalk != NULL && operations->offerTalk(talkDistance, context);
    }
}

void StaticStoryTalk_Update(StaticStoryActorType type, const StaticStoryProgression* progression, float talkDistance,
                            bool timePedestalOffered, StaticStoryTalkSession* session,
                            const StaticStoryTalkOperations* operations, void* context) {
    StaticStoryTalk_UpdateForPose(type, 0, progression, talkDistance, timePedestalOffered, session, operations,
                                  context);
}
