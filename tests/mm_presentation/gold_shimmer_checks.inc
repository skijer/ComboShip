// A winning model override receives its selected mesh and one optional gold
// overlay in both foreign host dispatchers, even with no legacy RGB fallback.
{
    const auto hasGold=[](const std::vector<Vtx>& mesh) {
        return std::any_of(mesh.begin(),mesh.end(),[](const Vtx& vertex) {
            return vertex.v.cn[0]==255 && vertex.v.cn[1]==212 && vertex.v.cn[2]==90;
        });
    };
    for (const char* slug : {"trident","roc_boots"}) for (int optional : {0,1}) {
        const std::string path=std::string("__OTR__objects/nei_gi_redesign/")+slug+"/gi_dl";
        ComboForeignDrawInfo ootInfo{};
        ootInfo.ok=true;
        ootInfo.drawKind=CW_DRAW_KIND_CUSTOM_GI;
        ootInfo.count=1;
        ootInfo.scale=1;
        ootInfo.dls[0]=path.c_str();
        ootInfo.neiShimmer=int(Kind::Gold)+1;
        ootInfo.itemShimmer=optional;
        Reset(); enabled=optional;
        selectedForeignInfo=&ootInfo;
        foreignFallbackCalls=0;
        GetItemEntry entry{}; entry.comboForeignCheck=RC_LINKS_POCKET;
        OOT_DrawComboForeign(&play,&entry);
        assert(foreignFallbackCalls==1 && arena.size()==size_t(optional) && stack.empty());
        if (optional) assert(hasGold(arena.front()));

        ComboForeignDrawInfoOOT mmInfo{};
        mmInfo.ok=true;
        mmInfo.drawKind=CW_DRAW_KIND_CUSTOM_GI;
        mmInfo.count=1;
        mmInfo.scale=1;
        mmInfo.dls[0]=path.c_str();
        mmInfo.neiShimmer=int(Kind::Gold)+1;
        mmInfo.itemShimmer=optional;
        Reset(); enabled=optional;
        selectedCustomLists=true;
        files.insert(path);
        selectedForeignInfoMM=&mmInfo;
        MM_DrawComboForeign(1);
        assert(Drawn()==std::vector<std::string>{path});
        assert(arena.size()==size_t(optional) && stack.empty());
        if (optional) assert(hasGold(arena.front()));
    }
    selectedForeignInfo=nullptr;
    selectedForeignInfoMM=nullptr;
    std::cout<<"PASS selected Trident/Roc's Boots overrides: gold optional shimmer in both foreign hosts\n";
}
