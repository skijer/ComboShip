#pragma once

#include "fast/RenderResourceLookup.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace Fast {

struct RenderCostCounter {
    uint64_t calls = 0;
    uint64_t nanos = 0;
    void Add(uint64_t elapsed) {
        ++calls;
        nanos += elapsed;
    }
};

struct RenderOpcodeCost : RenderCostCounter {
    const char* name = nullptr; // static opcode-table string, never a game resource
};

struct RenderResourceCost {
    std::string path;
    RenderCostCounter commands;
};

struct RenderTextureCost {
    std::string path;
    RenderCostCounter upload;
    uint64_t bytes = 0;
};

struct RenderCostReport {
    bool sampled = false;
    bool altRenderLookup = false;
    RenderResourceLookupStats vertexLookups, displayListLookups;
    uint64_t frameIndex = 0;
    uint64_t attemptId = 0;
    uint32_t gameTick = 0;
    int interpolationIndex = 0;
    float interpolationT = 1.0f;
    uint64_t totalNanos = 0;
    std::array<RenderOpcodeCost, 6 * 256> opcodes{};
    std::vector<RenderResourceCost> resources;
    std::vector<RenderTextureCost> textures;
    uint64_t resourceOverflow = 0;
    RenderCostCounter textureImport, upload, driverDraw, shaderCreate, framebufferSetup, framebufferFinish;
    uint64_t cacheHits = 0, cacheMisses = 0, cacheEvictions = 0;
    uint64_t uploadBytes = 0, vertices = 0, triangles = 0, submittedTriangles = 0;
    // Cumulative since interpreter initialization, including changes between draws.
    uint64_t cacheClearsTotal = 0, cacheDeletesTotal = 0;
    uint64_t cacheEntries = 0;
    std::string backend;
    uint32_t renderWidth = 0, renderHeight = 0, windowWidth = 0, windowHeight = 0, msaa = 0;
};

// Records only one of every 31 requested frames. No resource ownership, cache
// policy, graphics state or command data is changed. Explicit timestamps keep
// sampling and attribution independently testable without a ROM or graphics API.
class RenderCostProbe {
  public:
    // Prime cadence avoids locking onto one phase of the usual 2/3/4/6/8
    // interpolated frames per game tick. Reports also carry their exact phase.
    static constexpr unsigned SampleEvery = 31;
    static constexpr unsigned MaxResources = 512;

    bool BeginFrame(bool enabled, uint64_t now, bool everyFrame = false) {
        active = enabled && ((++requestedFrames - 1) % SampleEvery == 0 || everyFrame);
        report.sampled = active;
        if (!active) {
            return false;
        }
        report = {};
        report.sampled = true;
        report.frameIndex = requestedFrames;
        report.resources.push_back({ "<unmarked-or-overflow>", {} });
        report.textures.push_back({ "<native-mask-or-overflow>", {}, 0 });
        resourceIndex.clear();
        textureIndex.clear();
        ownerStack.clear();
        currentOwner = 0;
        pendingAddress = nullptr;
        pendingResource.clear();
        currentTexture = 0;
        frameStart = now;
        return true;
    }

    void EndFrame(uint64_t now) {
        if (active) {
            report.totalNanos = now >= frameStart ? now - frameStart : 0;
        }
        active = false;
    }

    bool Active() const {
        return active;
    }

    void PrepareResource(const void* address, const std::string& path) {
        if (active && address) { pendingAddress = address; pendingResource = path; }
    }
    void EnterCommand(const void* address) {
        if (pendingAddress == address && pendingAddress != nullptr) SetResource(pendingResource);
        pendingAddress = nullptr;
    }
    void SyncDepth(size_t depth) {
        if (!active) {
            return;
        }
        // A newly called/branched list starts unmarked. Shrinking restores the
        // caller's owner, including returns that discard branch sentinels.
        ownerStack.resize(depth, 0);
        currentOwner = depth == 0 ? 0 : ownerStack.back();
    }

    void SetResource(const std::string& path) {
        if (!active || ownerStack.empty()) {
            return;
        }
        auto found = resourceIndex.find(path);
        if (found != resourceIndex.end()) {
            currentOwner = found->second;
        } else if (report.resources.size() < MaxResources) {
            currentOwner = report.resources.size();
            resourceIndex.emplace(path, currentOwner);
            report.resources.push_back({ path, {} });
        } else {
            ++report.resourceOverflow;
            currentOwner = 0;
        }
        ownerStack.back() = currentOwner;
    }

    void AddCommand(uint8_t ucode, uint8_t opcode, const char* name, uint64_t nanos) {
        if (!active || ucode >= 6) {
            return;
        }
        auto& cost = report.opcodes[static_cast<size_t>(ucode) * 256 + opcode];
        cost.name = name;
        cost.Add(nanos);
        report.resources[currentOwner].commands.Add(nanos);
    }

    size_t SelectTexture(const std::string& path) {
        const size_t previous = currentTexture;
        auto found = textureIndex.find(path);
        if (found != textureIndex.end()) {
            currentTexture = found->second;
        } else if (report.textures.size() < MaxResources) {
            currentTexture = report.textures.size();
            textureIndex.emplace(path, currentTexture);
            report.textures.push_back({ path, {}, 0 });
        } else {
            currentTexture = 0;
        }
        return previous;
    }

    void RestoreTexture(size_t previous) {
        currentTexture = previous;
    }

    void AddUpload(uint64_t bytes, uint64_t nanos) {
        if (!active) {
            return;
        }
        report.upload.Add(nanos);
        report.uploadBytes += bytes;
        auto& texture = report.textures[currentTexture];
        texture.upload.Add(nanos);
        texture.bytes += bytes;
    }

    RenderCostReport& MutableReport() {
        return report;
    }
    const RenderCostReport& Report() const {
        return report;
    }

  private:
    const void* pendingAddress = nullptr;
    std::string pendingResource;
    bool active = false;
    uint64_t requestedFrames = 0, frameStart = 0;
    size_t currentOwner = 0, currentTexture = 0;
    std::vector<size_t> ownerStack;
    std::unordered_map<std::string, size_t> resourceIndex;
    std::unordered_map<std::string, size_t> textureIndex;
    RenderCostReport report;
};

class RenderTextureScope {
  public:
    RenderTextureScope(RenderCostProbe* probe, const std::string& path)
        : probe(probe), previous(probe ? probe->SelectTexture(path) : 0) {
    }
    ~RenderTextureScope() {
        if (probe) {
            probe->RestoreTexture(previous);
        }
    }
    RenderTextureScope(const RenderTextureScope&) = delete;
    RenderTextureScope& operator=(const RenderTextureScope&) = delete;

  private:
    RenderCostProbe* probe;
    size_t previous;
};

inline uint64_t RenderCostNow() {
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

// Driver/import scopes are inclusive and can overlap command timings. A null
// counter performs no clock calls on normal, unsampled frames.
class RenderCostScope {
  public:
    explicit RenderCostScope(RenderCostCounter* target) : target(target), start(target ? RenderCostNow() : 0) {
    }
    ~RenderCostScope() {
        if (target) {
            target->Add(RenderCostNow() - start);
        }
    }
    RenderCostScope(const RenderCostScope&) = delete;
    RenderCostScope& operator=(const RenderCostScope&) = delete;

  private:
    RenderCostCounter* target;
    uint64_t start;
};
} // namespace Fast
