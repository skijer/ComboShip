#pragma once
#include "ComboGameExport.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

// Skijer's NEIWOLF1 mesh/TRS export; v2 adds an optional PCM sound chunk.
// Both hosts use the same bounded validation before registering an SSBB model.
namespace NeiWolfAsset {
inline constexpr char kMagic[8] = { 'N', 'E', 'I', 'W', 'O', 'L', 'F', '1' };
inline constexpr uint32_t kMinVersion = 1, kMaxVersion = 2;
inline constexpr size_t kHeaderSize = 88, kAudioHeaderSize = 92;
inline constexpr size_t kMaxBlobSize = 64 * 1024 * 1024;
inline constexpr uint32_t kMaxBones = 64;
inline constexpr char kResourcePath[] = "objects/forms/wolf_link/gWolfLinkData";
inline constexpr char kHDResourcePath[] = "objects/forms/wolf_link/gWolfLinkHDData";
inline constexpr char kAltResourcePath[] = "alt/objects/forms/wolf_link/gWolfLinkData";
inline constexpr char kAltHDResourcePath[] = "alt/objects/forms/wolf_link/gWolfLinkHDData";
inline constexpr char kHDModelCVar[] = "gMods.WolfLink.UseHDModel";

inline uint16_t ReadU16(const uint8_t* p) {
    return (uint16_t)p[0] | (uint16_t)p[1] << 8;
}
inline int16_t ReadS16(const uint8_t* p) {
    return (int16_t)ReadU16(p);
}
inline uint32_t ReadU32(const uint8_t* p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
inline float ReadF32(const uint8_t* p) {
    const uint32_t bits = ReadU32(p);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}
// Check IEEE-754 bits so fast-math cannot optimize away non-finite rejection.
inline bool FloatOk(const uint8_t* p, float maxAbs) {
    const uint32_t bits = ReadU32(p) & 0x7FFFFFFFu;
    uint32_t limit;
    std::memcpy(&limit, &maxAbs, sizeof(limit));
    return bits < 0x7F800000u && bits <= limit;
}

// A loaded LUS Blob may have its standard 16-byte zero padding. The serialized
// total length remains authoritative; arbitrary extra data is never accepted.
inline size_t ResourcePayloadSize(const uint8_t* data, size_t size) {
    if (!data || size < kHeaderSize || size > kMaxBlobSize + 16 || std::memcmp(data, kMagic, 8) != 0)
        return 0;
    const uint32_t version = ReadU32(data + 8), total = ReadU32(data + 84);
    if (version < kMinVersion || version > kMaxVersion || total < (version == 2 ? kAudioHeaderSize : kHeaderSize) ||
        total > kMaxBlobSize || (size != total && size != (size_t)total + 16))
        return 0;
    for (size_t i = total; i < size; ++i)
        if (data[i] != 0)
            return 0;
    return total;
}

inline bool Validate(const std::vector<uint8_t>& blob) {
    if (blob.size() < kHeaderSize || blob.size() > kMaxBlobSize || std::memcmp(blob.data(), kMagic, 8) != 0 ||
        ReadU32(blob.data() + 8) < kMinVersion || ReadU32(blob.data() + 8) > kMaxVersion)
        return false;
    const size_t headerSize = ReadU32(blob.data() + 8) == 2 ? kAudioHeaderSize : kHeaderSize;
    if (blob.size() < headerSize)
        return false;
    const auto range = [&](uint32_t offset, uint64_t size) {
        return offset <= blob.size() && size <= blob.size() - offset;
    };
    const uint8_t* h = blob.data() + 12;
    auto value = [h](uint32_t i) { return ReadU32(h + i * 4); };
    const uint32_t vertices = value(0), triangles = value(1), bones = value(2), anims = value(3);
    const uint32_t width = value(4), height = value(5);
    if (!vertices || vertices > 65535 || vertices % 3 || triangles != vertices / 3 || !bones || bones > kMaxBones ||
        !anims || anims > 65535 || value(18) != blob.size() || width < 8 || width > 1024 || height < 8 ||
        height > 1024 || (width & (width - 1)) || (height & (height - 1)) || value(17) != width * height * 2)
        return false;
    struct Chunk {
        uint32_t offset;
        uint64_t size;
    };
    const Chunk chunks[] = { { value(6), (uint64_t)vertices * 20 },
                             { value(7), (uint64_t)vertices * 8 },
                             { value(8), (uint64_t)bones * 2 },
                             { value(9), (uint64_t)bones * 64 },
                             { value(10), (uint64_t)bones * 12 },
                             { value(11), (uint64_t)anims * 16 },
                             { value(12), value(13) },
                             { value(14), value(15) },
                             { value(16), value(17) } };
    for (size_t i = 0; i < sizeof(chunks) / sizeof(chunks[0]); ++i) {
        const Chunk& c = chunks[i];
        if (!c.size || c.offset < headerSize || !range(c.offset, c.size))
            return false;
        for (size_t j = 0; j < i; ++j)
            if (c.offset < chunks[j].offset + chunks[j].size && chunks[j].offset < c.offset + c.size)
                return false;
    }
    if ((value(9) | value(10) | value(11) | value(14)) & 3u)
        return false;
    for (uint32_t i = 0; i < vertices; ++i) {
        const uint8_t* v = blob.data() + value(6) + i * 20;
        if (!FloatOk(v, 32767.0f) || !FloatOk(v + 4, 32767.0f) || !FloatOk(v + 8, 32767.0f))
            return false;
        const uint8_t* w = blob.data() + value(7) + i * 8;
        uint32_t sum = 0;
        bool zero = false;
        for (uint32_t j = 0; j < 4; ++j) {
            if (w[j] >= bones || (zero && w[4 + j]))
                return false;
            zero |= w[4 + j] == 0;
            sum += w[4 + j];
        }
        if (sum != 255)
            return false;
    }
    for (uint32_t i = 0; i < bones; ++i) {
        int16_t parent = ReadS16(blob.data() + value(8) + i * 2);
        // Exporter armature traversal orders parents before children and has one root at 0.
        // This also rules out recursion cycles and disconnected subtrees.
        if (i == 0 ? parent != -1 : parent < 0 || (uint32_t)parent >= i)
            return false;
        for (uint32_t j = 0; j < 16; ++j)
            if (!FloatOk(blob.data() + value(9) + i * 64 + j * 4, 32767.0f))
                return false;
        for (uint32_t j = 0; j < 3; ++j)
            if (!FloatOk(blob.data() + value(10) + i * 12 + j * 4, 32767.0f))
                return false;
    }
    for (uint32_t i = 0; i < anims; ++i) {
        const uint8_t* e = blob.data() + value(11) + i * 16;
        uint32_t name = ReadU32(e), start = ReadU32(e + 12);
        uint16_t count = ReadU16(e + 4), animBones = ReadU16(e + 6);
        uint64_t size = (uint64_t)count * animBones * 36;
        if (!count || animBones != bones || name < value(12) || (uint64_t)name >= (uint64_t)value(12) + value(13) ||
            !std::memchr(blob.data() + name, 0, value(12) + value(13) - name) || !FloatOk(e + 8, 240.0f) ||
            ReadF32(e + 8) <= 0 || (start & 3u) || start < value(14) ||
            (uint64_t)start + size > (uint64_t)value(14) + value(15))
            return false;
        double maxScale[kMaxBones] = {};
        double maxTranslation[kMaxBones] = {};
        for (uint64_t j = 0; j < (uint64_t)count * bones; ++j) {
            const uint8_t* f = blob.data() + start + j * 36;
            for (uint32_t k = 0; k < 9; ++k)
                if (!FloatOk(f + k * 4, k < 3 ? 32767.0f : k < 6 ? 100000.0f : 64.0f))
                    return false;
            const uint32_t bone = j % bones;
            double translationSquared = 0.0;
            for (uint32_t k = 0; k < 3; ++k) {
                const double translation = ReadF32(f + k * 4);
                translationSquared += translation * translation;
                maxScale[bone] = std::max(maxScale[bone], std::fabs((double)ReadF32(f + (6 + k) * 4)));
            }
            maxTranslation[bone] = std::max(maxTranslation[bone], std::sqrt(translationSquared));
        }
        // Operator-norm bounds include every frame and fractional TRS interpolation.
        // Finite inputs alone do not prevent overflow through a chain of scaled parents.
        double worldScale[kMaxBones] = {};
        double worldTranslation[kMaxBones] = {};
        for (uint32_t bone = 0; bone < bones; ++bone) {
            const int16_t parent = ReadS16(blob.data() + value(8) + bone * 2);
            const double parentScale = parent < 0 ? 1.0 : worldScale[parent];
            const double parentTranslation = parent < 0 ? 0.0 : worldTranslation[parent];
            worldScale[bone] = parentScale * maxScale[bone];
            worldTranslation[bone] = parentTranslation + parentScale * maxTranslation[bone];
            if (worldScale[bone] > 100000.0 || worldTranslation[bone] > 32767.0)
                return false;
        }
    }
    if (ReadU32(blob.data() + 8) == 2 && value(19)) {
        const uint32_t audio = value(19);
        if ((audio & 3u) || audio < headerSize || !range(audio, 4))
            return false;
        // Version 2 appends the sound table and PCM samples after all geometry.
        for (const Chunk& chunk : chunks)
            if (audio < chunk.offset + chunk.size)
                return false;
        const uint32_t count = ReadU32(blob.data() + audio);
        const uint64_t entriesSize = 4 + (uint64_t)count * 44;
        if (count > 65535 || !range(audio, entriesSize))
            return false;
        for (uint32_t i = 0; i < count; ++i) {
            const uint8_t* entry = blob.data() + audio + 4 + i * 44;
            const uint32_t rate = ReadU32(entry + 32), samples = ReadU32(entry + 36);
            const uint32_t relative = ReadU32(entry + 40);
            if (!std::memchr(entry, 0, 32) || !rate || rate > 192000 || !samples || (relative & 1u) ||
                relative < entriesSize || (uint64_t)audio + relative > blob.size() ||
                !range(audio + relative, (uint64_t)samples * 2))
                return false;
        }
    }
    return true;
}

} // namespace NeiWolfAsset

// Copy ABI: 0 absent, 1 copied/size queried, -1 malformed or wrong resource type.
// A present rejected resource never falls through to a lower-priority file.
extern "C" COMBO_OOT_EXPORT int OOT_CopyWolfLinkResource(uint8_t* destination, size_t capacity, size_t* size,
                                                         const char** owner);
extern "C" COMBO_MM_EXPORT int MM_CopyWolfLinkResource(uint8_t* destination, size_t capacity, size_t* size,
                                                       const char** owner);
// The model-aware ABI preserves the legacy copy entry points and reports the
// exact selected key, including fallback on older port archives.
extern "C" COMBO_OOT_EXPORT int OOT_CopyWolfLinkModelResource(int useHD, uint8_t* destination, size_t capacity,
                                                              size_t* size, const char** owner, const char** path);
extern "C" COMBO_MM_EXPORT int MM_CopyWolfLinkModelResource(int useHD, uint8_t* destination, size_t capacity,
                                                            size_t* size, const char** owner, const char** path);
