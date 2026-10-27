#!/usr/bin/env python3
"""Compile the production location presenter with recorded graphics commands.

No game/ROM/pack is required. The native TitleCard_Update body is extracted
from z_actor.c; platform graphics/resource APIs are explicit fixture boundaries.
"""
from pathlib import Path
import json
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def function(source, name):
    start = source.index("void " + name + "(")
    brace = source.index("{", start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[start:end]

GLOBAL = r"""
#pragma once
#include <stdint.h>
#include <stddef.h>
typedef uint8_t u8; typedef uint16_t u16; typedef int16_t s16;
typedef float f32;
#define ALIGN_ASSET(n) __attribute__((aligned(n)))
#define FONT_CHAR_TEX_SIZE 128
#define MSGMODE_NONE 0
#define MSGMODE_SCENE_TITLE_CARD_FADE_IN_BACKGROUND 10
#define MSGMODE_SCENE_TITLE_CARD_FADE_OUT_BACKGROUND 14
#define G_TX_RENDERTILE 0
#define G_TF_BILERP 0
#define G_CC_MODULATEIA_PRIM 0
#define G_IM_FMT_IA 0
#define G_IM_FMT_I 1
#define G_IM_SIZ_8b 0
#define G_TX_NOMIRROR 0
#define G_TX_WRAP 0
#define G_TX_CLAMP 0
#define G_TX_NOMASK 0
#define G_TX_NOLOD 0
typedef struct { int unused; } GameState;
typedef struct { void* texture; s16 x,y,width,height,durationTimer,delayTimer,alpha,intensity; } TitleCardContext;
typedef struct { char schar[200]; } Buffer;
typedef struct { char charBuf[2][15360]; unsigned unk_11D88; } Font;
typedef struct {
  Buffer decodedBuffer; Font font; unsigned decodedTextLen;
  int msgMode, currentTextId, textColorAlpha, textboxColorAlphaCurrent, msgLength, stateTimer;
} MessageContext;
typedef struct { GameState state; int sceneId; MessageContext msgCtx; struct { TitleCardContext titleCtx; } actorCtx; } PlayState;
typedef struct { struct { unsigned language; } options; unsigned untouched[128]; } Save;
extern Save gSaveContext;
typedef struct { int kind,a,b,c,d; const void* texture; } Gfx;
void TitleCard_Update(GameState*, TitleCardContext*);
#define DECR(x) ((x) ? --(x) : 0)
static void Math_StepToS(s16* x, int target, int step) {
    if (*x < target) { *x += step; if (*x > target) *x = target; }
    else { *x -= step; if (*x < target) *x = target; }
}
static void Gfx_SetupDL39_Ptr(Gfx** p) { (void)p; }
#define gDPSetTextureFilter(p,...) ((void)(p))
#define gDPSetCombineMode(p,...) ((void)(p))
#define gDPSetEnvColor(p,...) ((void)(p))
#define gDPPipeSync(p) ((void)(p))
#define gDPSetPrimColor(p,m,l,r,g,b,a) do { Gfx* q=(p); *q=(Gfx){1,r,g,b,a,0}; } while(0)
#define gDPLoadTextureBlock(p,t,...) do { Gfx* q=(p); *q=(Gfx){2,0,0,0,0,t}; } while(0)
#define gDPLoadTextureBlock_4b(p,t,...) do { Gfx* q=(p); *q=(Gfx){3,0,0,0,0,t}; } while(0)
#define gSPTextureRectangle(p,x,y,r,b,...) do { Gfx* q=(p); *q=(Gfx){4,x,y,r,b,0}; } while(0)
enum {
#define DEFINE_SCENE(name,id,title,draw,r,c,e,m,h) id,
#define DEFINE_SCENE_UNSET(id) id,
#include "tables/scene_table.h"
#undef DEFINE_SCENE
#undef DEFINE_SCENE_UNSET
};
"""
BRIDGE = r"""
#pragma once
int ResourceMgr_FileExists(const char*);
int ResourceMgr_IsAltAssetsEnabled(void);
int ResourceMgr_FileAltExists(const char*);
"""
HARNESS = r"""
#include <assert.h>
#include <string.h>
#include <stdio.h>
Save gSaveContext;
f32 sNESFontWidths[160];
static int option, nativeAsset, altAsset, altEnabled, fontAsset, missingGlyph, fontProbes, nativeFontAsset, builtinAsset;
int CVarGetInteger(const char* key, int def) { (void)key; (void)def; return option; }
int OotLocationTitle_HasTexture(const char* p) {
    if (strstr(p,"custom/oot_location_titles/v1/")) return builtinAsset;
    assert(strstr(p,"custom/mm_location_titles/"));
    if (strstr(p,"/oot_native_v1/")) {
        if (strstr(p,"/profile")) return nativeFontAsset;
        unsigned code=0; assert(sscanf(strrchr(p,'/')+1,"%x",&code)==1);
        return nativeFontAsset && code!=(unsigned)missingGlyph;
    }
    if (strstr(p,"/font/")) {
        ++fontProbes;
        unsigned code=0; assert(sscanf(strrchr(p,'/')+1,"%x",&code)==1);
        return fontAsset && code!=(unsigned)missingGlyph;
    }
    return nativeAsset || (altEnabled && altAsset);
}
static PlayState p;
static Gfx commands[4096];
static void init(const char* text) {
    memset(&p, 0, sizeof(p));
    p.sceneId=SCENE_20SICHITAI2;
    p.msgCtx.currentTextId=0x116;
    p.msgCtx.msgMode=MSGMODE_SCENE_TITLE_CARD_FADE_IN_BACKGROUND;
    p.msgCtx.msgLength=88;
    memcpy(p.msgCtx.decodedBuffer.schar,text,strlen(text));
    // Native Message_DecodeNES stores a simple terminator at decodedTextLen,
    // excluding it from the reported glyph/control-stream length.
    p.msgCtx.decodedTextLen=strlen(text);
    p.msgCtx.decodedBuffer.schar[p.msgCtx.decodedTextLen]=MESSAGE_END;
    option=1; nativeAsset=altAsset=altEnabled=fontAsset=fontProbes=nativeFontAsset=builtinAsset=0; missingGlyph=-1;
    memset(&p.actorCtx.titleCtx, 0x42, sizeof(p.actorCtx.titleCtx));
    OotLocationTitle_Reset();
}
static int draw(void) {
    memset(commands,0,sizeof(commands));
    Gfx* out=commands;
    assert(OotLocationTitle_Draw(&p,&out));
    return (int)(out-commands);
}
int main(void) {
    for (int i=0;i<160;++i) sNESFontWidths[i]=8;
    memset(&gSaveContext,0x31,sizeof(gSaveContext));
    gSaveContext.options.language=1;
    Save saved=gSaveContext;
    init("AB");
    TitleCardContext busy=p.actorCtx.titleCtx;
    OotLocationTitle_Begin(&p);
    assert(sOwner==&p && sLines==1 && sWidths[0]==16);
    assert(draw()==0);
    TitleCardContext reference={0};
    reference.delayTimer=20; reference.durationTimer=80;
    int completed=0;
    for (int frame=0;frame<120;++frame) {
        TitleCard_Update(&p.state,&reference);
        int handled=OotLocationTitle_Update(&p);
        if (!completed) {
            assert(handled && p.msgCtx.textColorAlpha==reference.alpha);
            if (!reference.durationTimer && !reference.alpha && !reference.delayTimer) completed=1;
            else if (reference.alpha) {
                int n=draw(), loads=0, rectangles=0, prims=0;
                for(int j=0;j<n;++j) {
                    loads+=commands[j].kind==3; rectangles+=commands[j].kind==4;
                    if(commands[j].kind==1) {
                        ++prims; assert(commands[j].d==reference.alpha);
                    }
                }
                assert(loads==2 && rectangles==10 && prims==4);
                // First glyph's left outline starts one pixel left of centered x=152.
                int first=0; while(commands[first].kind!=4) ++first;
                assert(commands[first].a==151*4 && commands[first].b==112*4);
            }
        } else assert(!handled);
    }
    assert(completed && p.msgCtx.msgMode==MSGMODE_NONE && !p.msgCtx.msgLength && !p.msgCtx.currentTextId);
    assert(!memcmp(&saved,&gSaveContext,sizeof(saved)));
    assert(!memcmp(&busy,&p.actorCtx.titleCtx,sizeof(busy)));
    init("A\x11" "BB"); OotLocationTitle_Begin(&p);
    assert(sLines==2 && sWidths[0]==8 && sWidths[1]==16);
    init("AB"); option=0; OotLocationTitle_Begin(&p); assert(!sOwner);
    init("AB"); p.msgCtx.currentTextId=0x1BB2; OotLocationTitle_Begin(&p); assert(!sOwner);
    init("AB"); p.msgCtx.currentTextId=0x999; OotLocationTitle_Begin(&p); assert(!sOwner);
    init("A\xC2"); OotLocationTitle_Begin(&p); assert(!sOwner);
    // Native-length shapes: the terminal control lives at the excluded index.
    const u8 unsupportedTerminals[] = { MESSAGE_EVENT, MESSAGE_EVENT2, MESSAGE_PERSISTENT,
                                       MESSAGE_CONTINUE, MESSAGE_BOX_BREAK, MESSAGE_BOX_BREAK2 };
    for (unsigned i=0;i<sizeof(unsupportedTerminals);++i) {
        init("AB");
        p.msgCtx.decodedBuffer.schar[p.msgCtx.decodedTextLen]=unsupportedTerminals[i];
        MessageContext original=p.msgCtx;
        OotLocationTitle_Begin(&p);
        assert(!sOwner && !memcmp(&original,&p.msgCtx,sizeof(original)));
    }
    init("AB"); OotLocationTitle_Begin(&p); assert(sOwner==&p); // MESSAGE_END remains accepted.
    init("AB"); p.msgCtx.decodedTextLen=sizeof(p.msgCtx.decodedBuffer.schar);
    OotLocationTitle_Begin(&p); assert(!sOwner); // No readable terminal slot.
    init("AB"); p.msgCtx.decodedTextLen=sizeof(p.msgCtx.decodedBuffer.schar)-1;
    p.msgCtx.decodedBuffer.schar[p.msgCtx.decodedTextLen]=MESSAGE_EVENT;
    OotLocationTitle_Begin(&p); assert(!sOwner);

    init("A\x11" "A\x11" "A\x11" "A"); OotLocationTitle_Begin(&p); assert(!sOwner);
    char wide[50]; memset(wide,'W',49); wide[49]=0;
    init(wide); OotLocationTitle_Begin(&p); assert(!sOwner);
    init("AB"); nativeAsset=1; OotLocationTitle_Begin(&p);
    assert(sUseTexture && !strcmp(sTexturePath,"__OTR__custom/mm_location_titles/1/0116"));
    assert(!((uintptr_t)sTexturePath&1));
    for(int i=0;i<21;++i) OotLocationTitle_Update(&p);
    int n=draw(), cards=0;
    for(int i=0;i<n;++i) cards+=commands[i].kind==2;
    assert(cards==1);
    init("AB"); altAsset=1; OotLocationTitle_Begin(&p); assert(!sUseTexture);
    init("AB"); altAsset=altEnabled=1; OotLocationTitle_Begin(&p); assert(sUseTexture);
    // Interruption by another message, a mode change, scene exit or reset cannot
    // keep drawing the old card or mutate the replacement message.
    init("AB"); OotLocationTitle_Begin(&p); p.msgCtx.currentTextId=0x123;
    MessageContext interrupted=p.msgCtx;
    assert(!OotLocationTitle_Update(&p)); Gfx* out=commands;
    assert(!OotLocationTitle_Draw(&p,&out));
    assert(!memcmp(&interrupted,&p.msgCtx,sizeof(interrupted)));
    init("AB"); OotLocationTitle_Begin(&p); p.msgCtx.msgMode=MSGMODE_NONE;
    assert(!OotLocationTitle_Update(&p));
    init("AB"); OotLocationTitle_Begin(&p); ++p.sceneId; assert(!OotLocationTitle_Update(&p));
    init("AB"); OotLocationTitle_Begin(&p); OotLocationTitle_Reset(); assert(!OotLocationTitle_Update(&p));
    // Toggling the checkbox applies at the next title, keeping current fade coherent.
    init("AB"); OotLocationTitle_Begin(&p); option=0; assert(OotLocationTitle_Update(&p));
    // Optional HD font resources compose the exact decoded message; no scene
    // name lookup or general message font is replaced. The old presenter fails
    // this first recorded-load assertion when given the same available font.
    init("AB"); fontAsset=1; OotLocationTitle_Begin(&p);
    assert(sWidths[0]==16);
    for(int i=0;i<21;++i) OotLocationTitle_Update(&p);
    n=draw(); int fonts=0, nativeGlyphs=0, rectangles=0;
    for(int i=0;i<n;++i) {
        nativeGlyphs+=commands[i].kind==3;
        rectangles+=commands[i].kind==4;
        if(commands[i].kind==2) {
            const char* path=(const char*)commands[i].texture;
            assert(!((uintptr_t)path&1));
            assert(!strcmp(path,fonts ? "__OTR__custom/mm_location_titles/font/42" :
                                       "__OTR__custom/mm_location_titles/font/41"));
            ++fonts;
        }
    }
    assert(fonts==2 && nativeGlyphs==0 && rectangles==10 && fontProbes==2);
    // Any missing character falls back for the whole message, retaining the
    // same native outline, spacing and glyph buffer indices.
    init("AB"); fontAsset=1; missingGlyph='B'; OotLocationTitle_Begin(&p);
    for(int i=0;i<21;++i) OotLocationTitle_Update(&p);
    n=draw(); fonts=nativeGlyphs=0;
    for(int i=0;i<n;++i) { fonts+=commands[i].kind==2; nativeGlyphs+=commands[i].kind==3; }
    assert(fonts==0 && nativeGlyphs==2);
    init("A A\x11" "B"); fontAsset=1; OotLocationTitle_Begin(&p);
    assert(sLines==2 && sWidths[0]==22 && sWidths[1]==8 && fontProbes==2);
    // Whole-card overrides keep precedence and do not even probe the font.
    init("AB"); fontAsset=nativeAsset=1; OotLocationTitle_Begin(&p);
    assert(fontProbes==0);
    for(int i=0;i<21;++i) OotLocationTitle_Update(&p);
    n=draw(); cards=0;
    for(int i=0;i<n;++i) if(commands[i].kind==2) {
        ++cards; assert(!strcmp((const char*)commands[i].texture,"__OTR__custom/mm_location_titles/1/0116"));
    }
    assert(cards==1);
    // Unsupported scripting remains native even with a complete font pack.
    init("A\xC2"); fontAsset=1; OotLocationTitle_Begin(&p); assert(!sOwner);
    // Actual native OoT profile: true crop advances and 24px tiles; no
    // dialogue-width distortion. Old 16px namespace tests above still pass.
    init("AB"); nativeFontAsset=1; OotLocationTitle_Begin(&p);
    assert(sUseFont==2 && sWidths[0]==24);
    for(int i=0;i<21;++i) OotLocationTitle_Update(&p);
    n=draw(); fonts=nativeGlyphs=rectangles=0;
    for(int i=0;i<n;++i) {
        nativeGlyphs+=commands[i].kind==3;
        if(commands[i].kind==2) {
            assert(strstr((const char*)commands[i].texture,"/oot_native_v1/")); ++fonts;
        }
        if(commands[i].kind==4) { ++rectangles; assert(commands[i].c-commands[i].a==96); }
    }
    assert(fonts==2 && !nativeGlyphs && rectangles==10);
    init("A A\x11" "B"); nativeFontAsset=1; OotLocationTitle_Begin(&p);
    assert(sUseFont==2 && sLines==2 && sWidths[0]==31 && sWidths[1]==11);
    // Missing resource, unprovided native letter, and unprovided punctuation
    // all fall back for the complete title, even if the old pack is mounted.
    init("AB"); nativeFontAsset=fontAsset=1; missingGlyph='B'; OotLocationTitle_Begin(&p);
    assert(!sUseFont && sWidths[0]==16);
    init("AE"); nativeFontAsset=fontAsset=1; OotLocationTitle_Begin(&p);
    assert(!sUseFont && sWidths[0]==16);
    init("A&A"); nativeFontAsset=fontAsset=1; OotLocationTitle_Begin(&p);
    assert(!sUseFont && sWidths[0]==24);
    init("AB"); nativeFontAsset=nativeAsset=1; OotLocationTitle_Begin(&p); assert(sUseTexture && !sUseFont);
    init("A\xC2"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(!sOwner);
    init("Termina Field"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==106);
    init("East Clock Town"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==0);
    init("West Clock Town"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==146);
    init("North Clock Town"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==0);
    init("South Clock Town"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==154);
    init("Laundry Pool"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==110);
    init("Ikana Graveyard"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==140);
    init("Town Shooting Gallery"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==191);
    init("Honey & Darling's Shop"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==0);
    init("Treasure Chest Shop"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==176);
    init("Stock Pot Inn"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==116);
    init("The Mayor's Residence"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==185);
    init("Milk Bar"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==67);
    init("Bomb Shop"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==91);
    init("Curiosity Shop"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==126);
    init("Trading Post"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==106);
    init("Swordsman's School"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==171);
    init("Post Office"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==0);
    init("Lottery Shop"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==114);
    init("Beneath the Graveyard"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==196);
    init("Astral Observatory"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==0);
    init("Southern Swamp"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==144);
    init("Woodfall"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==74);
    init("Deku Palace"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==101);
    init("Tourist Information"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==159);
    init("Magic Hags' Potion Shop"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==208);
    init("Swamp Shooting Gallery"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==208);
    init("Deku King's Chamber"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==173);
    init("Woods of Mystery"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==154);
    init("Swamp Spider House"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==174);
    init("Woodfall Temple"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==135);
    init("Deku Shrine"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==101);
    init("Mountain Village"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==136);
    init("Goron Village"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==112);
    init("Goron Shrine"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==112);
    init("Snowhead"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==87);
    init("Goron Racetrack"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==143);
    init("Mountain Smithy"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==142);
    init("Goron Shop"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==101);
    init("Goron Graveyard"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==147);
    init("Snowhead Temple"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==148);
    init("Secret Shrine"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==117);
    init("Romani Ranch"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==116);
    init("Cucco Shack"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==113);
    init("Doggy Racetrack"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==147);
    init("Gorman Track"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==119);
    init("Mama's House"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==114);
    init("Great Bay Coast"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==142);
    init("Pinnacle Rock"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==115);
    init("Zora Hall"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==76);
    init("Waterfall Rapids"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==137);
    init("Pirates' Fortress"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==135);
    init("Great Bay"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==86);
    init("Marine Research Lab"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==170);
    init("Fisherman's Hut"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==129);
    init("Great Bay Temple"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==147);
    init("Fairy's Fountain"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==128);
    init("Oceanside Spider House"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==0);
    init("Stone Tower"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==106);
    init("Ikana Canyon"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==116);
    init("Ancient Castle of Ikana"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==196);
    init("Sakon's Hideout"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==132);
    init("Stone Tower Temple"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==167);
    init("Beneath the Well"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==141);
    init("Ghost Hut"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==86);
    init("Music Box House"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==135);
    init("Milk Road"); nativeFontAsset=1; OotLocationTitle_Begin(&p); assert(sUseFont==2);
    assert(sWidths[0]==79);
    /* BUILTIN_NATIVE_CASES */
    puts("location cards: native fade, glyph commands, eligibility, assets and interruption PASS");
}
"""

def main():
    source = (ROOT / "mm/2s2h/Enhancements/Graphics/OotLocationTitleCard.c").read_text()
    actor = (ROOT / "mm/src/code/z_actor.c").read_text()
    with tempfile.TemporaryDirectory(prefix="mm-location-title-") as d:
        tmp = Path(d)
        (tmp / "tables").mkdir()
        (tmp / "libultraship/bridge").mkdir(parents=True)
        (tmp / "global.h").write_text(GLOBAL)
        (tmp / "BenPort.h").write_text(BRIDGE)
        (tmp / "OotLocationTitleCard.h").write_text(
            (ROOT / "mm/2s2h/Enhancements/Graphics/OotLocationTitleCard.h").read_text())
        (tmp / "tables/scene_table.h").write_text((ROOT / "mm/include/tables/scene_table.h").read_text())
        (tmp / "message_data_fmt_nes.h").write_text((ROOT / "mm/include/message_data_fmt_nes.h").read_text())
        (tmp / "libultraship/bridge/consolevariablebridge.h").write_text("int CVarGetInteger(const char*, int);\n")
        scenes = (ROOT / 'mm/include/tables/scene_table.h').read_text()
        scene_titles = {}
        for scene, text in re.findall(r'DEFINE_SCENE\([^,]+,\s*(SCENE_\w+),\s*(0x[0-9A-Fa-f]+|0)\s*,', scenes):
            if int(text, 0):
                scene_titles.setdefault(f'{int(text, 0):04x}', scene)
        titles = json.loads((ROOT / 'tools/location_titles/assets/native-v3/manifest.json').read_text())['titles']
        assert set(scene_titles) == {row['id'] for row in titles}, 'all actual titled scene IDs must have built-in art'
        cases = []
        for row in titles:
            cases.append(f'''
    init({json.dumps(row['text'])}); builtinAsset=fontAsset=nativeFontAsset=1;
    p.sceneId={scene_titles[row['id']]}; p.msgCtx.currentTextId=0x{row['id']};
    OotLocationTitle_Begin(&p);
    assert(sUseTexture==2 && !sUseFont && !fontProbes);
    assert(!strcmp(sTexturePath,"__OTR__custom/oot_location_titles/v1/1/{row['id']}"));
    for(int i=0;i<21;++i) OotLocationTitle_Update(&p);
    n=draw(); cards=rectangles=0;
    for(int i=0;i<n;++i) {{
        cards+=commands[i].kind==2;
        if(commands[i].kind==4) {{
            ++rectangles;
            assert(commands[i].a==40*4 && commands[i].b==108*4);
            assert(commands[i].c==280*4-4 && commands[i].d==132*4-1);
        }}
    }}
    assert(cards==1 && rectangles==1); // halo is baked, never drawn five times
''')
        cases.append('''
    init("AB"); builtinAsset=nativeAsset=1; OotLocationTitle_Begin(&p);
    assert(sUseTexture==1 && !strcmp(sTexturePath,"__OTR__custom/mm_location_titles/1/0116"));
''')
        harness = HARNESS.replace('/* BUILTIN_NATIVE_CASES */', '\n'.join(cases))
        (tmp / "test.c").write_text(source + "\n" + function(actor, "TitleCard_Update") + "\n" + harness)
        binary = tmp / "test"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-Wno-unused-parameter", "-fsanitize=address,undefined", "-I", str(tmp),
                        str(tmp / "test.c"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True, env={**os.environ, "ASAN_OPTIONS": "detect_leaks=0"})
        check_resources(tmp, (ROOT / "mm/2s2h/Enhancements/Graphics/OotLocationTitleResources.cpp").read_text())


CPP_STUB = r"""
#pragma once
#include <memory>
#include <set>
#include <string>
namespace Ship {
struct Archive { std::set<std::string> files; bool HasFile(const std::string& p) { return files.count(p); } };
struct ResourceManager {
    std::shared_ptr<Archive> archive = std::make_shared<Archive>();
    bool alt = false;
    auto GetArchiveManager() { return archive; }
    bool IsAltAssetsEnabled() { return alt; }
};
inline std::shared_ptr<ResourceManager> manager;
struct Context {
    static Context* GetRawInstance() { static Context value; return &value; }
    auto GetResourceManager() { return manager; }
};
struct CrossRMRegistry {
    static auto Get(const std::string& owner) {
        if (owner != "mm") throw 1;
        return manager;
    }
};
}
"""
CPP_HARNESS = r"""
#include <cassert>
int main() {
    const char* path="__OTR__custom/mm_location_titles/1/0116";
    assert(!OotLocationTitle_HasTexture(path));
    Ship::manager=std::make_shared<Ship::ResourceManager>();
    auto archive=Ship::manager->archive;
    assert(!OotLocationTitle_HasTexture(path));
    archive->files.insert("custom/mm_location_titles/1/0116");
    assert(OotLocationTitle_HasTexture(path));
    archive->files.clear();
    archive->files.insert("custom/mm_location_titles/1/0116.meta");
    assert(OotLocationTitle_HasTexture(path));
    archive->files.clear();
    archive->files.insert("alt/custom/mm_location_titles/1/0116.meta");
    assert(!OotLocationTitle_HasTexture(path));
    Ship::manager->alt=true;
    assert(OotLocationTitle_HasTexture(path));
    Ship::manager->archive.reset();
    assert(!OotLocationTitle_HasTexture(path));
}
"""
def check_resources(tmp, source):
    (tmp / "ship/resource").mkdir(parents=True)
    for header in ("ship/Context.h", "ship/resource/ResourceManager.h", "ship/resource/CrossRMRegistry.h"):
        (tmp / header).write_text('#include "resource_stub.h"\n')
    (tmp / "resource_stub.h").write_text(CPP_STUB)
    (tmp / "resource.cpp").write_text(source + "\n" + CPP_HARNESS)
    for combo in (False, True):
        binary = tmp / ("resources-combo" if combo else "resources-mm")
        subprocess.run(["c++", "-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(tmp),
                        *(["-DCOMBO_BUILD"] if combo else []), str(tmp / "resource.cpp"), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
    print("location resources: native/meta/Alt/missing-owner in MM and Combo PASS")

if __name__ == "__main__":
    main()
