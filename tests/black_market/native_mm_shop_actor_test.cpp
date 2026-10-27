/* NATIVE MM SUPPORT */
/* NATIVE STOCK DATA */

static s32 CanBuy(PlayState *, EnGirlA *) {
  callbacks.push_back("canBuy");
  return buyResult;
}
static void Buy(PlayState *, EnGirlA *) { callbacks.push_back("buy"); }
static void BuyFanfare(PlayState *, EnGirlA *) {
  callbacks.push_back("fanfare");
}
static void Bought(PlayState *, EnGirlA *) { callbacks.push_back("bought"); }
static void Restock(PlayState *, EnGirlA *) { callbacks.push_back("restock"); }
static void Configure(EnGirlA &item, int slot, Vec3f position) {
  item = {};
  item.actor.id = ACTOR_EN_GIRLA;
  item.actor.params = slot;
  item.actor.world.pos = item.actor.focus.pos = position;
  item.actor.textId = 0x680 + slot;
  item.choiceTextId = 0x700 + slot;
  item.canBuyFunc = CanBuy;
  item.buyFunc = Buy;
  item.buyFanfareFunc = BuyFanfare;
  item.boughtFunc = Bought;
  item.restockFunc = Restock;
  item.getItemId = GI_RUPEE_GREEN;
}
extern "C" Actor *Actor_Spawn(ActorContext *, PlayState *, s16 id, f32 x, f32 y,
                              f32 z, s16, s16, s16, s32) {
  assert(id == ACTOR_EN_GIRLA && spawns < 8);
  auto &item = stock[spawns];
  Configure(item, spawns++, {x, y, z});
  return &item.actor;
}
static s32 PrivateInit(EnOssan *current, PlayState *) {
  ++adapterInits;
  assert(current->cursorPos.z == 1.5f && current->csId == 0 &&
         current->lookToShopKeeperFromShelfCsId == 3);
  assert(current->cursorColor.r == 0 && current->cursorColor.g == 80 &&
         current->cursorColor.b == 255);
  assert(current->skelAnime.jointTable == galJoints.data() &&
         current->skelAnime.morphTable == galMorphs.data());
  for (int i = 0; i < 8; ++i) {
    const auto p = sShops[ENOSSAN_PART_TIME_WORKER][i].spawnPos;
    Configure(stock[i], i, {float(p.x), float(p.y), float(p.z)});
    current->items[i] = &stock[i];
  }
  current->blinkTimer = 20;
  current->blinkFunc = EnOssan_WaitForBlink;
  return initWorks;
}
static s32 CanOffer(EnOssan *, PlayState *) { return canOffer; }
static s32 Begin(EnOssan *, PlayState *) {
  ++beginCalls;
  return canBegin;
}
static u16 ResolveText(EnOssan *, u16 text) { return text; }
static s32 CanStart(EnOssan *, PlayState *) { return canStart; }
static void Started(EnOssan *, PlayState *) { ++startCalls; }
static void ItemPositions(EnOssan *, s32 index, Vec3f *shelf, Vec3f *selected) {
  const auto p = sShops[ENOSSAN_PART_TIME_WORKER][index].spawnPos;
  *shelf = {float(p.x), float(p.y), float(p.z)};
  *selected = {index < 4 ? 35.0f : -35.0f, 68, -130};
}
static s32 Reserve(EnOssan *, PlayState *) {
  ++reservations;
  return reserveWorks;
}
static s32 Accepted(EnOssan *, PlayState *) { return receiptState; }
static s32 Complete(EnOssan *, PlayState *) { return receiptDone; }
static const ComboBlackMarketShopAdapter adapter{
    PrivateInit, CanOffer,      Begin,   ResolveText, CanStart,
    Started,     ItemPositions, Reserve, Accepted,    Complete};
extern "C" const ComboBlackMarketShopAdapter *
ComboBlackMarketShop_GetAdapter(EnOssan *current, PlayState *) {
  return market && current == &shop ? &adapter : nullptr;
}

