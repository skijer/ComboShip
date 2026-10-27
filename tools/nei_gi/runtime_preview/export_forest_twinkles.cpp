// Exact production particles and Mirror geometry, quantized like the Vtx
// upload.
#include "actors/ForestGiParticles.h"
#include "gameplay/MagicMirrorGeometry.h"
#include <cmath>
#include <fstream>

void Vertex(std::ofstream &out, NeiGi::Point p, uint32_t rgb, uint8_t alpha) {
  const float position[] = {std::round(p.x * 16) / 16,
                            std::round(p.y * 16) / 16,
                            std::round(p.z * 16) / 16};
  const uint8_t rgba[] = {uint8_t(rgb >> 16), uint8_t(rgb >> 8), uint8_t(rgb),
                          alpha};
  out.write(reinterpret_cast<const char *>(position), sizeof(position));
  out.write(reinterpret_cast<const char *>(rgba), sizeof(rgba));
}
void Write(std::ofstream &out, const NeiGi::Mesh &mesh) {
  const uint32_t count = mesh.count;
  out.write(reinterpret_cast<const char *>(&count), sizeof(count));
  for (size_t i = 0; i < mesh.count; ++i)
    Vertex(out, mesh.vertices[i].p, mesh.vertices[i].rgb,
           mesh.vertices[i].alpha);
}
int main(int argc, char **argv) {
  if (argc != 2)
    return 2;
  std::ofstream out(argv[1], std::ios::binary);
  const NeiGi::Basis camera{};
  for (uint32_t frame = 0; frame < 180; ++frame) {
    Write(out, NeiGi::SampleShimmer(frame, true, camera, NeiGi::Kind::Leaf));
    Write(out, ForestGiParticles::Portal(frame, true, camera));
    Write(out, ForestGiParticles::Mystery(frame, camera));
    Write(out, ForestGiParticles::Saria(frame, false, camera));
    Write(out, ForestGiParticles::Mirror(frame, camera));
    ComboMagicMirrorGeometry::Mesh mirror{};
    ComboMagicMirrorGeometry::Build(frame, mirror);
    const uint32_t count = mirror.triangleCount * 3;
    out.write(reinterpret_cast<const char *>(&count), sizeof(count));
    for (int b = 0; b < mirror.batchCount; ++b) {
      const auto &batch = mirror.batches[b];
      for (int t = 0; t < batch.triangleCount; ++t) {
        const auto &triangle = mirror.triangles[batch.firstTriangle + t];
        for (int index : {triangle.a, triangle.b, triangle.c}) {
          const auto &v = mirror.vertices[batch.firstVertex + index];
          Vertex(out, {v.x, v.y, v.z},
                 (uint32_t(v.r) << 16) | (uint32_t(v.g) << 8) | v.b, v.a);
        }
      }
    }
  }
  return out ? 0 : 1;
}
