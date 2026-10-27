#include <libultraship/libultra/gbi.h>
#include "combo/BlackMarketGranny.h"
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace BlackMarketGranny;

int main(int argc, char** argv) {
    assert(argc == 2);
    // Real donor vertices are supplied locally, never embedded in this test.
    auto vertices = std::make_shared<std::vector<Vtx>>(1189);
    FILE* file = std::fopen(argv[1], "rb"); assert(file);
    assert(std::fread(vertices->data(), sizeof(Vtx), vertices->size(), file) == vertices->size());
    std::fclose(file);
    const auto originalVertices = *vertices;
    const char* counterPath = kCounterVertices;
    const char* trianglePath = kCounterDraw;
    const char* texturePath = "scenes/shared/mahouya_scene/test_texture";
    auto root = std::make_shared<std::vector<Gfx>>(std::initializer_list<Gfx>{
        gsSPDisplayListOTRFilePath(trianglePath), gsSPEndDisplayList() });
    auto draw = std::make_shared<std::vector<Gfx>>(std::initializer_list<Gfx>{
        { 0x24000000, (uintptr_t)counterPath }, { 4, (0 << 16) | 603 },
        { 0x25000000, (uintptr_t)texturePath }, gsSP2Triangles(0,1,2,0,0,2,3,0), gsSPEndDisplayList() });
    const auto originalRoot = *root, originalDraw = *draw;
    bool haveTexture = true, haveVertices = true, haveRoot = true;
    Provider provider;
    provider.game = "mm";
    provider.display = [&](const std::string& path) -> Display {
        if (path == Exact(kOpaque) && haveRoot) return {root->data(),root->size(),root};
        if (path == Exact(kCounterDraw)) return {draw->data(),draw->size(),draw};
        return {};
    };
    provider.vertices = [&](const std::string& path) -> Vertices {
        if (path == Exact(kCounterVertices) && haveVertices)
            return {vertices->data(),vertices->size(),vertices};
        return {};
    };
    auto textureOwner = std::make_shared<int>(1);
    provider.texture = [&](const std::string& path) -> std::shared_ptr<void> {
        return haveTexture && path == Exact(texturePath) ? textureOwner : nullptr;
    };
    Room missing;
    haveRoot = false; assert(!missing.Load(provider) && !missing.Opaque()); haveRoot = true;
    haveTexture = false; assert(!missing.Load(provider) && !missing.Opaque()); haveTexture = true;
    haveVertices = false; assert(!missing.Load(provider) && !missing.Opaque()); haveVertices = true;
    Room room; assert(room.Load(provider) && room.Opaque());
    assert(std::memcmp(root->data(),originalRoot.data(),root->size()*sizeof(Gfx)) == 0);
    assert(std::memcmp(draw->data(),originalDraw.data(),draw->size()*sizeof(Gfx)) == 0);
    assert(std::memcmp(vertices->data(),originalVertices.data(),vertices->size()*sizeof(Vtx)) == 0);
    const auto& raised = room.Counter(); assert(raised.size() == 1197);
    int changed = 0;
    for (size_t i=0; i<originalVertices.size(); ++i) {
        Vtx expected = originalVertices[i];
        if (std::memcmp(&raised[i], &expected, sizeof(Vtx)) != 0) {
            ++changed; expected.v.ob[1] += 11;
            assert(std::memcmp(&raised[i], &expected, sizeof(Vtx)) == 0);
        }
    }
    assert(changed == 46);
    auto* child = reinterpret_cast<Gfx*>(room.Opaque()[0].words.w1);
    assert((child[0].words.w0 >> 24) == G_VTX);
    assert(child[0].words.w1 == (uintptr_t)(raised.data() + 603));
    assert(std::strcmp(reinterpret_cast<const char*>(child[2].words.w1),
                       "__OTR__@mm:alt/scenes/shared/mahouya_scene/test_texture") == 0);
    std::weak_ptr<void> retained = textureOwner;
    textureOwner.reset(); assert(!retained.expired());
    // A different donor layout must never get the audited index patch.
    vertices->resize(1188); Room incompatible; assert(!incompatible.Load(provider));
    assert(std::memcmp(room.Counter().data(),raised.data(),raised.size()*sizeof(Vtx)) == 0);
    std::puts("PASS: external dependency closure; exact routing; immutable source; 46 raised vertices; resource ownership");
}