static void Reset(bool privateShop) {
  play = {};
  player = {};
  shop = {};
  stock = {};
  graphics = {};
  gSaveContext = {};
  gPlayState = &play;
  market = privateShop;
  talkAccepted = advance = receiptDone = mirror = false;
  canOffer = canBegin = canStart = reserveWorks = initWorks = true;
  messageState = TEXT_STATE_NONE;
  objectLookups = objectLoads = spawns = kills = adapterInits = beginCalls =
      startCalls = 0;
  nativeSkeletonInits = animationChanges = receiptState = reservations =
      offers = 0;
  playerWaits = playerEnds = collisionQueries = 0;
  invert = 1;
  buyResult = CANBUY_RESULT_NEED_RUPEES;
  callbacks.clear();
  trace.clear();
  nativeCalls.clear();
  lastAAction = lastHud = 0;
  sCameraNextUID = 0;
  gSaveContext.save.day = 1;
  gSaveContext.save.time = CLOCK_TIME(22, 0);
  player.transformation = PLAYER_FORM_HUMAN;
  player.actor.world.pos = {0, 0, -80};
  play.actorCtx.actorLists[ACTORCAT_PLAYER].first = &player.actor;
  play.state.gfxCtx = &graphics;
  graphics.overlay.p = overlay.data();
  play.viewProjectionMtxF.xx = .005f;
  play.viewProjectionMtxF.yy = .005f;
  play.viewProjectionMtxF.ww = 1;
  InitNativeCameraTable();
  Camera_Init(&mainCamera, &play.view, &play.colCtx, &play);
  mainCamera.camId = CAM_ID_MAIN;
  mainCamera.focalActor = &player.actor;
  mainCamera.focalActorPosRot.pos = player.actor.world.pos;
  mainCamera.eye = mainCamera.eyeNext = {0, 70, -80};
  mainCamera.at = {0, 70, -195};
  play.cameraPtrs[0] = &mainCamera;
  Play_ChangeCameraStatus(&play, 0, CAM_STATUS_ACTIVE);
  for (int i = 0; i < 4; ++i) {
    cameraData[i] = {Vec3s{s16(i == 1   ? -35
                               : i == 2 ? 35
                                        : 0),
                           70, -80},
                     Vec3s{0, -32768, 0}, Vec3s{-1, s16(i == 0 ? -1 : 75), -1}};
    cameraInfo[i] = {CAM_SET_FIXED1, 3, cameraData[i].data()};
    cutscenes[i] = {s16(10 + i), -1,  s16(i), -1, s16(i == 3 ? -1 : i + 1),
                    0,           255, 4,      0,  32};
  }
  play.actorCsCamList = cameraInfo.data();
  CutsceneManager_Init(&play, cutscenes.data(), 4);
  sCutsceneMgr.retCamId = 0;
  shop.actor.id = ACTOR_EN_OSSAN;
  shop.actor.params = ENOSSAN_PART_TIME_WORKER;
  shop.actor.csId = 0;
  shop.actor.xzDistToPlayer = 40;
  shop.actor.objectSlot = 0;
  shop.actor.update = EnOssan_Update;
  shop.skelAnime.jointTable = galJoints.data();
  shop.skelAnime.morphTable = galMorphs.data();
  galJoints.fill({11, 22, 33});
  galMorphs.fill({44, 55, 66});
  EnOssan_Init(&shop.actor, &play);
  assert(shop.actionFunc == EnOssan_InitShop && !kills);
  assert(objectLookups == (market ? 0 : 1));
  EnOssan_Update(&shop.actor, &play);
  assert(shop.actionFunc == EnOssan_Idle && !kills);
  assert(adapterInits == (market ? 1 : 0) && spawns == (market ? 0 : 8));
  assert(nativeSkeletonInits == (market ? 0 : 1));
  if (market) {
    assert(shop.skelAnime.jointTable == galJoints.data() &&
           shop.skelAnime.morphTable == galMorphs.data());
    for (int i = 0; i < 24; ++i)
      assert(galJoints[i].x == 11 && galMorphs[i].x == 44);
  }
}

