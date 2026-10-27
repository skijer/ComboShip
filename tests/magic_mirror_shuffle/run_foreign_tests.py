"""Execute production portable Mirror descriptors, zero-DL resolver gates and host dispatch."""
from pathlib import Path
import os
import re
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
for host in ['soh', 'mm']:
    producer = (ROOT / ('combo/menu/ComboItemDrawOOT.h' if host == 'soh' else 'combo/menu/ComboItemDrawMM.h')).read_text()
    receiver = (ROOT / ('combo/menu/ComboForeignDrawMM.h' if host == 'soh' else 'combo/menu/ComboForeignDrawOOT.h')).read_text()
    item = 'RG_MAGIC_MIRROR' if host == 'soh' else 'RI_OOT_NEI_MAGIC_MIRROR'
    variable = 'effRg' if host == 'soh' else 'id'
    branch = re.search(r'    if \(' + variable + ' == ' + item + r'\)\s*return CwDescribeMagicMirror\(out\);', producer)[0]
    gate = receiver[receiver.index('    if (rcStatic == 0 ||'):].split('{', 1)[0]
    fixture = r'''
#include "global.h"
#include "ComboItemDrawABI.h"
#include "ComboMagicMirrorForeign.h"
#include "tests/test_require.h"
#include <cstring>
/* ENUMS */
static int draws;
static PlayState* received;
static float matrixScale = 1;
extern "C" void MagicMirror_DrawGI(PlayState* play) {
    received = play; ++draws; matrixScale *= /* NATIVE_SCALE */;
}
int Describe(/* TYPE */ /* VAR */, CwItemDrawInfo* out) { /* BRANCH */ return 0; }
bool WouldUseAnimatedFallback(const CwItemDrawInfo& raw) {
    int rcStatic = 1;
    /* GATE */ { return true; }
    return false;
}
int main() {
    static_assert(CW_DRAW_KIND_MAGIC_MIRROR == 42 && CW_DRAW_KIND_SILVER_RUPEE == 41);
    CwItemDrawInfo raw;
    std::memset(&raw, 0xA5, sizeof(raw));
    REQUIRE(Describe(/* IDENTITY */, &raw) == 1);
    REQUIRE(raw.drawKind == CW_DRAW_KIND_MAGIC_MIRROR && raw.dlistCount == 0 && raw.opCount == 0);
    REQUIRE(raw.scale == 0 && raw.itemShimmer == 0 && raw.neiShimmer == 0);
    REQUIRE(std::strcmp(raw.resolvedName, "Magic Mirror") == 0);
    REQUIRE(CwMinDlistsForKind(raw.drawKind) == 0 && !WouldUseAnimatedFallback(raw));
    REQUIRE(CwMinDlistsForKind(CW_DRAW_KIND_SILVER_RUPEE) == 1);
    REQUIRE(CwDescribeMagicMirror(nullptr) == 0);
    PlayState play{};
    REQUIRE(ComboDrawMagicMirrorForeign(&play, raw.drawKind));
    REQUIRE(draws == 1 && received == &play && matrixScale == /* NATIVE_SCALE */);
    REQUIRE(!ComboDrawMagicMirrorForeign(&play, CW_DRAW_KIND_SIMPLE) && draws == 1);
    puts("PASS native-header portable Mirror descriptor/resolver/dispatch: authored host draw, one scale, no asset fallback");
}
'''.replace('/* ENUMS */', '#include "soh/Enhancements/randomizer/randomizerEnums.h"' if host == 'soh' else '#include "2s2h/Rando/Types.h"').replace('/* NATIVE_SCALE */', '2.25f' if host=='soh' else '1.0f').replace('/* TYPE */','RandomizerGet' if host=='soh' else 'RandoItemId').replace('/* VAR */',variable).replace('/* IDENTITY */',item).replace('/* BRANCH */',branch).replace('/* GATE */',gate)
    with tempfile.TemporaryDirectory(prefix='mirror-foreign-' + host) as td:
        temp=Path(td); (temp/'test.cpp').write_text(fixture)
        flags=['-std=c++20','-w','-DF3DEX_GBI_2','-DCOMBO_BUILD']
        for p in ['',host,host+'/include',host+'/include/PR',host+'/src',host+'/assets','combo','combo/menu',host+('/soh' if host=='soh' else '/2s2h'),'libultraship/include']:
            flags += ['-I'+str(ROOT/p)]
        result=subprocess.run([os.environ.get('CXX','c++'),*flags,str(temp/'test.cpp'),'-o',str(temp/'test')],capture_output=True,text=True)
        if result.returncode: raise RuntimeError(host+'\n'+result.stdout+result.stderr)
        subprocess.run([str(temp/'test')],check=True)
