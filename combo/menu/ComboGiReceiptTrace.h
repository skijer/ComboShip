// Bounded receipt diagnostics for the MM GI regression candidate. Record the
// actual dispatch and fit outcome once per selected recipe, without changing
// the draw or substituting another mesh when a graph cannot be measured.
#ifndef COMBO_GI_RECEIPT_TRACE_H
#define COMBO_GI_RECEIPT_TRACE_H
#include <string>
#include <unordered_set>
#include <spdlog/spdlog.h>

namespace ComboGiReceiptTrace {
inline bool First(const std::string& key) {
    static std::unordered_set<std::string> seen;
    return seen.size() < 512 && seen.insert(key).second;
}
inline std::string Paths(const char* const* paths, int count) {
    std::string result;
    for (int i = 0; paths && i < count && i < 16; ++i) {
        if (i)
            result += " | ";
        result += paths[i] ? paths[i] : "<missing>";
    }
    return result;
}
inline void Native(int item, int pickup) {
    if (!pickup)
        return;
    const std::string key = "native:" + std::to_string(item) + ":" + std::to_string(pickup);
    if (First(key))
        SPDLOG_INFO("[ComboGI] receipt nativeMM={} pickup={} route=legacy-or-native", item, pickup);
}
inline void Dispatch(const char* name, int kind, const char* const* paths, int count, float scale, int nativeItem,
                     int pickup) {
    if (!pickup)
        return;
    const auto roots = Paths(paths, count);
    const std::string key = "draw:" + std::string(name ? name : "<native>") + ":" + std::to_string(kind) + ":" +
                            std::to_string(nativeItem) + ":" + roots;
    if (First(key))
        SPDLOG_INFO("[ComboGI] receipt name={} kind={} nativeMM={} pickup={} scale={} roots={}",
                    name ? name : "<native>", kind, nativeItem, pickup, scale, roots);
}
inline void Fit(const char* owner, const char* const* paths, int count, float scale, float tilt, int presentation,
                bool fitted, const float fit[2]) {
    if (presentation < 2)
        return;
    const auto roots = Paths(paths, count);
    const std::string key = "fit:" + std::string(owner ? owner : "<missing>") + ":" + roots + ":" +
                            std::to_string(presentation) + ":" + std::to_string(scale) + ":" + std::to_string(tilt) +
                            ":" + std::to_string(fitted) + ":" + std::to_string(fit[0]) + ":" + std::to_string(fit[1]);
    if (First(key))
        SPDLOG_INFO(
            "[ComboGI] receipt fit owner={} context={} measured={} inputScale={} tilt={} fitScale={} lift={} roots={}",
            owner ? owner : "<missing>", presentation, fitted, scale, tilt, fit[0], fit[1], roots);
}
} // namespace ComboGiReceiptTrace
#endif
