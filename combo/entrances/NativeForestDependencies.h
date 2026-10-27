#pragma once

#include <cstddef>
#include <cstdint>
#include <set>
#include <string>
#include <vector>

namespace NativeForestDependencies {
struct Graph {
    std::vector<std::string> core, stock;
};

inline bool SafePath(const std::string& path) {
    if (path.empty() || path.front() == '/' || path.back() == '/' || path.find('\\') != std::string::npos)
        return false;
    size_t begin = 0;
    while (begin < path.size()) {
        const size_t end = path.find('/', begin);
        const auto part = path.substr(begin, end == std::string::npos ? end : end - begin);
        if (part.empty() || part == "." || part == "..")
            return false;
        for (unsigned char byte : part)
            if (byte < 32 || byte == 127)
                return false;
        if (end == std::string::npos)
            break;
        begin = end + 1;
    }
    return true;
}

inline Graph Parse(const uint8_t* bytes, size_t size) {
    if (!bytes || size < 5 || size > 8 * 1024 * 1024 || bytes[0] != 1)
        return {};
    const unsigned coreCount = bytes[1] | (unsigned(bytes[2]) << 8);
    const unsigned stockCount = bytes[3] | (unsigned(bytes[4]) << 8);
    if (!coreCount || !stockCount || coreCount > 4096 || stockCount > 4096)
        return {};
    Graph graph;
    std::set<std::string> unique;
    size_t offset = 5;
    for (unsigned index = 0; index < coreCount + stockCount; ++index) {
        if (size - offset < 2)
            return {};
        const unsigned length = bytes[offset] | (unsigned(bytes[offset + 1]) << 8);
        offset += 2;
        if (!length || length > 512 || size - offset < length)
            return {};
        std::string path(reinterpret_cast<const char*>(bytes + offset), length);
        if (!SafePath(path) || !unique.insert(path).second)
            return {};
        (index < coreCount ? graph.core : graph.stock).push_back(std::move(path));
        offset += length;
    }
    if (offset != size) {
        if (size - offset != 16)
            return {};
        for (; offset < size; ++offset)
            if (bytes[offset])
                return {};
    }
    return graph;
}
} // namespace NativeForestDependencies
