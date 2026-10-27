#define NEI_GI_REWARD_FIXTURE
#define main BaselineRendererMain
#include "../mm_nei/renderer_runtime_test.cpp"
#undef main
#include "ComboRewardGi.h"
static const char* expectedGeometryOwner="oot";
static int nativeRewardLoads=0;
static Gfx nativeRewardLists[2];
extern "C" void* RewardFixture_LoadNativeGfx(const char*) {
    ++nativeRewardLoads;
    return &nativeRewardLists[mmAltEnabled ? 1 : 0];
}
extern "C" int ResourceMgr_GetRewardSurfaceForGame(const char* owner, const char* path, NeiGi::Mesh* out) {
    assert(!strcmp(owner,expectedGeometryOwner));
    *out = {};
    if (!strcmp(path,"missing")) return 0;
    out->Tri({{-10,-10,0},0xffffff,255,0,0},{{10,-10,0},0xffffff,255,1,0},{{0,10,0},0xffffff,255,.5,1});
    return 1;
}
extern "C" {
void Matrix_RotateZYX(s16 x, s16 y, s16 z, MatrixMode mode) {
    constexpr float radians=3.14159265358979323846f/32768.f;
    Matrix_RotateZF(z*radians,mode); Matrix_RotateYF(y*radians,mode); Matrix_RotateXF(x*radians,mode);
}
Gfx* Gfx_TexScrollEx(GraphicsContext*,u32,u32,s32,s32,s32,s32) { return &nativeRewardLists[0]; }
Gfx* Gfx_TwoTexScrollEx(GraphicsContext*,s32,u32,u32,s32,s32,s32,u32,u32,s32,s32,s32,s32,s32,s32) { return &nativeRewardLists[0]; }
}
#include "reward_mm_recipes.inc"
struct ComboForeignDrawInfoOOT {
    const char* dls[8]{};
    uint8_t primColorXlu[3]{255,255,255}, envColorXlu[3]{255,255,255};
    uint8_t primColorOpa[3]{255,255,255}, envColorOpa[3]{255,255,255};
};
#include "reward_foreign_jewel.inc"
int main() {
    PlayState play{}; GraphicsContext gfx{};
    alignas(16) static Gfx opa[0x6700], xlu[0x1000], overlay[0x800];
    play.state.gfxCtx=&gfx; gPlayState=&play;
    play.billboardMtxF.xx=play.billboardMtxF.yy=play.billboardMtxF.zz=1;
    auto reset=[&]() {
        gfx.polyOpa.p=opa; gfx.polyOpa.d=std::end(opa);
        gfx.polyXlu.p=xlu; gfx.polyXlu.d=std::end(xlu);
        gfx.overlay.p=overlay; gfx.overlay.d=std::end(overlay);
        Matrix_Translate(7,8,9,MTXMODE_NEW);
    };
    for (int profile=1;profile<=9;++profile) for (bool textures:{false,true})
        for (bool alt:{false,true}) for (bool effects:{false,true}) for (unsigned frame:{0u,1u,127u,128u,UINT32_MAX}) {
            reset(); ownerBase.clear(); ownerRegistered=true; ownerAltEnabled=mmAltEnabled=alt; itemEffects=effects;
            if (textures) { ownerBase.insert(RewardGi_Texture(profile)); ownerBase.insert("__OTR__objects/nei_reward_gi/metal"); }
            play.gameplayFrames=frame; const auto saved=current;
            NeiGi_DrawRewardMaterial(&play,profile,"surface","setting","oot");
            assert(matrices.empty() && !memcmp(&saved,&current,sizeof(saved)));
            int depth=0, scrollCalls=0, loads=0;
            bool matching=false;
            for (Gfx* cmd=xlu;cmd<gfx.polyXlu.p;++cmd) {
                const auto op=cmd->words.w0>>24;
                if(op==G_COMBO_RM_PUSH) ++depth;
                if(op==G_COMBO_RM_POP) --depth;
                assert(depth>=0);
                if(op==G_MOVEWORD && ((cmd->words.w0>>16)&255)==G_MW_SEGMENT && (cmd->words.w0&0xffff)==44) {
                    assert(false && "reward materials must preserve the scene's GPU segment binding");
                }
                if(op==G_DL && cmd->words.w1!=reinterpret_cast<uintptr_t>(&setupDl) && !(cmd->words.w1&1)) {
                    const auto* list=reinterpret_cast<const Gfx*>(cmd->words.w1);
                    if(list[0].words.w0>>24==G_SETTILESIZE) {
                        ++scrollCalls;
                        const auto offset=RewardGi_Scroll(frame);
                        assert(list[0].words.w0>>24==G_SETTILESIZE && list[1].words.w0>>24==G_ENDDL);
                        assert(((list[0].words.w0>>12)&0xfff)==offset.s && (list[0].words.w0&0xfff)==offset.t);
                    }
                }
                if(op==G_VTX) {
                    ++loads; const auto* v=reinterpret_cast<const Vtx*>(cmd->words.w1);
                    for(size_t i=0;i<((cmd->words.w0>>12)&255);++i)
                        matching |= (uint32_t(v[i].v.cn[0])<<16 | uint32_t(v[i].v.cn[1])<<8 | v[i].v.cn[2])==RewardGi_Color(profile);
                }
            }
            assert(depth==0 && matching && loads>0 && scrollCalls==(textures?1:0));
            assert(gfx.polyOpa.p<=gfx.polyOpa.d && gfx.polyXlu.p<=gfx.polyXlu.d);
        }
    for (int buffer=0;buffer<3;++buffer) {
        reset(); if(buffer==0) gfx.polyOpa.d=opa+1;
        if(buffer==1) gfx.polyXlu.d=xlu+1;
        if(buffer==2) gfx.overlay.d=overlay+1;
        const auto saved=current; const auto tail=gfx.polyOpa.d;
        NeiGi_DrawRewardMaterial(&play,1,"surface","setting","oot");
        assert(gfx.polyOpa.p==opa && gfx.polyOpa.d==tail && gfx.polyXlu.p==xlu && matrices.empty());
        assert(!memcmp(&saved,&current,sizeof(saved)));
    }
    expectedGeometryOwner="mm";
    const char* const surface[] = {
        "__OTR__objects/object_gi_medal/gGiForestMedallionFaceDL", "__OTR__objects/object_gi_medal/gGiFireMedallionFaceDL",
        "__OTR__objects/object_gi_medal/gGiWaterMedallionFaceDL", "__OTR__objects/object_gi_medal/gGiSpiritMedallionFaceDL",
        "__OTR__objects/object_gi_medal/gGiShadowMedallionFaceDL", "__OTR__objects/object_gi_medal/gGiLightMedallionFaceDL",
        "__OTR__objects/object_gi_jewel/gGiKokiriEmeraldGemDL", "__OTR__objects/object_gi_jewel/gGiGoronRubyGemDL",
        "__OTR__objects/object_gi_jewel/gGiZoraSapphireGemDL"
    };
    for(int i=0;i<9;++i) {
        Gfx* first=nullptr; Gfx* second=nullptr;
        for(bool alt:{false,true,false}) {
            reset(); nativeDisplayLists.clear(); nativeRewardLoads=0;
            mmAltEnabled=alt; ownerAltEnabled=!alt; ownerBase.clear();
            ownerBase.insert(RewardGi_Texture(i+1)); ownerBase.insert("__OTR__objects/nei_reward_gi/metal");
            if(i<6) DrawOotMedallion(surface[i],&first);
            else DrawOotStone(surface[i],&first,"setting",&second,255,255,255,255,255,255,255,255,255,255,255,255);
            assert(nativeRewardLoads==2 && nativeDisplayLists.size()==2);
            for(const auto* dl:nativeDisplayLists) assert(dl==&nativeRewardLists[alt?1:0]);
            assert(first==&nativeRewardLists[alt?1:0] && matrices.empty());
            assert(gfx.polyOpa.p<=gfx.polyOpa.d && gfx.polyXlu.p<=gfx.polyXlu.d);
        }
    }
    std::cout << "PASS production MM reward recipes: nine native imports, MM geometry owner, independent owner Alt choices, replacement refresh\n";
    reset(); nativeDisplayLists.clear();
    Gfx* fountainCache=nullptr;
    DrawOotMedallion(surface[0],&fountainCache,false);
    assert(gfx.polyXlu.p==xlu && nativeDisplayLists.size()==2 && matrices.empty());
    expectedGeometryOwner="oot";
    for (size_t capacity=200;capacity<2000;capacity+=37) {
        reset(); gfx.polyOpa.d=opa+capacity;
        ownerBase.clear(); ownerBase.insert(RewardGi_Texture(7)); ownerBase.insert("__OTR__objects/nei_reward_gi/metal");
        ComboForeignDrawInfoOOT info{}; info.dls[0]=surface[6]; info.dls[1]="setting";
        const auto saved=current;
        MM_DrawForeignJewel(&info);
        assert(gfx.polyOpa.p<=gfx.polyOpa.d && gfx.polyXlu.p<=gfx.polyXlu.d && matrices.empty());
        assert(!memcmp(&saved,&current,sizeof(saved)));
    }
    std::cout << "PASS production foreign jewel: allocating segment-restore tail completes before optional passes across near-capacity arenas\n";
    std::cout << "PASS native MM reward renderer: nine profiles, continuous scroll, matching tint/shimmer, Alt/toggle parity, arena/owner/pose restore\n";
}
