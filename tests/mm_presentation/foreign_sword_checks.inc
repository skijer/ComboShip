// Exercise the actual MM-to-OoT foreign dispatch with its cached descriptor.
// The harness supplies the extracted ComboForeignDrawInfo/OOT_DrawComboForeign
// and selects the descriptor through selectedForeignInfo. Other drawer stubs
// increment foreignFallbackCalls; ComboDrawMaskShimmer is the real shared glue.
{
    const auto samePacked = [](const std::vector<Vtx>& a, const std::vector<Vtx>& b) {
        return a.size() == b.size() &&
               !std::memcmp(a.data(), b.data(), a.size() * sizeof(Vtx));
    };
    const auto occurrences = [&](const std::vector<std::vector<Vtx>>& meshes, const std::vector<Vtx>& wanted) {
        return std::count_if(meshes.begin(), meshes.end(),
                             [&](const auto& mesh) { return samePacked(mesh, wanted); });
    };
    const auto hasPackedHue = [](const std::vector<Vtx>& mesh, uint32_t hue) {
        return std::any_of(mesh.begin(), mesh.end(), [&](const Vtx& vertex) {
            return vertex.v.cn[0] == uint8_t(hue >> 16) &&
                   vertex.v.cn[1] == uint8_t(hue >> 8) &&
                   vertex.v.cn[2] == uint8_t(hue) && vertex.v.cn[3] >= 80;
        });
    };
    struct ForeignSword {
        Kind kind;
        const char* slug;
        uint32_t hue;
    };
    const ForeignSword swords[] = {
        { Kind::KokiriSword, "kokiri_sword", 0x78C850 },
        { Kind::MmKokiriSword, "mm_kokiri_sword", 0xA46CFF },
        { Kind::RazorSword, "razor_sword", 0xBDD6EA },
        { Kind::GildedSword, "gilded_sword", 0xFFD45A },
        { Kind::MasterSword, "master_sword", 0x6F8FFF },
        { Kind::BiggoronSword, "biggoron_sword", 0xFF9A42 },
        { Kind::GiantsKnife, "giants_knife", 0xCFD9E6 },
        { Kind::GreatFairySword, "great_fairy_sword", 0x79BE84 },
        { Kind::FourSword, "four_sword", 0x315B2F },
        { Kind::SwordAura, "true_master_sword", 0xFFF4D6 },
    };
    for (const auto& sword : swords) {
        assert(NeiGi::ColorHex(sword.kind) == sword.hue);
        const std::string path = std::string("__OTR__objects/nei_gi_redesign/") + sword.slug + "/gi_dl";
        ComboForeignDrawInfo foreign{};
        foreign.ok = true;
        foreign.drawKind = CW_DRAW_KIND_NEI_GI;
        foreign.count = 1;
        foreign.scale = 1;
        foreign.dls[0] = path.c_str();
        foreign.neiEffect = static_cast<int>(sword.kind);
        foreign.itemShimmer = true;
        ComboMaskShimmerColor(0, foreign.itemShimmerColor);
        CwItemDrawInfo native{};
        native.drawKind = foreign.drawKind;
        native.dlistCount = 1;
        native.xluStartIndex = -1;
        native.scale = foreign.scale;
        native.dlists[0] = foreign.dls[0];
        native.neiEffect = foreign.neiEffect;
        native.itemShimmer = 1;
        GetItemEntry foreignEntry{};
        foreignEntry.comboForeignCheck = RC_LINKS_POCKET;

        for (uint32_t frame : {0u, 42u, 179u, 180u, 360u}) {
            // Render the real samplers separately to obtain their packed UV,
            // alpha, position and color records, independent of either host.
            Reset();
            play.gameplayFrames = frame;
            const auto camera = NeiGi_CameraBasis(&play);
            const auto sampledShimmer = NeiGi::SampleShimmer(frame, true, camera, sword.kind);
            const auto fit = NeiGi::FrameFit(*NeiGi::FindSwordFrameBounds(sword.kind), 1.f, false);
            NeiGi_DrawMesh(&play, NeiGi::SampleSpecial(sword.kind, frame, camera, 1.f / fit.scale));
            NeiGi_DrawMesh(&play, sampledShimmer);
            assert(arena.size() == 2);
            const auto intrinsic = arena[0];
            const auto shimmer = arena[1];
            assert(hasPackedHue(intrinsic, sword.hue));
            if (sword.kind == Kind::GreatFairySword) {
                if (frame % 360u == 0) assert(hasPackedHue(shimmer, 0x79BE84));
                if (frame % 360u == 180) assert(hasPackedHue(shimmer, 0x9382C4));
            } else {
                assert(hasPackedHue(shimmer, sword.hue));
            }
            if (sword.kind == Kind::FourSword) {
                const uint32_t clusterHues[] = {0x315B2F, 0xD8232D, 0x2289CF, 0x6D3593, 0x315B2F};
                assert(sampledShimmer.count == 5 * 72);
                for (size_t cluster = 0; cluster < std::size(clusterHues); ++cluster) {
                    assert(sampledShimmer.vertices[cluster * 72].rgb == clusterHues[cluster]);
                    assert(hasPackedHue(intrinsic, clusterHues[cluster]));
                    assert(hasPackedHue(shimmer, clusterHues[cluster]));
                }
            }
            arena.clear();
            ComboDrawMaskShimmer(&play, nullptr, foreign.itemShimmerColor, "mm");
            assert(arena.size() == 1);
            const auto genericShimmer = arena.front();
            assert(!samePacked(shimmer, genericShimmer));

            // itemShimmer requests one palette-aware shimmer even when the
            // optional setting is off. Both host bodies must submit identical
            // intrinsic and shimmer meshes, without a generic mask overlay.
            for (int optionalEffects : {0, 1}) {
                Reset();
                play.gameplayFrames = frame;
                enabled = optionalEffects;
                MM_DrawNeiGi(native);
                assert(Drawn() == std::vector<std::string>{path});
                assert(arena.size() == 2);
                assert(occurrences(arena, intrinsic) == 1 && occurrences(arena, shimmer) == 1);
                const auto directMeshes = arena;

                Reset();
                play.gameplayFrames = frame;
                enabled = optionalEffects;
                selectedForeignInfo = &foreign;
                foreignFallbackCalls = 0;
                OOT_DrawComboForeign(&play, &foreignEntry);
                assert(Drawn() == std::vector<std::string>{path});
                if (occurrences(arena, shimmer) != 1)
                    std::cerr << "foreign sword palette shimmer missing: " << sword.slug
                              << " frame=" << frame << " optional=" << optionalEffects << '\n';
                assert(arena.size() == directMeshes.size());
                assert(occurrences(arena, intrinsic) == 1);
                assert(occurrences(arena, shimmer) == 1);
                assert(occurrences(arena, genericShimmer) == 0);
                assert(foreignFallbackCalls == 0 && vanilla == 0);
                assert(stack.empty() && interpolation == 0 && matrix == 1 && matrixY == 0);
            }

            // A descriptor without a mandatory shimmer obeys the optional
            // toggle while retaining the sword's intrinsic effect.
            foreign.itemShimmer = false;
            native.itemShimmer = 0;
            for (int optionalEffects : {0, 1}) {
                Reset();
                play.gameplayFrames = frame;
                enabled = optionalEffects;
                selectedForeignInfo = &foreign;
                foreignFallbackCalls = 0;
                OOT_DrawComboForeign(&play, &foreignEntry);
                assert(Drawn() == std::vector<std::string>{path});
                assert(arena.size() == size_t(1 + optionalEffects));
                assert(occurrences(arena, intrinsic) == 1);
                assert(occurrences(arena, shimmer) == optionalEffects);
                assert(occurrences(arena, genericShimmer) == 0);
                assert(foreignFallbackCalls == 0 && vanilla == 0);
                assert(stack.empty() && interpolation == 0 && matrix == 1 && matrixY == 0);
            }
            foreign.itemShimmer = true;
            native.itemShimmer = 1;
        }
    }

    // SIMPLE foreign masks retain their existing mandatory overlay, including
    // ordinary, transformation and remains palettes, with the setting disabled.
    for (int profile : {0, 1, 2, 3, 4, 5, 6, 7, 8}) {
        ComboForeignDrawInfo mask{};
        mask.ok = true;
        mask.drawKind = CW_DRAW_KIND_SIMPLE;
        mask.count = 1;
        mask.scale = 1;
        mask.dls[0] = "__OTR__objects/object_gi_mask/gi_dl";
        mask.itemShimmer = true;
        ComboMaskShimmerColor(profile, mask.itemShimmerColor);
        Reset();
        ComboDrawMaskShimmer(&play, nullptr, mask.itemShimmerColor, "mm");
        assert(arena.size() == 1);
        const auto overlay = arena.front();
        Reset();
        selectedForeignInfo = &mask;
        foreignFallbackCalls = 0;
        GetItemEntry maskEntry{};
        maskEntry.comboForeignCheck = RC_LINKS_POCKET;
        OOT_DrawComboForeign(&play, &maskEntry);
        assert(foreignFallbackCalls == 1 && vanilla == 0);
        assert(arena.size() == 1 && samePacked(arena.front(), overlay));
        assert(stack.empty() && interpolation == 0 && matrix == 1 && matrixY == 0);
    }
    selectedForeignInfo = nullptr;
    foreignFallbackCalls = 0;
    Reset();
    std::cout << "PASS real MM-to-OoT foreign sword dispatch: ten palettes, intrinsic particles, one shared shimmer, optional toggle and mask overlays\n";
}

