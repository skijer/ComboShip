#!/usr/bin/env python3
"""Run the production descriptor when replacement geometry is unavailable.

Archive selection, CVar reads and absent model-fit graphs are controlled: the
real producer's ABI fields still drive the host's independent effect pass.
Actual selected graph fitting is exercised by the dedicated receipt fixture.
"""
import os
import re
from pathlib import Path
import subprocess
import tempfile

from run_mm_scene_randomization_tests import function

ROOT = Path(__file__).resolve().parents[2]
owner = (ROOT / 'soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp').read_text()
body = '\n'.join(function(owner, name) for name in ('PresentationShimmer', 'NeiGi_FillCrossGameInfo'))
source = r'''
#include <cassert>
#include <cstring>
#include "combo/menu/ComboItemDrawABI.h"
#include "soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h"
#include "soh/soh/Enhancements/randomizer/NeiGiFrameFit.h"
using NeiGi::Kind;
struct Vec3f {float x,y,z;};
struct Presentation {
    void (*draw)(); const char* opaque; const char* translucent; float scale;
    Kind effect; Vec3f effectCenter; bool alwaysShimmer;
    Kind shimmer=Kind::Neutral;
};
void Randomizer_DrawCaneSomariaUpgrade() {}
bool mod, selectedSword, resources; int enabled;
#define CVAR_NEI_GI_EFFECTS "gEnhancements.SkijerNEI.ItemEffects"
int CVarGetInteger(const char*,int) {return enabled;}
bool HasLegacyGiMod(const Presentation&,bool) {return mod;}
bool HasRedesignGiMod(const Presentation&) {return false;}
bool HasSelectedSword(const Presentation&,bool,bool (*)(const char*)) {return selectedSword;}
int OOT_NeiAltAssetsEnabled() {return true;}
int OOT_NeiResourceExists(const char*) {return resources;}
const char* NeiGi_BaseSwordPath(const char* path) {return path;}
''' + body + r'''
int main() {
    for(Kind kind : {Kind::Fire,Kind::Ice,Kind::Light,Kind::Hylia,Kind::Zonai,Kind::Demise,
                    Kind::Leaf,Kind::SlateBomb,Kind::SlateCycle,Kind::SlateStasis,
                    Kind::SlateCryonis,Kind::SlateSensor,Kind::Somaria,Kind::Pacci,
                    Kind::Pokeball,Kind::FourSword,Kind::MasterSword}) {
        const bool mandatory=NeiGi::IsSword(kind);
        Presentation p{nullptr,"__OTR__objects/nei_gi_redesign/test/gi_dl",nullptr,1,kind,{},mandatory};
        for(int effects : {0,1}) for(int selection : {0,1,2,3}) {
            enabled=effects; mod=selection==1; selectedSword=selection==2; resources=selection!=3;
            CwItemDrawInfo out{};
            const bool authored=NeiGi_FillCrossGameInfo(p,&out);
            assert(authored==(selection==0));
            assert(out.neiShimmer==int(kind)+1 && "declining the model discarded the item identity");
            assert(out.itemShimmer==(mandatory||effects));
            assert(out.stateDependent==2 && "live model selection cannot freeze after a grant");
            if(!authored) {
                assert(out.dlistCount==0 && out.neiEffect==0 && "mod mesh inherited authored energy");
            } else {
                assert(out.dlistCount==1 && out.neiEffect==int(kind));
            }
        }
    }
}
'''
with tempfile.TemporaryDirectory(prefix='nei-identity-') as temporary:
    path=Path(temporary)
    (path/'fixture.cpp').write_text(source)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),str(path/'fixture.cpp'),
                    '-o',str(path/'fixture')],check=True)
    subprocess.run([str(path/'fixture')],check=True)
print('PASS production NEI identity descriptors: 17 themes, authored/mod/custom/missing selection, effects off/on')