static const char *Action(EnOssanActionFunc action) {
  for (auto [pointer, name] :
       std::array<std::pair<EnOssanActionFunc, const char *>, 19>{
           {{EnOssan_InitShop, "init"},
            {EnOssan_Idle, "idle"},
            {EnOssan_BeginInteraction, "begin"},
            {EnOssan_Hello, "hello"},
            {EnOssan_FaceShopkeeper, "keeper"},
            {EnOssan_TalkToShopkeeper, "talk"},
            {EnOssan_LookToLeftShelf, "lookLeft"},
            {EnOssan_LookToRightShelf, "lookRight"},
            {EnOssan_BrowseLeftShelf, "left"},
            {EnOssan_BrowseRightShelf, "right"},
            {EnOssan_LookToShopkeeperFromShelf, "return"},
            {EnOssan_SelectItem, "select"},
            {EnOssan_CannotBuy, "cannot"},
            {EnOssan_CanBuy, "can"},
            {EnOssan_BuyItemWithFanfare, "buy"},
            {EnOssan_SetupItemPurchased, "receipt"},
            {EnOssan_ItemPurchased, "purchased"},
            {EnOssan_ContinueShopping, "continue"},
            {nullptr, "none"}}})
    if (action == pointer)
      return name;
  assert(false);
  return "unknown";
}
static void Frame(u16 button = 0, int x = 0, int y = 0,
                  bool shouldAdvance = false) {
  play.state.input[0] = {};
  play.state.input[0].press.button = button;
  play.state.input[0].rel.stick_x = x;
  play.state.input[0].rel.stick_y = y;
  advance = shouldAdvance;
  for (auto *item : shop.items)
    if (item)
      Actor_SetFocus(&item->actor, 5);
  nativeCalls.clear();
  EnOssan_Update(&shop.actor, &play);
  Camera *camera = Play_GetCamera(&play, Play_GetActiveCamId(&play));
  if (camera != &mainCamera) {
    assert(camera->setting == CAM_SET_FIXED1 &&
           camera->mode == CAM_MODE_NORMAL);
    assert(sCameraSettings[camera->setting].cameraModes[camera->mode].funcId ==
           CAM_FUNC_FIXED1);
    assert(Camera_Fixed1(camera));
  }
  graphics.overlay.p = overlay.data();
  EnOssan_DrawCursor(&play, &shop, shop.cursorPos.x, shop.cursorPos.y,
                     shop.cursorPos.z, shop.drawCursor);
  EnOssan_DrawStickDirectionPrompts(&play, &shop);
  assert(graphics.overlay.p >= overlay.data() &&
         graphics.overlay.p < overlay.data() + overlay.size());
  char row[512];
  std::snprintf(
      row, sizeof(row),
      "%s/%s cs%d:%d cur%d draw%d stick%d,%d move%d,%d tween%.3f msg%d:%x "
      "hide%d prompt%d,%d rgb%u,%u,%u,%u pos%.3f,%.3f cam%d:%d fov%.3f gfx%td",
      Action(shop.actionFunc), Action(shop.prevActionFunc), shop.cutsceneState,
      shop.csId, shop.cursorIndex, shop.drawCursor, shop.stickAccumX,
      shop.stickAccumY, shop.moveHorizontal, shop.moveVertical,
      shop.shopItemSelectedTween, messageState, play.msgCtx.currentTextId,
      bool(player.stateFlags2 & PLAYER_STATE2_20000000),
      shop.stickLeftPrompt.isEnabled, shop.stickRightPrompt.isEnabled,
      shop.cursorColor.r, shop.cursorColor.g, shop.cursorColor.b,
      shop.cursorColor.a, shop.cursorPos.x, shop.cursorPos.y,
      CutsceneManager_GetCurrentCsId(), camera->setting, camera->fov,
      graphics.overlay.p - overlay.data());
  std::string full(row);
  for (const auto &call : nativeCalls)
    full += " " + call;
  for (const auto &call : callbacks)
    full += " cb:" + call;
  for (auto *item : shop.items) {
    char position[96];
    if (item)
      std::snprintf(position, sizeof(position), " item%d:%.3f,%.3f,%.3f:%d",
                    item->actor.params, item->actor.world.pos.x,
                    item->actor.world.pos.y, item->actor.world.pos.z,
                    item->isSelected);
    else
      std::snprintf(position, sizeof(position), " null");
    full += position;
  }
  trace.emplace_back(full);
  CutsceneManager_Update();
  CutsceneManager_ClearWaiting();
  ++play.gameplayFrames;
  advance = false;
}
static void Until(EnOssanActionFunc wanted, int maximum = 12) {
  while (shop.actionFunc != wanted && maximum-- > 0)
    Frame();
  if (shop.actionFunc != wanted)
    std::fprintf(stderr, "wanted %s got %s\n", Action(wanted),
                 Action(shop.actionFunc));
  assert(shop.actionFunc == wanted);
}
static void Talk() {
  talkAccepted = true;
  Frame();
  Until(EnOssan_Hello);
  Frame(BTN_A, 0, 0, true);
  Until(EnOssan_FaceShopkeeper);
}
static void Bank(bool right) {
  Frame(0, right ? 80 : -80);
  Until(right ? EnOssan_BrowseRightShelf : EnOssan_BrowseLeftShelf);
  Frame();
}
static std::vector<std::string> Flow(bool privateShop) {
  Reset(privateShop);
  Talk();
  Bank(true);
  Frame(BTN_CUP);
  assert(shop.actionFunc == EnOssan_SelectItem && !shop.drawCursor);
  for (int i = 0; i < 6; ++i)
    Frame();
  assert(shop.shopItemSelectedTween == 1);
  Frame(BTN_B);
  assert(shop.actionFunc == EnOssan_BrowseRightShelf);
  for (int i = 0; i < 9; ++i)
    Frame();
  assert(shop.drawCursor && shop.shopItemSelectedTween == 0);
  Frame(0, 80, 80);
  assert(shop.cursorIndex == 3);
  Frame();
  Frame(0, -80);
  assert(shop.cursorIndex == 1);
  Frame();
  Frame(0, -80);
  Until(EnOssan_FaceShopkeeper);
  Bank(false);
  assert(shop.cursorIndex == 5);
  Frame(BTN_B);
  assert(shop.actionFunc == EnOssan_Idle && !shop.drawCursor &&
         !(player.stateFlags2 & PLAYER_STATE2_20000000));
  Frame();
  assert(CutsceneManager_GetCurrentCsId() == CS_ID_NONE);
  return trace;
}

