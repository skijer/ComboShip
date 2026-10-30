#pragma once

#include <functional>
#include <memory>

// A stack of its own for one game's loop, on the main thread. Only one fiber runs at a time: switching parks
// the current one mid-call and resumes the target where it last stopped, so the window, the graphics context
// and the event queue stay on the thread that created them.
class GameFiber {
  public:
    explicit GameFiber(std::function<void()> body);
    ~GameFiber();
    GameFiber(const GameFiber&) = delete;
    GameFiber& operator=(const GameFiber&) = delete;

    // Assumes the calling thread is the main thread and was prepared with EnterMainThread.
    void SwitchTo();
    // Leaves the current fiber for the main thread's own context.
    static void SwitchToMain();
    static void EnterMainThread();

  private:
    struct Impl;
    std::unique_ptr<Impl> mImpl;
};
