// Execute the production C ABI wrapper. Only the engine matrix/render boundary
// is replaced; samplers and dispatch run unchanged.
#ifdef NEI_USED_FX_MM
#include "mm/2s2h/Rando/NeiUsedMagicPresentation.cpp"
#else
#include "soh/Enhancements/randomizer/NeiUsedMagicPresentation.cpp"
#endif
#include <cassert>
#include <iostream>
#include <vector>
#ifdef NEI_USED_FX_MM
#include "mirror_cast_adapter_mm.inc.h"
#else
#include "mirror_cast_adapter_soh.inc.h"
#endif

namespace Fixture {
struct Pose {
  Vec3f position{};
  float scale = 1;
};
Pose pose;
std::vector<Pose> stack, draws;
std::vector<NeiGi::TextureMaterial> materials;
size_t vertices = 0;
bool textures = false;
size_t texturedVertices = 0;
uint32_t expectedFallbackColor = 0;
bool greenOnly = false;
size_t greenVertices = 0;
void CheckGreen(const NeiGi::Mesh &mesh) {
    if (!greenOnly)
        return;
    for (size_t i = 0; i < mesh.count; ++i) {
        const auto rgb = mesh.vertices[i].rgb;
        const auto red = (rgb >> 16) & 255, green = (rgb >> 8) & 255, blue = rgb & 255;
        assert(green > red && green > blue && "every mirror ritual layer must use green hex colors");
        ++greenVertices;
    }
}
} // namespace Fixture
#ifdef NEI_USED_FX_MM
using FixtureMatrixMode = MatrixMode;
#else
using FixtureMatrixMode = uint8_t;
#endif
extern "C" {
CustomItemState gCustomItemState{};
#ifdef NEI_USED_FX_MM
// Rod art belongs to the OoT owner even when the native MM manager has no copy.
uint8_t ResourceMgr_FileExists(const char *) { return 0; }
int NeiResource_Available(const char *) { return Fixture::textures; }
#else
uint8_t ResourceMgr_FileExists(const char *) { return Fixture::textures; }
#endif
uint8_t ResourceMgr_FileAltExists(const char *) { return 0; }
bool ResourceMgr_IsAltAssetsEnabled() { return false; }
void Matrix_MultVec3f(Vec3f *p, Vec3f *out) {
  *out = {Fixture::pose.position.x + p->x * Fixture::pose.scale,
          Fixture::pose.position.y + p->y * Fixture::pose.scale,
          Fixture::pose.position.z + p->z * Fixture::pose.scale};
}
void Matrix_Push() { Fixture::stack.push_back(Fixture::pose); }
void Matrix_Pop() {
  assert(!Fixture::stack.empty());
  Fixture::pose = Fixture::stack.back();
  Fixture::stack.pop_back();
}
void Matrix_Translate(float x, float y, float z, FixtureMatrixMode mode) {
  assert(mode == MTXMODE_NEW);
  Fixture::pose = {{x, y, z}, 1};
}
void Matrix_Scale(float x, float y, float z, FixtureMatrixMode mode) {
  assert(mode == MTXMODE_APPLY && x == y && y == z);
  Fixture::pose.scale *= x;
}
NeiGi::Basis NeiGi_CameraBasis(PlayState *) { return {}; }
void NeiGi_DrawMesh(PlayState *, const NeiGi::Mesh &mesh, NeiGi::Kind kind) {
    Fixture::CheckGreen(mesh);
    assert(kind == NeiGi::Kind::Neutral); // No resource lookup or Alt-owned texture pointer.
    if (!mesh.count)
        return;
    if (Fixture::expectedFallbackColor)
        for (size_t i = 0; i < mesh.count; ++i) {
            assert(mesh.vertices[i].rgb == Fixture::expectedFallbackColor);
            assert(mesh.vertices[i].alpha <= 72);
        }
  assert(mesh.count < mesh.vertices.size());
  Fixture::draws.push_back(Fixture::pose);
  Fixture::vertices += mesh.count;
}
bool NeiGi_DrawTexturedMesh(PlayState *, const NeiGi::Mesh &mesh,
                            const NeiGi::TextureMaterial &material) {
    Fixture::CheckGreen(mesh);
    assert(material.path && mesh.count < mesh.vertices.size());
    if (mesh.count)
        Fixture::materials.push_back(material);
    if (Fixture::textures)
        Fixture::texturedVertices += mesh.count;
    return Fixture::textures;
}
}
int main() {
  PlayState play{};
  Player player{};
  player.actor.world.pos = {100, 200, 300};
  player.meleeWeaponInfo[0].tip = {110, 250, 310};
  Vec3f velocity{18, 0, 0},
      trail[6] = {{100, 200, 300}, {82, 200, 300}, {64, 200, 300},
                  {46, 200, 300},  {28, 200, 300}, {10, 200, 300}};
  auto restored = [] {
    assert(Fixture::pose.position.x == 7 && Fixture::pose.scale == 3 &&
           Fixture::stack.empty());
  };
  Fixture::pose = {{7, 8, 9}, 3};
  NeiUsedMagic_DrawProjectile(&play, 2, &trail[0], &velocity, 2, 0);
  restored();
  assert(Fixture::draws.back().position.x == 100);
  NeiUsedMagic_DrawTrail(&play, 1, trail, 6, 2);
  restored();
  const auto before = Fixture::draws.size();
  NeiUsedMagic_DrawCharge(&play, &player, 0, 1);
  restored();
  assert(Fixture::draws.size() > before);
  assert(Fixture::draws.back().position.x == 100 && Fixture::draws.back().position.y == 205);
  assert(std::string(Fixture::materials.back().path) == "__OTR__objects/nei_rod_cast_poc6/fire_natural");
  gCustomItemState.fireRodCharging = 1;
  gCustomItemState.fireRodChargeLevel = 1;
  NeiUsedMagic_DrawChargeFocus(&play, 0);
  restored();
  assert(Fixture::draws.back().position.x == 7 &&
         Fixture::draws.back().position.y == 8);
  // The focus is exactly the current held-model matrix origin, not melee tip.
  assert(Fixture::draws.back().position.y != player.meleeWeaponInfo[0].tip.y);
  NeiUsedMagic_DrawSpin(&play, &player, 1, 500, true);
  restored();
  NeiUsedMagic_DrawBurst(&play, 2, &trail[0], 1.4f, .5f);
  restored();
  NeiUsedMagic_DrawPortal(&play, &player, 1, 255);
  restored();
  assert(Fixture::draws.back().position.y == 201);
  Fixture::greenOnly = true;
  for (bool textures : {false, true}) {
      Fixture::textures = textures;
      Fixture::materials.clear();
      MirrorCastAura(&play, &player, 1, 255);
      restored();
      assert(Fixture::draws.back().position.y == 201);
      assert(Fixture::materials.size() == 2);
      assert(std::string(Fixture::materials[0].path) == "__OTR__objects/magic_mirror/forest_seal");
      assert(!Fixture::materials[0].repeatS && !Fixture::materials[0].repeatT);
      assert(std::string(Fixture::materials[1].path) == "__OTR__objects/magic_mirror/forest_wisp");
      assert(Fixture::materials[1].repeatS && !Fixture::materials[1].repeatT);
  }
  assert(Fixture::greenVertices > 0);
  Fixture::materials.clear();
  Fixture::greenOnly = false;
  NeiUsedMagic_DrawPortal(&play, &player, 1, 255);
  assert(Fixture::materials.size() == 1 &&
         std::string(Fixture::materials[0].path) == "__OTR__objects/nei_used_magic/fire_wisp");
  Fixture::materials.clear();
  MirrorCastAura(&play, &player, 0, 255);
  MirrorCastAura(&play, &player, 1, 0);
  assert(Fixture::materials.empty());
  Fixture::greenOnly = Fixture::textures = false;
  const auto valid = Fixture::draws.size();
  NeiUsedMagic_DrawProjectile(nullptr, 2, &trail[0], &velocity, 2, 0);
  NeiUsedMagic_DrawProjectile(&play, -1, &trail[0], &velocity, 2, 0);
  NeiUsedMagic_DrawProjectile(&play, 8, &trail[0], &velocity, 2, 0);
  NeiUsedMagic_DrawTrail(&play, 2, nullptr, 6, 2);
  NeiUsedMagic_DrawTrail(&play, 2, trail, 500,
                         2); // Reject invalid array length, never over-read.
  NeiUsedMagic_DrawCharge(&play, nullptr, 2, 1);
  NeiUsedMagic_DrawPortal(&play, &player, 1, 0);
  NeiUsedMagic_DrawPortal(&play, &player, 0, 255);
  restored();
  assert(Fixture::draws.size() == valid);
  Fixture::textures = true;
  Fixture::vertices = Fixture::texturedVertices = 0;
  NeiUsedMagic_DrawProjectile(&play, 1, &trail[0], &velocity, 2, 0);
  restored();
  const auto complete =
      NeiUsedMagic::SampleProjectile(NeiGi::Kind::Ice, 0, 2, {18, 0, 0});
  assert(Fixture::texturedVertices == NeiUsedMagic::SampleProjectileSurface(
                                          NeiGi::Kind::Ice, 0, 2, {18, 0, 0})
                                          .count);
  assert(Fixture::vertices == complete.count);
  Fixture::materials.clear();
  NeiUsedMagic_DrawProjectile(&play, 0, &trail[0], &velocity, 2, 0);
  restored();
  assert(Fixture::materials.size() == 1);
  assert(std::string(Fixture::materials[0].path) ==
         "__OTR__objects/nei_rod_attack/fire_surge");
  assert(!Fixture::materials[0].repeatS && !Fixture::materials[0].repeatT);
  Fixture::materials.clear();
  NeiUsedMagic_DrawSpin(&play, &player, 2, 500, true);
  restored();
  assert(Fixture::materials.empty()); // Accepted Light rays have no release-wall texture.
  for (int element = 0; element < 2; ++element) {
    Fixture::materials.clear();
    NeiUsedMagic_DrawSpin(&play, &player, element, 500, true);
    restored();
    assert(Fixture::materials.size() == (element == 0 ? 2 : 3));
    const auto crest = Fixture::materials[Fixture::materials.size() - 2];
    const auto flow = Fixture::materials.back();
    assert(std::string(crest.path) == (element == 0 ? "__OTR__objects/nei_rod_cast_poc6/fire_natural"
                                                  : "__OTR__objects/nei_rod_cast_poc6/ice_fracture_release"));
    assert(std::string(flow.path) == (element == 0 ? "__OTR__objects/nei_rod_cast_poc6/fire_natural"
                                                 : "__OTR__objects/nei_rod_cast_poc6/ice_fracture_release"));
    assert(crest.repeatS && !crest.repeatT && flow.repeatS && flow.repeatT);
    if (element == 1)
      assert(std::string(Fixture::materials[0].path) == "__OTR__objects/nei_used_magic/ice_fracture");
  }
  Fixture::textures = false;
  const uint32_t fallbackColors[] = {0xFFB657, 0xA5E9FF, 0xFFF3B8};
  for (int element = 0; element < 3; ++element) {
    Fixture::expectedFallbackColor = fallbackColors[element];
    DrawAttackSurface(
        &play, NeiUsedMagic::SampleSpinSurface(Element(element), 0, 150, true),
        element, true);
    DrawAttackSurface(
        &play, NeiUsedMagic::SampleSpinFlow(Element(element), 0, 150, true, 1),
        element, true);
  }
  Fixture::expectedFallbackColor = 0;
  // Textured faces replace exactly their geometry fallback, never overlap it.
  std::cout << "USED VFX production dispatch: C ABI, matrix restoration, world "
               "placement and empty-state guards passed\n";
}
