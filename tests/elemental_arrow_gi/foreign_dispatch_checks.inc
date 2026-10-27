{
    // Execute both production foreign dispatchers and the real simple core
    // handlers. A carried shimmer flag must not duplicate intrinsic shimmer.
    for (int profile = 1; profile <= 3; ++profile) for (int assets : {0,1}) for (int effects : {0,1}) {
        Reset(); alt=ownerAlt=assets; enabled=effects; portableSongLists=true;
        ComboForeignDrawInfoOOT oot{};
        oot.ok=true; oot.drawKind=CW_DRAW_KIND_ELEMENTAL_ARROW; oot.neiEffect=profile;
        oot.count=1; oot.xluStart=-1; oot.scale=1; oot.itemShimmer=true;
        oot.dls[0]="__OTR__@oot:objects/object_gi_m_arrow/gGiMagicArrowDL";
        selectedForeignInfoMM=&oot;
        MM_DrawComboForeign(1);
        assert(Drawn()==std::vector<std::string>{oot.dls[0]} && arena.size()==3 && stack.empty());
        Reset(); alt=ownerAlt=assets; enabled=effects; portableSongLists=true;
        ComboForeignDrawInfo mm{};
        mm.ok=true; mm.drawKind=CW_DRAW_KIND_ELEMENTAL_ARROW; mm.neiEffect=profile;
        mm.count=1; mm.xluStart=-1; mm.scale=1; mm.itemShimmer=true;
        mm.dls[0]="__OTR__@mm:objects/object_gi_m_arrow/gGiMagicArrowAmmoDL";
        GetItemEntry entry{}; entry.comboForeignCheck=1;
        selectedForeignInfo=&mm;
        OOT_DrawComboForeign(&play,&entry);
        assert(Drawn()==std::vector<std::string>{mm.dls[0]} && arena.size()==3 && stack.empty());
    }
    for(int p=1;p<=3;++p) for(bool shelf:{false,true}) {
        Reset(); portableSongLists=true;
        ComboForeignDrawInfoOOT spell{};
        spell.ok=true;spell.drawKind=CW_DRAW_KIND_MAGIC_SPELL;spell.neiEffect=p;
        spell.count=3;spell.xluStart=0;spell.scale=1;spell.itemShimmer=true;
        spell.dls[0]="__OTR__@oot:objects/object_gi_goddess/gGiMagicSpellDiamondDL";
        spell.dls[1]="__OTR__@oot:objects/object_gi_goddess/gGiDinsFireColorDL";
        spell.dls[2]="__OTR__@oot:objects/object_gi_goddess/gGiMagicSpellOrbDL";
        selectedForeignInfoMM=&spell;
        MM_DrawComboForeign(1,shelf);
        assert(Drawn().empty() && arena.size()==5 && stack.empty());
        Reset();
        const RandoItemId items[]={RI_OOT_DINS_FIRE,RI_OOT_FARORES_WIND,RI_OOT_NAYRUS_LOVE};
        assert(MM_TryDrawNeiGi(items[p-1],shelf,0) && arena.size()==5 && stack.empty());
        // Execute the MM producer and reverse foreign dispatcher, retaining
        // the imported spell's MM editor namespace in an OoT location.
        Reset();
        CwItemDrawInfo raw{};
        assert(MM_FillElementalSpellDrawInfo(items[p-1],&raw)==1);
        assert(raw.drawKind==CW_DRAW_KIND_MAGIC_SPELL && raw.neiEffect==p && raw.dlistCount==3 &&
               raw.dlistCount>=CwMinDlistsForKind(raw.drawKind) && raw.opCount==0);
        ComboForeignDrawInfo imported{};
        imported.ok=true;imported.drawKind=raw.drawKind;imported.neiEffect=raw.neiEffect;
        imported.count=raw.dlistCount;imported.xluStart=raw.xluStartIndex;imported.scale=raw.scale;imported.itemShimmer=true;
        for(int i=0;i<3;++i) imported.dls[i]=raw.dlists[i];
        selectedForeignInfo=&imported;
        GetItemEntry entry{};entry.comboForeignCheck=1;entry.drawFunc=Randomizer_DrawComboForeign;
        if(shelf) assert(OOT_DrawComboForeignShop(&play,&entry));
        else OOT_DrawComboForeign(&play,&entry);
        assert(Drawn().empty() && arena.size()==5 && stack.empty());
        Reset(); portableSongLists=true; gfx.polyOpa.d=opa+128;
        spellFallbackAllowed=true;
        if(shelf) assert(OOT_DrawComboForeignShop(&play,&entry));
        else OOT_DrawComboForeign(&play,&entry);
        spellFallbackAllowed=false;
        assert(Drawn()==std::vector<std::string>({imported.dls[0],imported.dls[1],imported.dls[2]}));
        assert(allocations==1 && stack.empty() && gfx.polyOpa.p<=gfx.polyOpa.d && gfx.polyXlu.p<=gfx.polyXlu.d);
    }
    std::cout << "PASS native/imported spell dispatch: each profile, held/shelf, one intrinsic shimmer, no old diamond/orb duplicate\n";
    selectedForeignInfo=nullptr; selectedForeignInfoMM=nullptr;
    std::cout << "PASS real elemental-arrow foreign dispatch: both owner cores, Alt/toggle parity, exactly one intrinsic shimmer\n";
}
