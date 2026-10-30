#include "GameFiber.h"

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#else
#include <ucontext.h>
#endif

namespace {

// The games were written for a main thread's stack; deep call chains and large locals need the same room.
constexpr size_t kStackBytes = 64 * 1024 * 1024;

} // namespace

#ifdef _WIN32

namespace {
void* sMainFiber = nullptr;
} // namespace

struct GameFiber::Impl {
    std::function<void()> body;
    void* fiber = nullptr;
};

static void CALLBACK RunFiberBody(void* parameter) {
    static_cast<std::function<void()>*>(parameter)->operator()();
    // A fiber that returns ends its thread, so a finished body hands control back for good.
    SwitchToFiber(sMainFiber);
}

GameFiber::GameFiber(std::function<void()> body) : mImpl(std::make_unique<Impl>()) {
    mImpl->body = std::move(body);
    mImpl->fiber = CreateFiberEx(1024 * 1024, kStackBytes, FIBER_FLAG_FLOAT_SWITCH, RunFiberBody, &mImpl->body);
    if (mImpl->fiber == nullptr) {
        throw std::runtime_error("could not create a game fiber");
    }
}

GameFiber::~GameFiber() {
    DeleteFiber(mImpl->fiber);
}

void GameFiber::SwitchTo() {
    SwitchToFiber(mImpl->fiber);
}

void GameFiber::SwitchToMain() {
    SwitchToFiber(sMainFiber);
}

void GameFiber::EnterMainThread() {
    sMainFiber = ConvertThreadToFiberEx(nullptr, FIBER_FLAG_FLOAT_SWITCH);
    if (sMainFiber == nullptr) {
        throw std::runtime_error("could not prepare the main thread for game fibers");
    }
}

#else

namespace {
ucontext_t sMainContext;
ucontext_t* sCurrentContext = &sMainContext;
} // namespace

struct GameFiber::Impl {
    std::function<void()> body;
    std::vector<char> stack;
    ucontext_t context;
};

static void RunFiberBody(int high, int low) {
    const uintptr_t address = (static_cast<uintptr_t>(static_cast<unsigned>(high)) << 32) |
                              static_cast<uintptr_t>(static_cast<unsigned>(low));
    reinterpret_cast<std::function<void()>*>(address)->operator()();
    GameFiber::SwitchToMain();
}

GameFiber::GameFiber(std::function<void()> body) : mImpl(std::make_unique<Impl>()) {
    mImpl->body = std::move(body);
    mImpl->stack.resize(kStackBytes);
    getcontext(&mImpl->context);
    mImpl->context.uc_stack.ss_sp = mImpl->stack.data();
    mImpl->context.uc_stack.ss_size = mImpl->stack.size();
    mImpl->context.uc_link = &sMainContext;
    // makecontext only passes ints, so the body's address travels in two halves.
    const uintptr_t address = reinterpret_cast<uintptr_t>(&mImpl->body);
    makecontext(&mImpl->context, reinterpret_cast<void (*)()>(RunFiberBody), 2, static_cast<int>(address >> 32),
                static_cast<int>(address & 0xFFFFFFFFu));
}

GameFiber::~GameFiber() = default;

void GameFiber::SwitchTo() {
    ucontext_t* previous = sCurrentContext;
    sCurrentContext = &mImpl->context;
    swapcontext(previous, &mImpl->context);
}

void GameFiber::SwitchToMain() {
    ucontext_t* previous = sCurrentContext;
    sCurrentContext = &sMainContext;
    swapcontext(previous, &sMainContext);
}

void GameFiber::EnterMainThread() {
    sCurrentContext = &sMainContext;
}

#endif
