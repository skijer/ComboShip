#include "combo/rando/CrossDungeonGeneration.h"
#include <cassert>
#include <iostream>
using namespace ComboRando::CrossDungeon;
using Json=nlohmann::json;
int main() {
    std::vector<Dungeon> entries{
        {"d0",{0,10},{0,11},{0,11}}, {"d1",{1,20},{1,21},{1,21}},
        {"i0",{0,30},{0,31},{0,31}}, {"i1",{1,40},{1,41},{1,41}},
        {"i2",{0,50},{0,51},{0,51}}, {"i3",{1,60},{1,61},{1,61}}};
    entries[1].expanded=true;
    for(size_t i=2;i<entries.size();++i)entries[i].pool=Pool::Interior;
    // A confined native starting key requires the first dungeon and first interior
    // to stay reachable. Other members can still form a useful shuffled cycle.
    const auto valid=[](const Json& seed){
        const auto p=seed.at("permutation").get<std::vector<size_t>>();
        return p[0]==0 && p[2]==2;
    };
    bool exercisedRepair=false;
    for(uint64_t master=0;master<128;++master) {
        if(valid(GenerateSeed(entries,master)))continue;
        size_t probes=0;
        auto repaired=GenerateConstrainedSeed(entries,master,[&](const Json& candidate){++probes;return valid(candidate);});
        assert(probes<=2*entries.size()+2);
        if(!repaired)continue; // An identity-only target need not produce a nontrivial repair.
        exercisedRepair=true;
        assert(valid(*repaired));
        const auto p=repaired->at("permutation").get<std::vector<size_t>>();
        bool changed=false;
        for(size_t i=0;i<p.size();++i) {
            assert(entries[i].pool==entries[p[i]].pool);
            changed|=p[i]!=i;
        }
        assert(changed && p[0]==0 && p[1]==1 && p[2]==2);
        auto replay=GenerateConstrainedSeed(entries,master,valid);
        assert(replay && *replay==*repaired);
        assert(Decode(*repaired)==FromPermutation(entries,p));
    }
    assert(exercisedRepair);
    size_t impossible=0;
    assert(!GenerateConstrainedSeed(entries,123,[&](const Json&){++impossible;return false;}));
    assert(impossible==1); // An invalid identity is rejected, never certified by fallback.
    assert(!GenerateConstrainedSeed(entries,123,[](const Json& seed){
        const auto p=seed.at("permutation").get<std::vector<size_t>>();
        for(size_t i=0;i<p.size();++i)if(p[i]!=i)return false;
        return true;
    })); // Do not report an unshuffled identity as repaired.
    std::cout<<"constrained generation: sparse valid layouts, bounded probes, pool isolation and exact replay PASS\n";
}
