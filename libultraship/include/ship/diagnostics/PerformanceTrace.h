#pragma once
#include <cstdint>
#include <string>
#include <nlohmann/json.hpp>

// Implemented exclusively in the shared engine. No game-DLL callbacks or globals.
namespace Ship::PerformanceTrace {
struct Context { uint64_t tickId = 0; bool enabled = false; };
void BeginTick(uint64_t tickId) noexcept;
nlohmann::json EndTick() noexcept;
bool Enabled() noexcept;
uint64_t Now() noexcept;
Context CaptureContext() noexcept;
class ContextScope {
  public:
    explicit ContextScope(Context context) noexcept;
    ~ContextScope();
    ContextScope(const ContextScope&) = delete;
    ContextScope& operator=(const ContextScope&) = delete;
  private:
    Context previous;
    bool previousBound;
};
void Count(const char* name, uint64_t amount = 1) noexcept;
void Record(const char* category, const std::string& name, uint64_t start, uint64_t end,
            uintptr_t manager = 0, uintptr_t owner = 0, const char* detail = "") noexcept;
// Avoid implicit std::string allocation outside the noexcept boundary in C bridges.
void Record(const char* category, const char* name, uint64_t start, uint64_t end,
            uintptr_t manager = 0, uintptr_t owner = 0, const char* detail = "") noexcept;
class Scope {
  public:
    Scope(const char* category, const std::string& name, uintptr_t manager = 0, uintptr_t owner = 0,
          uint64_t minimumNs = 0) noexcept;
    ~Scope();
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    void Detail(const char* detail) noexcept { mDetail = detail; }
  private:
    Context mContext;
    const char* mCategory;
    std::string mName;
    uintptr_t mManager, mOwner;
    uint64_t mStart = 0, mMinimum;
    const char* mDetail = "";
};
}
