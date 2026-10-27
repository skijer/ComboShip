using namespace ComboRando::CrossDungeon;
using namespace ComboCrossMmCatalogue;
void InitRegions() {
    using namespace Rando::Logic;
    auto portal=[](const PortalDescriptor& p) {
        Regions[p.region].sceneId=1000+p.region;
        Regions[p.region].exits[p.exit]={p.entrance};
        Regions[p.sourceRegion].exits[p.entrance]={p.exit};
    };
    for(const auto& p:Interiors())portal(p);
    for(const auto& p:ExpandedDungeons())portal(p);
    for(const auto& g:Grottos()) {
        Regions[g.region].sceneId=SCENE_KAKUSIANA;
        Regions[g.region].exits[g.nativeReturn]={g.Entry()};
        Regions[g.sourceRegion].exits[g.Entry()]={g.nativeReturn};
    }
}
nlohmann::json ForeignGrottoSeed(int logical) {
    const auto* g=FindGrottoByEntry(ENTRANCE(GROTTOS,logical));assert(g);
    Dungeon foreign{"foreign",{0,100},{0,101},{0,102}};foreign.pool=Pool::Grotto;
    Dungeon mm{"mm",{1,g->Entry()},{1,g->Exit()},{1,g->Exit()}};mm.pool=Pool::Grotto;
    return Encode({foreign,mm},{1,0});
}
void CheckGrottoReturnPlayerStartModes() {
    int checked=0;
    for(const auto& hole:Grottos()) {
        if(!hole.holeReturn)continue;
        const auto seed=ForeignGrottoSeed(hole.logicalSpawn);
        assert(MM_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
        gSaveContext={};play.sceneId=1000;
        gSaveContext.respawn[RESPAWN_MODE_UNK_3].data=hole.hole.data;
        int32_t entry=hole.PhysicalEntry();
        assert(Combo_MM_CrossDungeonTransition(&entry));
        assert(Combo_MM_CompleteCrossDungeonTransition());
        assert(ComboCrossMmRuntime::PrepareSwitch());
        assert(gSaveContext.save.shipSaveInfo.pauseSaveEntrance>=0);
        // A foreign grave/interior now returns to this MM hole. Real save-load
        // state must not replace its native jump with a stationary start.
        gSaveContext.save.isOwlSave=true;
        gComboEntryIsResume=(checked%2)==0;
        assert(MM_QueueCrossDungeonArrival(hole.Exit()));
        NativeTitleArrival();
        assert(gSaveContext.respawnFlag==4);
        const int params=gSaveContext.respawn[RESPAWN_MODE_UNK_3].playerParams;
        assert(params==PLAYER_PARAMS(0xFF,PLAYER_START_MODE_GROTTO));
        assert(NativePlayerStartMode((params>>8)&0xF)==PLAYER_START_MODE_GROTTO &&
               "cross-game return must keep the native jump out of the hole");
        assert(gSaveContext.save.shipSaveInfo.pauseSaveEntrance==-1);
        assert(!gSaveContext.save.isOwlSave);
        ++checked;
    }
    assert(checked>0);
    assert(MM_SetCrossDungeonRuntimeSeed("null"));
    Combo_MM_LoadCrossDungeonCheckpoint(-1);
    gSaveContext={};
    gSaveContext.save.shipSaveInfo.pauseSaveEntrance=123;
    gSaveContext.save.isOwlSave=true;
    gComboEntryIsResume=true;
    assert(!MM_QueueCrossDungeonArrival(0xFFFF));
    NativeTitleArrival();
    assert(gSaveContext.save.shipSaveInfo.pauseSaveEntrance==123);
    assert(gSaveContext.save.isOwlSave);
    assert(NativePlayerStartMode(PLAYER_START_MODE_GROTTO)==PLAYER_START_MODE_D);
    assert(gSaveContext.save.shipSaveInfo.pauseSaveEntrance==-1);
    assert(!gSaveContext.save.isOwlSave);
    gSaveContext={};gComboEntryIsResume=false;
    std::cout<<"MM native player startup: "<<checked<<" grotto returns and native resume controls PASS\n";
}
int main() {
    InitRegions();
    CheckGrottoReturnPlayerStartModes();
    assert(nlohmann::json::parse(MM_DumpCrossDungeonCatalogue()).size()==5);
    assert(nlohmann::json::parse(MM_DumpCrossDungeonCatalogueForOptions("{\"dungeons\":false,\"bosses\":true}")).size()==4);
    assert(nlohmann::json::parse(MM_DumpCrossDungeonCatalogueForOptions("{\"bosses\":true}")).size()==9);
    auto all=nlohmann::json::parse(MM_DumpCrossDungeonCatalogueForOptions("{\"expanded\":true,\"interiors\":true,\"grottos\":true}"));
    assert(all.size()==73);
    assert(nlohmann::json::parse(MM_DumpCrossDungeonCatalogueForOptions("{\"dungeons\":false,\"grottos\":true}")).size()==26);
    assert(nlohmann::json::parse(MM_DumpCrossDungeonCatalogueForSeed(
        "{\"crossDungeonEntrances\":{\"version\":2,\"options\":{\"dungeons\":false,\"interiors\":true}}}")).size()==42);
    assert(nlohmann::json::parse(MM_DumpCrossDungeonCatalogueForSeed(
        "{\"crossDungeonEntrances\":{\"version\":2,\"options\":{\"dungeons\":true,\"expanded\":true,\"bosses\":true,\"interiors\":true,\"grottos\":true}}}")).size()==84);
    assert(nlohmann::json::parse(MM_DumpCrossDungeonCatalogueForSeed("{}")).size()==5);
    assert(nlohmann::json::parse(MM_DumpCrossDungeonCatalogueForSeed("{\"crossDungeonEntrances\":null}")).size()==5);
    assert(nlohmann::json::parse(MM_DumpCrossDungeonCatalogueForSeed(
        "{\"options\":{\"interiors\":true}}")).size()==5); // Consolidated options live in the nested entrance seed.
    assert(nlohmann::json::parse(MM_DumpCrossDungeonCatalogueForOptions("{\"grottos\":1}")).empty());
    for(const auto& g:Grottos()) {
        assert(Rando::Logic::GetRegionIdFromEntrance(g.Entry())==g.region);
        assert(Rando::Logic::GetRegionIdFromEntrance(g.Exit())==g.sourceRegion);
    }
    assert(Rando::Logic::GetRegionIdFromEntrance(ENTRANCE(MOUNTAIN_VILLAGE_SPRING,7))==RR_MOUNTAIN_VILLAGE);
    const auto* g=FindGrottoByEntry(ENTRANCE(GROTTOS,18));assert(g);
    canPlayTime=false;clockShuffle=false;assert(!MM_CanEnterCrossEntrance(g->Entry()));
    canPlayTime=true;assert(MM_CanEnterCrossEntrance(g->Entry()));
    clockShuffle=true;rawClocks=0;assert(!MM_CanEnterCrossEntrance(g->Entry()));
    rawClocks=0x20;assert(MM_CanEnterCrossEntrance(g->Entry()));
    assert(!MM_CanEnterCrossEntrance(0xF004));
    auto seed=ForeignGrottoSeed(18);
    assert(MM_SetCrossDungeonSeed(seed.dump().c_str()));
    ComboCrossMm::searching=true;
    assert(Combo_MM_CrossDungeonEntranceFromRegion(g->nativeReturn,g->region)==-1);
    // Another grotto sharing the same exterior entrance must not take this return's route.
    assert(Combo_MM_CrossDungeonEntranceFromRegion(g->nativeReturn,RR_TERMINA_FIELD_TALL_GRASS_GROTTO)==g->nativeReturn);
    ComboCrossMm::reachable={g->region};
    assert(nlohmann::json::parse(MM_GetCrossDungeonExits()).at(0)==EndpointJson({0,102}));
    ComboCrossMm::searching=false;
    assert(Combo_MM_CrossDungeonEntranceFromRegion(g->nativeReturn,g->region)==g->nativeReturn);
    Dungeon logicalBoss;logicalBoss.pool=Pool::Boss;
    ComboCrossMm::state.dungeons.push_back(logicalBoss);
    bool physicalGoht=false;
    auto& condition=Rando::Logic::Regions[g->region].exits[g->nativeReturn].condition;
    condition=[&] { return Combo_MM_CrossBossLogicalClear(COMBO_BOSS_GOHT,physicalGoht); };
    ComboCrossMm::state.logicalClears=1u<<COMBO_BOSS_GOHT;
    assert(nlohmann::json::parse(MM_GetCrossDungeonExits()).size()==1);
    assert(!ComboCrossMm::searching); // Restore the SearchGuard's completed state.
    physicalGoht=true;ComboCrossMm::state.logicalClears=0;
    assert(nlohmann::json::parse(MM_GetCrossDungeonExits()).empty());
    assert(!ComboCrossMm::searching); // Physical completion cannot open a logically closed source.
    ComboCrossMm::searching=true;
    assert(nlohmann::json::parse(MM_GetCrossDungeonExits()).empty());
    assert(ComboCrossMm::searching); // Preserve an already active oracle caller.
    ComboCrossMm::searching=false;
    condition=[] { throw std::runtime_error("condition failed"); return false; };
    try { MM_GetCrossDungeonExits();assert(false); } catch(const std::runtime_error&) {}
    assert(!ComboCrossMm::searching);
    condition=[] {return true;};
    assert(MM_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    gSaveContext={};play.sceneId=1000;gSaveContext.respawn[3].data=0x9A;
    int32_t index=ENTRANCE(GROTTOS,4);
    assert(Combo_MM_CrossDungeonTransition(&index)==1 && ComboCrossMmRuntime::pending);
    assert(!ComboCrossMmRuntime::PrepareSwitch());
    assert(Combo_MM_CompleteCrossDungeonTransition());
    assert(ComboCrossMmRuntime::PrepareSwitch() && queued==100);
    assert(gSaveContext.save.entrance==g->hole.entrance);
    assert(Combo_MM_SaveCrossDungeonCheckpoint()==g->Exit());
    assert(MM_QueueCrossDungeonArrival(g->Entry()));
    assert(Combo_MM_ConsumeCrossDungeonArrival()==g->PhysicalEntry());
    assert(gSaveContext.respawn[3].data==0x9A);
    assert(Combo_MM_ConsumeCrossDungeonArrival()==-1);
    assert(MM_QueueCrossDungeonArrival(g->Exit()));
    assert(Combo_MM_ConsumeCrossDungeonArrival()==g->hole.entrance && gSaveContext.respawnFlag==4);
    assert(Combo_MM_SaveCrossDungeonCheckpoint()==-1);
    Combo_MM_LoadCrossDungeonCheckpoint(g->Entry());
    assert(Combo_MM_ConsumeCrossDungeonRecovery()==g->PhysicalEntry());
    assert(gSaveContext.respawn[3].data==0x9A && Combo_MM_SaveCrossDungeonCheckpoint()==g->Entry());
    const auto* other=FindGrottoByEntry(ENTRANCE(GROTTOS,19));assert(other);
    Dungeon a{"a",{1,g->Entry()},{1,g->Exit()},{1,g->Exit()}};a.pool=Pool::Grotto;
    Dungeon b{"b",{1,other->Entry()},{1,other->Exit()},{1,other->Exit()}};b.pool=Pool::Grotto;
    const auto local=Encode({a,b},{1,0});
    assert(MM_SetCrossDungeonRuntimeSeed(local.dump().c_str()));
    gSaveContext={};gSaveContext.respawn[3].data=0x9A;play.sceneId=1000;
    index=ENTRANCE(GROTTOS,4);
    assert(!Combo_MM_CrossDungeonTransition(&index) && index==other->PhysicalEntry());
    assert(gSaveContext.respawn[3].data==0x3F); // Exactly one composed leg.
    ComboCrossMmRuntime::ObserveScene(SCENE_KAKUSIANA);
    assert(Combo_MM_SaveCrossDungeonCheckpoint()==other->Entry());
    play.sceneId=SCENE_KAKUSIANA;play.roomCtx.curRoom.num=4;
    gSaveContext.respawnFlag=1;index=other->hole.entrance;
    assert(!Combo_MM_CrossDungeonTransition(&index) && index==other->hole.entrance);
    gSaveContext.respawnFlag=4;index=other->hole.entrance;
    assert(!Combo_MM_CrossDungeonTransition(&index) && index==g->hole.entrance);
    assert(gSaveContext.respawnFlag==4 && gSaveContext.respawn[3].data==0);
    assert(Combo_MM_SaveCrossDungeonCheckpoint()==-1); // A routed return releases ownership immediately.
    ComboCrossMmRuntime::ObserveScene(99999);
    assert(Combo_MM_SaveCrossDungeonCheckpoint()==-1);
    auto malformed=local;malformed["dungeons"][0]["exit"]["index"]=0xF004;
    assert(!MM_SetCrossDungeonRuntimeSeed(malformed.dump().c_str()));
    assert(MM_SetCrossDungeonRuntimeSeed(seed.dump().c_str()));
    Combo_MM_LoadCrossDungeonCheckpoint(g->Entry());
    assert(MM_QueueCrossDungeonArrival(g->Exit()));
    gComboEntryIsResume=true;
    NativeTitleArrival();
    assert(gSaveContext.save.entrance==g->hole.entrance && gSaveContext.respawnFlag==4);
    assert(gSaveContext.respawn[3].pos.x==g->hole.x && gSaveContext.save.cutsceneIndex==0);
    assert(Combo_MM_ConsumeCrossDungeonRecovery()==-1);
    Combo_MM_LoadCrossDungeonCheckpoint(g->Entry());
    gComboEntryIsResume=false;gSaveContext.save.entrance=200;gSaveContext.respawnFlag=-7;
    NativeTitleArrival();
    assert(gSaveContext.save.entrance==200 && gSaveContext.respawnFlag==-7);
    gComboEntryIsResume=true;
    NativeTitleArrival();
    assert(gSaveContext.save.entrance==g->PhysicalEntry() && gSaveContext.respawnFlag==0);
    assert(gSaveContext.respawn[3].data==0x9A);
    // The real Bomb Shop portal pairs with the actual OoT potion rear doorway.
    PoolOptions shopOptions;shopOptions.dungeons=false;shopOptions.shops=true;
    std::vector<Dungeon> shopCatalogue;Append(shopCatalogue,shopOptions);
    const auto bomb=std::find_if(shopCatalogue.begin(),shopCatalogue.end(),[](const Dungeon& d){
        return d.entrance.index==ENTRANCE(BOMB_SHOP,0);
    });
    assert(bomb!=shopCatalogue.end() && bomb->exit.index==ENTRANCE(WEST_CLOCK_TOWN,6));
    Dungeon potion{"oot:potion-back",{0,OOT_ENTR_POTION_SHOP_KAKARIKO_BACK},
        {0,OOT_ENTR_KAKARIKO_VILLAGE_OUTSIDE_POTION_SHOP_BACK},{0,OOT_ENTR_KAKARIKO_VILLAGE_OUTSIDE_POTION_SHOP_BACK}};
    potion.pool=Pool::Shop;
    const auto shopSeed=Encode({potion,*bomb},{1,0});
    assert(MM_SetCrossDungeonRuntimeSeed(shopSeed.dump().c_str()));
    gSaveContext={};play.sceneId=Rando::Logic::Regions[RR_BOMB_SHOP].sceneId;
    assert(MM_QueueCrossDungeonArrival(bomb->entrance.index));
    assert(Combo_MM_ConsumeCrossDungeonArrival()==bomb->entrance.index);
    index=bomb->exit.index;
    assert(Combo_MM_CrossDungeonTransition(&index)==1 && ComboCrossMmRuntime::pending);
    assert(ComboCrossMmRuntime::pending->to==potion.returnTo);
    assert(Combo_MM_CompleteCrossDungeonTransition());
    assert(ComboCrossMmRuntime::PrepareSwitch() && queued==potion.returnTo.index);
    assert(gSaveContext.save.entrance==bomb->entrance.index);
    index=bomb->entrance.index;
    assert(Combo_MM_CrossDungeonTransition(&index)==1 && ComboCrossMmRuntime::pending->to==potion.entrance);
    Combo_MM_CancelCrossDungeonTransition();
    std::cout<<"MM adapters: options, logical region/source identity, raw clocks, one-leg runtime, native saves and recovery PASS\n";
}
