// A selected standalone owner sword must reach native MM as well as its
// foreign descriptor. No authored fallback may replace that selected mesh.
for (auto item : {RI_SWORD_KOKIRI, RI_SWORD_RAZOR, RI_SWORD_GILDED,
                  RI_OOT_MASTER_SWORD, RI_OOT_TRUE_MASTER_SWORD,
                  RI_OOT_BIGGORON_SWORD, RI_GREAT_FAIRY_SWORD}) {
  Reset(); ownerAlt = true; alt = true;
  const char* selected = item == RI_OOT_MASTER_SWORD || item == RI_OOT_TRUE_MASTER_SWORD ?
    "__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL" :
    item == RI_OOT_BIGGORON_SWORD || item == RI_GREAT_FAIRY_SWORD ?
    "__OTR__alt/objects/object_custom_equip/gCustomLongswordDL" :
    "__OTR__alt/objects/object_custom_equip/gCustomKokiriSwordDL";
  files.insert(selected);
  assert(MM_TryDrawNeiGi(item) && "native MM discarded the selected owner sword");
  assert(Drawn() == std::vector<std::string>{std::string("__OTR__@oot:") + (selected + 7)});
  assert(arena.size() == 2 && stack.empty() && matrix == 1 && matrixY == 0);
}
Reset(); ownerAlt = false;
std::cout << "PASS native MM selected owner swords: seven awards retain mesh, intrinsic particles and shimmer\n";

for (bool nativeMm : {false, true}) for (bool missingFlame : {false, true}) {
  Reset(); ownerAlt = alt = true; dinSword = true;
  const char* selected = "__OTR__objects/din_fire_sword/progressive/adult/SwordDL";
  files.insert(selected);
  const auto& layers = DinSwordGi::profiles[0];
  for (const auto* dependency : {layers.core, layers.flame, layers.coreVertices, layers.flameVertices,
                                 DinSwordGi::coreTexture, DinSwordGi::flameTexture})
    files.insert(!std::strncmp(dependency,"__OTR__",7)?dependency:std::string("__OTR__")+dependency);
  if (missingFlame) files.erase(std::string("__OTR__")+layers.flameVertices);
  if (nativeMm) assert(MM_TryDrawNeiGi(RI_OOT_MASTER_SWORD));
  else {
    GetItemEntry selectedEntry{}; selectedEntry.drawFunc=Randomizer_DrawMasterSword;
    assert(NeiGi_Draw(&play,&selectedEntry));
  }
  std::vector<std::string> expected{
    nativeMm ? std::string("__OTR__@oot:")+(selected+7) : selected};
  if (!missingFlame) {expected.push_back(layers.core);expected.push_back(layers.flame);}
  assert(Drawn()==expected && "native selected Din sword lost its layers or queued an incomplete pack");
  assert(arena.size()==2 && stack.empty() && matrix==1 && matrixY==0);
}
Reset(); ownerAlt=false;
std::cout << "PASS native OoT/MM Din GI layers: complete owner resources, missing-dependency fallback, sword particles and shimmer\n";

// A native MM GI replacement keeps its established route even while an OoT
// standalone equipment pack is also mounted.
for(auto [item,gi] : {
  std::pair{RI_SWORD_KOKIRI,"objects/object_gi_sword_1/gGiKokiriSwordGuardDL"},
  std::pair{RI_SWORD_RAZOR,"objects/object_gi_sword_2/gGiRazorSwordDL"},
  std::pair{RI_SWORD_GILDED,"objects/object_gi_sword_3/gGiGildedSwordEmptyDL"},
  std::pair{RI_GREAT_FAIRY_SWORD,"objects/object_gi_sword_4/gGiGreatFairysSwordBladeDL"}}) {
  Reset();ownerAlt=alt=true;
  files.insert("__OTR__alt/objects/object_custom_equip/gCustomKokiriSwordDL");
  files.insert("__OTR__alt/objects/object_custom_equip/gCustomLongswordDL");
  modFiles.insert({"mm",std::string("__OTR__")+gi});
  assert(!MM_TryDrawNeiGi(item) && "selected equipment displaced the native MM GI mod");
  assert(Drawn().empty() && arena.empty() && stack.empty());
}
Reset();ownerAlt=false;
std::cout << "PASS native MM GI mod priority over standalone equipment: all four native sword families\n";

// Missing shipped vanilla sword geometry must be handled before the legacy
// dispatch, which can borrow a local or donor base-path mod even with Alt OFF.
for (auto item : {RI_SWORD_KOKIRI, RI_SWORD_RAZOR, RI_SWORD_GILDED,
                  RI_OOT_MASTER_SWORD, RI_OOT_TRUE_MASTER_SWORD,
                  RI_OOT_BIGGORON_SWORD, RI_GREAT_FAIRY_SWORD, RI_OOT_EXT_FOUR_SWORD}) {
  Reset(); alt=false; ownerAlt=true;
  modFiles.insert({"mm","__OTR__objects/object_toki_objects/object_toki_objects_DL_001BD0"});
  CwItemDrawInfo missing{};
  assert(!MM_DescribeNeiGi(item,&missing));
  mmShopLegacyDraws=0;
  Rando::DrawResolvedItem(item,1,nullptr);
  assert(mmShopLegacyDraws==0 && "missing vanilla sword fell through to mod-capable legacy dispatch");
  assert(Drawn().empty() && arena.empty() && stack.empty());
}
Reset(); ownerAlt=false;
std::cout << "PASS native MM vanilla sword missing-resource dispatch: all eight awards refuse mod-capable fallback\n";
