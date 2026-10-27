#!/usr/bin/env python3
"""Trace market button text through OoT's actual formatter and native draw packets.

Font resource upload and cosmetic preferences are boundary doubles. The real
palette, cosmetic selection and I4 glyph drawing code emit the tested packet.
This does not boot either game or prove configured visual runtime behavior.
"""
import os
from pathlib import Path
import subprocess
import tempfile
from run_text_codec_tests import function, common_flags

ROOT = Path(__file__).resolve().parents[2]
formatter = (ROOT / 'soh/soh/Enhancements/custom-message/CustomMessageManager.cpp').read_text()
native = (ROOT / 'soh/src/code/z_message_PAL.c').read_text()
signatures = (
    'CustomMessage::CustomMessage(std::string english_, TextBoxType',
    'static size_t NextLineLength(', 'size_t CustomMessage::FindNEWLINE(',
    'bool CustomMessage::AddBreakString(', 'void CustomMessage::AutoFormatString(',
    'void CustomMessage::ReplaceSpecialCharacters(', 'void CustomMessage::EncodeColors(',
    'void CustomMessage::ReplaceColors(', 'void CustomMessage::ReplaceAltarIcons(',
    'std::string CustomMessage::MESSAGE_END(', 'std::string CustomMessage::ITEM_OBTAINED(',
    'std::string CustomMessage::NEWLINE(', 'std::string CustomMessage::COLOR(',
    'std::string CustomMessage::WAIT_FOR_INPUT(', 'std::string CustomMessage::PLAYER_NAME(',
)
cpp = r'''
#include "soh/Enhancements/custom-message/CustomMessageManager.h"
#include "combo/BlackMarketText.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <map>
#define SPDLOG_DEBUG(...) ((void)0)
extern "C" uint32_t MarketGlyph_PrimColor(unsigned color, int customized);
'''
cpp += formatter[formatter.index('using namespace std::literals'):formatter.index('CustomMessage::CustomMessage(')]
cpp += '\n' + '\n'.join(function(formatter, signature) for signature in signatures)
cpp += (ROOT / 'tests/black_market/oot_button_glyph_test.inc').read_text()
c = r'''
#include "global.h"
#include "message_data_static.h"
#include "soh/Enhancements/cosmetics/CosmeticsEditor.h"
#include <assert.h>
#include <string.h>
static GameInfo registers;
GameInfo* gGameInfo = &registers;
static int customized;
static s32 sCharTexSize, sCharTexScale;
int32_t CVarGetInteger(const char* key, int32_t fallback) {
    return customized && (strcmp(key, CVAR_COSMETIC("Message.Blue.Normal.Changed")) == 0 ||
                          strcmp(key, CVAR_COSMETIC("Message.Adjustable.Normal.Changed")) == 0) ? 1 : fallback;
}
Color_RGBA8 CosmeticsEditor_GetDefaultValue(const char* id) { (void)id; return (Color_RGBA8){0}; }
Color_RGBA8 CVarGetColor(const char* key, Color_RGBA8 fallback) {
    if (strcmp(key, CVAR_COSMETIC("Message.Blue.Normal.Value")) == 0)
        return (Color_RGBA8){17, 34, 51, 255};
    if (strcmp(key, CVAR_COSMETIC("Message.Adjustable.Normal.Value")) == 0)
        return (Color_RGBA8){68, 85, 102, 255};
    return fallback;
}
'''
c += native[native.index('#define MESSAGE_COSMETICS_HANDLE_COLOR'):native.index('#undef MESSAGE_COSMETICS_HANDLE_COLOR')]
c += '\n' + function(native, 'void Message_SetTextColor(')
c += '\n' + function(native, 'void Message_DrawTextChar(')
c += r'''
uint32_t MarketGlyph_PrimColor(unsigned color, int custom) {
    static PlayState play;
    static unsigned char glyph[FONT_CHAR_TEX_SIZE];
    Gfx packets[32], *end = packets;
    customized = custom;
    R_TEXT_CHAR_SCALE = 75;
    R_TEXT_ADJUST_COLOR_2_R = 70;
    R_TEXT_ADJUST_COLOR_2_G = 255;
    R_TEXT_ADJUST_COLOR_2_B = 80;
    play.msgCtx.textBoxType = TEXTBOX_TYPE_BLACK;
    play.msgCtx.textColorAlpha = 255;
    Message_SetTextColor(&play.msgCtx, color);
    Message_DrawTextChar(&play, glyph, &end);
    uint32_t rgba = 0;
    for (Gfx* p = packets; p < end; ++p)
        if ((p->words.w0 >> 24) == G_SETPRIMCOLOR)
            rgba = p->words.w1;
    assert(rgba && "native glyph draw must emit a primitive color packet");
    return rgba >> 8;
}
'''
includes = [ROOT, ROOT / 'combo', ROOT / 'combo/menu', ROOT / 'soh', ROOT / 'soh/include',
            ROOT / 'soh/include/PR', ROOT / 'soh/soh', ROOT / 'libultraship/include']
flags = common_flags(includes) + ['-DCVAR_PREFIX_COSMETIC="gCosmetics"', '-DCVAR_PREFIX_ENHANCEMENT="gEnhancements"']
with tempfile.TemporaryDirectory(prefix='market-button-glyphs-') as folder:
    folder = Path(folder)
    cpath, cpppath = folder / 'native.c', folder / 'test.cpp'
    cpath.write_text(c)
    cpppath.write_text(cpp)
    obj, binary = folder / 'native.o', folder / 'test'
    subprocess.run([os.environ.get('CC', 'cc'), '-std=gnu2x', *flags, '-c', str(cpath), '-o', str(obj)], check=True)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', *flags, str(cpppath), str(obj),
                    '-no-pie', '-Wl,--gc-sections', '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True, env={**os.environ, 'ASAN_OPTIONS': 'detect_leaks=0'})
