#include <cassert>
#include <iostream>
#if __has_include("combo/NeiRewardGiSurface.h")
#include "combo/NeiRewardGiSurface.h"
#include <map>
#include <string>
namespace Ship {
IResource::IResource(std::shared_ptr<ResourceInitData> init) : mInitData(init) {}
IResource::~IResource() = default;
std::shared_ptr<ResourceInitData> IResource::GetInitData() { return mInitData; }
}
namespace Fast {
DisplayList::DisplayList() : Resource(nullptr) {}
DisplayList::~DisplayList() = default;
Gfx* DisplayList::GetPointer() { return Instructions.data(); }
size_t DisplayList::GetPointerSize() { return Instructions.size() * sizeof(Gfx); }
Vertex::Vertex() : Resource(nullptr) {}
Vtx* Vertex::GetPointer() { return VertexList.data(); }
size_t Vertex::GetPointerSize() { return VertexList.size() * sizeof(Vtx); }
Matrix::Matrix() : Resource(nullptr) {}
Mtx* Matrix::GetPointer() { return &Matrx; }
size_t Matrix::GetPointerSize() { return sizeof(Mtx); }
}
namespace SOH {
Array::Array() : Resource(nullptr) {}
void* Array::GetPointer() { return Vertices.data(); }
size_t Array::GetPointerSize() { return Vertices.size() * sizeof(Vtx); }
}
struct Loader {
    std::map<std::string, std::shared_ptr<Ship::IResource>> files;
    auto operator()(const char* name) const {
        const auto it = files.find(name ? name : "");
        return it == files.end() ? std::shared_ptr<Ship::IResource>{} : it->second;
    }
    auto operator()(uint64_t) const { return (*this)("vertices"); }
};
int main() {
    Loader load;
    auto vertices = std::make_shared<Fast::Vertex>();
    for (const auto p : {std::array<int16_t, 3>{-20,-10,0}, {20,-10,0}, {0,20,0}}) {
        Vtx v{}; std::copy(p.begin(), p.end(), v.v.ob); vertices->VertexList.push_back(v);
    }
    load.files["vertices"] = vertices;
    auto list = std::make_shared<Fast::DisplayList>(); list->UCode = ucode_f3dex2;
    const auto command = [](uintptr_t a, uintptr_t b) { Gfx g{}; g.words.w0=a; g.words.w1=b; return g; };
    list->Instructions = {
        command(uintptr_t(G_VTX_OTR_FILEPATH)<<24, uintptr_t("vertices")), command(3,0),
        command(uintptr_t(G_TRI1)<<24 | 2u<<8 | 4u,0), command(uintptr_t(G_ENDDL)<<24,0)
    };
    load.files["face"] = list;
    NeiGi::Mesh surface{};
    assert(NeiGi::ReadRewardSurface(load,"face",surface) && surface.count == 3);
    for (size_t i=0;i<surface.count;++i) {
        assert(surface.vertices[i].p.z > 0 && surface.vertices[i].p.z < .5f);
        assert(surface.vertices[i].u >= 0 && surface.vertices[i].u <= 1);
        assert(surface.vertices[i].v >= 0 && surface.vertices[i].v <= 1);
    }
    // Fast's XML Triangle1 importer uses direct indices split between words;
    // unlike native TRI1, these indices are not packed at twice their value.
    list->Instructions[2] = command(uintptr_t(G_TRI1_OTR)<<24, 1u<<16 | 2u);
    assert(NeiGi::ReadRewardSurface(load,"face",surface) && surface.count == 3);
    list->Instructions[2] = command(uintptr_t(G_TRI1)<<24 | 2u<<8 | 4u,0);
    // Quadrangle and TRI2 carry two native packed triangles. A fourth corner
    // here also proves nonzero destination-cache slots are respected.
    Vtx corner{}; corner.v.ob[0]=-20; corner.v.ob[1]=20; vertices->VertexList.push_back(corner);
    list->Instructions[1] = command(4,7u<<16);
    Gfx quad{}; gSP1Quadrangle(&quad,7,8,9,10,0); list->Instructions[2]=quad;
    assert(NeiGi::ReadRewardSurface(load,"face",surface) && surface.count == 6);
    vertices->VertexList.pop_back(); list->Instructions[1]=command(3,0);
    list->Instructions[2] = command(uintptr_t(G_TRI1)<<24 | 2u<<8 | 4u,0);
    assert(NeiGi::ReadRewardSurface(load,"face",surface) && surface.count == 3);
    // The production traversal must honor destination cache slots and cached
    // binary vertex pointers, not reinterpret resource packets as triangles.
    list->Instructions[0] = command(uintptr_t(G_VTX_OTR_HASH)<<24 | 3u<<12 | 6u,
                                    reinterpret_cast<uintptr_t>(vertices->GetPointer()));
    list->Instructions[1] = command(0,1);
    NeiGi::Mesh hashed{};
    assert(NeiGi::ReadRewardSurface(load,"face",hashed) && hashed.count == surface.count);
    for (size_t i=0;i<surface.count;++i)
        assert(hashed.vertices[i].p.x == surface.vertices[i].p.x && hashed.vertices[i].p.y == surface.vertices[i].p.y);
    list->Instructions[2].words.w0 = uintptr_t(G_TRI1)<<24 | 2u<<8 | 62u;
    assert(!NeiGi::ReadRewardSurface(load,"face",surface) && surface.count == 0);
    list->Instructions[2].words.w0 = uintptr_t(G_TRI1)<<24 | 2u<<8 | 4u;
    list->Instructions.insert(list->Instructions.begin()+2, command(uintptr_t(G_DL)<<24,0x09000000));
    assert(NeiGi::ReadRewardSurface(load,"face",surface));
    list->Instructions[2].words.w1 = 0x09000001; // Native exporter segment tag.
    assert(NeiGi::ReadRewardSurface(load,"face",surface));
    list->Instructions[2].words.w1 = 0x06000000;
    assert(!NeiGi::ReadRewardSurface(load,"face",surface) && surface.count == 0);
    assert(!NeiGi::ReadRewardSurface(load,"missing",surface));
    std::cout << "PASS actual XML/hash geometry, native topology, projected UVs, material segments, malformed-graph rejection\n";
}
#else
int main() { std::cerr << "FAIL reward surface extraction is not implemented\n"; return 1; }
#endif
