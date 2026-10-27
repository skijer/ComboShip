// Included after the existing native fixtures and exact production functions.
#ifdef OOT_RESOURCE_ENTRY_TEST
static constexpr const char* kHost = "oot";
static Actor* EntryDoorActor() { return &nativeDoor.actor; }
static bool OpenResourceDoor() { return NativeOpen(&player, &play, &nativeDoor); }
static void UpdateResourcePlayer() { PlayerUpdate(); }
static u16 CurrentEntrance() { return gSaveContext.entranceIndex; }
static void CurrentEntrance(u16 value) { gSaveContext.entranceIndex = value; }
static u16 NextEntrance() { return play.nextEntranceIndex; }
static void NormalizeResourceEntry() { ComboBlackMarketOot_NormalizeEntry(); }
static void InitResourceScene(bool interior) {
    CollisionHeader oldCameraCollision{};
    sBrowseKeeper = &player.actor;
    sShopOriginalCollision = &oldCameraCollision;
    sShopBgCameras.resize(1);
    sShopBgIndex = 7;
    ResourceSceneInit(interior ? SCENE_SKIEJER_MARKET : SCENE_SKIEJER_FOREST);
    assert(!sBrowseKeeper && !sShopOriginalCollision && sShopBgCameras.empty() && sShopBgIndex == -1);
    // OoT clears this after its scene callback, before the first PlayerUpdate.
    gSaveContext.respawnFlag = 0;
}
#else
static constexpr const char* kHost = "mm";
static KnobDoorActor resourceDoor;
static Actor* EntryDoorActor() { return &resourceDoor.dyna.actor; }
static bool OpenResourceDoor() {
    EnDoor offered{}; offered.knobDoor = resourceDoor;
    offered.doorType = ENDOOR_TYPE_WHOLE_DAY; offered.typeVar.halfDayCheck = 0;
    play.actorCtx.halfDaysBit = HALFDAYBIT_DAY1_DAWN;
    NativeOffer(&offered, &play);
    assert(player.doorType == PLAYER_DOORTYPE_HANDLE);
    return NativeCaller(&player, &play, &resourceDoor);
}
static void UpdateResourcePlayer() { PlayerUpdate(&player.actor); }
static u16 CurrentEntrance() { return gSaveContext.save.entrance; }
static void CurrentEntrance(u16 value) { gSaveContext.save.entrance = value; }
static u16 NextEntrance() { return play.nextEntrance; }
static void NormalizeResourceEntry() { ComboBlackMarket_NormalizeEntry(); }
static void InitResourceScene(bool interior) {
    // MM clears this immediately before its OnSceneInit callback.
    gSaveContext.respawnFlag = 0;
    SceneInit(play.sceneId, 0);
}
#endif

static void SetResourceExterior(const char* forest) {
#ifdef OOT_RESOURCE_ENTRY_TEST
    play.sceneNum = SCENE_SKIEJER_FOREST;
    CurrentEntrance(ENTR_SKIEJER_FOREST);
    play.transiActorCtx.list = realDoors.data();
    play.transiActorCtx.numActors = realDoors.size();
    nativeDoor = {};
#else
    const bool solo = std::strcmp(forest, "mm") == 0;
    gComboGameMode = solo ? COMBO_GAME_MM : COMBO_GAME_BOTH;
    play.sceneId = solo ? COMBO_SOLO_FOREST_MM_SCENE : SCENE_LOST_WOODS;
    CurrentEntrance(solo ? COMBO_SOLO_FOREST_MM_ENTRANCE : ENTRANCE(LOST_WOODS, 0));
    play.transitionActors.list = realDoors.data();
    play.transitionActors.count = realDoors.size();
    resourceDoor = {};
#endif
    play.roomCtx.curRoom.num = 0;
    auto* door = EntryDoorActor();
    const auto& row = realDoors.at(1);
    door->id = ACTOR_EN_DOOR; door->category = ACTORCAT_DOOR;
    door->home.pos = door->world.pos = {float(row.pos.x), float(row.pos.y), float(row.pos.z)};
    door->params = 1 << 10;
#ifdef OOT_RESOURCE_ENTRY_TEST
    door->home.rot.y = door->shape.rot.y = row.rotY;
    player.actionFunc = Player_Action_80845EF8;
#else
    player.actionFunc = Player_Action_36;
#endif
    player.doorActor = door; player.doorType = PLAYER_DOORTYPE_HANDLE;
    play.transitionTrigger = TRANS_TRIGGER_OFF; play.transitionMode = TRANS_MODE_OFF;
    player.stateFlags1 = 0;
    player.actor.world.pos = {-450, 0, 0};
}

