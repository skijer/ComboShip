#!/usr/bin/env python3
"""Run the OoT Mirror text through the actual formatter and native close policy.

Drawing/audio are boundary doubles. This catches a receipt that reaches the
native persistent/event policy and therefore never releases the get-item action.
No executable or configured game runtime is exercised.
"""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def block(source, marker):
    start = source.index(marker)
    end = source.index("{", start) + 1
    depth = 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]


def case(source, marker, following):
    start = source.index(marker)
    return source[start:source.index(following, start)]


def main():
    formatter = (ROOT / "soh/soh/Enhancements/custom-message/CustomMessageManager.cpp").read_text()
    adapter = (ROOT / "soh/soh/Enhancements/Gameplay/MagicMirror.cpp").read_text()
    native = (ROOT / "soh/src/code/z_message_PAL.c").read_text()
    # Include exact width data and formatting logic. These messages have no
    # color-name, icon-name, or non-ASCII substitutions; assert that boundary.
    widths = formatter[formatter.index("static std::map<std::string, int> pixelWidthTable"):
                       formatter.index("CustomMessage::CustomMessage(")]
    methods = "\n".join(block(formatter, marker) for marker in (
        "static size_t NextLineLength(", "size_t CustomMessage::FindNEWLINE(",
        "bool CustomMessage::AddBreakString(", "void CustomMessage::AutoFormatString(",
        "std::string CustomMessage::MESSAGE_END()", "std::string CustomMessage::NEWLINE()",
        "std::string CustomMessage::WAIT_FOR_INPUT()", "std::string CustomMessage::PLAYER_NAME()"))
    draw = "\n".join(case(native, "            case " + name + ":", "            case " + following + ":")
                     for name, following in (("MESSAGE_BOX_BREAK", "MESSAGE_SHIFT"),
                                              ("MESSAGE_END", "MESSAGE_OCARINA"),
                                              ("MESSAGE_PERSISTENT", "MESSAGE_EVENT")))
    # This is the real native input policy, including the persistent refusal.
    update = case(native, "        case MSGMODE_TEXT_DONE:\n            if (Message_HandleCapeVisibilityChoice(play))",
                  "        case MSGMODE_TEXT_CLOSING:")
    update = re.sub(r"Audio_PlaySfxGeneral\([^;]*;", "TestAudio();", update)
    draw = re.sub(r"Audio_PlaySfxGeneral\([^;]*;", "TestAudio();", draw)
    player_source = (ROOT / "soh/src/overlays/actors/ovl_player_actor/z_player.c").read_text()
    player_source = block(player_source, "s32 func_8084DFF4(")
    closing = block(player_source, "if (Message_GetState(&play->msgCtx) == TEXT_STATE_CLOSING)")
    completed = block(player_source, "if (this->getItemId == GI_NONE && this->getItemEntry.objectId == OBJECT_INVALID)")
    source = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <map>
#include <string>
using namespace std::literals::string_literals;
using u16 = uint16_t; using u8 = uint8_t;
#define SPDLOG_DEBUG(...) ((void)0)
#include "soh/include/message_data_fmt.h"
#undef MESSAGE_END
#undef NEWLINE
#undef PLAYER_NAME
#undef WAIT_FOR_INPUT
struct MessageContext { int msgMode=0, textboxEndType=0, stateTimer=0, msgLength=0,
    choiceIndex=0, ocarinaMode=0, textDrawPos=0, msgBufPos=0; int msgBufDecodedWide[256]{}; bool textUnskippable=false; };
struct PlayState { int transitionTrigger=0,transitionType=0,nextEntranceIndex=0; MessageContext msgCtx; struct {int state=0;} csCtx; } play;
PlayState* gPlayState=&play;
enum { MSGMODE_NONE, MSGMODE_TEXT_DISPLAYING, MSGMODE_TEXT_DONE, MSGMODE_TEXT_AWAIT_NEXT,
    MSGMODE_TEXT_NEXT_MSG, MSGMODE_TEXT_CLOSING, MSGMODE_OCARINA_AWAIT_INPUT,
    MSGMODE_SONG_DEMONSTRATION_DONE };
