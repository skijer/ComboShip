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
#include <string>
#include <thread>

#include "../../soh/soh/OTRAudio.h"

using s16 = int16_t;
using u8 = uint8_t;
using u32 = uint32_t;
static int R_UPDATE_RATE = 3;
static constexpr int desired = 4096;
static std::atomic<bool> gFscAudioMuted{false};
static std::atomic<int> buffered{0};
static std::atomic<unsigned> synthUpdates{0}, synthFrames{0}, batches{0},
    midnaResets{0}, emoteCalls{0};
static std::atomic<bool> inspectPrefill{false}, prefillEntered{false},
    releasePrefill{false};

static int AudioPlayer_Buffered() { return buffered; }
static int AudioPlayer_GetDesiredBuffered() { return desired; }
static void AudioMgr_CreateNextAudioBuffer(s16 *buffer, u32 frames) {
  assert(frames == 528 || frames == 544);
  if (inspectPrefill && synthUpdates == 3) {
    prefillEntered = true;
    while (!releasePrefill) {
      std::this_thread::yield();
    }
  }
  std::fill_n(buffer, frames * 2, 1000);
  ++synthUpdates;
  synthFrames += frames;
}
static void WeatherSamplePlayer_Mix(s16 *buffer, size_t frames) {
  for (size_t i = 0; i < frames * 2; ++i) {
    assert(buffer[i] == 1000);
    buffer[i] += 200;
  }
}
static void MidnaAudio_Mix(s16 *buffer, size_t frames) {
  for (size_t i = 0; i < frames * 2; ++i) {
    assert(buffer[i] == 1200);
    buffer[i] += 300;
  }
}
static void MidnaAudio_Reset() { ++midnaResets; }
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
  assert(emoteCalls == (TEST_AUDIO_WORKER_EMOTES ? batches + 1 : 0));
  for (size_t i = 0; i < bytes / sizeof(s16); ++i) {
    assert(pcm[i] == (gFscAudioMuted ? 0 : (TEST_AUDIO_WORKER_EMOTES ? 1900 : 1500)));
  }
  buffered += bytes / (sizeof(s16) * 2);
  assert(buffered <= desired);
  ++batches;
}

#include "soh_audio_worker.inc"

static void start(int rate, int queued = 0) {
  std::lock_guard<std::mutex> lock(audio.mutex);
  R_UPDATE_RATE = rate;
  audio.running = true;
  audio.processing = false;
  buffered = queued;
  synthUpdates = synthFrames = batches = midnaResets = emoteCalls = 0;
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

// Unlike a predicate-based wait_for, this test cannot pass merely because the
// worker cleared processing before a timeout. The real graphics wait must be
// woken by the worker. A separate watchdog only releases a deadlocked test.
static bool frame(bool stopBeforeProcessing = false) {
  std::mutex completionMutex;
  std::condition_variable completion;
  bool done = false;
  std::atomic<bool> timedOut{false};
  std::thread watchdog([&]() {
    std::unique_lock<std::mutex> lock(completionMutex);
    if (!completion.wait_for(lock, std::chrono::seconds(1),
                             [&]() { return done; })) {
      timedOut = true;
      std::lock_guard<std::mutex> audioLock(audio.mutex);
      audio.processing = false; // Rescue only; timedOut makes this a failure.
      audio.cv_from_thread.notify_all();
    }
  });
  {
    std::unique_lock<std::mutex> Lock(audio.mutex);
    audio.processing = true;
    if (stopBeforeProcessing) {
      audio.running = false;
    }
    audio.cv_to_thread.notify_one();
#include "soh_audio_wait.inc"
  }
  {
    std::lock_guard<std::mutex> lock(completionMutex);
    done = true;
  }
  completion.notify_one();
  watchdog.join();
  return !timedOut;
}

static bool awaitRefill() {
  for (int i = 0; i < 100; ++i) {
    if (buffered >= 2400) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2));
  }
  return false;
}

int main(int argc, char **argv) {
  const std::string mode = argc > 1 ? argv[1] : "synthesis";
  if (mode == "serialization") {
    inspectPrefill = true;
    start(3);
    bool acquired = true;
    std::thread inspector([&]() {
      for (int i = 0; i < 100 && !prefillEntered; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
      }
      if (prefillEntered) {
        acquired = audio.mutex.try_lock();
        if (acquired) {
          audio.mutex.unlock();
        }
      }
      releasePrefill = true;
    });
    const bool completed = frame();
    inspector.join();
    stop();
    assert(completed && prefillEntered &&
           "test must reach reservoir synthesis and release graphics");
    assert(
        !acquired &&
        "reservoir synthesis must serialize with graph-thread MM bank updates");
    puts("PASS: reservoir prefill holds the shared audio mutex");
    return 0;
  }
  if (mode != "preservation") {
    const bool capacity = mode == "capacity";
    start(3, capacity ? desired : 0);
    const bool released = frame(mode == "shutdown");
    stop();
    if (!released) {
      std::fprintf(stderr,
                   "FAIL: %s request left the graphics consumer asleep until "
                   "the watchdog\n",
                   mode.c_str());
      return 1;
    }
    assert(capacity || mode == "shutdown" ? synthUpdates == 0
                                          : synthUpdates > 0);
    std::printf("PASS: %s releases the real graphics wait\n", mode.c_str());
    return 0;
  }

  start(3);
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  assert(synthUpdates == 0); // No synthesis before the first ready frame.
  stop();
  for (int rate : {1, 2, 3}) {
    start(rate);
    assert(frame());
    assert(awaitRefill());
    for (int stall = 0; stall < 8; ++stall) {
      buffered -= 2400;
      assert(awaitRefill()); // Self-pump without any graphics wake.
    }
    stop();
    const int64_t debt = static_cast<int64_t>(synthFrames) * 3 -
                         static_cast<int64_t>(synthUpdates) * 1600;
    assert(std::abs(debt) <= 48);
    gFscAudioMuted = true;
    start(rate);
    assert(frame());
    assert(awaitRefill());
    assert(midnaResets > 0);
    stop();
    gFscAudioMuted = false;
    start(rate);
    assert(frame());
    assert(awaitRefill());
    stop();
  }
  std::puts("PASS: priming, self-pump, sample cadence, mix-ins, muting and "
            "restart at divisors 1/2/3");
}
