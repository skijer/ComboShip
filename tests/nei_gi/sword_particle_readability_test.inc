// Catch whole-mesh framing that shrinks individual sword particles. Measure
// actual packed vertices and submitted GPU matrices, including 1/16 packing.
{
  struct ParticleCase {
    const char* slug;
    Kind kind;
    size_t a, b;
    float minimum;
    uint32_t hue;
  };
  const ParticleCase cases[] = {
      {"kokiri_sword", Kind::KokiriSword, 0, 2, 3.8f, 0x78C850},
      {"mm_kokiri_sword", Kind::MmKokiriSword, 0, 1, .5f, 0xA46CFF},
      {"razor_sword", Kind::RazorSword, 0, 1, .38f, 0xBDD6EA},
      {"gilded_sword", Kind::GildedSword, 0, 1, 1.f, 0xFFD45A},
      {"master_sword", Kind::MasterSword, 0, 1, .5f, 0x6F8FFF},
      {"true_master_sword", Kind::SwordAura, 0, 1, .7f, 0xFFF4D6},
      {"biggoron_sword", Kind::BiggoronSword, 0, 1, 1.f, 0xFF9A42},
      {"giants_knife", Kind::GiantsKnife, 0, 1, 1.f, 0xCFD9E6},
      {"great_fairy_sword", Kind::GreatFairySword, 0, 2, 4.2f, 0x79BE84},
      {"four_sword", Kind::FourSword, 0, 1, .5f, 0x315B2F},
  };
  for (const auto& item : cases) {
    float smallest = 1000.f, largest = 0.f;
    for (bool custom : {false, true})
      for (int context : {0, 1, 2, 3}) {
        Reset();
        const bool shop = context == 3;
        const int pickup = shop ? 0 : context;
        const float center[3] = {};
        const std::string path = std::string("__OTR__objects/nei_gi_redesign/") + item.slug + "/gi_dl";
        if (custom) {
          // A huge binary custom model requires a strong geometry correction.
          // Its units must not change the leaf/spark footprint.
          selectedFitScale = .13f;
          selectedFitLift = -20.f;
          NeiGi_DrawExternalPresentation(&play, "__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL",
                                         nullptr, .04f, int(item.kind), true, "oot", shop,
                                         1.5707963267948966f, true, pickup);
        } else {
          NeiGi_DrawPresentation(&play, path.c_str(), nullptr, 1.f, int(item.kind), center, true, nullptr, shop,
                                 pickup);
        }
        assert(arena.size() == 2 && effectSubmitted.size() == 2 && stack.empty());
        const auto& a = arena[0][item.a].v.ob;
        const auto& b = arena[0][item.b].v.ob;
        float squared = 0.f;
        for (int axis = 0; axis < 3; ++axis)
          squared += float(a[axis] - b[axis]) * float(a[axis] - b[axis]);
        const float span = std::sqrt(squared) * effectSubmitted[0].first;
        assert(span >= item.minimum && "sword particle is too small after GI framing");
        smallest = std::min(smallest, span);
        largest = std::max(largest, span);
        bool preservedHue = false;
        for (const auto& v : arena[1])
          preservedHue |= ((uint32_t(v.v.cn[0]) << 16) | (uint32_t(v.v.cn[1]) << 8) | v.v.cn[2]) == item.hue;
        if (item.kind != Kind::GreatFairySword)
          assert(preservedHue && "particle polish changed the sword's hex shimmer");
      }
    assert(largest - smallest < .1f && "particle size still follows pickup/shop/model framing");
  }
  std::cout << "PASS ten sword particle footprints remain readable across authored/custom world, shop and MM receipt fits\n";
}
