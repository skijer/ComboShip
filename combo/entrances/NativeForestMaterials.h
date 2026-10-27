#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace NativeForestMaterials {
inline constexpr const char* kPath = "custom/comboship/native_forest/stock_materials";
struct Layer {
    int8_t x = 0, y = 0;
    uint8_t width = 0, height = 0;
};
struct Binding {
    uint8_t segment = 0, count = 0;
    Layer layers[2];
};

inline std::vector<Binding> Parse(const uint8_t* bytes, size_t size) {
    // Stock Lost Woods has a valid empty material list; optional visual
    // treatments may supply tracks without changing native scene ownership.
    if (!bytes || size < 2 || bytes[0] != 1 || bytes[1] > 8)
        return {};
    std::vector<Binding> result;
    size_t offset = 2;
    unsigned used = 0;
    for (unsigned index = 0; index < bytes[1]; ++index) {
        if (size - offset < 2)
            return {};
        Binding binding;
        binding.segment = bytes[offset++];
        binding.count = bytes[offset++];
        if (binding.segment < 8 || binding.segment > 15 || binding.count == 0 || binding.count > 2 ||
            (used & (1U << binding.segment)) || size - offset < binding.count * 4U)
            return {};
        used |= 1U << binding.segment;
        for (unsigned layer = 0; layer < binding.count; ++layer) {
            const auto signedByte = [](uint8_t value) {
                return static_cast<int8_t>(value < 128 ? value : value - 256);
            };
            binding.layers[layer] = { signedByte(bytes[offset]), signedByte(bytes[offset + 1]), bytes[offset + 2],
                                      bytes[offset + 3] };
            if (!binding.layers[layer].width || !binding.layers[layer].height)
                return {};
            offset += 4;
        }
        result.push_back(binding);
    }
    // LUS's BlobFactory appends sixteen zero bytes to the resource payload.
    // Accept that exact padding while rejecting arbitrary trailing data.
    if (offset != size) {
        if (size - offset != 16)
            return {};
        for (; offset < size; ++offset)
            if (bytes[offset])
                return {};
    }
    return result;
}
} // namespace NativeForestMaterials
