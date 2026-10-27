#define NEI_GI_FIXTURE_BOUNDARY_ONLY
#include "../nei_gi/presentation_test.cpp"
// The focused fixture never selects these unrelated legacy callbacks.
extern "C" void Randomizer_DrawTrueMasterSwordFlame(PlayState*) { assert(false); }
extern "C" void Randomizer_DrawCaneSomariaUpgradeFlame(PlayState*) { assert(false); }
#if __has_include("ComboElementalArrowGi.h")
#include "ComboElementalArrowGi.h"
int main() {
    using namespace Fixture;
    // A lost restore, missing optional archive, duplicate shimmer, or graphics
    // arena overrun fails this real renderer/GBI boundary test.
    for (int profile = 1; profile <= 3; ++profile) for (bool texture : {false,true}) {
        for (int assets : {0,1}) for (int toggle : {0,1}) {
            Reset(); alt=assets; enabled=toggle;
            if(texture) files.insert(NeiArrowGi::Texture(profile));
            NeiGi_DrawElementalArrow(&play,profile);
            assert(arena.size() == 3 && !arena[0].empty() && !arena[1].empty() && !arena[2].empty());
            assert(stack.empty() && matrix==1 && matrixY==0);
            assert(gfx.polyOpa.p<=gfx.polyOpa.d && gfx.polyXlu.p<=gfx.polyXlu.d);
            const auto* p=xlu; int depth=0;
            for(;p<gfx.polyXlu.p;++p) {
                if((p->words.w0>>24)==G_COMBO_RM_PUSH) ++depth;
                if((p->words.w0>>24)==G_COMBO_RM_POP) --depth;
                assert(depth>=0);
            }
            assert(depth==0);
        }
    }
    for(int p=1;p<=3;++p) {
        Reset(); GetItemEntry entry{};
        const int gids[]={GID_ARROW_FIRE,GID_ARROW_ICE,GID_ARROW_LIGHT};entry.gid=gids[p-1];
        assert(NeiGi_DrawShop(&play,&entry));
        assert(Drawn()==std::vector<std::string>{"__OTR__objects/object_gi_m_arrow/gGiMagicArrowDL"});
        assert(arena.size()==3 && matrix==1 && stack.empty());
        Reset();
        const int spells[]={GID_DINS_FIRE,GID_FARORES_WIND,GID_NAYRUS_LOVE};entry.gid=spells[p-1];
        assert(NeiGi_DrawShop(&play,&entry) && arena.size()==5 && matrix==1 && stack.empty());
    }
    for(int invalid : {-1,0,4,100}) {
        Reset(); NeiGi_DrawElementalArrow(&play,invalid);
        assert(arena.empty() && allocations==0 && stack.empty());
    }
    for(int space : {0,1}) {
        Reset(); gfx.polyXlu.d=xlu+space;
        assert(!NeiGi_DrawElementalArrowShop(&play,1));
        assert(gfx.polyOpa.p==opa && gfx.polyXlu.p==xlu && allocations==0 && arena.empty() && stack.empty());
    }
    Reset();
    const size_t fallbackBytes=20*sizeof(Gfx)+sizeof(Mtx)+12*sizeof(Gfx);
    gfx.polyOpa.d=opa+(fallbackBytes/sizeof(Gfx))-1;
    assert(!NeiGi_CanDrawElementalSpellFallback(&play));
    gfx.polyOpa.d=opa+(fallbackBytes/sizeof(Gfx));
    assert(NeiGi_CanDrawElementalSpellFallback(&play));
    for(int space=0;space<32;++space) {
        Reset(); gfx.polyOpa.d=opa+space;
        NeiGi_DrawElementalArrow(&play,1);
        assert(allocations==0 && arena.empty() && gfx.polyXlu.p==xlu && stack.empty());
    }
    const char* rows[2][2][3] = {
        {{"gCosmetics.Arrows.Fire", "gCosmetics.Arrows.Ice", "gCosmetics.Arrows.Light"},
         {"gCosmetics.Magic.Dins", "gCosmetics.Magic.Farores", "gCosmetics.Magic.Nayrus"}},
        {{"gCosmetic.Effects.FireArrow", "gCosmetic.Effects.IceArrow", "gCosmetic.Effects.LightArrow"},
         {"gCosmetic.Magic.Dins", "gCosmetic.Magic.Farores", "gCosmetic.Magic.Nayrus"}}
    };
    for (int owner=0;owner<2;++owner) for (int spell=0;spell<2;++spell) for (int p=1;p<=3;++p) {
        auto draw=[&](bool shop) {
            if (spell) assert(NeiGi_DrawElementalSpell(&play,p,owner,shop));
            else NeiGi_DrawElementalArrowForOwner(&play,p,owner,shop);
        };
        Reset(); draw(false);
        const auto original=arena;
        const std::string row=rows[owner][spell][p-1];
        const std::string primary=row+(owner && !spell?"Prim":"Primary");
        const std::string secondary=row+(owner && !spell?"Sec":"Secondary");
        const std::string suffix=owner?".Color":".Value";
        // Deliberately different host/owner colors expose accidental host-row reads.
        cosmeticFlags[primary+".Changed"]=cosmeticFlags[secondary+".Changed"]=1;
        cosmeticColors[primary+suffix]={160,68,226};
        cosmeticColors[secondary+suffix]={32,204,120};
        arena.clear(); draw(false);
        assert(arena.size()==size_t(spell?5:3) && stack.empty() && matrix==1);
        bool green=false;
        for(const auto& v:arena.back()) {
            const auto* c=v.v.cn;
            const bool white=c[0]==255 && c[1]==255 && c[2]==255;
            assert(white || (c[0]==32 && c[1]==204 && c[2]==120));
            green|=!white;
        }
        assert(green && "the editor must tint the GI shimmer too");
        const auto full=arena;
        arena.clear(); draw(true);
        if(spell) for(size_t m=0;m<arena.size();++m) {
            // Fixed-point packing can merge different vertices after scaling.
            // Compare the actual submitted footprint rather than cache topology.
            for(int axis=0;axis<3;++axis) {
                int held=0,shelf=0;
                for(const auto& v:full[m]) held=std::max(held,std::abs(int(v.v.ob[axis])));
                for(const auto& v:arena[m]) shelf=std::max(shelf,std::abs(int(v.v.ob[axis])));
                assert(shelf<=held && shelf>0);
            }
        }
        cosmeticFlags.clear(); arena.clear(); draw(false);
        assert(arena.size()==original.size());
        for(size_t m=0;m<arena.size();++m) {
            assert(arena[m].size()==original[m].size());
            assert(!memcmp(arena[m].data(),original[m].data(),arena[m].size()*sizeof(Vtx)));
        }
        assert(gfx.polyOpa.p<=gfx.polyOpa.d && gfx.polyXlu.p<=gfx.polyXlu.d);
    }
    for(int profile=1;profile<=3;++profile) for(int buffer=0;buffer<3;++buffer) {
        Reset();
        if(buffer==0) gfx.polyOpa.d=opa+1;
        if(buffer==1) gfx.polyXlu.d=xlu+1;
        if(buffer==2) gfx.overlay.d=overlay+1;
        assert(!NeiGi_DrawElementalSpell(&play,profile,0,0));
        assert(allocations==0 && arena.empty() && gfx.polyXlu.p==xlu && stack.empty());
    }
    Reset();
    for(int p : {1,2,3,1,2,3}) assert(NeiGi_DrawElementalSpell(&play,p,0,1));
    assert(gfx.polyOpa.p<=gfx.polyOpa.d && gfx.polyXlu.p<=gfx.polyXlu.d && stack.empty());
    std::cout << "PASS all six live GI palettes in both owner namespaces, reset, shelf fit, and six-crystal shelf admission\n";
    std::cout << "PASS real elemental-arrow renderer: all profiles, Alt/toggle parity, missing-texture fallback, arena and owner restore\n";
}
#else
int main() { assert(false && "Elemental arrow renderer has not been implemented"); }
#endif