// Exercise the real sentinel callback through EnGirlA, using the native MM
// producer's routed recipe and the current serialized replacement bounds.
// Removing a fit breaks the five resized swords' shared model/effect shelf pose.
{
    struct Bounds {
        CustomDrawFunc draw;
        const char *slug, *name, *callback;
        std::array<float, 3> minimum, maximum;
        float spinningWidth, drawScale;
        bool translucent;
        int identity = 0;
        int gid = -1;
    };
    const Bounds bounds[] = {
#include "nei_gi_bounds.inc"
    };
    struct SwordFit {
        CustomDrawFunc draw;
        RandoItemId id;
        NeiGi::ShopFit fit;
    };
    const SwordFit swords[] = {
        {Randomizer_DrawMasterSword, RI_OOT_MASTER_SWORD, NeiGi::kMasterSwordShopFit},
        {Randomizer_DrawTrueMasterSword, RI_OOT_TRUE_MASTER_SWORD, NeiGi::kMasterSwordShopFit},
        {Randomizer_DrawGildedSword, RI_SWORD_GILDED, NeiGi::kGildedSwordShopFit},
        {Randomizer_DrawProgressiveBGS, RI_OOT_BIGGORON_SWORD, NeiGi::kBiggoronSwordShopFit},
        {Randomizer_DrawGreatFairySword, RI_GREAT_FAIRY_SWORD, NeiGi::kGreatFairySwordShopFit},
    };
    size_t checked = 0;
    for (const auto& b : bounds) {
        const auto sword = std::find_if(std::begin(swords), std::end(swords),
                                       [&](const auto& sword) { return b.draw == sword.draw; });
        if (sword == std::end(swords))
            continue;
        ++checked;
        const auto id = sword->id;
        const auto* frameBounds = NeiGi::FindFrameBounds((std::string("__OTR__objects/nei_gi_redesign/") + b.slug + "/gi_dl").c_str());
        assert(frameBounds);
        const auto worldFit = NeiGi::FrameFit(*frameBounds,b.drawScale,false);
        const auto fit = NeiGi::FrameFit(*frameBounds,b.drawScale,true);
        const std::string path = std::string("__OTR__objects/nei_gi_redesign/") + b.slug + "/gi_dl";
        for (bool useAlt : {false, true}) {
            for (int optionalEffects : {0, 1}) {
                Reset();
                enabled = optionalEffects;
                ownerAlt = alt = useAlt;
                files.insert((useAlt ? "alt/" : "") + path);
                CwItemDrawInfo native{};
                assert(MM_DescribeNeiGi(id, &native));
                assert(native.drawKind == CW_DRAW_KIND_NEI_GI);
                assert(std::strncmp(native.dlists[0], useAlt ? "__OTR__@oot:" : "__OTR__@oot-gi-base:",
                                    useAlt ? 12 : 20) == 0);
                ComboForeignDrawInfo foreign{};
                foreign.ok = true;
                foreign.drawKind = native.drawKind;
                foreign.count = native.dlistCount;
                foreign.xluStart = native.xluStartIndex;
                foreign.scale = native.scale;
                foreign.neiEffect = native.neiEffect;
                std::copy(std::begin(native.neiEffectCenter), std::end(native.neiEffectCenter),
                          foreign.neiEffectCenter);
                foreign.itemShimmer = native.itemShimmer;
                std::copy_n(native.dlists, native.dlistCount, foreign.dls);
                selectedForeignInfo = &foreign;
                foreignFallbackCalls = 0;
                GetItemEntry entry{};
                entry.drawFunc = Randomizer_DrawComboForeign;
                entry.comboForeignCheck = RC_LINKS_POCKET;
                GetItemEntry_Draw(&play, entry);
                assert(Drawn(true) == std::vector<std::string>{native.dlists[0]});
                assert(!submitted.empty() && submitted.front() == std::pair(worldFit.scale*b.drawScale, worldFit.lift));
                const auto worldMatrices = submitted;
                const auto worldMeshes = arena;
                const auto worldEffectMatrices = effectSubmitted;
                submitted.clear();
                arena.clear();
                effectSubmitted.clear();
                gfx.polyOpa.p = opa;
                gfx.polyXlu.p = xlu;
                matrix = .25f;
                matrixY = 6;
                EnGirlA shop{};
                shop.actor.params = SI_RANDOMIZED_ITEM;
                shopEntry = entry;
                EnGirlA_Draw(&shop.actor, &play);
                assert(Drawn(true) == std::vector<std::string>{native.dlists[0]});
                const auto [scale, y] = submitted.front();
                const float height = scale * (b.maximum[1] - b.minimum[1]);
                std::cout << "foreign " << b.slug << ": shelf bottom=" << y + scale * b.minimum[1]
                          << ", height=" << height << '\n' << std::flush;
                assert(height <= 19.f);
                // Float32 parent/local composition can lose one ULP at the
                // exact .5 shelf floor; retain a two-millionth-unit allowance.
                assert(y + scale * b.minimum[1] >= .5f - .000002f);
                assert(scale * b.spinningWidth <= 19.f);
                assert(submitted.size() == worldMatrices.size() && arena.size() == worldMeshes.size());
                for (size_t i = 0; i < submitted.size(); ++i) {
                    assert(std::abs(submitted[i].first - worldMatrices[i].first * .25f * fit.scale/worldFit.scale) < .000001f);
                    assert(std::abs(submitted[i].second - (6.f + .25f * fit.lift +
                        (worldMatrices[i].second-worldFit.lift) * .25f * fit.scale/worldFit.scale)) < .000001f);
                }
                assert(arena.size() == 2 && effectSubmitted.size() == 2 && worldEffectMatrices.size() == 2);
                // Quantized vertex-cache deduplication can pack a different
                // number of intrinsic vertices when glyph dimensions change.
                const auto sameAttributes = [](const Vtx& a, const Vtx& b) {
                    return !std::memcmp(a.v.cn, b.v.cn, 4) &&
                           !std::memcmp(a.v.tc, b.v.tc, 2 * sizeof(int16_t));
                };
                for (const auto& v : arena[0])
                    assert(std::any_of(worldMeshes[0].begin(), worldMeshes[0].end(),
                                       [&](const Vtx& w) { return sameAttributes(v, w); }));
                for (const auto& v : worldMeshes[0])
                    assert(std::any_of(arena[0].begin(), arena[0].end(),
                                       [&](const Vtx& w) { return sameAttributes(v, w); }));
                // The hex shimmer keeps its exact packed geometry. Intrinsic
                // glyphs cancel shelf framing while the caller's .25 scale remains.
                assert(arena[1].size() == worldMeshes[1].size() &&
                       !std::memcmp(arena[1].data(), worldMeshes[1].data(), arena[1].size() * sizeof(Vtx)));
                const size_t tip = native.neiEffect == int(Kind::GreatFairySword) ? 2 : 1;
                const auto span = [tip](const auto& mesh) {
                    float squared = 0.f;
                    for (int axis = 0; axis < 3; ++axis) {
                        const float d = float(mesh[0].v.ob[axis] - mesh[tip].v.ob[axis]);
                        squared += d * d;
                    }
                    return std::sqrt(squared);
                };
                const float worldSpan = span(worldMeshes[0]) * worldEffectMatrices[0].first;
                const float shelfSpan = span(arena[0]) * effectSubmitted[0].first / .25f;
                assert(std::abs(worldSpan - shelfSpan) < .1f && "foreign shelf framing shrank sword particles");
                assert(foreignFallbackCalls == 0 && vanilla == 0);
                assert(stack.empty() && interpolation == 0 && matrix == .25f && matrixY == 6);
            }
        }
    }
    assert(checked == 5);
    ownerAlt = false;
    selectedForeignInfo = nullptr;
    Reset();
    std::cout << "PASS foreign shelves for five resized swords: actual sentinel callback, base/Alt bounds, shared mesh/effect pose and bounded world draw\n";
}

