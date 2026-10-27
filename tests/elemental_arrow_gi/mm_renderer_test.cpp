#define main BaselineRendererMain
#include "../mm_nei/renderer_runtime_test.cpp"
#undef main
#include "../../soh/soh/Enhancements/randomizer/NeiElementalSpellGi.h"

int main() {
    PlayState play{};
    GraphicsContext gfx{};
    alignas(16) static Gfx opa[0x6700], xlu[0x1000], overlay[0x800];
    play.state.gfxCtx=&gfx; gPlayState=&play;
    play.billboardMtxF.xx=play.billboardMtxF.yy=play.billboardMtxF.zz=1;
    auto reset=[&]() {
        gfx.polyOpa.p=opa; gfx.polyOpa.d=std::end(opa);
        gfx.polyXlu.p=xlu; gfx.polyXlu.d=std::end(xlu);
        gfx.overlay.p=overlay; gfx.overlay.d=std::end(overlay);
        Matrix_Translate(7,8,9,MTXMODE_NEW);
    };
    size_t maxBytes=0, maxCommands=0;
    for (int profile=1; profile<=3; ++profile) for (bool present:{false,true})
        for (bool alt:{false,true}) for (bool enabled:{false,true}) {
            ownerBase.clear(); ownerRegistered=true;
            if (present) ownerBase.insert(NeiArrowGi::Texture(profile));
            ownerAltEnabled=mmAltEnabled=alt; itemEffects=enabled;
            for (unsigned frame=0; frame<360; frame+=17) {
                reset(); play.gameplayFrames=frame;
                const auto saved=current;
                NeiGi_DrawElementalArrow(&play,profile);
                assert(matrices.empty() && !memcmp(&saved,&current,sizeof(saved)));
                assert(gfx.polyOpa.p<=gfx.polyOpa.d && gfx.polyXlu.p<=gfx.polyXlu.d);
                int depth=0; unsigned pushes=0, vertexLoads=0, matricesLoaded=0;
                for (Gfx* cmd=xlu; cmd<gfx.polyXlu.p; ++cmd) {
                    const auto op=cmd->words.w0>>24;
                    if (op==G_COMBO_RM_PUSH) {++depth; ++pushes;}
                    if (op==G_COMBO_RM_POP) --depth;
                    if (op==G_VTX) ++vertexLoads;
                    if (op==G_MTX) ++matricesLoaded;
                    assert(depth>=0 && depth<=1);
                }
                assert(depth==0 && pushes==(present?1u:0u) && vertexLoads>=3 && matricesLoaded==4);
                maxBytes=std::max(maxBytes,size_t(reinterpret_cast<uintptr_t>(std::end(opa))-
                                                reinterpret_cast<uintptr_t>(gfx.polyOpa.d)));
                maxCommands=std::max(maxCommands,size_t(gfx.polyXlu.p-xlu));
            }
        }
    for (int arena=0; arena<3; ++arena) {
        reset();
        if (arena==0) gfx.polyOpa.d=opa+1;
        if (arena==1) gfx.polyXlu.d=xlu+1;
        if (arena==2) gfx.overlay.d=overlay+1;
        const auto saved=current;
        const auto tail=gfx.polyOpa.d;
        NeiGi_DrawElementalArrow(&play,1);
        assert(gfx.polyOpa.p==opa && gfx.polyOpa.d==tail && gfx.polyXlu.p==xlu && matrices.empty());
        assert(!memcmp(&saved,&current,sizeof(saved)));
        assert(!NeiGi_DrawElementalArrowShop(&play,1));
        assert(gfx.polyOpa.p==opa && gfx.polyOpa.d==tail && gfx.polyXlu.p==xlu && matrices.empty());
        assert(!memcmp(&saved,&current,sizeof(saved)));
    }
    for (int profile=1;profile<=3;++profile) for (bool shop:{false,true}) {
        reset(); const auto saved=current;
        assert(NeiGi_DrawElementalSpell(&play,profile,1,shop));
        assert(matrices.empty() && !memcmp(&saved,&current,sizeof(saved)));
        unsigned loaded=0; int depth=0;
        for(Gfx* cmd=xlu;cmd<gfx.polyXlu.p;++cmd) {
            const auto op=cmd->words.w0>>24;
            loaded+=op==G_MTX;
            if(op==G_COMBO_RM_PUSH) ++depth;
            if(op==G_COMBO_RM_POP) --depth;
        }
        assert(loaded==6 && depth==0 && gfx.polyOpa.p<=gfx.polyOpa.d && gfx.polyXlu.p<=gfx.polyXlu.d);
    }
    reset(); gfx.polyXlu.d=xlu+1;
    assert(!NeiGi_DrawElementalSpell(&play,1,1,0) && gfx.polyOpa.p==opa && gfx.polyXlu.p==xlu && matrices.empty());
    reset();
    const size_t fallbackBytes=20*sizeof(Gfx)+sizeof(Mtx)+12*sizeof(Gfx);
    gfx.polyOpa.d=opa+(fallbackBytes/sizeof(Gfx))-1;
    assert(!NeiGi_CanDrawElementalSpellFallback(&play));
    gfx.polyOpa.d=opa+(fallbackBytes/sizeof(Gfx));
    assert(NeiGi_CanDrawElementalSpellFallback(&play));
    reset();
    for(int p:{1,2,3,1,2,3}) assert(NeiGi_DrawElementalSpell(&play,p,1,1));
    assert(gfx.polyOpa.p<=gfx.polyOpa.d && gfx.polyXlu.p<=gfx.polyXlu.d && matrices.empty());
    std::cout << "PASS native MM spell render passes, held/shelf matrix restore, admission, six-crystal shelf\n";
    std::cout << "PASS native MM elemental renderer: all profiles, texture/fallback, Alt/toggle, owner/matrix restore, arena decline; peak "
              << maxBytes << " OPA bytes / " << maxCommands << " XLU commands\n";
}
