#pragma once

#include "NativeForestDependencies.h"
#include <ship/Context.h>
#include <ship/resource/ResourceManager.h>
#include <ship/resource/type/Blob.h>

namespace NativeForest {
inline NativeForestDependencies::Graph LoadDependencies(const char* path) {
    const auto* context = Ship::Context::GetRawInstance();
    if (!context)
        return {};
    const auto manager = context->GetResourceManager();
    const auto blob = std::dynamic_pointer_cast<Ship::Blob>(manager->LoadResource(path));
    return blob ? NativeForestDependencies::Parse(blob->Data.data(), blob->Data.size())
                : NativeForestDependencies::Graph{};
}
} // namespace NativeForest