mm = (ROOT/'mm/2s2h/Rando/NeiGiPresentation.cpp').read_text()
bindings = re.search(r'struct Binding \{.*?const Binding kBindings\[\] = \{.*?\n\};', mm, re.S)[0]
header = (ROOT/'mm/2s2h/Rando/NeiGiPresentation.h').read_text()
fallback = mm[mm.index('MM_NeiGiFallbackShimmer::MM_NeiGiFallbackShimmer'):]
# Instrument the real sizing helper so the model/effect call counts still
# protect sword-only sizing while its actual transform body also executes.
presentation_size = function((ROOT/'combo/menu/ComboSwordGiEffectFit.h').read_text(),
                             'ComboSwordGi_ApplyPresentationSize')
presentation_size = presentation_size.replace('{', '''{
    assert(NeiGi::IsSword(awardKind) && !shop && !mmPickup);
    ++sizeCalls;''', 1)
particle_scale = function((ROOT/'combo/menu/ComboSwordGiEffectFit.h').read_text(),
                          'ComboSwordGi_ParticleScale')
source = source[:source.index('int main() {')] + r'''
#include <algorithm>
#include "mm/2s2h/Rando/Types.h"
struct PlayState {uint32_t gameplayFrames=47;};
PlayState play; PlayState* gPlayState=&play;
Kind awardKind=Kind::Fire; bool mmMod; int pushes, draws, fitCalls, sizeCalls;
NeiGi::Mesh captured;
void Matrix_Push() {++pushes;} void Matrix_Pop() {assert(pushes>0);--pushes;}
constexpr int MTXMODE_APPLY=1;
void Matrix_Translate(float,float,float,int) {}
void Matrix_Scale(float,float,float,int) {}
NeiGi::Basis NeiGi_CameraBasis(PlayState*) {return {};}
void NeiGi_DrawMesh(PlayState*,const NeiGi::Mesh& mesh) {captured=mesh;++draws;}
int Describe(const char*,CwItemDrawInfo* out) {
    Presentation p{nullptr,"__OTR__objects/nei_gi_redesign/test/gi_dl",nullptr,1,awardKind,{},NeiGi::IsSword(awardKind)};
    return NeiGi_FillCrossGameInfo(p,out);
}
void* Combo_ResolveSym(const char*,const char* name) {
    return !strcmp(name,"OOT_GetNeiGiDrawInfo") ? reinterpret_cast<void*>(Describe) : nullptr;
}
bool ResourceMgr_IsAltAssetsEnabled() {return true;}
const char* NeiResource_Route(const char* path) {return path;}
int ResourceMgr_IsModAssetForGame(const char*,const char*) {return mmMod;}
int ResourceMgr_IsIkanaShieldGiAvailable() {assert(false && "identity fixture must not select the native shield pair");return 0;}
void ComboSwordGi_ApplyLegacyFit(const char* owner,Kind kind,bool shop=false,int pickup=0) {
    assert(!strcmp(owner,"mm") && kind==awardKind && NeiGi::IsSword(kind) && !shop && !pickup);
    ++fitCalls;
}
void ComboSwordGi_ApplyEffectFit(Kind kind,bool shop=false,int pickup=0) {
    assert(kind==awardKind && NeiGi::IsSword(kind) && !shop && !pickup);
}
''' + presentation_size + '\n' + particle_scale + '\n' + bindings + '\n' + header[header.index('class MM_NeiGiFallbackShimmer'):] + '\n'
source += function(mm,'HasMmLegacyGiMod')+'\n'+function(mm,'GetSelectedOwnerGi')+'\n'+function(mm,'MM_DescribeNeiGi')+'\n'+fallback
source += r'''
int main() {
    const std::pair<RandoItemId,Kind> awards[]={
        {RI_OOT_NEI_FIRE_ROD,Kind::Fire},{RI_OOT_NEI_ICE_ROD,Kind::Ice},
        {RI_OOT_NEI_LIGHT_ROD,Kind::Light},{RI_OOT_NEI_HYLIAS_GRACE,Kind::Hylia},
        {RI_OOT_NEI_ZONAI_PERMAFROST,Kind::Zonai},{RI_OOT_NEI_DEMISE_DESTRUCTION,Kind::Demise},
        {RI_OOT_NEI_SLATE_RUNE_BOMB,Kind::SlateBomb},{RI_OOT_NEI_SLATE_RUNE_MASTER_CYCLE,Kind::SlateCycle},
        {RI_OOT_NEI_SLATE_RUNE_STASIS,Kind::SlateStasis},{RI_OOT_NEI_SLATE_RUNE_CRYONIS,Kind::SlateCryonis},
        {RI_OOT_NEI_DESIRE_SENSOR,Kind::SlateSensor},{RI_OOT_EXT_CANE_OF_BYRNA,Kind::CaneBlue},
        {RI_OOT_EXT_FOUR_SWORD,Kind::FourSword},{RI_SWORD_KOKIRI,Kind::MmKokiriSword},
        {RI_SWORD_RAZOR,Kind::RazorSword},{RI_OOT_MASTER_SWORD,Kind::MasterSword},
        {RI_OOT_NEI_CANE_OF_SOMARIA,Kind::Somaria},{RI_OOT_NEI_CANE_PACCI_FLIP,Kind::Pacci},
        {RI_OOT_NEI_POKE_BALL,Kind::Pokeball}};
    for(const auto& award:awards) for(int effects:{0,1}) for(int selection:{1,2,3,4}) {
        awardKind=award.second; enabled=effects; resources=selection!=3;
        mod=selection==1; selectedSword=selection==2; mmMod=selection==4;
        CwItemDrawInfo info{};
        const bool authored=MM_DescribeNeiGi(award.first,&info);
        if(selection!=4 || award.first==RI_OOT_NEI_FIRE_ROD || award.first==RI_SWORD_KOKIRI) assert(!authored);
        if(selection==4 && award.first==RI_OOT_EXT_FOUR_SWORD)
            assert(authored && "A generic MM Kokiri GI mod must not own the OoT-authored Four Sword");
        assert(info.neiShimmer==int(award.second)+1);
        const bool intrinsic=NeiGi::IsSword(award.second)||award.first==RI_OOT_EXT_CANE_OF_BYRNA;
        assert(info.itemShimmer==(intrinsic||effects));
        draws=0; fitCalls=0; sizeCalls=0;
        {MM_NeiGiFallbackShimmer fallback(award.first);}
        assert(draws==(NeiGi::IsSword(award.second) ? 2 : int(intrinsic||effects)) &&
               "selected equipment lost intrinsic shimmer or sword energy");
        assert(fitCalls==int(NeiGi::IsSword(award.second)) && "only sword fallbacks enter the model-fit boundary");
        assert(sizeCalls==2*int(NeiGi::IsSword(award.second)) && "only swords size the separate model and effect passes");
        assert(pushes==0);
        if(draws) {
            const auto expected=NeiGi::SampleShimmer(play.gameplayFrames,true,{},award.second);
            assert(captured.count==expected.count);
            for(size_t i=0;i<captured.count;++i)
                assert(captured.vertices[i].rgb==expected.vertices[i].rgb &&
                       captured.vertices[i].p.x==expected.vertices[i].p.x);
        }
    }
}
'''
with tempfile.TemporaryDirectory(prefix='mm-nei-identity-') as temporary:
    path=Path(temporary)
    (path/'fixture.cpp').write_text(source)
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-I'+str(ROOT),str(path/'fixture.cpp'),
                    '-o',str(path/'fixture')],check=True)
    subprocess.run([str(path/'fixture')],check=True)
print('PASS native MM fallback: 19 themes retain exact shimmer; swords also retain intrinsic particles through mod selection')