enum { TEXTBOX_ENDTYPE_DEFAULT, TEXTBOX_ENDTYPE_FADING, TEXTBOX_ENDTYPE_PERSISTENT,
    TEXTBOX_ENDTYPE_EVENT, TEXTBOX_ENDTYPE_2_CHOICE, TEXTBOX_ENDTYPE_3_CHOICE, TEXTBOX_ENDTYPE_HAS_NEXT };
enum { TEXTBOX_ICON_TRIANGLE, TEXTBOX_ICON_SQUARE, DO_ACTION_RETURN, OCARINA_MODE_01,
    OCARINA_MODE_02, OCARINA_MODE_03, OCARINA_MODE_04 };
enum { TEXT_STATE_NONE,TEXT_STATE_DONE_HAS_NEXT,TEXT_STATE_CHOICE,TEXT_STATE_EVENT,
    TEXT_STATE_DONE_FADING,TEXT_STATE_DONE,TEXT_STATE_AWAITING_NEXT,TEXT_STATE_SONG_DEMO_DONE,
    TEXT_STATE_8,TEXT_STATE_9,TEXT_STATE_CLOSING };
namespace MirrorRuntime { constexpr u16 kReceiptText=0x3FEB, kPromptText=0x3FEC; }
static std::string loaded;
static int pages, returns;
struct CustomMessage {
    std::string value;
    explicit CustomMessage(const char* body):value(body){}
    void AutoFormat(){ AutoFormatString(value); }
    void LoadIntoFont(){loaded=value;}
    void ReplaceAltarIcons(std::string& str) const {assert(str.find('$')==str.npos);}
    void ReplaceColors(std::string& str) const {assert(str.find('%')==str.npos);}
    void ReplaceSpecialCharacters(std::string& str) const {
        for(unsigned char c:str) assert(c<128);
    }
    static std::string MESSAGE_END(); static std::string NEWLINE();
    static std::string WAIT_FOR_INPUT(); static std::string PLAYER_NAME();
    size_t FindNEWLINE(std::string&,size_t) const;
    bool AddBreakString(std::string&,size_t,std::string) const;
    void AutoFormatString(std::string&) const;
};
/* FORMATTER */
/* OPEN */
using Gfx=int;
bool sTextboxSkipped=false;
void TestAudio(){}
void Font_LoadMessageBoxIcon(void*,int){}
void Interface_SetDoAction(PlayState*,int){++returns;}
bool Message_HandleCapeVisibilityChoice(PlayState*){return false;}
bool Message_ShouldAdvance(PlayState*){return true;}
bool Message_ShouldAdvanceSilent(PlayState*){return true;}
void Message_ContinueTextbox(PlayState*,int){assert(false);}
void Message_CloseTextbox(PlayState* state){state->msgCtx.msgMode=MSGMODE_TEXT_CLOSING;state->msgCtx.stateTimer=2;}
#define YREG(n) 0
#define LANGUAGE_JPN 0
bool sTextIsCredits=false,sDisplayNextMessageAsEnglish=false;
int sNextTextId=0;
struct {int language=1;int eventInf[4]{};int nextCutsceneIndex=0;
    struct {int pendingIceTrapCount=0;} ship;} gSaveContext;
constexpr int GI_NONE=0, GI_MAX=0x7E, GI_GAUNTLETS_SILVER=0x35, OBJECT_INVALID=0;
constexpr int MOD_NONE=0, MOD_RANDOMIZER=1, RG_ICE_TRAP=99, ITEM_NONE=255;
#include "soh/soh/Enhancements/item-tables/ItemTableTypes.h"
struct Player {int getItemId=GI_MAX,unk_862=1;unsigned stateFlags1=0;
    GetItemEntry getItemEntry=GET_ITEM_NONE;};