static void PrepareResourceEntry(const char* scenario, const char* forest, bool stock) {
    Reset(); ConfigureResources(scenario, forest);
#ifdef OOT_RESOURCE_ENTRY_TEST
    ClearImports(); sNativeKeeperReady = false;
#else
    RegisterMarket();
    assert(sSceneEntranceTable[COMBO_BLACK_MARKET_ENTRANCE >> 9].table[0][0].sceneId == COMBO_BLACK_MARKET_SCENE);
    assert(std::strcmp(gSceneTable[COMBO_BLACK_MARKET_SCENE].segment.fileName, "skiejer_market") == 0);
#endif
    SetResourceExterior(forest);
    if (!stock) sSession.Bind(nullptr, -1);
    assert(DestinationAvailable());
    assert(RealForestAvailable(forest) == inputResources.at("full_real_inputs").get<bool>());
}

static int ResourceRupees() {
#ifdef OOT_RESOURCE_ENTRY_TEST
    return gSaveContext.rupees;
#else
    return gSaveContext.save.saveInfo.playerData.rupees;
#endif
}

static void TestResourceEntryCycle(const char* scenario, const char* forest, bool stock) {
    PrepareResourceEntry(scenario, forest, stock);
    const bool full = inputResources.at("full_real_inputs");
    assert(PackAvailable() == full);
#ifdef OOT_RESOURCE_ENTRY_TEST
    if (full && std::string(scenario).ends_with("_native"))
        assert(sNativeKeeperReady && importedGraphs.size() >= 16 && graphVertices > 0 && graphTextures > 0);
    assert(gEntranceTable[ENTR_SKIEJER_MARKET].scene == SCENE_SKIEJER_MARKET);
    assert(gEntranceTable[ENTR_SKIEJER_MARKET].spawn == 0);
#endif
    const auto outside = CurrentEntrance();
    const auto top = gSaveContext.respawn[RESPAWN_MODE_TOP];
    const auto disposable = gSaveContext.respawn[RESPAWN_MODE_RETURN];
    for (int cycle = 0; cycle < 3; ++cycle) {
        assert(OpenResourceDoor() && sEntering && MatchesTravel());
        assert(play.transitionTrigger == TRANS_TRIGGER_START);
        assert(diagnostics.back()["result"] == "entered");
        assert(diagnostics.back()["failed_checks"].empty());
#ifdef OOT_RESOURCE_ENTRY_TEST
        assert(NextEntrance() == ENTR_SKIEJER_MARKET && vanillaDoorCameras == 0);
#else
        assert(NextEntrance() == COMBO_BLACK_MARKET_ENTRANCE && vanillaDoors == 0);
#endif
        CurrentEntrance(NextEntrance()); NormalizeResourceEntry();
        if (CurrentEntrance() != NextEntrance() || sReturnPending)
            fprintf(stderr, "entry normalization rejected scenario=%s forest=%s stock=%d cycle=%d current=%u next=%u respawn=%d entering=%d matches=%d log=%s\n",
                    scenario, forest, stock, cycle, CurrentEntrance(), NextEntrance(), gSaveContext.respawnFlag,
                    sEntering, MatchesTravel(), diagnostics.back().dump().c_str());
        assert(CurrentEntrance() == NextEntrance() && !sReturnPending);
#ifdef OOT_RESOURCE_ENTRY_TEST
        play.sceneNum = SCENE_SKIEJER_MARKET;
#else
        play.sceneId = COMBO_BLACK_MARKET_SCENE;
#endif
        play.transitionTrigger = TRANS_TRIGGER_OFF; play.transitionMode = TRANS_MODE_OFF;
        player.stateFlags1 = 0;
        InitResourceScene(true);
        const auto& spawn = inputResources.at("destinations").at(kHost).at("spawn");
        player.actor.world.pos = {spawn[1], spawn[2], spawn[3]};
        assert(!ComboBlackMarket_AtExit(player.actor.world.pos.x, player.actor.world.pos.y, player.actor.world.pos.z));
        UpdateResourcePlayer();
        assert(play.transitionTrigger == TRANS_TRIGGER_OFF && Interior());
        assert(sSession.Active(gSaveContext.fileNum) == stock);
        if (!stock) {
            assert(!Purchase() && !sReceipt.pending && grants == 0 && writes == 0);
#ifndef OOT_RESOURCE_ENTRY_TEST
            u16 text = BlackMarketEntry::kUnavailableText; bool fromTable = true;
            MarketText(&text, &fromTable);
            assert(!fromTable && text < 0x4E20 && CustomMessage::last.msg.find("wares aren't ready") != std::string::npos);
#endif
        } else if (cycle == 0) {
            const int rupees = ResourceRupees();
            const int price = sSession.market["stock"][0]["price"];
            assert(Purchase() && sReceipt.pending && !sReceipt.delivered);
            assert(grants == 0 && writes == 0 && sSession.state.purchased == 0 && ResourceRupees() == rupees);
#ifdef OOT_RESOURCE_ENTRY_TEST
            MonitorReceipt();
            assert(sReceipt.offered && grants == 0 && writes == 0 && ResourceRupees() == rupees);
            const auto receipt = lastOfferedEntry;
            NativeAcquire(receipt);
#else
            Actor* receipt = OfferFixtureReceipt();
            assert(grants == 0 && writes == 0 && ResourceRupees() == rupees);
            GiveMarketReceipt(receipt, &play);
#endif
            assert(sReceipt.delivered && grants == 1 && writes == 1 && sSession.state.purchased == 1);
            assert(ResourceRupees() == rupees - price);
#ifdef OOT_RESOURCE_ENTRY_TEST
            NativeAcquire(receipt); // A repeated native notification cannot charge or grant again.
            player.stateFlags1 &= ~(PLAYER_STATE1_GETTING_ITEM | PLAYER_STATE1_IN_ITEM_CS);
            play.msgCtx.msgMode = 0; inCs = false; activeCamera = CAM_ID_MAIN;
            MonitorReceipt(); // Native GI/dialogue completion releases the delivered receipt.
            assert(browseStarts == 1 && browseMessages == 1 && sSelected == 0 && sBrowser.slot == 0 && sTalking);
#else
            GiveMarketReceipt(receipt, &play);
            MarketReceiptDestroyed(receipt, &play);
            CancelReceipt(true);
            assert(!sReceiptActor && std::holds_alternative<GIEventNone>(receiptInteractor.currentEvent));
#endif
            assert(!sReceipt.pending && grants == 1 && writes == 1 && sSession.state.purchased == 1);
            assert(ResourceRupees() == rupees - price);
            EndBrowsing(&play, false);
            assert(!sTalking);
        } else {
            assert(grants == 1 && writes == 1 && sSession.state.purchased == 1);
        }
        player.actor.world.pos = {-11, 0, 410}; UpdateResourcePlayer();
        assert(sReturnPending && play.transitionTrigger == TRANS_TRIGGER_START && NextEntrance() == outside);
        CurrentEntrance(NextEntrance()); NormalizeResourceEntry();
        SetResourceExterior(forest); InitResourceScene(false);
#ifdef OOT_RESOURCE_ENTRY_TEST
        UpdateResourcePlayer();
        const auto expected = OotBlackMarketDoorNative::Outside(OotBlackMarketDoorNative::Placement(realDoors.at(1), 1));
#else
        BlackMarketDoorNative::Pose pose;
        assert(BlackMarketDoorNative::Find(&play, pose)); const auto expected = BlackMarketDoorNative::Outside(pose);
#endif
        assert(!sReturnPending && !sReturnStaged);
        assert(std::abs(player.actor.world.pos.x - expected.x) < .01f);
        assert(std::abs(player.actor.world.pos.z - expected.z) < .01f);
        assert(player.actor.floorPoly && player.actor.floorPoly->normal.y >= 23170);
        assert(std::isfinite(player.actor.floorHeight));
        assert(!std::memcmp(&top, &gSaveContext.respawn[RESPAWN_MODE_TOP], sizeof(top)));
        assert(!std::memcmp(&disposable, &gSaveContext.respawn[RESPAWN_MODE_RETURN], sizeof(disposable)));
    }
    printf("PASS %s scenario=%s forest=%s stock=%d: native Open -> destination -> actual spawn -> stay -> static-collision return -> re-entry (3 cycles)\n",
           kHost, scenario, forest, stock);
}

