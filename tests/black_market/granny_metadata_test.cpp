#include "combo/BlackMarketGrannyScene.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
static auto Read(const char* path) {
    std::ifstream stream(path,std::ios::binary);assert(stream);
    return std::vector<char>(std::istreambuf_iterator<char>(stream),{});
}
int main(int argc,char** argv) {
    assert(argc==3);const auto scene=Read(argv[1]),room=Read(argv[2]);
    BlackMarketGranny::NativeMetadata metadata;
    assert(metadata.ReadScene(scene) && metadata.ReadRoom(room));
    assert(metadata.lights.size()==4 && metadata.decorations.size()==2);
    assert(metadata.decorations[0].id==8 && metadata.decorations[1].id==8);
    assert(metadata.decorations[0].x==130 && metadata.decorations[1].x==-139);
    assert(metadata.decorations[0].params==0x7f5);
    for (size_t size=0;size<scene.size();++size) {
        BlackMarketGranny::NativeMetadata truncated;
        assert(!truncated.ReadScene(std::vector<char>(scene.begin(),scene.begin()+size)));
    }
    for (size_t size=0;size<room.size();++size) {
        BlackMarketGranny::NativeMetadata truncated;
        assert(!truncated.ReadRoom(std::vector<char>(room.begin(),room.begin()+size)));
    }
    auto corrupt=scene;corrupt[68]=char(0xff);assert(!metadata.ReadScene(corrupt));
    std::cout<<"PASS: raw OoT metadata without Scene factory/cache; four light settings and two original flames; every truncated prefix rejected\n";
}
