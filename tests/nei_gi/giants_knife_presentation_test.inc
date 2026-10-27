// Giant's Knife and Biggoron share a native GID, but are different awards.
// Exercise the real presentation lookup and both native/randomizer identities.
{
  for (bool randomized : {false, true}) {
    Reset();
    GetItemEntry knife{};
    knife.gid = GID_SWORD_BGS;
    knife.getItemId = GI_SWORD_KNIFE;
    if (randomized) {
      knife.tableId = TABLE_RANDOMIZER;
      knife.drawItemId = RG_GIANTS_KNIFE;
    }
    const auto* presentation = FindPresentation(&knife);
    assert(presentation && std::string(presentation->opaque) ==
           "__OTR__objects/nei_gi_redesign/giants_knife/gi_dl");
    files.insert(presentation->opaque);
    assert(NeiGi_Draw(&play, &knife));
    assert(Drawn() == std::vector<std::string>{presentation->opaque});
    assert(stack.empty() && matrix == 1 && !arena.empty());

    files.insert("__OTR__alt/objects/object_custom_equip/gCustomLongswordDL");
    alt = 1;
    assert(NeiGi_Draw(&play, &knife));
    assert(Drawn().back() == "__OTR__alt/objects/object_custom_equip/gCustomLongswordDL");
    alt = 0;
    assert(NeiGi_Draw(&play, &knife));
    assert(Drawn().back() == presentation->opaque);
  }
  Reset();
  GetItemEntry bgs{};
  bgs.gid = GID_SWORD_BGS;
  bgs.getItemId = GI_SWORD_BGS;
  assert(std::string(FindPresentation(&bgs)->opaque) ==
         "__OTR__objects/nei_gi_redesign/biggoron_sword/gi_dl");
#ifdef COMBO_BUILD
  files.insert("__OTR__objects/nei_gi_redesign/giants_knife/gi_dl");
  CwItemDrawInfo info{};
  assert(OOT_GetNeiGiDrawInfoForAssets("giants_knife", false, &info) == 1);
  assert(std::string(info.dlists[0]) ==
         "__OTR__@oot-gi-base:objects/nei_gi_redesign/giants_knife/gi_dl");
#endif
  std::cout << "PASS distinct Giant's Knife award, native GID alias, Alt priority and base GI\n";
}
