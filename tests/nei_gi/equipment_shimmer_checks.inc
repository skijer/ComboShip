// Item identity survives archive selection and the optional ItemEffects toggle.
{
    struct EquipmentTheme { CustomDrawFunc draw; const char* slug; uint32_t primary, secondary; };
    const EquipmentTheme themes[] = {
        {Randomizer_DrawExtDivineShield,"divine_shield",0x875FCD,0xCFD9E6},
        {Randomizer_DrawExtSheikahShield,"sheikah_shield",0xDC4141,0x4090FF},
        {Randomizer_DrawExtShieldOfIkana,"shield_of_ikana",0xAFB4DC,0xFFD45A},
        {Randomizer_DrawExtMagicCape,"magic_cape",0xFFD45A,0xB80B0E},
        {Randomizer_DrawExtSpiritBreastplate,"spirit_breastplate",0xEB6E14,0xEB6E14},
        {Randomizer_DrawExtSagesTunic,"sages_tunic",0xF0F4FA,0xFFD45A},
        {Randomizer_DrawExtChampionsTunic,"champions_tunic",0x30CAFA,0x30CAFA},
        {Randomizer_DrawExtPegasusAnklet,"pegasus_anklet",0xFFD45A,0x16AB94},
        {Randomizer_DrawExtTrident,"trident",0xFFD45A,0xFFD45A},
        {Randomizer_DrawExtClimbBoots,"climb_boots",0xCFAC2E,0xCFD9E6},
        {Randomizer_DrawExtRocBoots,"roc_boots",0xFFD45A,0xFFD45A},
        {Randomizer_DrawExtCaneOfByrna,"cane_of_byrna",0x5A90FF,0x5A90FF},
        {Randomizer_DrawExtPendantOfMemories,"pendant_of_memories",0xFFD45A,0x4090FF},
        {Randomizer_DrawIronKnuckleAxe,"iron_knuckle_axe",0xCFD9E6,0xCFD9E6},
    };
    std::vector<Vtx> expectedStars;
    const auto checkStars=[&](const EquipmentTheme& theme) {
        int stars=0;
        for(const auto& vertices:arena) {
            if(vertices.size()!=expectedStars.size() || memcmp(vertices.data(),expectedStars.data(),vertices.size()*sizeof(Vtx)))continue;
            ++stars;bool primary=false,secondary=false,white=false;
            for(const auto& vertex:vertices) {
                const uint32_t rgb=uint32_t(vertex.v.cn[0])<<16|uint32_t(vertex.v.cn[1])<<8|vertex.v.cn[2];
                primary|=rgb==theme.primary;secondary|=rgb==theme.secondary;white|=rgb==0xFFFFFF;
                assert(rgb==theme.primary||rgb==theme.secondary||rgb==0xFFFFFF);
            }
            assert(primary&&secondary&&white);
        }
        if(stars!=1)std::cerr<<"equipment shimmer mismatch: "<<theme.slug<<" meshes="<<arena.size()<<" matches="<<stars<<"\n";
        assert(stars==1 && "each equipment draw needs one identity shimmer");
        assert(stack.empty()&&matrix==1&&matrixY==0);
    };
    for(const auto& theme:themes)for(int optional:{0,1}) {
        entry={};entry.drawFunc=theme.draw;
        const auto* binding=FindPresentation(&entry);
        assert(binding&&binding->alwaysShimmer&&NeiGi::ColorHex(PresentationShimmer(*binding))==theme.primary);
        assert(NeiGi::FindFrameBounds(binding->opaque)->effect==binding->effect);
        if(binding->effect==Kind::Neutral) {
            const auto shimmer=PresentationShimmer(*binding);
            assert(!NeiGi::IsSword(shimmer)&&!NeiGi::IsRod(shimmer)&&!NeiGi::IsSpell(shimmer)&&!NeiGi::IsSpecial(shimmer));
        }
        Reset();
        NeiGi_DrawMesh(&play,NeiGi::SampleShimmer(play.gameplayFrames,true,NeiGi_CameraBasis(&play),PresentationShimmer(*binding)));
        assert(arena.size()==1);expectedStars=arena.front();
        for(bool present:{false,true}) {
            Reset();enabled=optional;
            if(present)files.insert(binding->opaque);
            assert(NeiGi_Draw(&play,&entry));
            assert(fallback==!present);
            checkStars(theme);
        }
#ifdef COMBO_BUILD
        for(bool present:{false,true}) {
            Reset();enabled=optional;ownerAlt=true;
            if(present)files.insert(binding->opaque);
            CwItemDrawInfo info{};
            assert(OOT_GetNeiGiDrawInfoForAssets(theme.slug,1,&info)==int(present));
            assert(info.itemShimmer&&info.neiShimmer==int(PresentationShimmer(*binding))+1&&info.stateDependent==2);
            if(present)assert(info.neiEffect==int(binding->effect));
            for(const auto& mm:kBindings)if(!strcmp(mm.slug,theme.slug)) {
                CwItemDrawInfo native{};
                assert(MM_DescribeNeiGi(mm.id,&native)==present);
                assert(native.itemShimmer&&native.neiShimmer==info.neiShimmer);
                gfx.polyOpa.p=opa;gfx.polyXlu.p=xlu;arena.clear();
                if(present)MM_DrawNeiGi(native);
                else {MM_NeiGiFallbackShimmer overlay(mm.id);}
                checkStars(theme);
            }
            // Exercise both actual foreign dispatcher endings, including their
            // old upper-ID guards, with the producer's declined mesh identity.
            Reset();enabled=optional;
            ComboForeignDrawInfo oot{};oot.ok=true;oot.drawKind=CW_DRAW_KIND_SIMPLE;
            oot.count=1;oot.xluStart=-1;oot.scale=1;oot.dls[0]="__OTR__fixture/equipment";
            oot.itemShimmer=info.itemShimmer;oot.neiShimmer=info.neiShimmer;
            selectedForeignInfo=&oot;
            GetItemEntry foreign{};foreign.comboForeignCheck=RC_LINKS_POCKET;
            OOT_DrawComboForeign(&play,&foreign);checkStars(theme);
            Reset();enabled=optional;selectedCustomLists=true;
            ComboForeignDrawInfoOOT mm{};mm.ok=true;mm.drawKind=CW_DRAW_KIND_SIMPLE;
            mm.count=1;mm.xluStart=-1;mm.scale=1;mm.dls[0]="__OTR__fixture/equipment";
            mm.itemShimmer=info.itemShimmer;mm.neiShimmer=info.neiShimmer;
            selectedForeignInfoMM=&mm;
            MM_DrawComboForeign(1);checkStars(theme);
            selectedForeignInfo=nullptr;selectedForeignInfoMM=nullptr;
            if(present) {
                for(int host:{0,1}) {
                    Reset();enabled=optional;
                    if(host==0) {
                        oot.drawKind=CW_DRAW_KIND_NEI_GI;oot.dls[0]=info.dlists[0];oot.scale=info.scale;
                        oot.neiEffect=info.neiEffect;
                        selectedForeignInfo=&oot;
                        OOT_DrawComboForeign(&play,&foreign);
                    } else {
                        mm.drawKind=CW_DRAW_KIND_NEI_GI;mm.dls[0]=info.dlists[0];mm.scale=info.scale;
                        mm.neiEffect=info.neiEffect;
                        selectedForeignInfoMM=&mm;
                        MM_DrawComboForeign(1);
                    }
                    checkStars(theme);
                    selectedForeignInfo=nullptr;selectedForeignInfoMM=nullptr;
                }
            }
        }
#endif
    }
    for(int optional:{0,1})for(bool present:{false,true}) {
        entry={};entry.drawFunc=Randomizer_DrawExtFourSword;
        const auto* binding=FindPresentation(&entry);assert(binding&&binding->alwaysShimmer);
        Reset();NeiGi_DrawMesh(&play,NeiGi::SampleShimmer(play.gameplayFrames,true,NeiGi_CameraBasis(&play),Kind::FourSword));
        const auto stars=arena.front();
        Reset();enabled=optional;alt=1;
#ifdef COMBO_BUILD
        ownerAlt=true;
#endif
        if(present)files.insert(binding->opaque);
        assert(NeiGi_Draw(&play,&entry));
        assert(std::count_if(arena.begin(),arena.end(),[&](const auto& mesh){return mesh.size()==stars.size()&&!memcmp(mesh.data(),stars.data(),stars.size()*sizeof(Vtx));})==1);
        for(uint32_t color:NeiGi::FourSwordColors) {
            bool shown=false;
            for(const auto& mesh:arena)for(const auto& vertex:mesh)
                shown|=(uint32_t(vertex.v.cn[0])<<16|uint32_t(vertex.v.cn[1])<<8|vertex.v.cn[2])==color;
            assert(shown);
        }
        assert(stack.empty()&&matrix==1);
    }
    std::cout<<"PASS all 15 added equipment entries: exact star palettes, intrinsic shimmer, native/foreign and missing-model fallback\n";
    for(int optional:{0,1}) {
        Reset();enabled=optional;
        entry={};entry.drawFunc=Randomizer_DrawMmTradeQuest;
        entry.drawItemId=RG_MM_PENDANT_OF_MEMORIES;
        entry.getItemId=RG_MM_PENDANT_OF_MEMORIES;
        const auto* binding=FindPresentation(&entry);
        assert(binding&&binding->opaque==nullptr&&binding->alwaysShimmer&&PresentationShimmer(*binding)==Kind::PendantOfMemories);
        assert(NeiGi_Draw(&play,&entry)&&fallback==1);
        const auto shown=arena;
        Reset();NeiGi_DrawMesh(&play,NeiGi::SampleShimmer(play.gameplayFrames,true,NeiGi_CameraBasis(&play),Kind::PendantOfMemories));
        assert(shown.size()==1&&shown.front().size()==arena.front().size()&&!memcmp(shown.front().data(),arena.front().data(),arena.front().size()*sizeof(Vtx)));
#ifdef COMBO_BUILD
        CwItemDrawInfo alias{};
        assert(!NeiGi_DescribeEntry(&entry,&alias)&&alias.itemShimmer&&alias.neiShimmer==int(Kind::PendantOfMemories)+1&&alias.stateDependent==2);
#endif
        entry.drawItemId=entry.getItemId=RG_MM_MOONS_TEAR;
        assert(!FindPresentation(&entry));
    }
    std::cout<<"PASS winning imported Pendant alias keeps its native trade callback and gold/blue shimmer\n";
}