constexpr int VB_PLAY_NABOORU_CAPTURED_CS=0,ENTR_DESERT_COLOSSUS_EAST_EXIT=0;
constexpr int TRANS_TRIGGER_START=1,TRANS_TYPE_SANDSTORM_END=0,PLAYER_STATE1_IN_CUTSCENE=1;
#define IS_RANDO false
bool GameInteractor_Should(int,bool){return false;}
void Player_TryCsAction(PlayState*,void*,int){}
void Player_SetPendingFlag(Player*,PlayState*){}
#define osSyncPrintf(...) ((void)0)
void DrawControl(unsigned char character) {
    auto* play=gPlayState; auto* msgCtx=&play->msgCtx; auto* font=(void*)nullptr;
    Gfx unused=0; Gfx* gfx=&unused; Gfx** gfxP=&gfx;
    switch(character) { /* DRAW */ default: break; }
}
void NativeAdvance() {auto* msgCtx=&play.msgCtx; auto* play=gPlayState;
    switch(msgCtx->msgMode) { /* UPDATE */ default: break; }
}
/* GETSTATE */
void NativeReceiveClosing(Player* current,PlayState* play) { /* CLOSE_RECEIPT */ }
bool NativeReceiptFinished(Player* current) { /* COMPLETED */ return false; }
void RunReceipt() {
    play={}; loaded.clear(); pages=returns=0;
    u16 text=0x3FEB;bool fromTable=true;OpenMirrorText(&text,&fromTable);
    assert(!fromTable&&!loaded.empty());
    play.msgCtx.msgMode=MSGMODE_TEXT_DISPLAYING;
    for(unsigned char c:loaded) {
        DrawControl(c);
        if(play.msgCtx.msgMode==MSGMODE_TEXT_AWAIT_NEXT) {
            ++pages;play.msgCtx.msgMode=MSGMODE_TEXT_DISPLAYING;
        }
        if(play.msgCtx.msgMode==MSGMODE_TEXT_DONE) break;
    }
    NativeAdvance();
    if(play.msgCtx.msgMode!=MSGMODE_TEXT_CLOSING) {
        fprintf(stderr,"FAIL Mirror receipt refuses A: native mode=%d endType=%d (persistent=%d)\n",
                play.msgCtx.msgMode,play.msgCtx.textboxEndType,TEXTBOX_ENDTYPE_PERSISTENT);
        std::abort();
    }
    assert(pages>=1&&returns==1);
    Player current;
    current.getItemEntry.objectId=1;
    current.getItemEntry.getItemId=GI_MAX;
    current.getItemEntry.drawFunc=+[](PlayState*,GetItemEntry*){};
    play.msgCtx.msgLength=loaded.size();
    NativeReceiveClosing(&current,&play); // Closing timer=2 is still native-owned.
    assert(!NativeReceiptFinished(&current));
    play.msgCtx.stateTimer=1;
    NativeReceiveClosing(&current,&play);
    assert(NativeReceiptFinished(&current)&&current.unk_862==0);
    assert(current.getItemEntry.itemId==ITEM_NONE);
    // The prompt follows the same loader but must retain its choice byte.
    text=0x3FEC;fromTable=true;OpenMirrorText(&text,&fromTable);
    assert(!fromTable&&loaded.find('\x1B')!=loaded.npos&&loaded.back()=='\x02');
    puts("PASS OoT Mirror receipt: native pages reach ordinary END and A closes; prompt retains choice");
}
int main(){RunReceipt();}
'''
    # The header macro is needed by the real native switch; the class method
    # sharing its name has already been parsed before this point.
    source = source.replace("/* FORMATTER */", widths + "\n" + methods)
    source = source.replace("/* OPEN */", block(adapter, "void OpenMirrorText("))
    source = source.replace("/* DRAW */", draw.replace("MESSAGE_END:", "0x02:"))
    source = source.replace("/* UPDATE */", update)
    source = source.replace("/* GETSTATE */", block(native, "u8 Message_GetState("))
    source = source.replace("/* CLOSE_RECEIPT */", re.sub(r"\bthis\b", "current", closing))
    source = source.replace("/* COMPLETED */", re.sub(r"\bthis\b", "current", completed))
    with tempfile.TemporaryDirectory(prefix="mirror-receipt-") as folder:
        path = Path(folder)
        (path / "test.cpp").write_text(source)
        subprocess.run([os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
                        "-fsanitize=address,undefined", "-fno-sanitize-recover=all", "-no-pie",
                        "-I" + str(ROOT), str(path / "test.cpp"), "-o", str(path / "test")], check=True)
        subprocess.run([str(path / "test")], check=True,
                       env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})


if __name__ == "__main__":
    main()