static void Select() {
  Frame(BTN_A);
  assert(shop.actionFunc == EnOssan_SelectItem);
  for (int i = 0; i < 6; ++i)
    Frame();
  assert(shop.shopItemSelectedTween == 1);
}
static void CheckJoystick() {
  for (int strength : {31, 60, 80}) {
    Reset(true);
    Talk();
    Bank(true);
    Frame(0, 80);
    assert(shop.cursorIndex == 2);
    Frame();
    const int firstRepeat = 500 / strength + 1;
    Frame(0, -strength);
    assert(shop.cursorIndex == 0);
    for (int frame = 2; frame < firstRepeat; ++frame) {
      Frame(0, -strength);
      assert(shop.actionFunc == EnOssan_BrowseRightShelf &&
             shop.cursorIndex == 0);
    }
    Frame(0, -strength);
    assert(shop.actionFunc == EnOssan_LookToShopkeeperFromShelf);
  }
  Reset(true);
  Talk();
  Bank(true);
  for (int i = 0; i < 20; ++i)
    Frame(0, 30, -30);
  assert(shop.cursorIndex == 0 && !shop.stickAccumX && !shop.stickAccumY);
  Frame(BTN_DUP | BTN_DRIGHT);
  assert(shop.cursorIndex == 0 && "native MM shop navigation uses the stick");
  invert = -1;
  Frame(0, -80, 80);
  assert(shop.cursorIndex == 3);
  Frame(0, 80);
  assert(shop.cursorIndex == 1 && shop.moveHorizontal &&
         shop.stickAccumX == -80);
  Frame();
  assert(shop.stickAccumX == 0);
  Reset(true);
  Talk();
  invert = -1;
  Frame(0, 80);
  Until(EnOssan_BrowseLeftShelf);
  assert(shop.cursorIndex == 4);
  std::puts("MM actual EnOssan: magnitude-dependent repeat, deadzone, "
            "release/reversal, diagonal and inversion passed");
}
static void CheckCursor() {
  Reset(true);
  Talk();
  Bank(true);
  auto *camera = Play_GetCamera(&play, Play_GetActiveCamId(&play));
  assert(std::abs(camera->eye.x - 35) > .2f &&
         "fixture must still be inside the native bank interpolation");
  Frame(BTN_A);
  assert(shop.actionFunc == EnOssan_SelectItem &&
         "native selection has no pose-settle gate");
  Reset(true);
  Talk();
  Bank(true);
  for (int index = 0; index < 8; ++index) {
    shop.cursorIndex = index;
    shop.actionFunc =
        index < 4 ? EnOssan_BrowseRightShelf : EnOssan_BrowseLeftShelf;
    Frame();
    const float x = shop.cursorPos.x, y = shop.cursorPos.y;
    assert(x == s16(160 + stock[index].actor.focus.pos.x * .8f));
    assert(y == s16(120 - stock[index].actor.focus.pos.y * .6f));
    assert(shop.cursorColor.r == 0 && shop.cursorColor.b == 255 &&
           shop.cursorColor.a == 255);
    assert(shop.cursorColor.g >= 0 && shop.cursorColor.g <= 80);
    assert((overlay[0].words.w0 >> 24) == G_SETPRIMCOLOR);
    assert((overlay[0].words.w1 & 0xFFFFFF) ==
           ((shop.cursorColor.g << 16) | 0xFFFF));
    bool found = false;
    for (auto *packet = overlay.data(); packet < graphics.overlay.p; ++packet) {
      if ((packet->words.w0 >> 24) != G_TEXRECT)
        continue;
      assert(((packet->words.w0 >> 12) & 0xFFF) == int((x + 24) * 4));
      assert((packet->words.w0 & 0xFFF) == int((y + 24) * 4));
      assert(((packet->words.w1 >> 12) & 0xFFF) == int((x - 24) * 4));
      assert((packet->words.w1 & 0xFFF) == int((y - 24) * 4));
      assert(packet[2].words.w1 == ((682u << 16) | 682));
      found = true;
      break;
    }
    assert(found);
    mirror = true;
    Frame();
    assert(shop.cursorPos.x == 320 - x && shop.cursorPos.y == y);
    mirror = false;
  }
  std::puts("MM actual EnOssan: native projection, all8 mirrored cursor "
            "anchors, pulse color and48px GBI rectangle passed");
}
static void CheckAvailabilityAndCancellation() {
  Reset(true);
  canOffer = false;
  Frame();
  assert(!offers && shop.actionFunc == EnOssan_Idle);
  canOffer = true;
  Frame();
  assert(offers == 1);
  canBegin = false;
  talkAccepted = true;
  Frame();
  assert(beginCalls == 1 && shop.actionFunc == EnOssan_Idle &&
         !(player.stateFlags2 & PLAYER_STATE2_20000000));
  assert(messageState == TEXT_STATE_CLOSING &&
         CutsceneManager_GetCurrentCsId() == CS_ID_NONE);
  Reset(true);
  canStart = false;
  talkAccepted = true;
  Frame();
  for (int i = 0; i < 4; ++i)
    Frame(BTN_B);
  assert(shop.actionFunc == EnOssan_BeginInteraction && !startCalls &&
         messageState == TEXT_STATE_NONE);
  canStart = true;
  Frame();
  assert(shop.actionFunc == EnOssan_Hello && startCalls == 1);
  Frame(BTN_A, 0, 0, true);
  Frame(0, 80);
  assert(shop.actionFunc == EnOssan_LookToRightShelf);
  Frame(BTN_B);
  assert(shop.actionFunc == EnOssan_LookToRightShelf);
  Until(EnOssan_BrowseRightShelf);
  Frame(BTN_B);
  assert(shop.actionFunc == EnOssan_Idle);
  Reset(true);
  Talk();
  shop.items[0] = nullptr;
  Bank(true);
  assert(shop.cursorIndex == 2);
  stock[2].isOutOfStock = true;
  Frame(BTN_A);
  assert(shop.actionFunc == EnOssan_BrowseRightShelf);
  stock[2].isOutOfStock = false;
  Frame(BTN_A);
  for (int i = 0; i < 5; ++i) {
    Frame(BTN_B);
    assert(shop.actionFunc == EnOssan_SelectItem);
  }
  Frame(BTN_B);
  assert(shop.actionFunc == EnOssan_BrowseRightShelf);
  std::puts("MM actual EnOssan: talk/start denial, queued retry, native turn "
            "input, missing/soldout stock and early item-cancel passed");
}
static void CheckFormsAndInitFailure() {
  for (int form = 0; form < PLAYER_FORM_MAX; ++form) {
    Reset(true);
    player.transformation = static_cast<PlayerTransformation>(form);
    Talk();
    Bank(true);
    Frame(BTN_B);
    assert(shop.actionFunc == EnOssan_Idle);
    assert(nativeSkeletonInits == 0 && animationChanges == 0 &&
           "private Gal must not run a Trading Post skeleton animation for any "
           "form");
  }
  Reset(false);
  player.transformation = PLAYER_FORM_DEKU;
  talkAccepted = true;
  Frame();
  Until(EnOssan_Hello);
  Frame(BTN_A, 0, 0, true);
  assert(shop.actionFunc == EnOssan_Idle &&
         "ordinary Trading Post Deku refusal must remain native");
  Reset(true);
  initWorks = false;
  shop.actionFunc = EnOssan_InitShop;
  Frame();
  assert(kills == 1 && !shop.actor.update && adapterInits == 2 &&
         nativeSkeletonInits == 0);
  std::puts("MM actual EnOssan: all5 private forms, unchanged ordinary Deku "
            "refusal and adapter-init failure passed");
}
static void CheckPurchases() {
  for (int result :
       {CANBUY_RESULT_NO_ROOM, CANBUY_RESULT_NO_ROOM_2,
        CANBUY_RESULT_NEED_EMPTY_BOTTLE, CANBUY_RESULT_NEED_RUPEES,
        CANBUY_RESULT_CANNOT_GET_NOW, CANBUY_RESULT_CANNOT_GET_NOW_2}) {
    Reset(true);
    Talk();
    Bank(true);
    Select();
    buyResult = result;
    const auto camera = Play_GetActiveCamId(&play);
    Frame(0, 0, 0, true);
    assert(shop.actionFunc == EnOssan_CannotBuy && reservations == 0 &&
           callbacks == std::vector<std::string>{"canBuy"});
    assert(Play_GetActiveCamId(&play) == camera && shop.cutsceneState == 2);
    Frame(0, 0, 0, true);
    assert(shop.actionFunc == EnOssan_BrowseRightShelf);
  }
  Reset(true);
  Talk();
  Bank(true);
  Select();
  buyResult = CANBUY_RESULT_SUCCESS_2;
  Frame(0, 0, 0, true);
  assert(shop.actionFunc == EnOssan_CanBuy &&
         callbacks == (std::vector<std::string>{"canBuy", "buy", "bought"}));
  Frame(0, 0, 0, true);
  assert(shop.actionFunc == EnOssan_BrowseRightShelf &&
         callbacks.back() == "restock");
  assert(shop.shopItemSelectedTween == 0 && stock[0].actor.world.pos.z == -195);
  Reset(true);
  Talk();
  Bank(true);
  Select();
  buyResult = CANBUY_RESULT_SUCCESS_1;
  reserveWorks = false;
  const auto bankCamera = Play_GetActiveCamId(&play);
  Frame(0, 0, 0, true);
  assert(shop.actionFunc == EnOssan_CannotBuy && reservations == 1);
  assert(shop.cutsceneState == 2 && Play_GetActiveCamId(&play) == bankCamera &&
         sCutsceneMgr.endCsId == CS_ID_NONE);
  assert(callbacks == std::vector<std::string>{"canBuy"});
  for (bool continueShopping : {false, true}) {
    Reset(true);
    Talk();
    Bank(true);
    Select();
    buyResult = CANBUY_RESULT_SUCCESS_1;
    Frame(0, 0, 0, true);
    assert(shop.actionFunc == EnOssan_BuyItemWithFanfare && reservations == 1 &&
           !shop.drawCursor);
    assert(!(player.stateFlags2 & PLAYER_STATE2_20000000) &&
           shop.cutsceneState == 0);
    assert(callbacks ==
           (std::vector<std::string>{"canBuy", "fanfare", "bought"}));
    for (int i = 0; i < 3; ++i)
      Frame();
    assert(shop.actionFunc == EnOssan_BuyItemWithFanfare && reservations == 1);
    receiptState = 1;
    Frame();
    assert(shop.actionFunc == EnOssan_SetupItemPurchased);
    messageState = TEXT_STATE_DONE;
    Frame(BTN_A, 0, 0, true);
    assert(shop.actionFunc == EnOssan_SetupItemPurchased &&
           "native private receipt owns its whole tutorial/choice chain");
    receiptDone = true;
    Frame();
    assert(shop.actionFunc == EnOssan_ItemPurchased);
    talkAccepted = true;
    Frame();
    Until(EnOssan_ContinueShopping);
    assert(play.msgCtx.currentTextId == 0x642 &&
           messageState == TEXT_STATE_CHOICE && shop.csId == 0);
    play.msgCtx.choiceIndex = continueShopping ? 0 : 1;
    Frame(0, 0, 0, true);
    assert(callbacks.back() == "restock");
    assert(shop.actionFunc ==
           (continueShopping ? EnOssan_FaceShopkeeper : EnOssan_Idle));
  }
  Reset(true);
  Talk();
  Bank(true);
  Select();
  buyResult = CANBUY_RESULT_SUCCESS_1;
  Frame(0, 0, 0, true);
  receiptState = -1;
  Frame();
  assert(shop.actionFunc == EnOssan_Idle);
  Reset(false);
  Talk();
  Bank(true);
  Select();
  buyResult = CANBUY_RESULT_SUCCESS_1;
  Frame(0, 0, 0, true);
  assert(shop.actionFunc == EnOssan_BuyItemWithFanfare && reservations == 0);
  assert(callbacks ==
         (std::vector<std::string>{"canBuy", "fanfare", "offer", "bought"}));
  shop.actor.parent = &player.actor;
  Frame();
  assert(shop.actionFunc == EnOssan_SetupItemPurchased && !shop.actor.parent);
  messageState = TEXT_STATE_DONE;
  Frame(BTN_A, 0, 0, true);
  assert(shop.actionFunc == EnOssan_ItemPurchased);
  std::puts("MM actual EnOssan: all failure classes, both native success "
            "paths, reservation failure, receipt pending/accepted/cancelled, "
            "continue/end and vanilla delivery passed");
}

