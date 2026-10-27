using namespace ComboRando::CrossDungeon;
using namespace ComboCrossMmCatalogue;
const BetterMapSelectGrottoRespawnInfo& NativeGrotto(int logical) {
    switch(logical) {
    case 17:return sBetterMapSelectCowGrottoInfo[0];
    case 18:return sBetterMapSelectChestGrottoInfo[1];
    case 19:return sBetterMapSelectChestGrottoInfo[0];
    case 20:return sBetterMapSelectChestGrottoInfo[2];
    case 21:return sBetterMapSelectChestGrottoInfo[4];
    case 22:return sBetterMapSelectChestGrottoInfo[3];
    case 23:return sBetterMapSelectChestGrottoInfo[5];
    case 24:return sBetterMapSelectChestGrottoInfo[6];
    case 25:return sBetterMapSelectChestGrottoInfo[7];
    case 26:return sBetterMapSelectChestGrottoInfo[8];
    case 27:return sBetterMapSelectCowGrottoInfo[1];
    case 28:return sBetterMapSelectChestGrottoInfo[9];
    case 29:return sBetterMapSelectChestGrottoInfo[12];
    case 30:return sBetterMapSelectChestGrottoInfo[11];
    case 31:return sBetterMapSelectChestGrottoInfo[10];
    default:return sBetterMapSelectGrottoInfo[logical];
    }
}
void AssertNativeDescriptor(const RespawnDescriptor& actual, const BetterMapSelectRespawnInfo& native) {
    assert(actual.entrance==native.entrance && actual.room==native.roomIndex && actual.data==native.data);
    assert(actual.yaw==native.yaw && actual.x==native.pos.x && actual.y==native.pos.y && actual.z==native.pos.z);
}
int main() {
    CheckNativePortals();
    // Retail shops are separate from minigames, services and the back hideout.
    const std::set<int> shopDoors={ENTRANCE(MAGIC_HAGS_POTION_SHOP,0),ENTRANCE(ZORA_HALL_ROOMS,5),
        ENTRANCE(CURIOSITY_SHOP,0),ENTRANCE(TRADING_POST,0),ENTRANCE(BOMB_SHOP,0),ENTRANCE(MILK_BAR,0),ENTRANCE(GORON_SHOP,0)};
    const std::set<int> otherDoors={ENTRANCE(MARINE_RESEARCH_LAB,0),ENTRANCE(ZORA_HALL_ROOMS,2),
        ENTRANCE(ZORA_HALL_ROOMS,3),ENTRANCE(ZORA_HALL_ROOMS,1),ENTRANCE(ZORA_HALL_ROOMS,0),
        ENTRANCE(SWAMP_SHOOTING_GALLERY,0),ENTRANCE(TOURIST_INFORMATION,0),
        ENTRANCE(MUSIC_BOX_HOUSE,0),ENTRANCE(RANCH_HOUSE,0),ENTRANCE(RANCH_HOUSE,1),
        ENTRANCE(CUCCO_SHACK,0),ENTRANCE(DOGGY_RACETRACK,0),ENTRANCE(ASTRAL_OBSERVATORY,1),
        ENTRANCE(TREASURE_CHEST_SHOP,0),ENTRANCE(HONEY_AND_DARLINGS_SHOP,0),
        ENTRANCE(MAYORS_RESIDENCE,0),ENTRANCE(TOWN_SHOOTING_GALLERY,0),ENTRANCE(STOCK_POT_INN,0),
        ENTRANCE(STOCK_POT_INN,1),ENTRANCE(CURIOSITY_SHOP,1),ENTRANCE(SWORDMANS_SCHOOL,0),
        ENTRANCE(POST_OFFICE,0),ENTRANCE(LOTTERY_SHOP,0),ENTRANCE(MOUNTAIN_SMITHY,0)};
    auto narrowOptions=ParseOptions({{"dungeons",false},{"shops",true},{"doorInteriors",true}});
    std::vector<Dungeon> narrow;Append(narrow,narrowOptions);
    assert(narrow.size()==shopDoors.size()+otherDoors.size());
    for(const auto& d:narrow) {
        assert(static_cast<int>(d.pool)==(shopDoors.count(d.entrance.index)?4:5));
        assert(shopDoors.count(d.entrance.index)||otherDoors.count(d.entrance.index));
    }
    for(int open:{ENTRANCE(DEKU_SCRUB_PLAYGROUND,0),ENTRANCE(FISHERMANS_HUT,0),ENTRANCE(GHOST_HUT,0),
        ENTRANCE(GORON_SHRINE,0),ENTRANCE(GORON_GRAVERYARD,0),ENTRANCE(ASTRAL_OBSERVATORY,0),
        ENTRANCE(FAIRY_FOUNTAIN,0),ENTRANCE(FAIRY_FOUNTAIN,1),ENTRANCE(FAIRY_FOUNTAIN,2),
        ENTRANCE(FAIRY_FOUNTAIN,3),ENTRANCE(FAIRY_FOUNTAIN,4),ENTRANCE(CLOCK_TOWER_INTERIOR,1)})
        for(const auto& d:narrow)assert(d.entrance.index!=open);
    auto broadOptions=narrowOptions;broadOptions.interiors=true;
    std::vector<Dungeon> broad;Append(broad,broadOptions);assert(broad.size()==42);
    for(const auto& d:broad)assert(d.pool==(shopDoors.count(d.entrance.index)?Pool::Shop:Pool::Interior));
    auto withoutShops=broadOptions;withoutShops.shops=false;
    std::vector<Dungeon> nonretail;Append(nonretail,withoutShops);
    assert(nonretail.size()==42-shopDoors.size());
    for(const auto& d:nonretail)assert(!shopDoors.count(d.entrance.index));
    for(bool broadChoice:{false,true})for(bool doorChoice:{false,true})for(bool shopChoice:{false,true}) {
        auto choice=ParseOptions({{"dungeons",false},{"interiors",broadChoice},{"doorInteriors",doorChoice},{"shops",shopChoice}});
        std::vector<Dungeon> selected;Append(selected,choice);size_t retail=0;
        for(const auto& d:selected)if(shopDoors.count(d.entrance.index)) {
            ++retail;assert(shopChoice && d.pool==Pool::Shop);
        }
        assert(retail==(shopChoice?shopDoors.size():0));
    }
    for(int version:{2,3}) {
        const auto oldBroad=OptionsFromSeed({{"version",version},{"options",{{"dungeons",false},{"interiors",true}}}});
        std::vector<Dungeon> selected;Append(selected,oldBroad);assert(selected.size()==42);
        for(const auto& d:selected)assert(d.pool==Pool::Interior);
    }
    const auto oldShops=OptionsFromSeed({{"version",3},{"options",{{"dungeons",false},{"shops",true}}}});
    std::vector<Dungeon> oldRetail;Append(oldRetail,oldShops);assert(oldRetail.size()==6);
    for(const auto& d:oldRetail)assert(d.entrance.index!=ENTRANCE(GORON_SHOP,0));
    PoolOptions options;
    std::vector<Dungeon> catalog;
    Append(catalog,options);
    assert(catalog.empty());
    options.expanded=true;
    Append(catalog,options);
    assert(catalog.size()==7);
    for (const auto& d:catalog) assert(d.expanded && d.pool==Pool::Dungeon);
    assert(catalog[4].entrance.index==ENTRANCE(BENEATH_THE_WELL,0));
    assert(catalog[5].entrance.index==ENTRANCE(BENEATH_THE_WELL,1));
    catalog.clear(); options={};options.interiors=true;options.grottos=true;
    Append(catalog,options);
    assert(catalog.size()==61);
    for(const auto& d:catalog)
        assert(d.entrance.index!=ENTRANCE(CLOCK_TOWER_INTERIOR,1));
    Validate(catalog);
    std::set<int> expected={0,1,2,3,5,7,9,11,12,13,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
    assert(Grottos().size()==expected.size());
    for(const auto& g:Grottos()) {
        assert(expected.erase(g.logicalSpawn)==1);
        const auto& native=NativeGrotto(g.logicalSpawn);
        AssertNativeDescriptor(g.down,native.downRespawn);
        AssertNativeDescriptor(g.hole,native.grottoRespawn);
        assert(FindGrottoByEntry(g.Entry())==&g);
        assert(FindGrottoByExit(g.Exit())==&g);
        assert(GrottoExitForRegion(g.region)==g.Exit());
        assert(RegionForGrottoReturn(g.Exit())==g.sourceRegion);
        assert(CanonicalGrottoLoad(g.PhysicalEntry(),g.hole.data)==g.Entry());
        gSaveContext={}; gSaveContext.respawnFlag=-2;
        snowhead=(g.logicalSpawn==23);
        const int physical=ComboMmMaterializeGrotto(g.Entry());
        assert(physical==g.PhysicalEntry());
        assert(gSaveContext.respawnFlag==0);
        if(g.holeReturn) {
            const auto& rs=gSaveContext.respawn[3];
            assert(rs.data==g.hole.data && rs.entrance==g.hole.entrance);
            assert(rs.pos.x==g.hole.x && rs.yaw==g.hole.yaw);
            assert(gSaveContext.respawn[0].entrance==g.down.entrance);
            assert(gSaveContext.respawn[2].pos.z==g.down.z);
            assert(ComboMmCanonicalGrottoLoad(physical)==g.Entry());
        }
        play.sceneId=SCENE_KAKUSIANA;play.roomCtx.curRoom.num=g.room;
        assert(LogicalGrottoForRoom(g.room,g.hole.data)==g.Entry());
        if(g.holeReturn) {
            gSaveContext.respawnFlag=4;
            assert(ComboMmCanonicalGrottoExit(g.hole.entrance,SCENE_KAKUSIANA)==g.Exit());
            gSaveContext.respawnFlag=1;
            assert(ComboMmCanonicalGrottoExit(g.hole.entrance,SCENE_KAKUSIANA)==g.hole.entrance);
        } else {
            gSaveContext.respawnFlag=0;
            assert(ComboMmCanonicalGrottoExit(g.nativeReturn,SCENE_KAKUSIANA)==g.Exit());
        }
        assert(ComboMmCanonicalGrottoExit(0x1234,0)==0x1234);
        assert(ComboMmCanonicalGrottoExit(0x1234,SCENE_KAKUSIANA)==0x1234);
        const int exterior=ComboMmMaterializeGrotto(g.Exit());
        if(g.holeReturn) {
            assert(exterior==g.hole.entrance && gSaveContext.respawnFlag==4);
            const auto& returned=gSaveContext.respawn[3];
            assert(returned.roomIndex==g.hole.room && returned.pos.x==g.hole.x);
            assert(returned.pos.y==g.hole.y && returned.pos.z==g.hole.z && returned.yaw==g.hole.yaw);
            assert(returned.data==0 && returned.playerParams==0x04FF);
            assert(gSaveContext.respawn[0].pos.z==returned.pos.z && gSaveContext.respawn[2].yaw==returned.yaw);
        } else {
            assert(exterior==g.nativeReturn && gSaveContext.respawnFlag==0);
        }
    }
    assert(expected.empty());
    assert(CanonicalGrottoLoad(ENTRANCE(GROTTOS,4),0)==ENTRANCE(GROTTOS,4));
    assert(CanonicalGrottoLoad(ENTRANCE(GROTTOS,10),0)==ENTRANCE(GROTTOS,10));
    assert(!FindGrottoByExit(0xF004) && RegionForGrottoReturn(0xF004)==RR_MAX);
    assert(GrottoExitForRegion(RR_MAX)==-1);
    gSaveContext.respawnFlag=-7;
    gSaveContext.respawn[3].data=0xAA;
    assert(ComboMmMaterializeGrotto(1234)==1234);
    assert(gSaveContext.respawnFlag==-7 && gSaveContext.respawn[3].data==0xAA);
    assert(ComboMmMaterializeGrotto(-1)==-1);
    assert(ComboMmMaterializeGrotto(0x10000)==0x10000);
    assert(RegionForGrottoEntry(-1)==RR_MAX);
    auto* activePlay=gPlayState;gPlayState=nullptr;
    assert(ComboMmCanonicalGrottoExit(0xFFFF,SCENE_KAKUSIANA)==0xFFFF);
    gPlayState=activePlay;
    gSaveContext={};snowhead=true;
    const auto* cow=FindGrottoByEntry(ENTRANCE(GROTTOS,27));
    assert(cow && cow->hole.x==2077 && cow->hole.y==333 && cow->hole.z==-215 && cow->hole.yaw==-13654);
    assert(ComboMmMaterializeGrotto(cow->Exit())==ENTRANCE(GREAT_BAY_COAST,0));
    assert(gSaveContext.respawnFlag==4 && gSaveContext.respawn[3].playerParams==0x04FF);
    assert(gSaveContext.respawn[3].data==0 && gSaveContext.respawn[0].pos.x==2077);
    assert(gSaveContext.respawn[3].tempSwitchFlags==0 && gSaveContext.respawn[3].unk_18==0);
    snowhead=false;
    assert(ComboMmMaterializeGrotto(ENTRANCE(GROTTOS,23))==ENTRANCE(GROTTOS,4));
    assert(gSaveContext.respawn[0].entrance==ENTRANCE(MOUNTAIN_VILLAGE_WINTER,8));
    assert(gSaveContext.respawn[0].pos.x==345 && gSaveContext.respawn[0].pos.y==8);
    assert(gSaveContext.respawn[3].data==0x3B);
    assert(ComboMmMaterializeGrotto(0xF000+23)==ENTRANCE(MOUNTAIN_VILLAGE_WINTER,8));
    assert(gSaveContext.respawn[3].pos.x==345 && gSaveContext.respawn[3].pos.y==8);
    snowhead=true;
    assert(ComboMmMaterializeGrotto(0xF000+23)==ENTRANCE(MOUNTAIN_VILLAGE_SPRING,0));
    woodfall=true;
    assert(ComboMmMaterializeGrotto(0xF000+21)==ENTRANCE(SOUTHERN_SWAMP_CLEARED,0));
    assert(ComboMmMaterializeGrotto(0xF000+24)==ENTRANCE(PATH_TO_GORON_VILLAGE_SPRING,0));
    std::cout<<"MM catalogue: 7 expanded, 42 interiors, 26 grottos; materialization and boundary cases passed\n";
}
