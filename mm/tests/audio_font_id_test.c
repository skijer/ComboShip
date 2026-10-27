// Exercise the real MM engine structures. This catches narrowing anywhere in
// the player -> channel -> note/slow-load/cache lifetime of a host font ID.
#include "test_require.h"

// Platform handles are opaque to this storage test. Defining the os_voice
// include guard keeps the real engine header while avoiding unrelated OS ABI.
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

int main(void) {
    const int fontIds[] = { 254, 255, 256, 257, 511, 512 };

    for (size_t i = 0; i < sizeof(fontIds) / sizeof(fontIds[0]); ++i) {
        SequenceData sequence = { 0 };
        SequencePlayer player = { 0 };
        SequenceChannel channel = { 0 };
        NotePlaybackState note = { 0 };
        AudioSlowLoad pending = { 0 };
        AudioCacheEntry cache = { 0 };
        SampleCacheEntry sampleCache = { 0 };

        sequence.fonts[0] = fontIds[i];
        player.defaultFont = sequence.fonts[0];
        channel.fontId = player.defaultFont;
        note.fontId = channel.fontId;
        pending.seqOrFontId = note.fontId;
        cache.id = pending.seqOrFontId;
        sampleCache.sampleBankId = cache.id;

        REQUIRE(sequence.fonts[0] == fontIds[i]);
        REQUIRE(player.defaultFont == fontIds[i]);
        REQUIRE(channel.fontId == fontIds[i]);
        REQUIRE(note.fontId == fontIds[i]);
        REQUIRE(pending.seqOrFontId == fontIds[i]);
        REQUIRE(cache.id == fontIds[i]);
        REQUIRE(sampleCache.sampleBankId == fontIds[i]);
    }
}
