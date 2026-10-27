#include "test_require.h"
#define PR_OS_VOICE_H
#define PR_OS_MESSAGE_H
typedef void* OSMesg;
typedef struct {
    int unused;
} OSMesgQueue;
typedef struct {
    int unused;
} OSIoMesg;
typedef struct {
    int unused;
} OSTask;
typedef struct {
    int unused;
} OSPiHandle;
typedef struct {
    int unused;
} Acmd;
#include "z64audio.h"
#include "seqcmd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN16(value) (((value) + 15) & ~(uintptr_t)15)
#define ARRAY_COUNT(value) (sizeof(value) / sizeof((value)[0]))
#define CURRENT_DAY 1

AudioContext gAudioCtx;
ActiveSequence gActiveSeqs[SEQ_PLAYER_MAX];
char** gSequenceMap;
size_t gSequenceMapSize;
char** gFontMap;
size_t gFontMapSize;
u8 seqCachePolicyMap[MAX_AUTHENTIC_SEQID];
u8 sSeqCmdWritePos;
u8 sSeqCmdReadPos;
u8 sStartSeqDisabled;
u32 sAudioSeqCmds[256];
// This fixture exercises sequence/font routing. The native season fixture runs
// the real storm-channel composer through the production command queue.
u32 MMWeather_ResolveAmbienceSeqCmd(u32 cmd) {
    return cmd;
}
static const uint8_t sClockTownDaySeqIds[4] = {
    NA_BGM_CLOCK_TOWN_DAY_1,
    NA_BGM_CLOCK_TOWN_DAY_2,
    NA_BGM_CLOCK_TOWN_DAY_3,
    NA_BGM_CLOCK_TOWN_DAY_1,
};

static SequenceData sequences[513];
static SoundFont fonts[513];
static Instrument instruments[513];
static Instrument* instrumentPtrs[513];
static char names[513][8];
static char* sequenceNames[513 + 0xF];
static char* fontNames[513];
static u8 sequenceStatus[513 + 0xF];
static u8 fontStatus[513];
static u8 script[] = { 0xFF };
static u16 queuedSeqId;
static u8 cachedSequence[16];
static u32 cacheLookups;

typedef void SoundFontData;
typedef struct {
    u16 numInstruments;
    u16 numDrums;
    u16 numSfx;
} UnloadedFonts;

void* AudioLoad_SyncLoad(s32 tableType, u32 id, s32* didAllocate);

SequenceData ResourceMgr_LoadSeqByName(const char* path) {
    return sequences[strtoul(path, NULL, 10)];
}
SequenceData* ResourceMgr_LoadSeqPtrByName(const char* path) {
    return &sequences[strtoul(path, NULL, 10)];
}
SoundFont* ResourceMgr_LoadAudioSoundFontByName(const char* path) {
    return &fonts[strtoul(path, NULL, 10)];
}
u32 AudioLoad_GetRealTableIndex(s32 tableType, u32 id) {
    return id;
}
void* AudioHeap_SearchCaches(s32 tableType, s32 cache, s32 id) {
    return id >= 0 && (size_t)id < gFontMapSize && fontStatus[id] >= LOAD_STATUS_COMPLETE ? &fonts[id] : NULL;
}
void* AudioLoad_SearchCaches(s32 tableType, s32 id) {
    REQUIRE(tableType == SEQUENCE_TABLE);
    ++cacheLookups;
    return NULL;
}
AudioTable* AudioLoad_GetLoadTable(s32 tableType) {
    return NULL;
}
void* AudioHeap_AllocCached(s32 tableType, size_t size, s32 cache, s32 id) {
    REQUIRE(tableType == SEQUENCE_TABLE);
    REQUIRE(size <= sizeof(cachedSequence));
    return cachedSequence;
}
void AudioLoad_SyncDma(uintptr_t devAddr, u8* ramAddr, size_t size, s32 medium) {
    memcpy(ramAddr, (const void*)devAddr, size);
}
void AudioLoad_SyncDmaUnkMedium(uintptr_t devAddr, u8* ramAddr, size_t size, s32 unkMediumParam) {
    REQUIRE(false);
}
void AudioLoad_SetSampleFontLoadStatusAndApplyCaches(s32 sampleBankId, s32 loadStatus) {
    REQUIRE(false);
}
void AudioScript_SequencePlayerDisable(SequencePlayer* player) {
    player->enabled = false;
}
void AudioScript_ResetSequencePlayer(SequencePlayer* player) {
    player->enabled = false;
}
SoundFontData* AudioLoad_SyncLoadFont(u32 id) {
    s32 didAllocate;
    return AudioLoad_SyncLoad(FONT_TABLE, id, &didAllocate);
}
u8* AudioLoad_SyncLoadSeq(s32 id) {
    return id >= 0 && (size_t)id < gSequenceMapSize ? script : NULL;
}
void GameInteractor_ExecuteOnSeqPlayerInit(s32 player, s32 sequence) {
}
u16 AudioEditor_GetReplacementSeq(u16 id) {
    return id == 1 ? 256 : id == 2 ? 512 : id;
}
u16 AudioEditor_GetOriginalSeq(u16 id) {
    return id;
}

