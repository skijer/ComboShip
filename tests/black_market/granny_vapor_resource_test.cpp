#include "combo/BlackMarketGranny.h"
#include "resource/type/Array.h"
#include <cassert>
#include <cstdio>
#include <fast/lus_gbi.h>
#include <fast/resource/type/Vertex.h>
/* PRODUCTION ADAPTER */
int main() {
  auto nativeInfo = std::make_shared<Ship::ResourceInitData>();
  nativeInfo->Type = 0x4f415252;
  auto native = std::make_shared<SOH::Array>(nativeInfo);
  native->ArrayType = SOH::ArrayResourceType::Vertex;
  native->ArrayCount = 56;
  native->Vertices.resize(56);
  native->Vertices[0].v.ob[0] = 123;
  auto converted = BlackMarketGranny::NativeVaporVertices(native);
  assert(converted.data != nullptr &&
         std::memcmp(converted.data, native->Vertices.data(),
                     56 * sizeof(Vtx)) == 0 &&
         converted.count == 56 && converted.owner &&
         "original OARR vertex resource must reach Granny vapor validation");
  native->ArrayCount = 55;
  assert(!BlackMarketGranny::NativeVaporVertices(native).data);
  native->ArrayCount = 56;
  native->ArrayType = SOH::ArrayResourceType::Scalar;
  assert(!BlackMarketGranny::NativeVaporVertices(native).data);
  auto modernInfo = std::make_shared<Ship::ResourceInitData>();
  modernInfo->Type = 0x4f565458;
  auto modern = std::make_shared<Fast::Vertex>(modernInfo);
  modern->VertexList.resize(56);
  assert(BlackMarketGranny::NativeVaporVertices(modern).count == 56);
  assert(!BlackMarketGranny::NativeVaporVertices({}).data);
  std::weak_ptr<void> retained = converted.owner;
  native.reset();
  assert(!retained.expired());
  converted = {};
  assert(retained.expired());
  std::puts(
      "PASS Granny native vapor resource boundary: real legacy OARR/Vertex and "
      "Fast::Vertex owners, type/count rejection and pinned lifetime");
}
