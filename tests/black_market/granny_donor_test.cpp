#include "combo/BlackMarketGranny.h"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
using namespace BlackMarketGranny;
static std::ifstream input;
static uint32_t U32() { uint32_t n; input.read(reinterpret_cast<char*>(&n),4); assert(input); return n; }
static uint64_t U64() { uint64_t n; input.read(reinterpret_cast<char*>(&n),8); assert(input); return n; }
static std::string String() { std::string s(U32(),'\0'); input.read(s.data(),s.size()); assert(input); return s; }
int main(int argc,char** argv) {
    assert(argc==3); input.open(argv[1],std::ios::binary); assert(input);
    std::map<std::string,std::shared_ptr<std::vector<Gfx>>> displays;
    std::map<std::string,std::shared_ptr<std::vector<Vtx>>> vertices;
    std::set<std::string> textures; std::deque<std::string> paths;
    const unsigned resources=U32();
    for (unsigned r=0;r<resources;++r) {
        auto path=String(); const unsigned kind=U32(),count=U32();
        if (kind==0) {
            auto list=std::make_shared<std::vector<Gfx>>();
            for (unsigned i=0;i<count;++i) {
                Gfx g{}; g.words.w0=U32();g.words.w1=U64();auto reference=String();
                if (!reference.empty()) { paths.push_back(reference);g.words.w1=(uintptr_t)paths.back().c_str(); }
                list->push_back(g);
            }
            displays[path]=list;
        } else if (kind==1) {
            auto data=std::make_shared<std::vector<Vtx>>(count);
            input.read(reinterpret_cast<char*>(data->data()),count*sizeof(Vtx));assert(input);vertices[path]=data;
        } else { assert(kind==2 && count==0); textures.insert(path); }
    }
    auto originalDisplays=std::map<std::string,std::vector<Gfx>>{};
    auto originalVertices=std::map<std::string,std::vector<Vtx>>{};
    for (auto& [name,list]:displays) originalDisplays[name]=*list;
    for (auto& [name,list]:vertices) originalVertices[name]=*list;
    Provider provider; provider.game="oot";
    provider.display=[&](const std::string& path)->Display {
        auto at=displays.find(path);return at==displays.end()?Display{}:Display{at->second->data(),at->second->size(),at->second};
    };
    provider.vertices=[&](const std::string& path)->Vertices {
        auto at=vertices.find(path);return at==vertices.end()?Vertices{}:Vertices{at->second->data(),at->second->size(),at->second};
    };
    provider.texture=[&](const std::string& path)->std::shared_ptr<void> {
        return textures.contains(path)?std::make_shared<int>(1):nullptr;
    };
    Room room; if (!room.Load(provider)) { std::cerr<<room.Error()<<'\n';return 1; }
    size_t unchanged=0,loads=0,textureReferences=0; std::set<std::string> visited,usedTextures;
    std::function<void(const std::string&,Gfx*)> compare=[&](const std::string& path,Gfx* copy) {
        if (!visited.insert(path).second) return;
        const auto& source=*displays.at(path);
        for (size_t i=0;i<source.size();++i) {
            const auto& g=source[i];const auto op=g.words.w0>>24;
            if (op==0x27) {
                assert(copy[i].words.w0==((g.words.w0&0xffffff)|(G_DL<<24)));
                compare(Exact(reinterpret_cast<const char*>(g.words.w1)),reinterpret_cast<Gfx*>(copy[i].words.w1));
            } else if (op==0x24) {
                const auto& args=source[i+1].words;
                const auto name=Exact(reinterpret_cast<const char*>(g.words.w1));
                const Vtx* data=name==Exact(kCounterVertices)?room.Counter().data():vertices.at(name)->data();
                Gfx expected={gsSPVertex(data+(args.w1&0xffff),args.w0,args.w1>>16)};
                assert(copy[i].words.w0==expected.words.w0 && copy[i].words.w1==expected.words.w1);
                ++i;assert((copy[i].words.w0>>24)==G_SPNOOP);++loads;
            } else if (op==0x25) {
                const auto name=Exact(reinterpret_cast<const char*>(g.words.w1));usedTextures.insert(name);
                assert(copy[i].words.w0==g.words.w0);
                assert(std::string(reinterpret_cast<const char*>(copy[i].words.w1))=="__OTR__@oot:"+name);
                ++textureReferences;
            } else if (op==G_ENDDL && path==Exact(kCounterDraw)) {
                assert((copy[i].words.w0>>24)==G_VTX && (copy[i+6].words.w0>>24)==G_ENDDL);
            } else { assert(std::memcmp(&copy[i],&g,sizeof(Gfx))==0);++unchanged; }
        }
    };
    compare(Exact(kOpaque),room.Opaque());
    for (auto& [name,list]:displays) assert(std::memcmp(list->data(),originalDisplays.at(name).data(),list->size()*sizeof(Gfx))==0);
    for (auto& [name,list]:vertices) assert(std::memcmp(list->data(),originalVertices.at(name).data(),list->size()*sizeof(Vtx))==0);
    std::ofstream output(argv[2],std::ios::binary);output.write(reinterpret_cast<const char*>(room.Counter().data()),room.Counter().size()*sizeof(Vtx));output.close();
    auto missing=*usedTextures.begin();textures.erase(missing);Room partial;assert(!partial.Load(provider) && !partial.Opaque());
    std::cout<<"{\"lists\":"<<visited.size()<<",\"unchanged_commands\":"<<unchanged
        <<",\"vertex_loads\":"<<loads<<",\"texture_references\":"<<textureReferences
        <<",\"textures\":"<<usedTextures.size()<<",\"missing_texture_rejected\":true,\"original_buffers_unchanged\":true}\n";
}