static void TestResourceFailures(const char* scenario, const char* forest) {
    const bool full = inputResources.at("full_real_inputs");
    if (full) for (const char* path : {"custom/comboship/native_forest/geometry/Z2_LOST_WOODS_room_00DL_002548",
                            "scenes/nonmq/Z2_LOST_WOODS/Z2_LOST_WOODS_room_02DL_000D90"}) {
        PrepareResourceEntry(scenario, forest, true);
        assert(ResourceMgr_FileExists(path) || MmAssets_BaseResourceExists(path)); hiddenFiles.insert(path);
        assert(!RealForestAvailable(forest) && "missing real private or base ancestry must fail the forest dependency gate");
    }
    PrepareResourceEntry(scenario, forest, true); baseFiles.clear();
    assert(!RealForestAvailable(forest) && "visual packs cannot replace a missing genuine base owner");
    for (const auto* path :
#ifdef OOT_RESOURCE_ENTRY_TEST
         BlackMarketEntry::kOotDestination
#else
         BlackMarketEntry::kMmDestination
#endif
    ) {
        PrepareResourceEntry(scenario, forest, false); assert(ResourceMgr_FileExists(path)); hiddenFiles.insert(path);
        assert(!DestinationAvailable());
        assert(OpenResourceDoor() && !sEntering && play.transitionTrigger == TRANS_TRIGGER_OFF);
        assert(diagnostics.back()["reason"] == "destination_missing");
        assert(diagnostics.back()["resources"]["destination_missing"] == nlohmann::json::array({path}));
#ifdef OOT_RESOURCE_ENTRY_TEST
        assert(player.actionFunc == Player_Action_Idle && !nativeDoor.playerIsOpening);
        assert(doorErrors == 1 && vanillaDoorCameras == 0);
#else
        assert(stoppedDoors == 1 && doorErrors == 1 && vanillaDoors == 0);
#endif
    }
    // Missing optional art is already represented by *_native. Missing keeper
    // and room-material resources affect presentation readiness, while the
    // independent structural destination still permits entry and safe exit.
    if (full) for (const char* path : {"objects/object_bg/gTreasureChestShopGalSkel",
                            "custom/skiejer_market/geometry/mat_mahouya_room_0DL_006548_GranmaShop00"}) {
        PrepareResourceEntry(scenario, forest, false);
        assert(ResourceMgr_FileExists(path) || MmAssets_BaseResourceExists(path)); hiddenFiles.insert(path);
        assert(!PackAvailable() && DestinationAvailable());
        assert(OpenResourceDoor() && sEntering && diagnostics.back()["result"] == "entered");
    }
#ifdef OOT_RESOURCE_ENTRY_TEST
    if (full) for (const char* path : {"objects/object_bg/gTreasureChestShopGalHeadLimb",
                            "objects/object_bg/gTreasureChestShopGalHeadDL",
                            "objects/object_bg/gTreasureChestShopGalEyeOpenUpTex"}) {
        PrepareResourceEntry(scenario, forest, true); assert(MmAssets_BaseResourceExists(path)); hiddenFiles.insert(path);
        assert(!PackAvailable() && "real native keeper hierarchy/graph failures must fail presentation preflight");
        assert(DestinationAvailable());
    }
#endif
    PrepareResourceEntry(scenario, forest, true);
    realDoors.push_back(realDoors.at(1)); SetResourceExterior(forest);
#ifdef OOT_RESOURCE_ENTRY_TEST
    assert(OpenResourceDoor() && !sEntering && play.transitionTrigger == TRANS_TRIGGER_OFF);
#else
    assert(ComboBlackMarket_TryDoorEntrance(&play, EntryDoorActor()) != COMBO_BLACK_MARKET_DOOR_ENTERED);
    assert(!sEntering && play.transitionTrigger == TRANS_TRIGGER_OFF);
#endif
    assert(diagnostics.back()["result"] != "entered");
    PrepareResourceEntry(scenario, forest, true); play.pauseCtx.state = 1;
    assert(OpenResourceDoor() && !sEntering && play.transitionTrigger == TRANS_TRIGGER_OFF);
    assert(diagnostics.back()["checks"]["pause_closed"] == false);
    printf("PASS %s %s failures: every structural path blocks; duplicate marker and pause protected%s\n", kHost,
           full ? "real-resource" : "bundled structural", full ? "; keeper/material failure stays independent" : "; base owner absent");
}

