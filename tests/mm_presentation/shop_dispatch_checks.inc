// Execute the actual MM shelf callback, item conversion/context dispatcher,
// native selector and actual foreign dispatch against the shared renderer.
Reset();ownerAlt=false;
files.insert("__OTR__objects/nei_gi_redesign/four_sword/gi_dl");
MmShopEnGirlA mmShelf{};mmShelf.actor.id=ACTOR_EN_GIRLA;
mmShelf.actor.world.rot.z=1;mmShopChecks[1].randoItemId=RI_OOT_EXT_FOUR_SWORD;
matrix=.25f;matrixY=6;
EnGirlA_RandoDrawFunc(&mmShelf.actor,&play);
const auto* mmFour=NeiGi::FindFrameBounds("__OTR__objects/nei_gi_redesign/four_sword/gi_dl");
assert(mmFour && !submitted.empty());
assert(submitted.front().second+submitted.front().first*mmFour->minimum.y>=.5f &&
       "actual native MM shop callback did not propagate shop fitting");
for(const auto& binding:kBindings) {
    for(bool foreign:{false,true}) {
        Reset();ownerAlt=false;
        const auto path=std::string("__OTR__objects/nei_gi_redesign/")+binding.slug+"/gi_dl";
        files.insert(path);files.insert(path.substr(0,path.size()-5)+"gi_xlu_dl");
        CwItemDrawInfo recipe{};assert(MM_DescribeNeiGi(binding.id,&recipe));
        const auto* bounds=NeiGi::FindFrameBounds(recipe.dlists[0]);assert(bounds);
        const auto fit=NeiGi::FrameFit(*bounds,recipe.scale,true);
        ComboForeignDrawInfoOOT mmForeign{};mmForeign.ok=true;
        mmForeign.count=recipe.dlistCount;mmForeign.xluStart=recipe.xluStartIndex;
        mmForeign.scale=recipe.scale;mmForeign.drawKind=recipe.drawKind;
        mmForeign.neiEffect=recipe.neiEffect;mmForeign.neiShimmer=recipe.neiShimmer;
        std::copy_n(recipe.dlists,recipe.dlistCount,mmForeign.dls);
        std::copy_n(recipe.neiEffectCenter,3,mmForeign.neiEffectCenter);
        mmForeign.itemShimmer=recipe.itemShimmer;mmForeign.neiSomariaUpgrade=recipe.neiSomariaUpgrade;
        selectedForeignInfoMM=&mmForeign;
        mmShopChecks[1].randoItemId=foreign?RI_COMBO_FOREIGN:binding.id;
        matrix=.25f;matrixY=6;mmShopLegacyDraws=0;
        EnGirlA_RandoDrawFunc(&mmShelf.actor,&play);
        assert(mmShopLegacyDraws==0 && !submitted.empty());
        const auto [scale,y]=submitted.front();
        assert(std::abs(scale-.25f*fit.scale*recipe.scale)<.00001f);
        assert(std::abs(y-(6+.25f*fit.lift))<.00001f);
        assert(y+scale*bounds->minimum.y>=.5f-.00001f);
        assert(y+scale*bounds->maximum.y <= (std::strcmp(binding.slug,"rocs_feather")?19.f:21.5f)+.00001f);
        assert(scale*bounds->spinningWidth<=19.f+.00001f);
        assert(matrix==.25f && matrixY==6 && stack.empty());
        // The exact same production item path keeps its world fit without an
        // EnGirlA actor; this includes imported/foreign descriptions.
        Reset();mmShopLegacyDraws=0;selectedForeignInfoMM=&mmForeign;
        files.insert(path);files.insert(path.substr(0,path.size()-5)+"gi_xlu_dl");
        const auto worldFit=NeiGi::FrameFit(*bounds,recipe.scale,false);
        Rando::DrawItem(foreign?RI_COMBO_FOREIGN:binding.id,1,nullptr);
        assert(mmShopLegacyDraws==0 && !submitted.empty());
        assert(std::abs(submitted.front().first-worldFit.scale*recipe.scale)<.00001f);
        assert(std::abs(submitted.front().second-worldFit.lift)<.00001f);
        assert(matrix==1 && matrixY==0 && stack.empty());
    }
}
selectedForeignInfoMM=nullptr;Reset();ownerAlt=false;
std::cout<<"PASS actual MM shop callback/context/native+foreign dispatch: every binding fits shelf bounds; world draws retain their fit\n";
