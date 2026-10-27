/* Execute the production carpet draw against real actor types and GBI packets.
 * GPU/resource lookup and the unrelated wave simulation stop at the boundary. */
#include <stdio.h>
#include <string.h>
#include "tests/test_require.h"
#include "src/overlays/actors/ovl_En_Jsjutan/z_en_jsjutan.h"
#include "overlays/ovl_En_Jsjutan/ovl_En_Jsjutan.h"

static Gfx commands[64];
static Vtx oddCarpet[144], evenCarpet[144], oddShadow[144], evenShadow[144];
static Mtx matrix;
u8 sShadowTex[2048];
Gfx gCullBackDList[] = { gsSPSetGeometryMode(G_CULL_BACK), gsSPEndDisplayList() };

void func_80A89860(EnJsjutan* carpet, PlayState* play) {
}
void func_80A89A6C(EnJsjutan* carpet, PlayState* play) {
}
f32 Math_SinS(s16 angle) {
    return 0.0f;
}
f32 Math_CosS(s16 angle) {
    return 1.0f;
}
void Gfx_SetupDL_25Opa(GraphicsContext* gfx) {
}
void FrameInterpolation_RecordOpenChild(const void* source, int line) {
}
void FrameInterpolation_RecordCloseChild(void) {
}
void Matrix_Translate(f32 x, f32 y, f32 z, u8 mode) {
}
void Matrix_Scale(f32 x, f32 y, f32 z, u8 mode) {
}
Mtx* Matrix_NewMtx(GraphicsContext* gfx, char* file, s32 line) {
    return &matrix;
}
void* ResourceGetDataByName(const char* name) {
    return sShadowTex;
}
void gSPDisplayList(Gfx* command, Gfx* list) {
    __gSPDisplayList(command, list);
}
void gSPSegment(void* command, int segment, uintptr_t target) {
    __gSPSegment((Gfx*)command, segment, target);
}
void gSPSegmentLoadRes(void* command, int segment, uintptr_t target) {
    if (target != (uintptr_t)sShadowTex) {
        const char* path = (const char*)target;
        if (strcmp(path, sCarpetOddVtx) == 0)
            target = (uintptr_t)oddCarpet;
        else if (strcmp(path, sCarpetEvenVtx) == 0)
            target = (uintptr_t)evenCarpet;
        else if (strcmp(path, gShadowOddVtx) == 0)
            target = (uintptr_t)oddShadow;
        else {
            REQUIRE(strcmp(path, sShadowEvenVtx) == 0);
            target = (uintptr_t)evenShadow;
        }
    }
    __gSPSegment((Gfx*)command, segment, target);
}

/* PRODUCTION_CARPET_DRAW */

static void checkCase(int frame, int type) {
    static PlayState play;
    GraphicsContext gfx = { 0 };
    Actor parent = { 0 };
    EnJsjutan carpet = { 0 };
    uintptr_t segments[16] = { 0 };
    unsigned modelDraws = 0;
    memset(&play, 0, sizeof(play));
    memset(commands, 0, sizeof(commands));
    gfx.polyOpa.p = commands;
    play.state.gfxCtx = &gfx;
    play.gameplayFrames = frame;
    carpet.dyna.actor.parent = &parent;
    carpet.dyna.actor.params = type;
    carpet.unk_175 = 1;
    carpet.shadowAlpha = 100.0f;
    segments[12] = (uintptr_t)gCullBackDList;
    EnJsjutan_Draw(&carpet.dyna.actor, &play);

    for (Gfx* cmd = commands; cmd < gfx.polyOpa.p; ++cmd) {
        unsigned opcode = (cmd->words.w0 >> 24) & 0xff;
        if (opcode == G_MOVEWORD && ((cmd->words.w0 >> 16) & 0xff) == G_MW_SEGMENT) {
            unsigned segment = (cmd->words.w0 & 0xffff) / 4;
            REQUIRE(segment < 16);
            segments[segment] = cmd->words.w1;
        } else if (opcode == G_DL && cmd->words.w1 == (uintptr_t)sModelDL) {
            /* Both mesh submissions retain the appropriate alternating buffer. */
            uintptr_t expected = modelDraws == 0 ? (uintptr_t)(frame & 1 ? oddShadow : evenShadow)
                                                 : (uintptr_t)(frame & 1 ? oddCarpet : evenCarpet);
            REQUIRE(segments[12] == expected);
            ++modelDraws;
        }
    }
    REQUIRE(modelDraws == 2);

    /* Later native/Alt model limbs execute this culling jump. Vertex data must
     * never be dispatched as its display list. This assertion catches the leak. */
    Gfx cullCall;
    __gSPDisplayList(&cullCall, (Gfx*)0x0C000001);
    uintptr_t destination = segments[cullCall.words.w1 >> 24] + (cullCall.words.w1 & 0x00fffffe);
    if (destination != (uintptr_t)gCullBackDList) {
        Vtx* vertices = (Vtx*)destination;
        uint64_t badWord = UINT64_C(0xFF0078000174022F);
        vertices[45].n.ob[1] = -512;
        memcpy((u8*)&vertices[45] + 8, &badWord, sizeof(badWord));
        Gfx packet;
        memcpy(&packet, &vertices[45], sizeof(packet));
        fprintf(stderr,
                "FAIL: later culling jump executes carpet Vtx[45]: opcode=0x%02lx "
                "w1=0x%016lx segment=%u (table has 16 slots)\n",
                (unsigned long)((packet.words.w0 >> 24) & 0xff), (unsigned long)packet.words.w1,
                (unsigned)(packet.words.w1 >> 24));
    }
    REQUIRE(destination == (uintptr_t)gCullBackDList);
    REQUIRE((gCullBackDList[0].words.w0 >> 24) == G_GEOMETRYMODE);
    REQUIRE((gCullBackDList[1].words.w0 >> 24) == G_ENDDL);
}

int main(void) {
    for (int frame = 0; frame < 2; ++frame)
        for (int type = 0; type < 2; ++type)
            checkCase(frame, type);
    puts("PASS production carpet draw: both frame buffers and actor types preserve mesh draws "
         "and return a valid culling display list to later models");
}