int main(int argc, char** argv) {
    setvbuf(stdout, nullptr, _IONBF, 0);
    assert(argc == 2); ReadEntryInput(argv[1]);
    if (!inputResources.at("full_real_inputs").get<bool>()) {
#ifdef OOT_RESOURCE_ENTRY_TEST
        for (bool stock : {false, true}) TestResourceEntryCycle("oot_native", "oot", stock);
        TestResourceFailures("oot_native", "oot");
#else
        for (const char* forest : {"combined", "mm"})
            for (bool stock : {false, true}) TestResourceEntryCycle("mm_native", forest, stock);
        TestResourceFailures("mm_native", "combined"); TestResourceFailures("mm_native", "mm");
#endif
        puts("BOUNDARY: bundled structural routes only; base owner and keeper are absent, full forest readiness fails closed; no gameplay proof");
        return 0;
    }
#ifdef OOT_RESOURCE_ENTRY_TEST
    for (const char* scenario : {"oot_native", "oot_mods_2ship_visuals", "oot_authored_market"})
        for (bool stock : {false, true}) TestResourceEntryCycle(scenario, "oot", stock);
    for (bool stock : {false, true}) TestResourceEntryCycle("oot_authored_market", "authored_oot", stock);
    TestResourceFailures("oot_native", "oot");
#else
    for (const char* scenario : {"mm_native", "mm_mods_2ship_visuals", "mm_authored_market"})
        for (const char* forest : {"combined", "mm"})
            for (bool stock : {false, true}) TestResourceEntryCycle(scenario, forest, stock);
    for (bool stock : {false, true}) TestResourceEntryCycle("mm_authored_market", "authored_mm", stock);
    TestResourceFailures("mm_native", "combined");
    TestResourceFailures("mm_native", "mm");
#endif
    puts("BOUNDARY: controller/resource executable passed; configured game loop, resource manager, rendering and collision traversal remain untested");
}
