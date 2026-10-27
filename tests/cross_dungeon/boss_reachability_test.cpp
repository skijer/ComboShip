#include "combo/rando/CrossDungeonReachability.h"
#include "combo/rando/CrossDungeonState.h"
#include <cassert>
#include <functional>
#include <iostream>
using namespace ComboRando::CrossDungeon;
struct Oracle {
    std::function<void(const char*)> SetDungeonAccess;
    std::function<const char*()> GetDungeonExits;
    std::function<uint8_t()> GetPortalOpen;
    std::function<uint16_t()> GetBossDefeated;
    std::function<int(int32_t)> CanEnterArrival;
    BossSourceMap CrossBossSources = EmptyBossSources();
};
int main() {
    HostState hosts[] = {HostState(0),HostState(1)};
    Oracle oracles[2];
    uint16_t defeated[2] = {};
    bool mmHasTime = false;
    for(int game=0;game<2;++game) {
        oracles[game].SetDungeonAccess = [&,game](const char* raw){hosts[game].SetAccess(nlohmann::json::parse(raw));};
        oracles[game].GetDungeonExits = [game](){return game==0 ? "[{\"game\":1,\"index\":240}]" : "[]";};
        oracles[game].GetBossDefeated = [&,game](){return defeated[game];};
        oracles[game].CrossBossSources[0]=8;
        oracles[game].CrossBossSources[8]=0;
    }
    oracles[0].GetPortalOpen=[](){return uint8_t(0);};
    oracles[1].CanEnterArrival=[&](int32_t){return mmHasTime;};
    auto query = [&](const Oracle& oracle,const std::vector<std::string>& owned) {
        int game=&oracle==&oracles[0]?0:1;
        std::unordered_set<std::string> checks;
        bool fight=std::find(owned.begin(),owned.end(),"weapon")!=owned.end();
        defeated[game]=0;
        if(game==0) {
            if(fight) defeated[0]=1u<<0; // Physical Gohma clears logical Odolwa.
            if(hosts[0].logicalClears&1u)checks.insert("logical Deku world");
        } else {
            mmHasTime=std::find(owned.begin(),owned.end(),"Song of Time")!=owned.end();
            if(hosts[1].arrivals.contains(240)) {
                if(fight)defeated[1]=1u<<8; // Physical Odolwa clears logical Gohma.
                if(hosts[1].logicalClears&(1u<<8))checks.insert("logical Woodfall world");
            }
        }
        return checks;
    };
    auto result=QueryJoint(oracles[0],oracles[1],{"weapon"},{"weapon","Song of Time"},false,query);
    assert(result.oot.contains("logical Deku world"));
    assert(result.mm.contains("logical Woodfall world"));
    // Direct native queries outside the joint closure must start with native roots and no clears.
    assert(hosts[0].logicalClears==0&&hosts[1].logicalClears==0);
    assert(hosts[0].arrivals.empty()&&hosts[1].arrivals.empty());
    // A fresh inventory query must not inherit roots, defeat results or a previous time-song gate.
    result=QueryJoint(oracles[0],oracles[1],{}, {},false,query);
    assert(result.oot.empty()&&result.mm.empty());
    assert(hosts[0].logicalClears==0&&hosts[1].arrivals.empty());
    std::cout<<"joint bosses: physical-to-source closure, current-inventory time gate and fresh-query isolation PASS\n";
}
