#pragma once
// Observe raw ownership only. Do not call extract/reconcile/progressive resolvers here.
#include <array>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <type_traits>

namespace ItemGrantAudit {
struct Field {
    const char* name = nullptr;
    size_t index = 0;
    uint64_t value = 0;
    uint64_t empty = 0;
};
struct Snapshot {
    // 512 obtained + 512 applied + 128 legacy counts + inventory/custom flags.
    std::array<Field, 2048> fields;
    size_t size = 0;
    int file = -1;
    int mode = -1;
    int saveType = -1;
    uint64_t seed = 0;
    bool overflow = false;
    void Add(const char* name, uint64_t value, size_t index = 0, uint64_t empty = 0) {
        if (size == fields.size()) {
            overflow = true;
            return;
        }
        fields[size++] = { name, index, value, empty };
    }
    template <typename T, size_t N> void Array(const char* name, const T (&values)[N], uint64_t empty = 0) {
        for (size_t i = 0; i < N; ++i) {
            // Keep signed byte counters at their native width (e.g. absent keys = 0xff).
            Add(name, static_cast<typename std::make_unsigned<T>::type>(values[i]), i, empty);
        }
    }
};
class Tracker {
    Snapshot previous;
    bool initialized = false;
    unsigned suspensionDepth = 0;
    uint64_t samples = 0, changes = 0, sequence = 0, checkpoints = 0;
    struct Origin {
        const char* name;
        int item;
        int check;
        bool quiet;
    };
    std::array<Origin, 64> origins;
    size_t depth = 0;
    size_t excessDepth = 0;

  public:
    template <typename Sink> void Observe(const Snapshot& next, const char* phase, Sink sink, bool force = false) {
        if (suspensionDepth != 0) {
            return;
        }
        ++samples;
        const bool baseline = !initialized || next.file != previous.file || next.saveType != previous.saveType ||
                              next.seed != previous.seed;
        const bool context = baseline || next.mode != previous.mode;
        size_t changed = 0;
        bool schemaChanged = initialized && next.size != previous.size;
        for (size_t i = 0; i < next.size; ++i) {
            const auto& f = next.fields[i];
            const bool sameField = initialized && i < previous.size && f.name == previous.fields[i].name &&
                                   f.index == previous.fields[i].index;
            schemaChanged |= initialized && !sameField;
            const auto before = baseline || !sameField ? f.empty : previous.fields[i].value;
            changed += f.value != before;
        }
        changes += baseline ? 0 : changed;
        if (changed || context || force || next.overflow || schemaChanged) {
            std::ostringstream record;
            record << "seq=" << ++sequence << " phase=" << phase << " file=" << next.file << " mode=" << next.mode
                   << " saveType=" << next.saveType << " seed=" << next.seed << " baseline=" << baseline
                   << " fields=" << next.size << " samples=" << samples << " changes=" << changes
                   << " overflow=" << next.overflow << " schemaChanged=" << schemaChanged << " kind="
                   << (baseline  ? "state-present"
                       : changed ? "ownership-delta"
                                 : "coverage")
                   << " origins=";
            if (depth == 0) {
                record << "unscoped-interval";
            }
            for (size_t i = 0; i < depth; ++i) {
                const auto& origin = origins[i];
                record << origin.name << '(' << origin.item << ',' << origin.check << ")/";
            }
            if (excessDepth != 0) {
                record << "scope-overflow/";
            }
            for (size_t i = 0; i < next.size; ++i) {
                const auto& f = next.fields[i];
                const bool sameField = initialized && i < previous.size && f.name == previous.fields[i].name &&
                                       f.index == previous.fields[i].index;
                const auto before = baseline || !sameField ? f.empty : previous.fields[i].value;
                if (f.value != before) {
                    record << ' ' << f.name << '[' << f.index << "]=0x" << std::hex << before << "->0x" << f.value
                           << std::dec;
                }
            }
            sink(record.str());
        }
        // Copy only captured fields; unused capacity is deliberately uninitialized.
        for (size_t i = 0; i < next.size; ++i) {
            previous.fields[i] = next.fields[i];
        }
        previous.size = next.size;
        previous.file = next.file;
        previous.mode = next.mode;
        previous.saveType = next.saveType;
        previous.seed = next.seed;
        initialized = true;
    }
    template <typename Sink> void Checkpoint(const Snapshot& s, const char* phase, Sink sink) {
        if (suspensionDepth == 0) {
            Observe(s, phase, sink, ++checkpoints % 600 == 0);
        }
    }
    template <typename Sink>
    void Begin(const Snapshot& s, const char* name, int item, int check, bool quiet, Sink sink) {
        // Flush earlier unexplained writes BEFORE adding the incoming scope's attribution.
        Observe(s, "before", sink);
        if (depth == origins.size() || excessDepth != 0) {
            ++excessDepth;
            return;
        }
        origins[depth++] = { name, item, check, quiet };
        if (!quiet) {
            Observe(s, "enter", sink, true);
        }
    }
    template <typename Sink> void End(const Snapshot& s, Sink sink) {
        if (excessDepth != 0) {
            Observe(s, "after", sink, true);
            --excessDepth;
            return;
        }
        if (depth == 0) {
            sink("ERROR unbalanced scope");
            return;
        }
        Observe(s, "after", sink, !origins[depth - 1].quiet);
        --depth;
    }
    template <typename Sink> void Suspend(const Snapshot& s, Sink sink) {
        Observe(s, "oracle-live-before", sink, true);
        ++suspensionDepth;
    }
    template <typename Sink> void Resume(const Snapshot& s, Sink sink) {
        if (suspensionDepth == 0) {
            sink("ERROR resume without suspension");
            return;
        }
        --suspensionDepth;
        // Compare restored live state to the live snapshot, never to oracle scratch ownership.
        Observe(s, "oracle-live-restored", sink, true);
    }
};
} // namespace ItemGrantAudit
