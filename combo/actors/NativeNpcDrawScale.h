#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>

namespace NativeNpcDrawScale {
/* Optional OARR Scalar/U16: [0x4e44, version=1, float low16, float high16].
 * Both games already decode this scalar type. Malformed/absent data is inert. */
template <typename Array> inline float Read(const Array* resource) {
    if (!resource || resource->ArrayType != decltype(resource->ArrayType)::Scalar ||
        resource->ArrayScalarType != decltype(resource->ArrayScalarType)::ZSCALAR_U16 || resource->ArrayCount != 4 ||
        resource->Scalars.size() != 4 || resource->Scalars[0].u16 != 0x4e44 || resource->Scalars[1].u16 != 1)
        return 1.f;
    uint32_t bits = resource->Scalars[2].u16 | (uint32_t(resource->Scalars[3].u16) << 16);
    float factor;
    std::memcpy(&factor, &bits, sizeof(factor));
    return std::isfinite(factor) && factor >= .5f && factor <= 2.f ? factor : 1.f;
}
} // namespace NativeNpcDrawScale
