// Offline adapter uses exactly the native quantized geometry and texture.
#include "combo/BlackMarketLock.h"
extern "C" int BlackMarketLock_Preview(const float minimum[3], const float maximum[3], uint32_t tick, int selected,
                                       float paddingScale, float* output) {
    using namespace BlackMarketVisual;
    const auto mesh =
        BuildLock({ { minimum[0], minimum[1], minimum[2] }, { maximum[0], maximum[1], maximum[2] }, true }, tick,
                  selected != 0, paddingScale);
    if(!mesh.valid) return 0;
    constexpr int indices[]={0,1,2,0,2,3};
    int count=0;
    for(int quad=0;quad<kLockQuads;++quad) for(int index:indices) {
        const auto& v=mesh.vertices[quad*4+index];
        float* row=output+count*9;
        row[0]=mesh.center.x+v.x/kLockVertexScale;
        row[1]=mesh.center.y+v.y/kLockVertexScale;
        row[2]=mesh.center.z+v.z/kLockVertexScale;
        row[3]=v.s/(32.f*kLockTextureWidth);
        row[4]=v.t/(32.f*kLockTextureHeight);
        for(int c=0;c<4;++c) row[5+c]=mesh.color[c]/255.f;
        ++count;
    }
    return count;
}
extern "C" const uint8_t* BlackMarketLock_Texture() { return BlackMarketVisual::kLockTexture.data(); }
