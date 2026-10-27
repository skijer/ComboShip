// The producer and the MM host draw body run against the renderer's existing real-header fixture.
CwItemDrawInfo exported{};
assert(OOT_GetNeiGiDrawInfo(nullptr, &exported) == 0);
assert(OOT_GetNeiGiDrawInfo("unknown", &exported) == 0);
// The native MM item must request its purple Kokiri recipe from the owner.
// Its bridge identity cannot share OoT's green progressive Kokiri recipe.
Reset();
files.insert("__OTR__objects/nei_gi_redesign/mm_kokiri_sword/gi_dl");
CwItemDrawInfo mmKokiri{};
assert(MM_DescribeNeiGi(RI_SWORD_KOKIRI, &mmKokiri));
assert(std::strcmp(mmKokiri.dlists[0], "__OTR__@oot-gi-base:objects/nei_gi_redesign/mm_kokiri_sword/gi_dl") == 0);
assert(NeiGi::ColorHex(static_cast<Kind>(mmKokiri.neiEffect)) == 0xA46CFF);
files.insert("__OTR__objects/nei_gi_redesign/kokiri_sword/gi_dl");
CwItemDrawInfo ootKokiri{};
assert(OOT_GetNeiGiDrawInfo("kokiri_sword", &ootKokiri) == 1);
assert(std::strcmp(ootKokiri.dlists[0], "__OTR__objects/nei_gi_redesign/kokiri_sword/gi_dl") == 0);
assert(NeiGi::ColorHex(static_cast<Kind>(ootKokiri.neiEffect)) == 0x78C850);
assert(mmKokiri.neiEffect != ootKokiri.neiEffect);
for (const auto& origin : {std::pair{mmKokiri, 0xA46CFFu}, std::pair{ootKokiri, 0x78C850u}}) {
    Reset();
    MM_DrawNeiGi(origin.first);
    assert(Drawn(true) == std::vector<std::string>{origin.first.dlists[0]});
    assert(!arena.empty()); // Intrinsic particles are carried without the optional toggle.
    bool carriesColor = false;
    for (const auto& mesh : arena)
        for (const auto& v : mesh)
            carriesColor |= ((uint32_t(v.v.cn[0]) << 16) | (uint32_t(v.v.cn[1]) << 8) | v.v.cn[2]) == origin.second;
    assert(carriesColor);
    const auto intrinsicCount = arena.size();
    Reset(); enabled = 1;
    MM_DrawNeiGi(origin.first);
    assert(arena.size() == intrinsicCount + (origin.first.itemShimmer ? 0 : 1));
    const auto shown = arena;
    arena.clear();
    NeiGi_DrawMesh(&play, NeiGi::SampleShimmer(play.gameplayFrames, true,
        NeiGi_CameraBasis(&play), static_cast<Kind>(origin.first.neiEffect)));
    assert(arena.size() == 1);
    const auto& want = arena.front();
    assert(std::count_if(shown.begin(), shown.end(), [&](const auto& got) {
        return got.size() == want.size() && !std::memcmp(got.data(), want.data(), want.size() * sizeof(Vtx));
    }) == 1);
    assert(stack.empty() && interpolation == 0 && matrix == 1);
}
Reset();

