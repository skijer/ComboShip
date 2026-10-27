#include "ship/diagnostics/PerformanceTrace.h"
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>
#include <map>
#include <algorithm>
#include <tuple>
#include <thread>
#include <string_view>

namespace Ship::PerformanceTrace {
namespace {
constexpr size_t MaxEvents = 1024;
constexpr size_t MaxSlowEvents = 256;
constexpr size_t MaxAggregates = 4096;
constexpr size_t MaxCounters = 128;
constexpr size_t MaxPhases = 128;
constexpr size_t MaxActors = 2048;
struct Event {
    uint64_t tick, start, duration, thread;
    uintptr_t manager, owner;
    std::string category, name, detail;
    bool late;
};
std::mutex mutex;
std::atomic<uint64_t> activeTick{0};
std::atomic<bool> active{false};
thread_local Context local;
thread_local bool bound = false;
std::vector<Event> events;
std::map<std::string, uint64_t> counts;
using Key = std::tuple<uint64_t, std::string, std::string, uintptr_t, uintptr_t>;
struct Aggregate { uint64_t count = 0, total = 0, max = 0; };
std::map<Key, Aggregate> aggregates, phases;
struct ActorAggregate { Aggregate update, draw; };
std::map<Key, ActorAggregate> actors;
uint64_t phaseOverflow = 0, actorOverflow = 0;
bool IsFixedPhase(std::string_view category) {
    return category == "play_draw" || category == "play_draw_detail" || category == "play_update" ||
           category == "actor_draw_all" || category == "actor_draw_detail" ||
           category == "actor_update_detail" || category == "weather";
}
void Accumulate(Aggregate& value, uint64_t duration) {
    ++value.count;
    value.total += duration;
    value.max = std::max(value.max, duration);
}
uint64_t overflow = 0, counterOverflow = 0, aggregateOverflow = 0, lateEvents = 0, lateCounters = 0;
size_t resourceEvents = 0, slowEvents = 0, lifetimePeakRecords = 0;
std::atomic<uint64_t> recordNanos{0}, countNanos{0}, recordCalls{0}, countCalls{0}, collectionErrors{0}, outstandingScopes{0};
struct CollectorTimer {
    std::atomic<uint64_t>& nanos;
    std::atomic<uint64_t>& calls;
    uint64_t start = Now();
    ~CollectorTimer() {
        nanos.fetch_add(Now() - start, std::memory_order_relaxed);
        calls.fetch_add(1, std::memory_order_relaxed);
    }
};
void Add(Context context, const char* category, std::string_view name, uint64_t start, uint64_t end,
         uintptr_t manager, uintptr_t owner, const char* detail, bool emitEvent = true) noexcept {
    if (!context.enabled) return;
    CollectorTimer timer{recordNanos, recordCalls};
    try {
        std::lock_guard<std::mutex> lock(mutex);
        bool late = !active.load() || context.tickId != activeTick.load();
        if (late) ++lateEvents;
        const auto duration = end >= start ? end - start : 0;
        Key key{context.tickId, std::string(std::string_view(category ? category : "").substr(0, 128)),
                std::string(name.substr(0, 512)), manager, owner};
        const auto categoryView = std::string_view(category ? category : "");
        const bool fixed = IsFixedPhase(categoryView);
        auto& destination = fixed ? phases : aggregates;
        const auto limit = fixed ? MaxPhases : MaxAggregates;
        auto it = destination.find(key);
        if (it == destination.end() && destination.size() < limit) it = destination.emplace(key, Aggregate{}).first;
        if (it != destination.end()) Accumulate(it->second, duration);
        else if (fixed) ++phaseOverflow;
        else ++aggregateOverflow;
        const bool actorUpdate = categoryView == "actor_update" || categoryView == "actor.update";
        const bool actorDraw = categoryView == "actor_draw" || categoryView == "actor.draw";
        if (actorUpdate || actorDraw) {
            // Rank actor identity as one unit, retaining both update and draw even
            // when one is much cheaper. Resource bursts cannot consume this reserve.
            Key actorKey = key;
            std::get<1>(actorKey) = "actor";
            auto actor = actors.find(actorKey);
            if (actor == actors.end() && actors.size() < MaxActors)
                actor = actors.emplace(std::move(actorKey), ActorAggregate{}).first;
            if (actor != actors.end()) Accumulate(actorUpdate ? actor->second.update : actor->second.draw, duration);
            else ++actorOverflow;
        }
        const bool resource = std::string_view(category ? category : "").starts_with("resource.");
        if (!emitEvent || (!resource && duration < 2000000)) return;
        if ((resource && resourceEvents >= MaxEvents) || (!resource && slowEvents >= MaxSlowEvents)) {
            ++overflow; return;
        }
        if (resource) ++resourceEvents; else ++slowEvents;
        events.push_back({ context.tickId, start, end >= start ? end - start : 0,
                           static_cast<uint64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id())), manager, owner,
                           std::string(std::string_view(category ? category : "").substr(0, 128)), std::string(name.substr(0, 512)),
                           std::string(std::string_view(detail ? detail : "").substr(0, 128)), late });
        lifetimePeakRecords = std::max(lifetimePeakRecords, events.size());
    } catch (...) { collectionErrors.fetch_add(1, std::memory_order_relaxed); }
}
}
uint64_t Now() noexcept {
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}
Context CaptureContext() noexcept {
    if (bound) return local;
    if (!active.load(std::memory_order_acquire)) return {};
    return { activeTick.load(), true };
}
bool Enabled() noexcept { return CaptureContext().enabled; }
void BeginTick(uint64_t tickId) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex);
        activeTick.store(tickId);
        active.store(true, std::memory_order_release);
    } catch (...) { collectionErrors.fetch_add(1, std::memory_order_relaxed); }
}
nlohmann::json EndTick() noexcept {
    try {
        std::vector<Event> output;
        std::map<std::string, uint64_t> counters;
        std::map<Key, Aggregate> summary, phaseSummary;
        std::map<Key, ActorAggregate> actorSummary;
        uint64_t dropped, droppedCounters, droppedAggregates, droppedPhases, droppedActors, late, lateCount, tick, peakRecords;
        uint64_t measuredRecordNs, measuredCountNs, measuredRecordCalls, measuredCountCalls, errors, outstanding;
        {
            std::lock_guard<std::mutex> lock(mutex);
            tick = activeTick.load();
            measuredRecordNs = recordNanos.exchange(0);
            measuredCountNs = countNanos.exchange(0);
            measuredRecordCalls = recordCalls.exchange(0);
            measuredCountCalls = countCalls.exchange(0);
            errors = collectionErrors.exchange(0);
            outstanding = outstandingScopes.load(std::memory_order_relaxed);
            active.store(false, std::memory_order_release);
            output.swap(events);
            counters.swap(counts);
            summary.swap(aggregates);
            phaseSummary.swap(phases);
            actorSummary.swap(actors);
            droppedPhases = phaseOverflow; phaseOverflow = 0;
            droppedActors = actorOverflow; actorOverflow = 0;
            resourceEvents = slowEvents = 0;
            peakRecords = lifetimePeakRecords;
            droppedAggregates = aggregateOverflow; aggregateOverflow = 0;
            droppedCounters = counterOverflow; counterOverflow = 0;
            dropped = overflow; late = lateEvents; lateCount = lateCounters;
            overflow = lateEvents = lateCounters = 0;
        }
        auto rows = nlohmann::json::array();
        for (const auto& e : output) {
            rows.push_back({ {"origin_tick", e.tick}, {"category", e.category}, {"name", e.name},
                             {"thread_id", e.thread}, {"start_ns", e.start}, {"duration_ns", e.duration}, {"manager", e.manager},
                             {"cache_owner", e.owner}, {"detail", e.detail}, {"late", e.late || e.tick != tick} });
        }
        std::vector<std::pair<Key, Aggregate>> sorted(summary.begin(), summary.end());
        std::stable_sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) {
            return a.second.total > b.second.total;
        });
        auto totals = nlohmann::json::array();
        auto phaseTotals = nlohmann::json::array();
        auto row = [](const Key& key, const Aggregate& a) {
            const auto& [origin, category, name, manager, owner] = key;
            return nlohmann::json{{"origin_tick", origin}, {"category", category}, {"name", name},
                                  {"manager", manager}, {"cache_owner", owner}, {"count", a.count},
                                  {"total_ns", a.total}, {"max_ns", a.max}};
        };
        for (size_t i = 0; i < std::min<size_t>(64, sorted.size()); ++i)
            totals.push_back(row(sorted[i].first, sorted[i].second));
        for (const auto& [key, value] : phaseSummary) phaseTotals.push_back(row(key, value));
        std::vector<std::pair<Key, ActorAggregate>> sortedActors(actorSummary.begin(), actorSummary.end());
        std::stable_sort(sortedActors.begin(), sortedActors.end(), [](const auto& a, const auto& b) {
            return a.second.update.total + a.second.draw.total > b.second.update.total + b.second.draw.total;
        });
        auto actorTotals = nlohmann::json::array();
        for (size_t i = 0; i < std::min<size_t>(64, sortedActors.size()); ++i) {
            const auto& [key, a] = sortedActors[i];
            const auto& [origin, category, name, manager, owner] = key;
            actorTotals.push_back({{"origin_tick", origin}, {"category", category}, {"name", name},
                                   {"manager", manager}, {"cache_owner", owner},
                                   {"update_count", a.update.count}, {"draw_count", a.draw.count},
                                   {"update_ns", a.update.total}, {"draw_ns", a.draw.total},
                                   {"update_max_ns", a.update.max}, {"draw_max_ns", a.draw.max}});
        }
        return {{"collector_record_ns", measuredRecordNs}, {"collector_count_ns", measuredCountNs},
                {"collector_record_calls", measuredRecordCalls}, {"collector_count_calls", measuredCountCalls},
                {"collector_errors", errors}, {"outstanding_scopes", outstanding}, {"peak_records", output.size()},
                {"lifetime_peak_records", peakRecords}, {"peak_aggregate_keys", summary.size()},
                {"collector_semantics", "summed wall ns including lock waits; workers overlap; excludes JSON finalization"},
                {"phase_aggregates", std::move(phaseTotals)}, {"phase_keys", phaseSummary.size()},
                {"phase_key_limit", MaxPhases}, {"overflow_phases", droppedPhases},
                {"actor_aggregates", std::move(actorTotals)}, {"actor_keys", actorSummary.size()},
                {"actor_key_limit", MaxActors}, {"overflow_actors", droppedActors},
                {"actor_keys_omitted", sortedActors.size() > 64 ? sortedActors.size() - 64 : 0},
                {"aggregates", std::move(totals)}, {"aggregate_keys", summary.size()},
                {"aggregate_keys_omitted", sorted.size() > 64 ? sorted.size() - 64 : 0},
                {"overflow_aggregates", droppedAggregates}, {"tick", tick}, {"events", std::move(rows)}, {"counts", counters},
                {"overflow_events", dropped}, {"overflow_counters", droppedCounters}, {"late_events", late}, {"late_counters", lateCount},
                {"resource_event_limit", MaxEvents}, {"slow_event_limit", MaxSlowEvents}, {"counter_limit", MaxCounters},
                {"duration_semantics", "inclusive; resource stages may overlap enclosing waits"},
                {"counter_semantics", "collection-window totals; late_counters counts calls from closed origin ticks"}};
    } catch (...) {
        active.store(false, std::memory_order_release);
        collectionErrors.fetch_add(1, std::memory_order_relaxed);
        // Null is allocation-free and tells the caller this trace could not be serialized.
        return nlohmann::json();
    }
}
ContextScope::ContextScope(Context context) noexcept : previous(local), previousBound(bound) {
    local = context; bound = true;
}
ContextScope::~ContextScope() { local = previous; bound = previousBound; }
void Count(const char* name, uint64_t amount) noexcept {
    const auto context = CaptureContext();
    if (!context.enabled) return;
    CollectorTimer timer{countNanos, countCalls};
    try {
        std::lock_guard<std::mutex> lock(mutex);
        if (!active.load() || context.tickId != activeTick.load()) ++lateCounters;
        const auto key = std::string(std::string_view(name ? name : "").substr(0, 128));
        auto it = counts.find(key);
        if (it != counts.end()) it->second += amount;
        else if (counts.size() < MaxCounters) counts.emplace(key, amount);
        else ++counterOverflow;
    } catch (...) { collectionErrors.fetch_add(1, std::memory_order_relaxed); }
}
void Record(const char* category, const std::string& name, uint64_t start, uint64_t end,
            uintptr_t manager, uintptr_t owner, const char* detail) noexcept {
    Add(CaptureContext(), category, name, start, end, manager, owner, detail);
}
void Record(const char* category, const char* name, uint64_t start, uint64_t end,
            uintptr_t manager, uintptr_t owner, const char* detail) noexcept {
    Add(CaptureContext(), category, std::string_view(name ? name : ""), start, end, manager, owner, detail);
}
Scope::Scope(const char* category, const std::string& name, uintptr_t manager, uintptr_t owner,
             uint64_t minimumNs) noexcept
    : mContext(CaptureContext()), mCategory(category), mManager(manager), mOwner(owner), mMinimum(minimumNs) {
    if (mContext.enabled) {
        try {
            mName = name.substr(0, 512);
            mStart = Now();
            outstandingScopes.fetch_add(1, std::memory_order_relaxed);
        } catch (...) { mContext.enabled = false; collectionErrors.fetch_add(1, std::memory_order_relaxed); }
    }
}
Scope::~Scope() {
    if (!mContext.enabled) return;
    const auto end = Now();
    Add(mContext, mCategory, mName, mStart, end, mManager, mOwner, mDetail, end - mStart >= mMinimum);
    outstandingScopes.fetch_sub(1, std::memory_order_relaxed);
}
}
