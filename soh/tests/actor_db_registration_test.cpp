// Run the production ActorDB and built-in Ivan registration with native headers.
// Legacy actor implementations and SW97 hooks are link boundaries for this test.
#include "soh/ActorDB.h"
#include "src/overlays/actors/ovl_En_Partner/z_en_partner.h"
#include "test_require.h"

#include <cstdio>

extern "C" {
#define DEFINE_ACTOR(name, actorId, allocType)                                                                      \
    ActorInit name##_InitVars = {                                                                                   \
        actorId, ACTORCAT_MISC, 0, OBJECT_GAMEPLAY_KEEP, sizeof(Actor), nullptr, nullptr, nullptr, nullptr, nullptr \
    };
#define DEFINE_ACTOR_INTERNAL(name, actorId, allocType) DEFINE_ACTOR(name, actorId, allocType)
#define DEFINE_ACTOR_UNSET(actorId)
#include "tables/actor_table.h"
#undef DEFINE_ACTOR
#undef DEFINE_ACTOR_INTERNAL
#undef DEFINE_ACTOR_UNSET

s16 gEnPartnerId = -1;
static Actor* callbackActor;
static PlayState* callbackPlay;
static int callbacks;

void EnPartner_Init(Actor* actor, PlayState* play) {
    REQUIRE(actor == callbackActor && play == callbackPlay);
    REQUIRE(callbacks++ == 0);
}
void EnPartner_Destroy(Actor* actor, PlayState* play) {
    REQUIRE(actor == callbackActor && play == callbackPlay);
    REQUIRE(callbacks++ == 1);
}
void EnPartner_Update(Actor* actor, PlayState* play) {
    REQUIRE(actor == callbackActor && play == callbackPlay);
    REQUIRE(callbacks++ == 2);
}
void EnPartner_Draw(Actor* actor, PlayState* play) {
    REQUIRE(actor == callbackActor && play == callbackPlay);
    REQUIRE(callbacks++ == 3);
}
}

static int sw97Registrations, sw97Hooks;
void Sw97_RegisterActors() {
    REQUIRE(ActorDB::Instance->RetrieveId("En_Partner") == gEnPartnerId);
    ++sw97Registrations;
}
void Sw97_RegisterHooks() {
    REQUIRE(sw97Registrations == 1);
    ++sw97Hooks;
}

int main() {
    ActorDB database;
    ActorDB::Instance = &database;
    REQUIRE(database.RetrieveEntry(ACTOR_PLAYER).entry.valid);
    REQUIRE(database.RetrieveId("En_Partner") == -1);

    database.AddBuiltInCustomActors();
    REQUIRE(gEnPartnerId == ACTOR_ID_EXTRA_MAX);
    REQUIRE(database.RetrieveId("En_Partner") == gEnPartnerId);
    const ActorDBEntry& ivan = database.RetrieveEntry(gEnPartnerId).entry;
    REQUIRE(ivan.valid && ivan.category == ACTORCAT_ITEMACTION);
    REQUIRE(ivan.flags == (ACTOR_FLAG_UPDATE_CULLING_DISABLED | ACTOR_FLAG_DRAW_CULLING_DISABLED |
                           ACTOR_FLAG_HOOKSHOT_PULLS_PLAYER | ACTOR_FLAG_CAN_PRESS_SWITCHES));
    REQUIRE(ivan.objectId == OBJECT_GAMEPLAY_KEEP);
    REQUIRE(ivan.instanceSize == sizeof(EnPartner));
    REQUIRE(ivan.init == EnPartner_Init && ivan.destroy == EnPartner_Destroy);
    REQUIRE(ivan.update == EnPartner_Update && ivan.draw == EnPartner_Draw);
    REQUIRE(ivan.reset == nullptr && ivan.numLoaded == 0);
    REQUIRE(sw97Registrations == 1 && sw97Hooks == 1);

    EnPartner actor{};
    PlayState play{};
    callbackActor = &actor.actor;
    callbackPlay = &play;
    ivan.init(callbackActor, callbackPlay);
    ivan.destroy(callbackActor, callbackPlay);
    ivan.update(callbackActor, callbackPlay);
    ivan.draw(callbackActor, callbackPlay);
    REQUIRE(callbacks == 4);

    ActorDBInit custom;
    custom.name = "DynamicFixture";
    custom.desc = "Dynamic actor";
    REQUIRE(database.AddEntry(custom).entry.id == ACTOR_ID_EXTRA_MAX + 1);
    ActorDBInit fixed;
    fixed.name = "FixedFixture";
    fixed.desc = "SoH 9.3 fixed actor";
    fixed.id = ACTOR_EN_PARTNER;
    REQUIRE(database.AddEntry(fixed).entry.id == ACTOR_EN_PARTNER);
    custom.name = "NextDynamicFixture";
    REQUIRE(database.AddEntry(custom).entry.id == ACTOR_ID_EXTRA_MAX + 2);
    std::puts("PASS production ActorDB: Ivan size/callbacks, dynamic IDs, fixed IDs and SW97 hooks");
}