// Legacy fallback keeps each sword's shimmer palette when authored resources
// are missing. Every sword, including Four Sword, retains its mandatory
// identity shimmer and its own intrinsic particles in this route.
for (const auto& sword : {
    std::pair{RI_SWORD_KOKIRI, Kind::MmKokiriSword},
    std::pair{RI_SWORD_RAZOR, Kind::RazorSword},
    std::pair{RI_SWORD_GILDED, Kind::GildedSword},
    std::pair{RI_OOT_MASTER_SWORD, Kind::MasterSword},
    std::pair{RI_OOT_TRUE_MASTER_SWORD, Kind::SwordAura},
    std::pair{RI_OOT_BIGGORON_SWORD, Kind::BiggoronSword},
    std::pair{RI_GREAT_FAIRY_SWORD, Kind::GreatFairySword},
    std::pair{RI_OOT_EXT_FOUR_SWORD, Kind::FourSword},
    std::pair{RI_OOT_IRON_KNUCKLE_AXE, Kind::IronKnuckleAxe}}) {
    for (int optionalEffects : {0, 1}) {
        Reset(); enabled = optionalEffects;
        {
            MM_NeiGiFallbackShimmer fallbackShimmer(sword.first);
            Matrix_Scale(7, 7, 7, MTXMODE_APPLY); // Original model mutates its own pose.
        }
        const bool active = true;
        assert(arena.size() == 1 + NeiGi::IsSword(sword.second));
        assert(stack.empty() && interpolation == 0 && loads == 0);
        if (active) {
            assert(matrix == 1 && matrixY == 0);
            const auto got = arena.back();
            arena.clear();
            NeiGi_DrawMesh(&play, NeiGi::SampleShimmer(play.gameplayFrames, true,
                NeiGi_CameraBasis(&play), sword.second));
            assert(arena.size() == 1);
            const auto& want = arena.front();
            assert(got.size() == want.size() && !std::memcmp(got.data(), want.data(), want.size() * sizeof(Vtx)));
        }
    }
}
Reset();
for (const auto& item : {
    std::pair{ RI_OOT_NEI_CANE_OF_SOMARIA, Kind::Somaria },
    std::pair{ RI_OOT_NEI_CANE_SOMARIA_PLATFORM, Kind::Somaria },
    std::pair{ RI_OOT_NEI_CANE_PACCI_FLIP, Kind::Pacci },
    std::pair{ RI_OOT_NEI_CANE_PACCI_STONE, Kind::Pacci },
    std::pair{ RI_OOT_NEI_CANE_PACCI_ULTRAHAND, Kind::Pacci },
    std::pair{ RI_OOT_NEI_POKE_BALL, Kind::Pokeball },
    std::pair{ RI_OOT_EXT_TRIDENT, Kind::Gold },
    std::pair{ RI_OOT_EXT_ROC_BOOTS, Kind::Gold } }) {
    for (int optionalEffects : {0, 1}) {
        Reset(); enabled = optionalEffects;
        { MM_NeiGiFallbackShimmer shimmer(item.first); }
        const bool shimmer=optionalEffects||item.second==Kind::Gold;
        assert(arena.size() == size_t(shimmer));
        if (shimmer) {
            const auto got = arena.front();
            arena.clear();
            NeiGi_DrawMesh(&play, NeiGi::SampleShimmer(play.gameplayFrames, true, NeiGi_CameraBasis(&play), item.second));
            assert(got.size() == arena.front().size());
            assert(!std::memcmp(got.data(), arena.front().data(), got.size() * sizeof(Vtx)));
        }
        assert(stack.empty());
    }
}
Reset(); enabled = 1;
{ MM_NeiGiFallbackShimmer shimmer(RI_OOT_NEI_MARIO_MASK); }
assert(arena.empty() && stack.empty()); // Legacy mask drawer supplies its one intrinsic overlay.
Reset();
// Season descriptors never require a rod or authored season mesh, and MM's
// generic native bridge must accept their zero-display-list weather recipes.
const CustomDrawFunc seasonCallbacks[] = {Randomizer_DrawSeasonSpring, Randomizer_DrawSeasonSummer,
                                         Randomizer_DrawSeasonAutumn, Randomizer_DrawSeasonWinter};
