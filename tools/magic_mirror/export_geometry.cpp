#include "../../combo/gameplay/MagicMirrorGeometry.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>

int main(int argc, char** argv) {
    using namespace ComboMagicMirrorGeometry;
    Mesh mesh;
    Build(argc > 1 ? static_cast<uint32_t>(std::strtoul(argv[1], nullptr, 10)) : 0, mesh);
    std::cout << std::setprecision(9) << "{\"vertexPrecision\":" << kVertexPrecision << ",\"vertices\":[";
    for (int i = 0; i < mesh.vertexCount; ++i) {
        const auto& v = mesh.vertices[i];
        const bool glass = i >= mesh.batches[mesh.opaqueBatchCount].firstVertex &&
                           i < mesh.batches[mesh.glassEndBatch].firstVertex;
        const Color color = glass ? ReflectionModulation({v.r, v.g, v.b, v.a}) : Color{v.r, v.g, v.b, v.a};
        if (i) std::cout << ',';
        std::cout << '[' << v.x << ',' << v.y << ',' << v.z << ',' << int(color.r) << ',' << int(color.g) << ','
                  << int(color.b) << ',' << int(color.a) << ']';
    }
    std::cout << "],\"reflectionUV\":[";
    for (int i = 0; i < mesh.vertexCount; ++i) {
        if (i) std::cout << ',';
        std::cout << '[' << ReflectionS(mesh.vertices[i].x) << ',' << ReflectionT(mesh.vertices[i].y) << ']';
    }
    std::cout << "],\"triangles\":[";
    for (int b = 0; b < mesh.batchCount; ++b) {
        const auto& batch = mesh.batches[b];
        for (int i = 0; i < batch.triangleCount; ++i) {
            const auto& t = mesh.triangles[batch.firstTriangle + i];
            if (batch.firstTriangle + i) std::cout << ',';
            std::cout << '[' << batch.firstVertex + t.a << ',' << batch.firstVertex + t.b << ','
                      << batch.firstVertex + t.c << ',' << (b < mesh.opaqueBatchCount ? 0 : 1) << ','
                      << (b >= mesh.opaqueBatchCount && b < mesh.glassEndBatch ? 1 : 0) << ']';
        }
    }
    std::cout << "]}\n";
}
