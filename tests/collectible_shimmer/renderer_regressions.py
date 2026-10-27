#!/usr/bin/env python3
"""Exercise the authored sampler and production color-overlay renderer.

Only GPU submission is captured; geometry, palette application and guards are real.
"""
from pathlib import Path
import argparse
import os
import subprocess
import tempfile
from native_regressions import ROOT, function


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--baseline',action='store_true')
    args=parser.parse_args()
    archived=ROOT.parent/'shimmer_inputs'
    source_root=archived if args.baseline else ROOT
    src=(source_root/'soh/soh/Enhancements/randomizer/NeiGiMeshRenderer.inc').read_text()
    policy=ROOT/'soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h'
    if not policy.exists():
        policy=archived/'soh/soh/Enhancements/randomizer/NeiGiEffectPolicy.h'
    code=r'''
#include <cassert>
#include <cstdio>
#include <cstring>
#include <algorithm>
'''+f'#include "{policy}"\n'+r'''
struct GraphicsContext {};
struct PlayState { struct { GraphicsContext* gfxCtx; } state; uint32_t gameplayFrames; };
using NeiGi::Kind;
NeiGi::Mesh captured{}; int submissions=0; const char* submittedOwner=nullptr;
NeiGi::Basis NeiGi_CameraBasis(PlayState*) { return {}; }
void NeiGi_DrawShimmerMesh(PlayState*,const NeiGi::Mesh& mesh,const char* owner) { captured=mesh; submittedOwner=owner; ++submissions; }
'''
    code+=function(src,'NeiGi_DrawColorShimmerOverlay')+'\n'
    code+=function(src,'NeiGi_DrawShimmerOverlay')+'\n'
    tinted=function(src,'NeiGi_DrawTintedShimmerOverlay')
    if not tinted:
        # Baseline has no explicit RGB boundary: demonstrate the neutral collision.
        tinted='void NeiGi_DrawTintedShimmerOverlay(PlayState* p,const uint8_t c[4],const char* o) { NeiGi_DrawShimmerOverlay(p,c,o); }'
    code+=tinted+r'''
int main() {
    GraphicsContext gfx; PlayState play{{&gfx},47};
    const auto authored=NeiGi::SampleShimmer(47,true);
    const uint8_t neutral[4]={220,225,240,255};
    NeiGi_DrawShimmerOverlay(&play,neutral,"oot");
    assert(captured.count==authored.count && captured.count>0);
    for(size_t i=0;i<authored.count;++i) assert(captured.vertices[i].rgb==authored.vertices[i].rgb);
    for(const auto* owner : {"oot","mm"}) {
        for(const auto color : {0xDCE1F0u,0x046D21u,0x0D61F1u,0x000000u}) {
            const uint8_t rgba[4]={uint8_t(color>>16),uint8_t(color>>8),uint8_t(color),255};
            NeiGi_DrawTintedShimmerOverlay(&play,rgba,owner);
            assert(std::strcmp(submittedOwner,owner)==0 && captured.count==authored.count);
            for(size_t i=0;i<authored.count;++i) {
                const auto& a=authored.vertices[i]; const auto& b=captured.vertices[i];
                assert(a.p.x==b.p.x && a.p.y==b.p.y && a.p.z==b.p.z && a.alpha==b.alpha && a.u==b.u && a.v==b.v);
                assert(b.rgb==(a.rgb==0xFFFFFF ? (color==0 ? 0x505060u : 0xFFFFFFu) : color));
            }
        }
    }
    const int before=submissions;
    NeiGi_DrawTintedShimmerOverlay(nullptr,neutral,"oot");
    NeiGi_DrawTintedShimmerOverlay(&play,nullptr,"mm");
    play.state.gfxCtx=nullptr; NeiGi_DrawTintedShimmerOverlay(&play,neutral,"mm");
    assert(submissions==before);
    puts("renderer: exact authored geometry/alpha, explicit neutral RGB, legacy palette, owner/guards passed");
}
'''
    with tempfile.TemporaryDirectory(prefix='collectible-renderer-') as tmp:
        srcfile=Path(tmp)/'renderer.cpp'; srcfile.write_text(code)
        binary=Path(tmp)/'renderer'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror',str(srcfile),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)

if __name__=='__main__': main()