const char* seasonNames[] = {"season_spring", "season_summer", "season_autumn", "season_winter"};
for (int season = 0; season < 4; ++season) {
    Reset();
    GetItemEntry weather{};
    weather.drawFunc = seasonCallbacks[season];
    exported = {};
    assert(NeiGi_DescribeEntry(&weather, &exported) == 1);
    assert(exported.drawKind == CW_DRAW_KIND_SEASON_GI && exported.neiEffect == season + 1);
    assert(exported.dlistCount == 0 && exported.opCount == 0 && exported.xluStartIndex == -1);
    CwItemDrawInfo native{};
    assert(OOT_GetNeiGiDrawInfo(seasonNames[season], &native) == 1);
    assert(native.drawKind == exported.drawKind && native.neiEffect == exported.neiEffect);
    assert(native.dlistCount == 0 && native.opCount == 0);
    MM_DrawNeiGi(native);
    assert(!arena.empty() && gfx.polyXlu.p > xlu);
    assert(Drawn().empty() && gfx.polyOpa.p == opa && fallback == 0);
    assert(stack.empty() && interpolation == 0 && matrix == 1 && loads == 0);
}
Reset();
files.insert("__OTR__objects/nei_gi_redesign/rod_of_seasons/gi_dl");
assert(OOT_GetNeiGiDrawInfo("rod_of_seasons", &exported) == 1);
assert(exported.drawKind == CW_DRAW_KIND_NEI_GI && exported.neiEffect == int(Kind::SeasonCycle));
MM_DrawNeiGi(exported);
assert(Drawn() == std::vector<std::string>{"__OTR__objects/nei_gi_redesign/rod_of_seasons/gi_dl"});
assert(!arena.empty() && stack.empty() && interpolation == 0 && loads == 0);
// Inactive OoT owns its own Alt mode, independently of the active MM context.
Reset();
files.insert("alt/__OTR__objects/nei_gi_redesign/fire_rod/gi_dl");
ownerAlt = true;
alt = false;
assert(OOT_GetNeiGiDrawInfo("fire_rod", &exported) == 1);
ownerAlt = false;
alt = true;
assert(OOT_GetNeiGiDrawInfo("fire_rod", &exported) == 0);
int authored = 0;
for (const auto& item : kPresentations) {
    if (!item.opaque) continue;
    Reset();
    files.insert(item.opaque);
    if (item.translucent) files.insert(item.translucent);
    GetItemEntry bound{}; bound.drawFunc = item.draw; bound.drawItemId=item.identity; bound.gid=item.nativeGid;
    exported = {};
    assert(NeiGi_DescribeEntry(&bound, &exported) == 1);
    assert(exported.drawKind == CW_DRAW_KIND_NEI_GI);
    assert(exported.neiEffect == static_cast<int>(item.effect));
    assert(exported.scale == item.scale);
    assert(exported.neiSomariaUpgrade == (item.draw == Randomizer_DrawCaneSomariaUpgrade));
    assert(std::strcmp(exported.dlists[0], item.opaque) == 0);
    const std::string slug = std::string(item.opaque).substr(std::strlen("__OTR__objects/nei_gi_redesign/"));
    CwItemDrawInfo native{};
    assert(OOT_GetNeiGiDrawInfo(slug.substr(0, slug.find('/')).c_str(), &native) == 1);
    assert(native.neiSomariaUpgrade == 0); // native MM chooses concrete skill, never OoT inventory
    MM_DrawNeiGi(exported);
    const auto paths = Drawn();
    assert(std::count(paths.begin(), paths.end(), item.opaque) == 1);
    if (item.translucent) assert(std::count(paths.begin(), paths.end(), item.translucent) == 1);
    assert(stack.empty() && interpolation == 0 && matrix == 1);
    if (NeiGi::IsRod(item.effect) || NeiGi::IsSpell(item.effect)) assert(!arena.empty());
    if (exported.neiSomariaUpgrade) assert((flameColors == std::vector<std::array<unsigned,3>>{{255,60,60}}));
    // Every new binding also shares the old NEI shimmer in the MM host.
    const auto offMeshes = arena;
    const auto offPaths = paths;
    Reset(); enabled = 1;
    MM_DrawNeiGi(exported);
    assert(Drawn() == offPaths);
    assert(arena.size() == offMeshes.size() + (exported.itemShimmer ? 0 : 1));
    const auto shown = arena;
    arena.clear();
    NeiGi_DrawMesh(&play, NeiGi::SampleShimmer(play.gameplayFrames, true,
                       NeiGi_CameraBasis(&play), PresentationShimmer(item)));
    assert(arena.size() == 1);
    const auto &want = arena.front();
    size_t matching = 0;
    for (const auto &got : shown)
      if (got.size() == want.size() &&
          !std::memcmp(got.data(), want.data(), want.size() * sizeof(Vtx))) ++matching;
    assert(matching == 1);
    assert(stack.empty() && interpolation == 0 && matrix == 1);
    Reset(); // missing archives: let the original native item render
    exported = {};
    assert(NeiGi_DescribeEntry(&bound, &exported) == 0);
    if (item.translucent) {
        files.insert(item.opaque);
        assert(NeiGi_DescribeEntry(&bound, &exported) == 0); // incomplete split pass never queues half a GI
    }
    ++authored;
}
assert(authored == 68);
std::cout << "PASS real owner descriptors and MM host draw: all authored bindings, effects, split passes, concrete Somaria flame, missing-resource fallback\n";
Reset();

// The same callback represents six concrete wand awards; identity must select
// the new mesh and effect, independent of another game's inventory.
Reset(); files.insert("__OTR__objects/nei_gi_redesign/water_rod/gi_dl");
GetItemEntry water{}; water.drawFunc=Randomizer_DrawElementalWand; water.drawItemId=RG_WAND_WATER_ROD;
CwItemDrawInfo waterInfo{};
assert(NeiGi_DescribeEntry(&water,&waterInfo)==1);
assert(!strcmp(waterInfo.dlists[0],"__OTR__objects/nei_gi_redesign/water_rod/gi_dl"));
assert(waterInfo.neiEffect==int(Kind::Water));
Reset();

// Selected sword availability is owned by OoT even while MM is active.
Reset();
files.insert("__OTR__objects/nei_gi_redesign/master_sword/gi_dl");
files.insert("__OTR__alt/objects/object_custom_equip/gCustomMasterSwordDL");
ownerAlt=true; alt=false;
assert(OOT_GetNeiGiDrawInfo("master_sword",&exported)==0);
ownerAlt=false; alt=true;
assert(OOT_GetNeiGiDrawInfo("master_sword",&exported)==1);
Reset();
