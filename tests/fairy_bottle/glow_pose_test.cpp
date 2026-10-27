// Execute each host's real matrix replacement and the shared glow callback.
// Both native fairy-core DLs multiply segment 01 (the camera billboard), so
// the CPU limb pose must not supply a second copy of that rotation.
#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdio>
using f32 = float;
using s32 = int32_t;
struct MtxF {
    float xx, yx, zx, wx, xy, yy, zy, wy, xz, yz, zz, wz, xw, yw, zw, ww;
};
struct PlayState { uint32_t gameplayFrames; MtxF billboardMtxF; };
struct Gfx {};
struct Actor {};
struct Vec3f {};
struct Vec3s {};
constexpr int MTXMODE_APPLY = 1;
MtxF current;
MtxF* sCurrentMatrix = &current;
#define SQ(x) ((x)*(x))
void FrameInterpolation_RecordMatrixReplaceRotation(MtxF*) {}
void Matrix_Scale(float x, float y, float z, int mode) {
    assert(mode == MTXMODE_APPLY);
    current.xx *= x; current.yx *= x; current.zx *= x;
    current.xy *= y; current.yy *= y; current.zy *= y;
    current.xz *= z; current.yz *= z; current.zz *= z;
}
#include "glow_production.inc"
std::array<float,3> Transform(const MtxF& m, std::array<float,3> p) {
    return {m.xx*p[0]+m.xy*p[1]+m.xz*p[2],
            m.yx*p[0]+m.yy*p[1]+m.yz*p[2],
            m.zx*p[0]+m.zy*p[1]+m.zz*p[2]};
}
float Norm(std::array<float,3> p) { return std::sqrt(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]); }
int main() {
    for (float yaw : {0.f,.4f,1.5707963f,2.3f}) for (float pitch : {0.f,.25f,-.6f})
    for (uint32_t frame : {0u,4u,12u,31u}) {
        const float c=std::cos(yaw),s=std::sin(yaw),cp=std::cos(pitch),sp=std::sin(pitch);
        PlayState play{frame,{c,0,-s,0,s*sp,cp,c*sp,0,s*cp,-sp,c*cp,0,0,0,0,1}};
        current = {0,0,-.012f,0,0,.012f,0,0,.012f,0,0,0,10,20,30,1};
#ifdef COMBO_FAIRY_HOST_MM
        const int limb=6;
#else
        const int limb=8;
#endif
        Gfx* dl=nullptr;
        ComboFairyBottle_GlowLimb(&play,limb,&dl,nullptr,nullptr,nullptr,&dl);
        assert(current.xw==10 && current.yw==20 && current.zw==30);
        const float expectedScale=.012f*(1.5f+.15f*std::sin((frame%16u)*.3926990817f));
        assert(std::abs(Norm({current.xx,current.yx,current.zx})-expectedScale)<.000001f);
        // Native G_MTX segment-01 multiplication: CPU pose * billboard.
        const auto cameraNormal=Transform(play.billboardMtxF,{0,0,1});
        const auto renderedNormal=Transform(current,cameraNormal);
        const float alignment=(renderedNormal[0]*cameraNormal[0]+renderedNormal[1]*cameraNormal[1]+
                               renderedNormal[2]*cameraNormal[2])/Norm(renderedNormal);
        assert(alignment>.99999f && "fairy core receives the camera rotation twice and turns edge-on");
    }
    puts("PASS fairy core faces camera once; preserved position and GI scale across yaw/pitch/pulse");
}
