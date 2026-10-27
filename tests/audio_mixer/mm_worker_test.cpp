#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <thread>

using s16 = int16_t;
using u8 = uint8_t;
using u32 = uint32_t;
static int R_UPDATE_RATE = 3;
static struct {
  std::thread thread;
  std::condition_variable cv_to_thread, cv_from_thread;
  std::mutex mutex;
  bool running = false;
  bool processing = false;
  int framesPerUpdate = 1;
} audio;
static std::atomic<bool> gFscAudioMuted{false};
static std::atomic<int> buffered{0};
static unsigned synthUpdates, synthFrames, batches, weatherCalls, midnaCalls,
    midnaResets, emoteCalls;
static int desired = 4096;

static int AudioPlayer_Buffered() { return buffered; }
static int AudioPlayer_GetDesiredBuffered() { return desired; }
static void AudioMgr_CreateNextAudioBuffer(s16 *buffer, u32 frames) {
  assert(frames == 528 || frames == 544 || frames == 560);
  std::fill_n(buffer, frames * 2, 1000);
  ++synthUpdates;
  synthFrames += frames;
}
static void MMWeatherAudio_Mix(s16 *buffer, size_t frames) {
  for (size_t i = 0; i < frames * 2; ++i) {
    assert(buffer[i] == 1000);
    buffer[i] += 200;
  }
  ++weatherCalls;
}
static void MMMidnaAudio_Mix(s16 *buffer, size_t frames) {
  for (size_t i = 0; i < frames * 2; ++i) {
    assert(buffer[i] == 1200);
    buffer[i] += 300;
  }
  ++midnaCalls;
}
static void MMMidnaAudio_Reset() { ++midnaResets; }
#if TEST_AUDIO_WORKER_EMOTES
static void ComboEmotes_MixAudio(s16 *buffer, size_t frames) {
  for (size_t i = 0; i < frames * 2; ++i) {
    assert(buffer[i] == 1500);
    buffer[i] += 400;
  }
  ++emoteCalls;
}
#endif
static void AudioPlayer_Play(const u8 *buffer, size_t bytes) {
  const auto *pcm = reinterpret_cast<const s16 *>(buffer);
  assert(weatherCalls == batches + 1 && midnaCalls == batches + 1);
  assert(emoteCalls == (TEST_AUDIO_WORKER_EMOTES ? batches + 1 : 0));
  for (size_t i = 0; i < bytes / sizeof(s16); ++i) {
    assert(pcm[i] == (gFscAudioMuted ? 0 : (TEST_AUDIO_WORKER_EMOTES ? 1900 : 1500)));
  }
  buffered += bytes / (sizeof(s16) * 2);
  assert(buffered <= desired);
  ++batches;
}

static int diagnosticsMode = 0;
static int CVarGetInteger(const char *key, int fallback) {
  return std::strcmp(key, "gDeveloperTools.MMAudioPCMTrace") == 0
             ? diagnosticsMode
             : fallback;
}
template <class... T> static void TestAudioLog(const T &...) {}
#define SPDLOG_INFO(...) TestAudioLog(__VA_ARGS__)
#define SPDLOG_ERROR(...) TestAudioLog(__VA_ARGS__)
#include "../../combo/audio/MMAudioTraceHost.h"
#include "mm_audio_worker.inc"

static void start() {
  std::lock_guard<std::mutex> lock(audio.mutex);
  audio.running = true;
  audio.processing = false;
  buffered = 0;
  synthUpdates = synthFrames = batches = weatherCalls = midnaCalls =
      midnaResets = emoteCalls = 0;
  audio.thread = std::thread(OTRAudio_Thread);
}
static void stop() {
  {
    std::lock_guard<std::mutex> lock(audio.mutex);
    audio.running = false;
  }
  audio.cv_to_thread.notify_all();
  audio.thread.join();
  assert(!audio.processing);
}
static void frame(int rate) {
  std::unique_lock<std::mutex> lock(audio.mutex);
  R_UPDATE_RATE = rate;
  audio.framesPerUpdate = rate;
  audio.processing = true;
  audio.cv_to_thread.notify_one();
  assert(audio.cv_from_thread.wait_for(lock, std::chrono::seconds(2),
                                       [] { return !audio.processing; }));
}
static bool awaitRefill() {
  // No graphics wake is sent. The production worker must refill on its own.
  for (int i = 0; i < 100; ++i) {
    if (buffered >= 2400) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  return false;
}

int main(int argc, char **argv) {
  diagnosticsMode = argc > 1 ? std::atoi(argv[1]) : 0;
  start();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  {
    std::lock_guard<std::mutex> lock(audio.mutex);
    assert(synthUpdates ==
           0); // No premature synthesis before a ready game frame.
  }
  stop(); // Shutdown before the first frame must wake/join cleanly.

  for (int rate : {1, 2, 3}) {
    start();
    frame(rate);
    for (int i = 0; i < 8 && buffered < 2400; ++i) {
      frame(rate);
    }
    assert(awaitRefill());
    for (int stall = 0; stall < 8; ++stall) {
      buffered -=
          2400; // Consume 75 ms of sound while graphics remains stalled.
      if (!awaitRefill()) {
        std::fprintf(stderr, "FAIL: MM audio starved while the graphics thread "
                             "did not wake it\n");
        stop();
        return 1;
      }
    }
    {
      std::unique_lock<std::mutex> lock(audio.mutex);
      // Filling the backend must stop both synthesis and sequencer advancement.
      buffered = desired;
      unsigned before = synthUpdates;
      lock.unlock();
      frame(rate);
      std::this_thread::sleep_for(std::chrono::milliseconds(15));
      lock.lock();
      assert(synthUpdates == before);
      const int64_t debt = static_cast<int64_t>(synthFrames) * 3 -
                           static_cast<int64_t>(synthUpdates) * 1600;
      assert(std::abs(debt) <=
             48); // Exactly 32 kHz/60 in the long run, all update divisors.

      gFscAudioMuted = true;
      buffered = 0;
    }
    assert(awaitRefill());
    {
      std::lock_guard<std::mutex> lock(audio.mutex);
      assert(midnaResets != 0);
      gFscAudioMuted = false;
      buffered = 0;
    }
    frame(rate); // Active-game output and mix-ins resume together.
    assert(awaitRefill());
    stop();
  }
  // A transition restarts the worker unprimed, with no stale request or sample
  // debt.
  start();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  {
    std::lock_guard<std::mutex> lock(audio.mutex);
    assert(synthUpdates == 0);
  }
  stop();
  std::puts("PASS MM production audio worker: stalled rendering, queue bounds, "
            "tempo, mute/mix-ins, restart and shutdown");
}
