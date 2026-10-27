"""Execute OoT's native message icon loader at the staged texture boundary."""
from pathlib import Path
import os
import subprocess
import tempfile
from run_time_pedestal_tests import functions

ROOT = Path(__file__).resolve().parents[2]
loader = functions((ROOT / 'soh/src/code/z_message_PAL.c').read_text())['Message_LoadItemIcon']
source = r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include "soh/include/z64item.h"
#include "combo/menu/ComboItemReceiptPresentation.h"
using s16 = int16_t; using u16 = uint16_t; using u8 = uint8_t;
struct MessageContext { char textboxSegment[512]{}; int msgBufPos = 0, choiceNum = 0; };
struct InterfaceContext { u8 mapPalette[32]{}; };
struct PlayState { MessageContext msgCtx; InterfaceContext interfaceCtx; } play;
constexpr int LANGUAGE_ENG = 0, MESSAGE_STATIC_TEX_SIZE = 0;
static bool sDisplayNextMessageAsEnglish = false, hasReceiptIcon = false;
static struct { int language = 0; } gSaveContext;
static const char* gItemIcons[160]{};
static CwItemReceiptPresentation sItemReceiptPresentation{};
static int R_TEXTBOX_ICON_XPOS, R_TEXTBOX_ICON_YPOS, R_TEXTBOX_ICON_SIZE, R_TEXT_INIT_XPOS = 100;
static int Message_HasItemReceiptIcon() { return hasReceiptIcon; }
static void* ExtInv_GetItemIcon(u16) { return nullptr; }
#define lusprintf(...) ((void)0)
#define osSyncPrintf(...) ((void)0)
#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))
static void StageCopy(uintptr_t dst, const void* src, size_t size) { std::memcpy(reinterpret_cast<void*>(dst), src, size); }
#define memcpy(dst, src, size) StageCopy(uintptr_t(dst), src, size)
''' + loader + r'''
int main() {
    const char* full = "__OTR__textures/icon_item_24_static/gQuestIconHeartPieceTex";
    const char* hud = "__OTR__textures/icon_item_static/gHeartPieceIcon1Tex";
    const char* bow = "__OTR__textures/icon_item_static/gItemIconBowTex";
    gItemIcons[ITEM_HEART_PIECE] = full;
    gItemIcons[ITEM_HEART_PIECE_2] = hud;
    gItemIcons[ITEM_BOW] = bow;
    Message_LoadItemIcon(&play, ITEM_HEART_PIECE_2, 20);
    assert(!std::strcmp(play.msgCtx.textboxSegment, full) &&
           "Native Piece of Heart receipts must select the full inventory texture rather than the HUD progress wedge");
    assert(R_TEXTBOX_ICON_SIZE == 24 && play.msgCtx.msgBufPos == 1 && play.msgCtx.choiceNum == 1);
    assert(gItemIcons[ITEM_HEART_PIECE_2] == hud && "The HUD's heart progress texture must keep its native slot");
    Message_LoadItemIcon(&play, ITEM_BOW, 20);
    assert(!std::strcmp(play.msgCtx.textboxSegment, bow) && R_TEXTBOX_ICON_SIZE == 32);
    hasReceiptIcon = true;
    std::strcpy(sItemReceiptPresentation.iconPath, "__OTR__@oot:textures/icon_item_24_static/gQuestIconHeartPieceTex");
    Message_LoadItemIcon(&play, ITEM_HEART_PIECE_2, 20);
    assert(!std::strcmp(play.msgCtx.textboxSegment, sItemReceiptPresentation.iconPath));
}
'''
with tempfile.TemporaryDirectory(prefix='native-heart-receipt-') as tmp:
    path, binary = Path(tmp) / 'test.cpp', Path(tmp) / 'test'
    path.write_text(source)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++20', '-Wall', '-Wextra', '-Werror',
                    '-I' + str(ROOT), str(path), '-o', str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print('PASS native full Heart Piece texture, unchanged HUD progress slot, other item icon and receipt override')
