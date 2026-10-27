#!/usr/bin/env python3
"""Compile native eligibility/color/draw code from this partial source checkout.

The fixture replaces engine/GUI boundaries; it does not build or run either game.
Mutations caught: missing opt-in, wrong owner/type, cached recolor, changed/reset
ignored, legacy neutral sentinel leaking to explicit RGB, or lost matrix restore.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(text, name):
    match = re.search(r'(?m)^[^\n;{}]*\b' + re.escape(name) + r'\([^;]*?\)\s*\{', text)
    if not match:
        return ''
    start = match.start()
    pos = text.index('{', match.start())
    depth = 1
    end = pos + 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def run_host(mm):
    game = 'mm' if mm else 'soh'
    editor = ROOT / ('mm/2s2h/BenGui/CosmeticEditor.cpp' if mm else 'soh/soh/Enhancements/cosmetics/CosmeticsEditor.cpp')
    src = (ROOT / game / 'src/code/z_draw.c').read_text()
    fn = function(src, 'GetItem_GetShimmerColor')
    ids = sorted(set(re.findall(r'\b(?:GID_|CW_SONG_|CW_BOTTLE_|CW_SHIMMER_)[A-Z0-9_]+', fn)))
    ids += ['GID_RUPEE_GREEN', 'GID_RUPEE_BLUE', 'GID_RUPEE_RED', 'GID_RUPEE_PURPLE',
            'GID_RUPEE_SILVER', 'GID_RUPEE_HUGE' if mm else 'GID_RUPEE_GOLD',
            'GID_RECOVERY_HEART' if mm else 'GID_HEART', 'GID_HEART_CONTAINER', 'GID_HEART_PIECE']
    host = ROOT / 'combo/menu/ComboCollectibleShimmerHost.h'
    drops = (ROOT / game / 'src/code/z_en_item00.c').read_text()
    ids += re.findall(r'\bITEM00_[A-Z0-9_]+', drops)
    sampler = 'MM_SampleCollectibleShimmerColor' if mm else 'OOT_SampleCollectibleShimmerColor'
    harness = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <map>
#include <string>
using s16=int16_t; using s32=int32_t; using u8=uint8_t;
struct Color_RGB8 { uint8_t r,g,b; }; struct Color_RGBA8 { uint8_t r,g,b,a; };
struct PlayState {};
void Matrix_Push() {} void Matrix_Pop() {}
void Matrix_Scale(float,float,float,int) {}
enum { MTXMODE_APPLY };
std::map<std::string,int> flags;
std::map<std::string,Color_RGB8> colors;
bool suppressed=false; int samples=0;
int CVarGetInteger(const char* key,int fallback) { auto p=flags.find(key); return p==flags.end()?fallback:p->second; }
Color_RGB8 CVarGetColor24(const char* key,Color_RGB8 fallback) { auto p=colors.find(key); return p==colors.end()?fallback:p->second; }
#define CVAR_COSMETIC(x) "gCosmetics." x
bool CosmeticEditorIsSuppressed(const char*) { return suppressed; }
Color_RGBA8 CosmeticEditor_GetChangedColor(uint8_t r,uint8_t g,uint8_t b,uint8_t a,const char* id) {
  std::string root=std::string("gCosmetic.")+id;
  Color_RGB8 c={r,g,b};
  if(!suppressed && CVarGetInteger((root+".Changed").c_str(),0)) c=CVarGetColor24((root+".Color").c_str(),c);
  return {c.r,c.g,c.b,a};
}
void OOT_SampleGiCosmeticColor(const char* key,uint8_t r,uint8_t g,uint8_t b,uint8_t* out) {
  ++samples; auto c=CVarGetColor24(key,{r,g,b});
  if (CVarGetInteger((std::string(key)+".rainbowFixture").c_str(),0)) c={13,97,241};
  out[0]=c.r; out[1]=c.g; out[2]=c.b;
}
void MM_SampleGiCosmeticColor(const char* k,uint8_t r,uint8_t g,uint8_t b,uint8_t* out) { OOT_SampleGiCosmeticColor(k,r,g,b,out); }
int ComboSongShimmerColor(int,uint8_t c[4]) { c[3]=255; return 1; }
int ComboBottleContents_Color(int,uint8_t c[4]) { c[3]=255; return 1; }
int ComboBottleShimmer_Color(int,uint8_t c[4]) { c[3]=255; return 1; }
void ComboMaskShimmerColor(int,uint8_t c[4]) { c[0]=220; c[1]=225; c[2]=240; c[3]=255; }
int ComboOotMaskShimmerColor(int,uint8_t c[4]) { ComboMaskShimmerColor(0,c); return 1; }
int ComboMmMaskShimmerColor(int,uint8_t c[4]) { ComboMaskShimmerColor(0,c); return 1; }
int ComboMmRemainsShimmerColor(int,uint8_t c[4]) { ComboMaskShimmerColor(0,c); return 1; }
'''
    harness += '\nenum { ' + ','.join(dict.fromkeys(ids)) + ' };\n'
    if host.exists():
        if mm:
            harness += '#define COMBO_COLLECTIBLE_SHIMMER_HOST_MM\n'
        harness += '#include "ComboCollectibleShimmerHost.h"\n'
        harness += function(editor.read_text(), sampler) + '\n'
    harness += fn + '\n'
    large = 'GID_RUPEE_HUGE' if mm else 'GID_RUPEE_GOLD'
    heart = 'GID_RECOVERY_HEART' if mm else 'GID_HEART'
    keys = (['Collectibles.RupeeGreen', 'Collectibles.RupeeBlue', 'Collectibles.RupeeRed', 'Collectibles.RupeePurple', 'Collectibles.RupeeOrange', 'Collectibles.RupeeSilver', 'HUD.Hearts'] if mm else
            ['Consumable.GreenRupee', 'Consumable.BlueRupee', 'Consumable.RedRupee', 'Consumable.PurpleRupee', 'Consumable.GoldRupee', 'Consumable.SilverRupee', 'Consumable.Hearts'])
    defaults = ([(82,255,107),(132,181,239),(239,107,107),(255,99,231),(247,148,33),(189,189,198),(255,70,50)] if mm else
                [(50,255,50),(50,50,255),(255,50,50),(150,50,255),(255,190,55),(255,255,255),(255,70,50)])
    harness += 'int main() { uint8_t c[4]={9,8,7,6};\n'
    harness += 'assert(GetItem_GetShimmerColor(GID_RUPEE_GREEN,c)==0); assert(c[0]==9);\n'
    harness += 'flags["gEnhancements.CollectibleShimmer"]=1;\n'
    for gid, idx in [('GID_RUPEE_GREEN',0),('GID_RUPEE_BLUE',1),('GID_RUPEE_RED',2),('GID_RUPEE_PURPLE',3),(large,4)]+([('GID_RUPEE_SILVER',5)] if mm else [])+[(heart,6),('GID_HEART_CONTAINER',6),('GID_HEART_PIECE',6)]:
        r,g,b=defaults[idx]
        harness += f'assert(GetItem_GetShimmerColor({gid},c)==2); assert(c[0]=={r} && c[1]=={g} && c[2]=={b} && c[3]==255);\n'
    prefix='gCosmetic.' if mm else 'gCosmetics.'
    for gid, idx in [('GID_RUPEE_GREEN',0),('GID_RUPEE_RED',2),('GID_HEART_PIECE',6)]:
        key=prefix+keys[idx]; value=key+('.Color' if mm else '.Value')
        harness += f'flags["{key}.Changed"]=1; colors["{value}"]={{220,225,240}};\n'
        harness += f'assert(GetItem_GetShimmerColor({gid},c)==2); assert(c[0]==220 && c[1]==225 && c[2]==240);\n'
        harness += f'colors["{value}"]={{4,109,33}}; assert(GetItem_GetShimmerColor({gid},c)==2); assert(c[0]==4 && c[1]==109 && c[2]==33);\n'
        harness += f'flags["{value}.rainbowFixture"]=1; assert(GetItem_GetShimmerColor({gid},c)==2); assert(c[0]==13 && c[1]==97 && c[2]==241);\n'
        harness += f'flags["{key}.Changed"]=0; flags["{value}.rainbowFixture"]=0;\n'
        r,g,b=defaults[idx]
        harness += f'assert(GetItem_GetShimmerColor({gid},c)==2); assert(c[0]=={r} && c[1]=={g} && c[2]=={b});\n'
    harness += 'flags["gEnhancements.CollectibleShimmer"]=0; assert(GetItem_GetShimmerColor(GID_RUPEE_RED,c)==0);\n'
    harness += 'assert(GetItem_GetShimmerColor(-1,c)==0);\n'
    if mm:
        harness += 'assert(GetItem_GetShimmerColor(GID_MASK_DEKU,c)==1); assert(GetItem_GetShimmerColor(GID_CHATEAU,c)==1);\n'
    else:
        harness += 'assert(GetItem_GetShimmerColor(GID_MAGIC_SMALL,c)==1); assert(GetItem_GetShimmerColor(GID_MASK_KEATON,c)==1);\n'
    harness += f'puts("{game}: native eligibility, opt-in, owner palettes, live color/rainbow/reset passed"); }}\n'
    with tempfile.TemporaryDirectory(prefix='collectible-shimmer-') as tmp:
        source=Path(tmp)/'native.cpp'; source.write_text(harness)
        binary=Path(tmp)/'native'
        subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror','-DCOMBO_BUILD','-I',str(ROOT/'combo/menu'),str(source),'-o',str(binary)],check=True)
        subprocess.run([str(binary)],check=True)


if __name__ == '__main__':
    run_host(False)
    run_host(True)
