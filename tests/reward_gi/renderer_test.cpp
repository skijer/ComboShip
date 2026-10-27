#define NEI_GI_FIXTURE_BOUNDARY_ONLY
#define NEI_GI_REWARD_FIXTURE
#include "../nei_gi/presentation_test.cpp"
#include "ComboRewardGi.h"
extern "C" void Randomizer_DrawTrueMasterSwordFlame(PlayState*) { assert(false); }
extern "C" void Randomizer_DrawCaneSomariaUpgradeFlame(PlayState*) { assert(false); }
extern "C" int ResourceMgr_GetRewardSurfaceForGame(const char* owner, const char* path, NeiGi::Mesh* out) {
    assert(!strcmp(owner,"oot"));
    *out = {};
    if (!strcmp(path,"missing")) return 0;
    out->Tri({{-10,-10,0},0xffffff,255,0,0},{{10,-10,0},0xffffff,255,1,0},{{0,10,0},0xffffff,255,.5,1});
    return 1;
}
int main() {
    using namespace Fixture;
    for (int profile=1; profile<=9; ++profile) for (bool textures : {false,true})
        for (int assets : {0,1}) for (int toggle : {0,1}) {
            Reset(); alt=assets; enabled=toggle;
            if (textures) { files.insert(RewardGi_Texture(profile)); files.insert("__OTR__objects/nei_reward_gi/metal"); }
            for (unsigned frame : {0u,1u,127u,128u,UINT32_MAX}) {
                arena.clear(); play.gameplayFrames=frame;
                // GPU bindings need not equal the CPU segmented-address table.
                __gSPSegment(gfx.polyXlu.p++,11,reinterpret_cast<uintptr_t>(&setupDl));
                Gfx* begin=gfx.polyXlu.p;
                NeiGi_DrawRewardMaterial(&play,profile,"surface","setting","oot");
                assert(arena.size()==(textures?3u:1u));
                assert(stack.empty() && matrix==1 && matrixY==0);
                bool tinted=false;
                for (const auto& v:arena.back()) {
                    const uint32_t rgb=uint32_t(v.v.cn[0])<<16 | uint32_t(v.v.cn[1])<<8 | v.v.cn[2];
                    assert(rgb==0xffffff || rgb==RewardGi_Color(profile));
                    tinted |= rgb==RewardGi_Color(profile);
                }
                assert(tinted);
                int depth=0, scrollCalls=0;
                uintptr_t gpuSegment11=reinterpret_cast<uintptr_t>(&setupDl);
                for (Gfx* cmd=begin; cmd<gfx.polyXlu.p; ++cmd) {
                    const auto op=cmd->words.w0>>24;
                    if (op==G_COMBO_RM_PUSH) ++depth;
                    if (op==G_COMBO_RM_POP) --depth;
                    if (op==G_MOVEWORD && ((cmd->words.w0>>16)&255)==G_MW_SEGMENT &&
                        (cmd->words.w0&0xffff)==11*4) {
                        gpuSegment11=cmd->words.w1;
                    }
                    if (op==G_DL && cmd->words.w1!=reinterpret_cast<uintptr_t>(&setupDl) && !(cmd->words.w1&1)) {
                        const auto* list=reinterpret_cast<const Gfx*>(cmd->words.w1);
                        if(list[0].words.w0>>24==G_SETTILESIZE) {
                            ++scrollCalls; const auto offset=RewardGi_Scroll(frame);
                            assert(list[1].words.w0>>24==G_ENDDL);
                            assert(((list[0].words.w0>>12)&0xfff)==offset.s && (list[0].words.w0&0xfff)==offset.t);
                        }
                    }
                    assert(depth>=0);
                }
                assert(gpuSegment11==reinterpret_cast<uintptr_t>(&setupDl));
                assert(depth==0 && scrollCalls==(textures?1:0));
                assert(gfx.polyOpa.p<=gfx.polyOpa.d && gfx.polyXlu.p<=gfx.polyXlu.d);
            }
        }
    for (int profile : {-1,0,10}) {
        Reset(); NeiGi_DrawRewardMaterial(&play,profile,"surface","setting","oot");
        assert(allocations==0 && arena.empty());
    }
    for (int buffer=0;buffer<3;++buffer) {
        Reset(); files.insert(RewardGi_Texture(1)); files.insert("__OTR__objects/nei_reward_gi/metal");
        if (buffer==0) gfx.polyOpa.d=opa+1;
        if (buffer==1) gfx.polyXlu.d=xlu+1;
        if (buffer==2) gfx.overlay.d=overlay+1;
        NeiGi_DrawRewardMaterial(&play,1,"surface","setting","oot");
        assert(allocations==0 && arena.empty() && gfx.polyXlu.p==xlu && stack.empty());
    }
    Reset(); files.insert(RewardGi_Texture(1));
    NeiGi_DrawRewardMaterial(&play,1,"missing","missing","oot");
    assert(arena.size()==1 && "unsupported geometry keeps its native draw and matching shimmer");
    std::cout << "PASS actual reward renderer: nine profiles, shared scroll fragment, hex shimmer, missing-resource fallback, Alt/toggle parity, arena/owner/GPU-segment/pose restore\n";
    return 0;
}