static void CheckOriginalKeeperTalk() {
  for (bool privateShop : {false, true}) {
    Reset(privateShop); Talk();
    play.msgCtx.choiceIndex = 0;
    Frame(BTN_A, 0, 0, true);
    assert(shop.actionFunc == EnOssan_TalkToShopkeeper && messageState == TEXT_STATE_EVENT);
    Frame(BTN_A, 0, 0, true);
    assert(shop.actionFunc == EnOssan_FaceShopkeeper && messageState == TEXT_STATE_CHOICE &&
           reservations == 0 && callbacks.empty());
  }
  std::puts("MM actual EnOssan: ordinary and adapter-with-null-dialogue retain native Talk EVENT/return");
}
int main(int argc, char **argv) {
  if (argc == 2 && std::string(argv[1]) == "vanilla-trace") {
    for (const auto &frame : Flow(false))
      std::puts(frame.c_str());
    return 0;
  }
  auto expected = Flow(false);
  auto actual = Flow(true);
  assert(expected.size() == actual.size());
  for (size_t i = 0; i < expected.size(); ++i) {
    if (expected[i] != actual[i])
      std::fprintf(stderr, "frame%zu native: %s\n        private: %s\n", i,
                   expected[i].c_str(), actual[i].c_str());
    assert(expected[i] == actual[i]);
  }
  assert(beginCalls == 1 && startCalls == 4);
  std::puts(
      "MM actual EnOssan: private/default frame parity, single-B exit, both "
      "banks, diagonal input, C-Up, native tween and cursor GBI passed");
  CheckOriginalKeeperTalk();
  CheckJoystick();
  CheckCursor();
  CheckAvailabilityAndCancellation();
  CheckFormsAndInitFailure();
  CheckPurchases();
}
