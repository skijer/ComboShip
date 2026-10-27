#include "combo/rando/CrossDungeonSeed.h"
#include <cassert>
#include <iostream>
using namespace ComboRando::CrossDungeon;
using nlohmann::json;
static json D(const char* id, int game, int entry, int exit, int pool) {
    return {{"id", id}, {"entrance", EndpointJson({game,entry})},
            {"exit", EndpointJson({game,exit})}, {"returnTo", EndpointJson({game,exit})},
            {"escapeExits", json::array()}, {"pool", pool}};
}
int main() {
    auto entries = json::array({D("oot:d",0,100,101,0),D("mm:d",1,200,201,0),
                                D("oot:i",0,300,301,2),D("mm:i",1,400,401,2)});
    json seed={{"version",2},{"enabled",true},{"dungeons",entries},{"permutation",{1,0,3,2}},
               {"options",{{"dungeons",true},{"bosses",false},{"interiors",true},{"grottos",false},{"expanded",false}}}};
    std::vector<Route> routes;
    try { routes=Decode(seed); }
    catch(const std::exception& e) {std::cerr<<"FAIL typed pool seed rejected: "<<e.what()<<'\n';return 1;}
    assert(Resolve(routes,Phase::AfterNative,{0,100})==(Endpoint{1,200}));
    assert(Resolve(routes,Phase::AfterNative,{0,300})==(Endpoint{1,400}));
    auto bad=seed;bad["permutation"]={2,0,3,1};
    bool rejected=false;try{Decode(bad);}catch(const std::exception&){rejected=true;}assert(rejected);
    // Every record is visited; the old 64-record limit cannot silently truncate interiors/grottos.
    auto large=seed;large["dungeons"]=json::array();large["permutation"]=json::array();
    for(int i=0;i<140;++i){large["dungeons"].push_back(D(("i:"+std::to_string(i)).c_str(),i%2,1000+i*2,1001+i*2,2));large["permutation"].push_back(i);}
    assert(Decode(large).size()==280);
    // Boss warps compose through the owning dungeon; reverse-door and completion returns differ.
    seed["options"]["bosses"]=true;seed["options"]["interiors"]=false;
    entries=json::array({D("oot:d",0,100,101,0),D("mm:d",1,200,201,0),D("oot:b",0,110,111,1),D("mm:b",1,210,0xF108,1)});
    entries[0]["escapeExits"]={EndpointJson({0,120})};entries[1]["escapeExits"]={EndpointJson({1,220})};
    for(int i=2;i<4;++i){int game=i-2;entries[i]["boss"]=game?8:0;entries[i]["owner"]=game?"mm:d":"oot:d";
        entries[i]["escapeExits"]={EndpointJson({game,game?220:120})};
        entries[i]["warpReturn"]=EndpointJson({game,game?201:101});entries[i]["reverseCheckpoint"]=EndpointJson({game,game?200:100});}
    entries[3]["hasReverse"]=false;entries[3]["returnTo"]=EndpointJson({1,200});
    seed["dungeons"]=entries;routes=Decode(seed);
    assert(Resolve(routes,Phase::BeforeNative,{1,220})==(Endpoint{1,201}));
    assert(Resolve(routes,Phase::BeforeNative,{0,120})==(Endpoint{0,101}));
    assert(Resolve(routes,Phase::BeforeNative,{0,111})==(Endpoint{1,200}));
    assert(Resolve(routes,Phase::BeforeNative,{1,0xF108})==(Endpoint{1,0xF108}));
    std::set<std::tuple<Phase,int,int>> unique;
    for(const auto& route:routes)assert(unique.insert({route.phase,route.from.game,route.from.index}).second);
    std::vector<Dungeon> catalogue;
    for(const auto& entry:entries)catalogue.push_back(ParseDungeon(entry));
    for(uint64_t seedValue=0;seedValue<30;++seedValue)
        assert(Decode(GenerateSeed(catalogue,seedValue))==Generate(catalogue,seedValue,true));
    // Decoupled native blue warps can already resolve to a different pool's forward entrance.
    // Its enabled overlay must compose once, even when no parent escape alias identifies it.
    auto nativeForward=catalogue;
    nativeForward[0].escapeExits.clear();nativeForward[1].escapeExits.clear();
    nativeForward[2].warpReturn=Endpoint{0,100};
    nativeForward[2].escapeExits={{0,120}};
    const auto forwardRoutes=FromPermutation(nativeForward,{1,0,3,2});
    assert(Resolve(forwardRoutes,Phase::BeforeNative,{1,220})==(Endpoint{1,200}));
    auto anotherBoss=nativeForward[2];anotherBoss.id="oot:another";anotherBoss.boss=1;
    anotherBoss.entrance={0,130};anotherBoss.exit={0,131};anotherBoss.returnTo={0,132};
    anotherBoss.escapeExits={{0,140}};anotherBoss.warpReturn=Endpoint{0,101};
    nativeForward.push_back(anotherBoss);
    nativeForward[2].warpReturn=Endpoint{0,130};
    const auto bossForwardRoutes=FromPermutation(nativeForward,{1,0,4,2,3});
    assert(Resolve(bossForwardRoutes,Phase::BeforeNative,{0,140})==(Endpoint{1,210}));
    for(const auto& invalid:nlohmann::json::array({true,0.5,4294967296ULL,8})) {
        auto malformed=seed;malformed["dungeons"][2]["boss"]=invalid;
        bool failed=false;try{Decode(malformed);}catch(const std::exception&){failed=true;}
        assert(failed);
    }
    std::cout<<"typed pools: v2 replay, isolated categories, large catalogue and boss/dungeon composition PASS\n";
}