#undef AUDIOCMD_GLOBAL_INIT_SEQPLAYER
#undef AUDIOCMD_GLOBAL_INIT_SEQPLAYER_SKIP_TICKS
#undef AUDIOCMD_SEQPLAYER_FADE_VOLUME_SCALE
#define AUDIOCMD_GLOBAL_INIT_SEQPLAYER(player, seq, fade) (queuedSeqId = (seq))
#define AUDIOCMD_GLOBAL_INIT_SEQPLAYER_SKIP_TICKS(player, seq, ticks) (queuedSeqId = (seq))
#define AUDIOCMD_SEQPLAYER_FADE_VOLUME_SCALE(player, volume) ((void)0)

#include "mm_audio_runtime_functions.inc"

int main(void) {
    for (int id = 0; id <= 512; ++id) {
        snprintf(names[id], sizeof(names[id]), "%d", id);
        sequenceNames[id] = fontNames[id] = names[id];
        sequences[id] = (SequenceData){ .seqData = (char*)script, .seqDataSize = 1, .numFonts = 1, .resolvedFont = id };
        sequences[id].fonts[0] = id;
        instrumentPtrs[id] = &instruments[id];
        fonts[id].numInstruments = 1;
        fonts[id].instruments = &instrumentPtrs[id];
    }
    gSequenceMap = sequenceNames;
    gSequenceMapSize = 513;
    gFontMap = fontNames;
    gFontMapSize = ARRAY_COUNT(fontNames);
    gAudioCtx.seqLoadStatus = sequenceStatus;
    gAudioCtx.fontLoadStatus = fontStatus;
    gAudioCtx.audioBufferParameters.numSequencePlayers = SEQ_PLAYER_MAX;
    REQUIRE(!AudioLoad_IsSeqLoadComplete(513 + 0xF - 1));

    SequenceChannel channel = { 0 };
    SequencePlayer* player = &gAudioCtx.seqPlayers[0];
    player->channels[0] = &channel;
    const int ids[] = { 254, 255, 256, 257, 511, 512 };
    for (size_t i = 0; i < ARRAY_COUNT(ids); ++i) {
        int id = ids[i];
        s32 didAllocate = true;
        REQUIRE(AudioLoad_SyncLoad(FONT_TABLE, id, &didAllocate) == &fonts[id]);
        REQUIRE(!didAllocate);
        REQUIRE(fontStatus[id] == LOAD_STATUS_PERMANENT);
        REQUIRE(AudioLoad_SyncInitSeqPlayerInternal(0, id, 0));
        REQUIRE(player->seqId == id);
        REQUIRE(player->defaultFont == id);
        REQUIRE(AudioLoad_IsFontLoadComplete(id));
        AudioScript_SequencePlayerSetupChannels(player, 1);
        REQUIRE(channel.fontId == id);
        REQUIRE(AudioPlayback_GetInstrumentInner(channel.fontId, 0) == &instruments[id]);
    }
    REQUIRE(cacheLookups == 0);
    s32 didAllocate = true;
    REQUIRE(AudioLoad_SyncLoad(FONT_TABLE, gFontMapSize, &didAllocate) == NULL);
    REQUIRE(!didAllocate);
    fontNames[512] = NULL;
    REQUIRE(AudioLoad_SyncLoad(FONT_TABLE, 512, &didAllocate) == NULL);
    fontNames[512] = names[512];

    sequences[256].cachePolicy = CACHE_LOAD_TEMPORARY;
    sequences[256].medium = MEDIUM_CART;
    REQUIRE(AudioLoad_SyncLoad(SEQUENCE_TABLE, 256, &didAllocate) == cachedSequence);
    REQUIRE(didAllocate);
    REQUIRE(cacheLookups == 1);
    REQUIRE(cachedSequence[0] == script[0]);
    REQUIRE(sequenceStatus[256] == LOAD_STATUS_COMPLETE);
    sequences[256].medium = MEDIUM_UNK;
    REQUIRE(AudioLoad_SyncLoad(SEQUENCE_TABLE, 256, &didAllocate) == NULL);
    sequences[256].medium = MEDIUM_CART;

    sequences[512].resolvedFont = -1;
    sequences[512].numFonts = 2;
    sequences[512].fonts[0] = 257;
    sequences[512].fonts[1] = 512;
    u32 fontCount = 0;
    s32* fontList = AudioLoad_GetFontsForSequence(512, &fontCount);
    REQUIRE(fontList == sequences[512].fonts);
    REQUIRE(fontCount == 2);
    player->seqId = 512;
    player->defaultFont = 512;
    channel.seqPlayer = player;
    channel.fontId = 1;
    AudioScript_SelectChannelFont(&channel, 0);
    REQUIRE(channel.fontId == 512);
    AudioScript_SelectChannelFont(&channel, 1);
    REQUIRE(channel.fontId == 257);
    AudioScript_SelectChannelFont(&channel, 2);
    REQUIRE(channel.fontId == 257);

    Note notes[2] = { 0 };
    gAudioCtx.notes = notes;
    gAudioCtx.numNotes = 2;
    notes[0].playbackState.fontId = 512;
    notes[1].playbackState.fontId = 0;
    notes[0].playbackState.priority = notes[1].playbackState.priority = 2;
    AudioHeap_ReleaseNotesForFont(512);
    REQUIRE(notes[0].playbackState.adsr.action.s.release);
    REQUIRE(!notes[1].playbackState.adsr.action.s.release);

    // The permanent cache is host allocated and grows beyond the old 32 slots.
    static u8 pool[600 * 16];
    static AudioCacheEntry permanentEntries[600];
    gAudioCtx.permanentPool = (AudioAllocPool){ .startAddr = pool, .curAddr = pool, .size = sizeof(pool) };
    REQUIRE(AudioHeap_AllocPermanent(SEQUENCE_TABLE, 1, 16) == NULL);
    REQUIRE(gAudioCtx.permanentPool.count == 0 && gAudioCtx.permanentPool.curAddr == pool);
    gAudioCtx.permanentEntries = permanentEntries;
    REQUIRE(AudioHeap_AllocPermanent(SEQUENCE_TABLE, 1, 16) == NULL);
    REQUIRE(gAudioCtx.permanentPool.count == 0 && gAudioCtx.permanentPool.curAddr == pool);
    gAudioCtx.permanentEntriesCapacity = ARRAY_COUNT(permanentEntries);
    gAudioCtx.permanentPool.count = -1;
    REQUIRE(AudioHeap_AllocPermanent(SEQUENCE_TABLE, 1, 16) == NULL);
    REQUIRE(gAudioCtx.permanentPool.curAddr == pool);
    gAudioCtx.permanentPool.count = 0;
    // A failed pool allocation must not publish an entry or consume capacity.
    REQUIRE(AudioHeap_AllocPermanent(SEQUENCE_TABLE, 1, sizeof(pool) + 16) == NULL);
    REQUIRE(gAudioCtx.permanentPool.count == 0 && permanentEntries[0].addr == NULL);
    for (size_t i = 0; i < ARRAY_COUNT(ids); ++i) {
        void* cached = AudioHeap_AllocPermanent(SEQUENCE_TABLE, ids[i], 16);
        REQUIRE(cached != NULL);
        REQUIRE(AudioHeap_SearchPermanentCache(SEQUENCE_TABLE, ids[i]) == cached);
    }
    REQUIRE(AudioHeap_SearchPermanentCache(SEQUENCE_TABLE, 0) == NULL);
    for (size_t i = ARRAY_COUNT(ids); i < ARRAY_COUNT(permanentEntries); ++i) {
        void* cached = AudioHeap_AllocPermanent(SEQUENCE_TABLE, 1000 + i, 16);
        REQUIRE(cached != NULL);
        REQUIRE(AudioHeap_SearchPermanentCache(SEQUENCE_TABLE, 1000 + i) == cached);
    }
    REQUIRE(gAudioCtx.permanentPool.count == gAudioCtx.permanentEntriesCapacity);
    u8* fullPoolAddress = gAudioCtx.permanentPool.curAddr;
    REQUIRE(AudioHeap_AllocPermanent(SEQUENCE_TABLE, 2000, 16) == NULL);
    REQUIRE(gAudioCtx.permanentPool.count == gAudioCtx.permanentEntriesCapacity);
    REQUIRE(gAudioCtx.permanentPool.curAddr == fullPoolAddress);

    queuedSeqId = 0;
    AudioSeq_StartSequence(0, 1, 0, 0);
    REQUIRE(queuedSeqId == 256);
    AudioSeq_StartSequence(0, 2, 0, 0);
    REQUIRE(queuedSeqId == 512);
    // Stale side-channel state from an unrelated queued start must not change
    // this command's replacement. Ocarina uses player SFX and must not poison a
    // later SFX-player initialization either.
    gAudioCtx.seqReplaced[0] = true;
    gAudioCtx.seqToPlay[0] = 511;
    AudioSeq_StartSequence(0, 1, 0, 0);
    REQUIRE(queuedSeqId == 256);
    gAudioCtx.seqReplaced[SEQ_PLAYER_SFX] = true;
    gAudioCtx.seqToPlay[SEQ_PLAYER_SFX] = 511;
    AudioSeq_StartSequence(SEQ_PLAYER_SFX, 2, 0, 0);
    REQUIRE(queuedSeqId == 512);
    puts("PASS MM streamed audio runtime boundaries and command-carried replacements");
}
