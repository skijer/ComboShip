// Export actual source-authored shelf vertices and resource bindings.
#include "combo/BlackMarketShelves.h"
#include <cstdio>
#include <string_view>
int main(int argc,char** argv) {
    using namespace BlackMarketShelves;
    const bool granny=argc>1 && std::string_view(argv[1])=="granny";
    std::printf("{\"boxes\":["); bool comma=false;
    for (const auto& b:kBoxes) {
        if (comma)std::printf(",");comma=true;
        std::printf("[[%d,%d,%d],[%d,%d,%d]]",b.low.x,b.low.y,b.low.z,b.high.x,b.high.y,b.high.z);
    }
    std::printf("],\"corners\":[");comma=false;
    for (const auto& b:kBoxes) {
        if(comma)std::printf(",");comma=true;
        std::printf("[");bool pointComma=false;
        for(const auto& p:Corners(b)) {
            if(pointComma)std::printf(",");pointComma=true;
            std::printf("[%d,%d,%d]",p.x,p.y,p.z);
        }
        std::printf("]");
    }
    std::printf("],\"vertices\":[");comma=false;
    for (const auto& v:Vertices(granny)) {
        if (comma)std::printf(",");comma=true;
        std::printf("[%d,%d,%d,%d,%d,%u,%u,%u,%u]",v.position.x,v.position.y,v.position.z,v.s,v.t,v.color[0],v.color[1],v.color[2],v.color[3]);
    }
    std::printf("],\"materials\":[");comma=false;
    for(size_t box=0;box<kBoxes.size();++box)for(size_t face=0;face<6;++face) {
        if(comma)std::printf(",");comma=true;std::printf("%d",int(FaceMaterial(box,face,granny)));
    }
    std::printf("],\"flow_texture\":\"%s\",\"tint\":[%u,%u,%u,255],\"wood_texture\":\"%s\",\"atlas_texture\":\"%s\"}\n",
        kFlowTexture+7,kDefaultTint[0],kDefaultTint[1],kDefaultTint[2],kWoodTexture,kAtlasTexture);
}
