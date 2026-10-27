// Real MM bank, dispatcher and emitter-stop wrapper; synthesis/output is the
// boundary.
#include "mods/sound_translator/mm_audio_sfx.h"
#include "mods/sound_translator/mm_sfx_ids.h"
#include "mods/transformation_masks/assets/mm_asset_loader.h"
#include <cassert>
#include <cstdio>
#include <cstring>

static unsigned lockDepth, lockCalls;
static signed char channelIO[16][8];
static unsigned triggers;
extern "C" void OTRAudio_LockMutex(void) {
  ++lockDepth;
  ++lockCalls;
}
extern "C" void OTRAudio_UnlockMutex(void) {
  assert(lockDepth == 1);
  --lockDepth;
}
extern "C" void Audio_SetVolScale(u8, u8, u8, u8) {}
extern "C" void lusprintf(const char *, int32_t, int32_t, const char *, ...) {}
extern "C" int MmSfxSynth_IsReady(void) { return 1; }
extern "C" void MmSfxSynth_SetChannelState(int, float, float frequency,
                                           signed char, signed char) {
  assert(frequency > 0.0f);
}
extern "C" int MmSfxSynth_ReadChannelIO(int channel, int port) {
  return channelIO[channel][port];
}
extern "C" void MmSfxSynth_WriteChannelIO(int channel, int port,
                                          signed char value) {
  channelIO[channel][port] = value;
  if (port == 0 && value == 1)
    channelIO[channel][1] =
        0; // Sequence_0 owns this channel until its note finishes.
  if (port == 4) {
    assert(value == 0); // Native Sequence_0 low byte for MM's 0x2900 call.
    ++triggers;
  }
  if (port == 5)
    assert(value == 2); // Native large-environment-bank index bits.
}
extern "C" s32 MmSfxBridge_Play(u16, f32, Vec3f *) {
  assert(false);
  return 0;
}
extern "C" void MmSfxBridge_RefreshProperties(u16, Vec3f *, f32) {
  assert(false);
}
extern "C" void MmSfxBridge_StopByMmSfxId(u16) { assert(false); }
extern "C" s32 MmSfxBridge_IsActive(u16) {
  assert(false);
  return 0;
}

/* PRODUCTION_EMITTER_STOP */

static bool contains(Vec3f *position, u16 id) {
  for (u8 index = gMmSfxBanks[2][0].next; index != 0xFF;
       index = gMmSfxBanks[2][index].next)
    if (gMmSfxBanks[2][index].posX == &position->x &&
        gMmSfxBanks[2][index].sfxId == id)
      return true;
  return false;
}
static void queue(Vec3f *position, u16 id = MM_NA_SE_EV_MONKEY_VO_WALK) {
  AudioMmSfx_PlaySfx(id, position, 4, &gMmSfxDefaultFreqAndVolScale,
                     &gMmSfxDefaultFreqAndVolScale, &gMmSfxDefaultReverb);
}
static void tick() {
  AudioMmSfx_ProcessRequests();
  AudioMmSfx_ProcessActiveSfx();
}
int main() {
  Vec3f second = {30.0f, 0.0f, 100.0f};
  // Cold cleanup must not traverse zero-filled bank links or initialize audio.
  MmSfx_StopAtPos(0x2900, &second);
  MmSfx_StopAtPos(0x2900, nullptr);
  MmSfx_StopAtPos(0xFFFF, &second);
  assert(lockDepth == 0 && triggers == 0);
  AudioMmSfx_Reset();
  memset(channelIO, -1, sizeof(channelIO));
  auto *first = new Vec3f{-30.0f, 0.0f, 100.0f};
  queue(first);
  queue(&second);
  tick();
  assert(contains(first, 0x2900) && contains(&second, 0x2900));
  assert(triggers == 2);
  unsigned before = lockCalls;
  MmSfx_StopAtPos(0x2900, first);
  assert(lockCalls == before + 1 && lockDepth == 0);
  assert(!contains(first, 0x2900) && contains(&second, 0x2900));
  delete first;
  tick(); // ASan catches any actor-position reference surviving destruction.
  assert(contains(&second, 0x2900));

  auto *queued = new Vec3f{};
  queue(queued);
  queue(&second, 0x28EF); // Native monkey footstep is unrelated to the call.
  MmSfx_StopAtPos(0x2900, queued);
  delete queued;
  AudioMmSfx_ProcessRequests(); // Pending requests must also be safe after
                                // free.
  assert(contains(&second, 0x2900) && contains(&second, 0x28EF));
  MmSfx_StopAtPos(0x2900, &second);
  assert(!contains(&second, 0x2900) && contains(&second, 0x28EF));
  AudioMmSfx_StopById(0x28EF);
  AudioMmSfx_ProcessRequests();
  // Re-entering the room can reuse the same position and native call.
  queue(&second);
  tick();
  assert(contains(&second, 0x2900) && triggers == 3);
  MmSfx_StopAtPos(0x2900, &second);
  assert(lockDepth == 0);
  puts("PASS native MM monkey Sequence_0 dispatch, per-emitter/queued cleanup, "
       "cold stop and re-entry (ASan/UBSan)");
}
