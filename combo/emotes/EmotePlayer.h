#pragma once
#include "EmoteCatalog.h"
#include <bit>
#include <cmath>
#include <cstring>
#include <span>

namespace ComboEmotes {
struct Pose {
    std::array<int16_t, kJointValues> joints{};
    int16_t appearance = 0;
};

template <typename NativePlayer> inline void ClearDrawOffsets(NativePlayer& player) {
    player.headLimbRot = {};
    player.upperLimbRot = {};
    player.upperLimbYawSecondary = 0;
}

inline bool DecodeFrame(std::span<const uint8_t> bytes, size_t frame, Pose& out) {
    if (bytes.empty() || bytes.size() % kFrameBytes || frame >= bytes.size() / kFrameBytes)
        return false;
    const auto* data = bytes.data() + frame * kFrameBytes;
    for (size_t i = 0; i < kJointValues; ++i) {
        out.joints[i] = std::bit_cast<int16_t>(static_cast<uint16_t>((data[2 * i] << 8) | data[2 * i + 1]));
    }
    out.appearance = std::bit_cast<int16_t>(static_cast<uint16_t>((data[132] << 8) | data[133]));
    const uint16_t appearance = static_cast<uint16_t>(out.appearance);
    // Both human player renderers index fixed face/hand display-list tables.
    return (appearance & 0xf) <= 8 && ((appearance >> 4) & 0xf) <= 4 && ((appearance >> 8) & 0xf) <= 2 &&
           ((appearance >> 12) & 0xf) <= 2;
}

inline bool ValidateClip(std::span<const uint8_t> bytes, size_t frames) {
    if (!frames || bytes.size() / kFrameBytes != frames || bytes.size() % kFrameBytes)
        return false;
    Pose pose;
    for (size_t frame = 0; frame < frames; ++frame)
        if (!DecodeFrame(bytes, frame, pose))
            return false;
    return true;
}

// Both MM's aligned raw joint buffers and OoT's Vec3s arrays use native endian
// values in RAM. memcpy also handles MM buffers with non-aligned byte starts.
inline bool WritePose(std::span<uint8_t> buffer, size_t offset, const Pose& pose) {
    if (offset > buffer.size() || buffer.size() - offset < kFrameBytes)
        return false;
    std::memcpy(buffer.data() + offset, pose.joints.data(), kJointValues * sizeof(int16_t));
    std::memcpy(buffer.data() + offset + 132, &pose.appearance, sizeof(int16_t));
    return true;
}

inline int SelectBinding(const std::array<uint32_t, kSlots>& chords, uint32_t held, uint32_t pressed) {
    int selected = -1;
    int specificity = 0;
    for (size_t i = 0; i < chords.size(); ++i) {
        const uint32_t chord = chords[i];
        const int bits = std::popcount(chord);
        if (chord && !(chord & ~uint32_t(0xefff)) && (held & chord) == chord && (pressed & chord) &&
            bits > specificity) {
            selected = static_cast<int>(i);
            specificity = bits;
        }
    }
    return selected;
}

class Playback {
  public:
    bool Start(int id, size_t frames) {
        Stop();
        if (!Find(id) || frames == 0)
            return false;
        mId = id;
        mFrames = frames;
        return true;
    }
    void Tick(double seconds) {
        if (!Active())
            return;
        if (!std::isfinite(seconds) || seconds < 0) {
            Stop();
            return;
        }
        mTime += seconds;
        if (mTime * 20.0 + 1e-9 >= static_cast<double>(mFrames))
            Stop();
    }
    void Stop() {
        mId = 0;
        mFrames = 0;
        mTime = 0;
    }
    bool Active() const {
        return mId != 0;
    }
    int Id() const {
        return mId;
    }
    size_t Frame() const {
        return Active() ? static_cast<size_t>(mTime * 20.0 + 1e-9) : 0;
    }

  private:
    int mId = 0;
    size_t mFrames = 0;
    double mTime = 0;
};
} // namespace ComboEmotes
