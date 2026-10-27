// Run the unchanged production raw importer against an actual OTR texture.
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <string>
#include <vector>
#define SPDLOG_ERROR(...) ((void)0)
#define G_IM_SIZ_32b 3
namespace Fast {
enum class TextureType { RGBA32bpp=1, RGBA16bpp=2, Palette4bpp=3, Palette8bpp=4 };
struct Init { std::string Path; };
struct Texture {
    uint32_t ImageDataSize;
    std::shared_ptr<Init> GetInitData() { return std::make_shared<Init>(); }
};
struct RawTexMetadata {
    uint16_t width, height;
    float h_byte_scale, v_pixel_scale;
    TextureType type;
    std::shared_ptr<Texture> resource;
};
struct LoadedTexture {
    RawTexMetadata raw_tex_metadata;
    const uint8_t* addr;
    uint32_t size_bytes,orig_size_bytes,full_image_line_size_bytes,line_size_bytes;
};
struct Tile { uint32_t tmem_index,siz,line_size_bytes; };
struct Rdp { LoadedTexture loaded_texture[1]; Tile texture_tile[1]; };
struct Api { int GetMaxTextureSize() { return 4096; } };
struct Masked { uint8_t* replacementData; };
class Interpreter {
  public:
    Rdp state{};
    Rdp* mRdp=&state;
    Api api;
    Api* mRapi=&api;
    std::vector<uint8_t> scratch=std::vector<uint8_t>(2*1024*1024);
    uint8_t* mTexUploadBuffer=scratch.data();
    std::map<std::string,Masked> mMaskedTextures;
    uint32_t uploadedWidth=0,uploadedHeight=0;
    std::vector<uint8_t> uploaded;
    std::string GetBaseTexturePath(const std::string& path) { return path; }
    void ImportTextureCi4(int,bool) { assert(false); }
    void ImportTextureCi8(int,bool) { assert(false); }
    void UploadTextureMeasured(const uint8_t* pixels,uint32_t w,uint32_t h) {
        uploadedWidth=w; uploadedHeight=h;
        uploaded.assign(pixels,pixels+uint64_t(w)*h*4);
    }
    void ImportTextureRaw(int,bool);
};
#include "npc_raw_import_production.inc"
}
static std::vector<uint8_t> read(const char* path) {
    std::ifstream f(path,std::ios::binary);
    assert(f.good());
    return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
}
template<class T> static T scalar(const std::vector<uint8_t>& raw,size_t at) {
    assert(at+sizeof(T)<=raw.size()); T v; std::memcpy(&v,raw.data()+at,sizeof(v)); return v;
}
int main(int argc,char** argv) {
    assert(argc==3);
    auto raw=read(argv[1]),expected=read(argv[2]);
    assert(raw.size()>=92&&raw[0]==0&&scalar<uint32_t>(raw,8)==1);
    assert(scalar<uint32_t>(raw,76)==1); // TEX_FLAG_LOAD_AS_RAW path.
    Fast::Interpreter gfx;
    auto& loaded=gfx.state.loaded_texture[0]; auto& meta=loaded.raw_tex_metadata;
    meta.type=Fast::TextureType(scalar<uint32_t>(raw,64));
    meta.width=scalar<uint32_t>(raw,68);meta.height=scalar<uint32_t>(raw,72);
    meta.h_byte_scale=scalar<float>(raw,80);meta.v_pixel_scale=scalar<float>(raw,84);
    meta.resource=std::make_shared<Fast::Texture>();
    meta.resource->ImageDataSize=scalar<uint32_t>(raw,88);
    assert(raw.size()==92+meta.resource->ImageDataSize);
    loaded.addr=raw.data()+92;
    loaded.orig_size_bytes=32*16*2;
    loaded.size_bytes=loaded.orig_size_bytes*meta.h_byte_scale*meta.v_pixel_scale;
    loaded.line_size_bytes=loaded.full_image_line_size_bytes=64*meta.h_byte_scale;
    gfx.state.texture_tile[0]={0,2,64}; // Actual native Saria RGBA16 tile.
    gfx.ImportTextureRaw(0,false);
    bool exact=gfx.uploadedWidth==256&&gfx.uploadedHeight==128&&gfx.uploaded==expected;
    std::printf("{\"width\":%u,\"height\":%u,\"bytes\":%zu,\"decoded_colors_exact\":%s}\n",
                gfx.uploadedWidth,gfx.uploadedHeight,gfx.uploaded.size(),exact?"true":"false");
    return exact?0:2;
}
