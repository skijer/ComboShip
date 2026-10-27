#pragma once
#include <nlohmann/json.hpp>
#include <algorithm>
#include <mutex>
#include <string>
#include <vector>

namespace ComboRando::CrossDungeon {
// Native save workers may serialize while the main thread acknowledges an item. Check names
// survive native Context reconstruction; no GetItemEntry or pointer crosses the host handoff.
class PendingChecks {
  public:
    void Capture(const std::vector<std::string>& names) {
        nlohmann::json payload = names;
        Load(payload);
    }
    void Load(const nlohmann::json& payload) {
        std::vector<std::string> next;
        if (payload.is_array() && payload.size() <= 4096) {
            for (const auto& name : payload) {
                if (!name.is_string() || name.get_ref<const std::string&>().empty() ||
                    name.get_ref<const std::string&>().size() > 256) {
                    next.clear();
                    break;
                }
                const auto value = name.get<std::string>();
                if (std::find(next.begin(), next.end(), value) == next.end())
                    next.push_back(value);
            }
        }
        std::lock_guard lock(mutex);
        pending = std::move(next);
        restored = false;
    }
    nlohmann::json Save() const {
        std::lock_guard lock(mutex);
        return pending;
    }
    std::vector<std::string> RestoreOnce() {
        std::lock_guard lock(mutex);
        if (restored)
            return {};
        restored = true;
        return pending;
    }
    void Acknowledge(const std::string& name) {
        std::lock_guard lock(mutex);
        pending.erase(std::remove(pending.begin(), pending.end(), name), pending.end());
    }

  private:
    mutable std::mutex mutex;
    std::vector<std::string> pending;
    bool restored = false;
};
} // namespace ComboRando::CrossDungeon
