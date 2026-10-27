#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
// Win32 exposes these legacy qualifiers before MM's shared geometry headers.
#define near
#define far
#endif

#include "entrances/LakeHyliaBridge.h"

#if !defined(near) || !defined(far)
#error "Shared geometry must preserve the caller Windows SDK macros."
#endif

int main() {
    const auto water = ComboLakeHylia::BuildWaterGeometry(ComboLakeHylia::kFullWaterY);
    const auto mesh = ComboLakeHylia::BuildGeometry();
    assert(!water.empty() && water.size() <= ComboLakeHylia::kMaxWaterQuads);
    assert(!mesh.solid.empty());
    assert(ComboLakeHylia::BuildWaterGeometry(-2000).empty());
    for (const auto& face : water) {
        for (const auto point : { face.a, face.b, face.c, face.d })
            assert(std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z));
    }
    puts("PASS shared Lake geometry under Windows SDK near/far macros: full water, drained state and solid mesh");
}
