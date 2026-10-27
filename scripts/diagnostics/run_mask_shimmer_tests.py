#!/usr/bin/env python3
"""Exercise native and cross-game mask/remains shimmer with both engines' real headers/GBI."""
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]

# Mirror the declarations visible before the first shimmer expansion in draw.cpp.
# An earlier fixture defined interpolation stubs in this TU and hid the C++ linkage bug.
oot_draw = (ROOT / 'soh/soh/Enhancements/randomizer/draw.cpp').read_text()
oot_prelude = oot_draw.split('#include "ComboMaskShimmer.h"', 1)[0]
oot_interpolation = ('#include "soh/frame_interpolation.h"'
                     if '#include "soh/frame_interpolation.h"' in oot_prelude else '')
foreign = (ROOT / 'combo/menu/ComboForeignAnim.h').read_text()
interpolation_declarations = re.findall(r'^void FrameInterpolation_Record(?:OpenChild|CloseChild)\([^;]*\);$', foreign, re.M)
assert len(interpolation_declarations) == 2
foreign_linkage = 'extern "C" {\n' + '\n'.join(interpolation_declarations) + '\n}'

def function(source, name):
    match = re.search(r'^(?:(?:static|inline) )?(?:void|s32|bool) ' + re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    if not match:
        raise ValueError(name)
    pos, depth = match.end(), 1
    while depth:
        depth += (source[pos] == '{') - (source[pos] == '}')
        pos += 1
    return source[match.start():pos]

flags = ['-DF3DEX_GBI_2', '-DCOMBO_BUILD', '-DLOG_LEVEL_GAME_PRINTS=0', '-DCONTROLLERBUTTONS_T=uint32_t',
         '-DNON_EQUIVALENT', '-DNON_MATCHING']
if '--sanitize' in sys.argv:
    flags += ['-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer', '-fno-pie', '-no-pie']
with tempfile.TemporaryDirectory(prefix='mask-shimmer-') as temporary:
    build = Path(temporary)
    for host, game in [('mm', 'mm'), ('oot', 'soh')]:
        native = (ROOT / game / 'src/code/z_draw.c').read_text()
        names = ['GetItem_GetShimmerColor', 'GetItem_Draw']
        if host == 'mm':
            names.insert(1, 'GetItem_DrawShimmer')
        fixture = (ROOT / 'tests/mask_shimmer/presentation_test.cpp').read_text()
        interpolation = ('#include "2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h"'
                         if host == 'mm' else oot_interpolation)
        fixture = fixture.replace('/* PRODUCTION_DRAW_PRELUDE */', interpolation)
        fixture = fixture.replace('/* PRODUCTION_FOREIGN_LINKAGE */', foreign_linkage)
        fixture = fixture.replace('/* PRODUCTION_NATIVE */', 'extern "C" {\n' + '\n'.join(function(native, n) for n in names) + '\n}')
        callbacks = ''
        if host == 'mm':
            callbacks = '\n'.join(function((ROOT / 'mm/2s2h/Rando/DrawItem.cpp').read_text(), name)
                                  for name in ['DrawOotRutosLetter', 'DrawOotBottleWithShimmer'])
        if host == 'oot':
            draw = oot_draw.replace('extern "C" void Randomizer_Draw', 'void Randomizer_Draw')
            callbacks = '\n'.join(function(draw, 'Randomizer_Draw'+name+'Upgrade') for name in
                                  ['Defense','Speed','Power','CrawlSpeed','ClimbSpeed','PushSpeed'])
        fixture = fixture.replace('/* PRODUCTION_RPG_DRAWS */', callbacks)
        seasonal = ('\n'.join(function(oot_draw, n) for n in
                               ['Randomizer_DrawNeiRodOfSeasons', 'DrawSeasonCommon'])
                    if host == 'oot' else
                    function((ROOT / 'mm/2s2h/Rando/DrawItem.cpp').read_text(), 'DrawOotNeiSeason'))
        fixture = fixture.replace('/* PRODUCTION_SEASON_DRAWS */', seasonal)
        native_dispatch = ''
        if host == 'mm':
            nei = (ROOT / 'mm/2s2h/Rando/NeiGiPresentation.cpp').read_text()
            bindings = nei[nei.index('struct Binding {'):nei.index('} // namespace')]
            native_dispatch = '''
#include "Rando/Types.h"
static int ownerLookups;
void* Combo_ResolveSym(const char*, const char*) { ++ownerLookups; return nullptr; }
#include "ComboSongDrawMM.h"
#include "ComboItemDrawABI.h"
''' + bindings + '''
extern "C" const char* NeiResource_Route(const char*) { assert(false); return nullptr; }
void DrawOotSlateRuneFlame(u8, u8, u8) { assert(false); }
void MM_DrawNeiGi(const CwItemDrawInfo&,bool shop=false,int mmPickup=0);
bool MM_TryDrawNeiGi(RandoItemId,bool shop=false,int mmPickup=0);
void DrawSong(RandoItemId) {assert(false && "songs have their own production dispatcher fixture");}
''' + function(nei, 'MM_DescribeNeiGi') + '\n' + function(nei, 'MM_DrawNeiGi') + '\n' + function(nei, 'MM_TryDrawNeiGi')
        fixture = fixture.replace('/* PRODUCTION_NEI_NATIVE_DISPATCH */', native_dispatch)
        blue = ''
        if host == 'oot':
            foreign_oot = (ROOT / 'combo/menu/ComboForeignDrawOOT.h').read_text()
            macros = foreign_oot[foreign_oot.index('#define COMBO_FOREIGN_MTX'):foreign_oot.index('// Biggoron')]
            blue = macros + function(foreign_oot, 'OOT_DrawForeignBlueFire')
        fixture = fixture.replace('/* PRODUCTION_BLUE_FIRE */', blue)

        if '--forest-particles' in sys.argv:
            fixture = '#include "' + str(ROOT / 'combo/actors/ForestGiParticles.h') + '"\n' + fixture
            fixture = '#include "' + str(ROOT / 'combo/actors/TempleSagesZeldaAura.h') + '"\n' + fixture
            fixture = fixture.replace('int main() {', 'int main() {\n#include "' +
                                      str(ROOT / 'tests/forest_gi_particles/renderer_checks.inc') + '"\n', 1)

        source = build / 'test.cpp'
        source.write_text(fixture)
        includes = [game, game+'/include', game+'/include/PR', game+'/src', game+'/assets', game+'/2s2h',
                    game+'/soh', game+'/mods', 'libultraship/include', 'libultraship/src', 'combo/menu', 'soh']
        bridge = build / 'interpolation.cpp'
        native_header = ('2s2h/Enhancements/FrameInterpolation/FrameInterpolation.h'
                         if host == 'mm' else 'soh/frame_interpolation.h')
        bridge.write_text('#include "z64.h"\n#include "' + native_header + '"\n'
                          'void FrameInterpolation_RecordOpenChild(const void*, int) {}\n'
                          'void FrameInterpolation_RecordCloseChild() {}\n')
        c_glue = build / 'c_boundary.c'
        c_glue.write_text('#include "' + ('global.h' if host == 'mm' else 'z64.h') + '"\n'
                          '#include "ComboMaskShimmer.h"\n'
                          'void Fixture_DrawCShimmer(PlayState* play, const uint8_t color[4]) {\n'
                          '    ComboDrawMaskShimmer(play, 0, color, 0);\n}\n')
        c_object = build / 'c_boundary.o'
        subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu2x', *flags,
                        *['-I'+str(ROOT/p) for p in includes], '-c', str(c_glue), '-o', str(c_object)], check=True)
        binary = build / host
        subprocess.run([os.environ.get('CXX', 'c++'), '-std=gnu++20', *flags,
                        *(['-DHOST_MM'] if host == 'mm' else []), *['-I'+str(ROOT/p) for p in includes],
                        str(source), str(bridge), str(c_object), '-o', str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
