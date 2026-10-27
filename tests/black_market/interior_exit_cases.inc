// Uses the existing native-controller harness and actual player Open hook.
static void TestInteriorExitDoor() {
    TransitionActorEntry entry{};
    EnDoor door{};
    const auto actorOf = [&]() -> Actor& {
#ifdef OOT_INTERIOR_EXIT_TEST
        return door.actor;
#else
        return door.knobDoor.dyna.actor;
#endif
    };
    const auto prepare = [&] {
        Reset(); entry = {}; door = {}; sTalking = false; sDoorCollision.releases = 0;
        entry.id = -ACTOR_EN_DOOR;
        entry.sides[0].room = 0; entry.sides[1].room = -1;
        entry.pos = { -10, 0, 420 };
#ifdef OOT_INTERIOR_EXIT_TEST
        play.sceneNum = SCENE_SKIEJER_MARKET;
        play.transiActorCtx.list = &entry; play.transiActorCtx.numActors = 1;
        entry.sides[0].effects = entry.sides[1].effects = -1;
        entry.params = 3 << 7;
        play.actorCtx.actorLists[ACTORCAT_DOOR].head = &actorOf();
        sKeeperSpawned = true;
#else
        play.sceneId = COMBO_BLACK_MARKET_SCENE;
        play.transitionActors.list = &entry; play.transitionActors.count = 1;
        entry.sides[0].bgCamIndex = entry.sides[1].bgCamIndex = -1;
        entry.rotY = 127; entry.params = 7;
        play.actorCtx.actorLists[ACTORCAT_DOOR].first = &actorOf();
#endif
        play.roomCtx.curRoom.num = 0;
        auto& actor = actorOf();
        actor.id = ACTOR_EN_DOOR; actor.category = ACTORCAT_DOOR;
        actor.params = entry.params; actor.room = 0;
        actor.home.pos = actor.world.pos = { -10, 0, 420 };
        actor.update = actor.draw = [](Actor*, PlayState*) {};
        player.actor.world.pos = { -10, 0, 381 };
        player.doorActor = &actor; player.doorType = PLAYER_DOORTYPE_HANDLE;
#ifndef OOT_INTERIOR_EXIT_TEST
        player.actionFunc = Player_Action_36;
#endif
    };
    const auto tick = [&] {
#ifdef OOT_INTERIOR_EXIT_TEST
        PlayerUpdate();
#else
        PlayerUpdate(&player.actor);
#endif
    };
    prepare();
    const auto marketBefore = sSession.market;
    const auto stateBefore = sSession.state.purchased;
#ifdef OOT_INTERIOR_EXIT_TEST
    const u16 destinationBefore = sReturnEntrance;
    NativeOpen(&player, &play, &door);
    assert(play.nextEntranceIndex == destinationBefore && vanillaDoorCameras == 0);
#else
    const u16 destinationBefore = ReturnEntrance();
    NativeCaller(&player, &play, &door.knobDoor);
    assert(play.nextEntrance == destinationBefore && vanillaDoors == 0);
#endif
    assert(sDoorCollision.releases == 1 && sReturnPending && play.transitionTrigger == TRANS_TRIGGER_START &&
           "native interior Open must use the already working return transition");
    assert(sSession.market == marketBefore && sSession.state.purchased == stateBefore && grants == 0 && writes == 0);
    for (float z : { 381.f, 401.f, 419.f, 439.f }) {
        prepare(); player.actor.world.pos.z = z; tick();
        assert(!sReturnPending && play.transitionTrigger == TRANS_TRIGGER_OFF &&
               "a live visible door must offer Open before the old z400 walk trigger");
    }
    prepare(); player.actor.world.pos.z = 440; tick();
    assert(sReturnPending && play.transitionTrigger == TRANS_TRIGGER_START &&
           "walking past the native noncolliding leaf must retain a grounded escape");
    for (int failure = 0; failure < 4; ++failure) {
        prepare(); player.actor.world.pos.z = 410;
        if (failure == 0) actorOf().update = nullptr;
        if (failure == 1) actorOf().draw = nullptr;
        if (failure == 2) actorOf().home.pos.x += 5;
        if (failure == 3) entry.params ^= 1;
        tick();
        assert(sReturnPending && play.transitionTrigger == TRANS_TRIGGER_START &&
               "a missing or mismatched native door must preserve the original walk exit");
    }
    for (int blocked = 0; blocked < 4; ++blocked) {
        prepare();
        if (blocked == 0) play.pauseCtx.state = static_cast<decltype(play.pauseCtx.state)>(1);
        if (blocked == 1) sTalking = true;
        if (blocked == 2) play.gameOverCtx.state = static_cast<decltype(play.gameOverCtx.state)>(1);
#ifdef OOT_INTERIOR_EXIT_TEST
        if (blocked == 3) gSaveContext.health = 0;
        NativeOpen(&player, &play, &door);
        assert(!door.playerIsOpening && doorErrors == 1 && vanillaDoorCameras == 0);
        assert(player.actionFunc != Player_Action_80845EF8);
#else
        if (blocked == 3) gSaveContext.save.saveInfo.playerData.health = 0;
        NativeCaller(&player, &play, &door.knobDoor);
        assert(!door.knobDoor.requestOpen && doorErrors == 1 && vanillaDoors == 0);
        assert(player.actionFunc != Player_Action_36);
#endif
        assert(sDoorCollision.releases == 0 && !sReturnPending && play.transitionTrigger == TRANS_TRIGGER_OFF && grants == 0 && writes == 0 &&
               "native blocked-Open cleanup must not leave an opening animation or start a transition");
    }
    prepare();
    std::puts("PASS native interior exit: actual player Open owns return, no early walk bypass, supported far-side fallback and failed-door escape; stock/progression untouched");
}