// A shelf fit never turns an unrelated, unresolved or legacy recipe into an
// authored replacement. The existing dispatcher remains responsible for its
// fallback, including a native producer without replacement resources.
{
    ComboForeignDrawInfo foreign{};
    foreign.ok = true;
    foreign.drawKind = CW_DRAW_KIND_NEI_GI;
    foreign.count = 1;
    foreign.scale = 1;
    foreign.dls[0] = "__OTR__@oot:objects/nei_gi_redesign/biggoron_sword/gi_dl";
    foreign.neiEffect = static_cast<int>(Kind::BiggoronSword);
    GetItemEntry entry{};
    entry.drawFunc = Randomizer_DrawComboForeign;
    entry.comboForeignCheck = RC_LINKS_POCKET;
    for (int rejected : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}) {
        Reset();
        matrix = .25f;
        matrixY = 6;
        selectedForeignInfo = &foreign;
        auto candidate = foreign;
        auto candidateEntry = entry;
        selectedForeignInfo = &candidate;
        switch (rejected) {
            case 0: selectedForeignInfo = nullptr; break;
            case 1: candidateEntry.comboForeignCheck = RC_UNKNOWN_CHECK; break;
            case 2: candidateEntry.drawFunc = UnrelatedDraw; break;
            case 3: candidate.drawKind = CW_DRAW_KIND_GORON_SWORD; break;
            case 4: candidate.neiEffect = static_cast<int>(Kind::KokiriSword); break;
            case 5: candidate.count = 0; break;
            case 6: candidate.dls[0] = nullptr; break;
            case 7: candidate.ok = false; break;
            case 8: candidate.count = 2; candidate.xluStart = 1; break;
            case 9: candidate.scale = 0; break;
        }
        assert(!NeiGi_DrawShop(&play, &candidateEntry));
        assert(submitted.empty() && arena.empty() && Drawn().empty());
        assert(stack.empty() && interpolation == 0 && matrix == .25f && matrixY == 6);
        // Legacy swords retain their original draw callback and scale.
        if (rejected == 3) {
            foreignFallbackCalls = 0;
            shopEntry = candidateEntry;
            EnGirlA shop{};
            shop.actor.params = SI_RANDOMIZED_ITEM;
            EnGirlA_Draw(&shop.actor, &play);
            assert(foreignFallbackCalls == 1 && vanilla == 0);
            assert(matrix == .25f * 7.f && matrixY == 6 && stack.empty());
        }
    }
    Reset();
    ownerAlt = false;
    CwItemDrawInfo missing{};
    assert(!MM_DescribeNeiGi(RI_OOT_MASTER_SWORD, &missing));
    assert(!MM_DescribeNeiGi(RI_OOT_TRUE_MASTER_SWORD, &missing));
    assert(!MM_DescribeNeiGi(RI_SWORD_GILDED, &missing));
    assert(!MM_DescribeNeiGi(RI_OOT_BIGGORON_SWORD, &missing));
    assert(!MM_DescribeNeiGi(RI_GREAT_FAIRY_SWORD, &missing));
    selectedForeignInfo = nullptr;
    matrix = .25f;
    matrixY = 6;
    shopEntry = entry;
    EnGirlA shop{};
    shop.actor.params = SI_RANDOMIZED_ITEM;
    EnGirlA_Draw(&shop.actor, &play);
    assert(vanilla == 1 && Drawn().empty() && submitted.empty());
    assert(matrix == .25f && matrixY == 6 && stack.empty());
    assert(!NeiGi_DrawShop(nullptr, &entry) && !NeiGi_DrawShop(&play, nullptr));
    selectedForeignInfo = nullptr;
    Reset();
    std::cout << "PASS foreign shelf guards: unrelated/unknown/native-missing/legacy/incomplete recipes preserve fallback poses\n";
}
