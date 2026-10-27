#!/usr/bin/env python3
"""Require native 2Ship GI preference even with authored resources installed."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r'^(?:static\s+)?[^\n;{}]+\b' + re.escape(name) + r'\([^;{}]*\)\s*\{', source, re.M)
    assert match, name
    end, depth = match.end(), 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[match.start():end]


def main():
    oot = (ROOT/'soh/soh/Enhancements/randomizer/NeiGiPresentation.cpp').read_text()
    mm = (ROOT/'mm/2s2h/Rando/NeiGiPresentation.cpp').read_text()
    types = (ROOT/'mm/2s2h/Rando/Types.h').read_text()
    enum = re.search(r'typedef enum \{\s*RI_UNKNOWN,.*?\} RandoItemId;', types, re.S)[0]
    code = r'''
#include <cassert>
#include <cstring>
#include <cstdio>
struct Presentation { void (*draw)(); const char* opaque; };
void Randomizer_DrawProgressiveMasterSword() {}
void Randomizer_DrawExtShieldOfIkana() {}
static bool nativeAvailable, anyMod;
int ResourceMgr_IsIkanaShieldGiAvailable() { return nativeAvailable; }
int ResourceMgr_IsModAsset(const char*) { return anyMod; }
int ResourceMgr_IsModAssetForGame(const char*, const char*) { return anyMod; }
''' + enum + '\n' + function(oot,'HasLegacyGiMod') + '\n' + function(mm,'HasMmLegacyGiMod') + r'''
int main() {
    Presentation shield{Randomizer_DrawExtShieldOfIkana,
                        "__OTR__objects/nei_gi_redesign/shield_of_ikana/gi_dl"};
    nativeAvailable = true;
    anyMod = false;
    assert(HasLegacyGiMod(shield) && "OoT must decline the authored GI when native 2Ship GI exists");
    assert(HasLegacyGiMod(shield, true) && "foreign OoT presentation must prefer native 2Ship GI");
    assert(HasMmLegacyGiMod(RI_SHIELD_MIRROR) && "MM must decline authored GI even without a native mod");
    nativeAvailable = false;
    anyMod = false;
    assert(!HasLegacyGiMod(shield) && !HasMmLegacyGiMod(RI_SHIELD_MIRROR) &&
           "missing native GI must retain the authored fallback");
    anyMod = true;
    assert(!HasLegacyGiMod(shield) && !HasMmLegacyGiMod(RI_SHIELD_MIRROR) &&
           "unrelated worn/model mods must not suppress the last-resort authored fallback");
    Presentation other{nullptr, "__OTR__objects/nei_gi_redesign/trident/gi_dl"};
    assert(HasLegacyGiMod(other) && "other GI mod-priority rules must stay intact");
    anyMod = false;
    assert(!HasLegacyGiMod(other));
    std::puts("PASS: native 2Ship GI beats authored in both games and foreign selector; fallback and unrelated priorities retained");
}
'''
    with tempfile.TemporaryDirectory(prefix='ikana-native-priority-') as temporary:
        path=Path(temporary)
        (path/'test.cpp').write_text(code)
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++20','-Wall','-Wextra','-Werror',
                        str(path/'test.cpp'),'-o',str(path/'test')],check=True)
        subprocess.run([str(path/'test')],check=True)


if __name__ == '__main__':
    main()
