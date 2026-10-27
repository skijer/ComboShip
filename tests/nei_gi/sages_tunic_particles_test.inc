struct MedallionDraw {
    std::string face;
    ParticleProbe::Pose pose;
};

std::vector<MedallionDraw> Medallions() {
    using namespace Fixture;
    std::vector<MedallionDraw> result;
    size_t matrix = 0;
    const ParticleProbe::Pose* current = nullptr;
    bool setup26 = false;
    int bodies = 0;
    for (Gfx* p=opa; p!=gfx.polyOpa.p; ++p) {
        const auto op = p->words.w0 >> 24;
        if (op == G_DL) setup26 = p->words.w1 == reinterpret_cast<uintptr_t>(&ParticleProbe::setup26);
        if (op == G_MTX) current = &ParticleProbe::submitted.at(matrix++);
        if (op != G_DL_OTR_FILEPATH) continue;
        const char* path = reinterpret_cast<const char*>(p->words.w1);
        if (std::strstr(path,"MedallionFaceDL")) {
            assert(setup26 && current && "Sage medallions require native SETUPDL_26 and their own matrix");
            result.push_back({path,*current});
        } else if (std::strstr(path,"gGiMedallionDL")) ++bodies;
    }
    assert(bodies == int(result.size()) && "each colored face must keep its medallion body");
    return result;
}

void SameMedallions(const std::vector<MedallionDraw>& expected, float scale, float lift) {
    const auto got = Medallions();
    assert(got.size() == expected.size() && "authored Sage mesh bypassed its intrinsic medallion fountain");
    for (size_t i=0;i<got.size();++i) {
        assert(got[i].face == expected[i].face && "medallion identity/color resource changed");
        for (int row=0;row<4;++row) for(int col=0;col<4;++col) {
            float want = expected[i].pose[row*4+col];
            if (row<3) want *= scale;
            if (row==1 && col==3) want += lift;
            assert(std::fabs(got[i].pose[row*4+col]-want) < .0002f &&
                   "Sage medallion launch, gravity, spin, pop-in or shrink-out changed");
        }
    }
}

int main() {
    using namespace Fixture;
    const char* model = "__OTR__objects/nei_gi_redesign/sages_tunic/gi_dl";
    const char* faces[] = {gGiForestMedallionFaceDL,gGiFireMedallionFaceDL,gGiWaterMedallionFaceDL,
                          gGiSpiritMedallionFaceDL,gGiShadowMedallionFaceDL,gGiLightMedallionFaceDL};
    auto prepare = [&](int frame, int effects, int selectedAlt) {
        Reset(); enabled=effects; alt=selectedAlt;
        play.gameplayFrames=frame; play.state.frames=frame;
        files.insert(model);
        for(const auto* face:faces) files.insert(face);
        files.insert(gGiMedallionDL);
    };
    GetItemEntry entry{};entry.drawFunc=Randomizer_DrawExtSagesTunic;
    const auto* bounds=NeiGi::FindFrameBounds(model);assert(bounds);
    for (int frame=0;frame<80;++frame) {
        prepare(frame,0,0);
        Randomizer_DrawExtSagesTunic(&play,&entry);
        const auto baseline=Medallions();
        assert(baseline.size()>=4 && stack.empty());
        for(int effects:{0,1}) for(int selectedAlt:{0,1}) for(bool shop:{false,true}) {
            prepare(frame,effects,selectedAlt);
            assert(shop ? NeiGi_DrawShop(&play,&entry) : NeiGi_Draw(&play,&entry));
            const auto fit=NeiGi::FrameFit(*bounds,1,shop);
            SameMedallions(baseline,fit.scale,fit.lift);
            const auto paths=Drawn();
            assert(std::count(paths.begin(),paths.end(),model) == 1);
            assert(arena.size() == 1 && "intrinsic Sage shimmer must survive either optional-effects setting");
            assert(stack.empty() && matrix==1 && matrixY==0);

            // The same shared composition is used by imported MM items and
            // foreign receipts. Owner selection must remain balanced.
            prepare(frame,effects,selectedAlt);
            CwItemDrawInfo info{};
            info.drawKind=CW_DRAW_KIND_NEI_GI;
            info.dlists[0]="__OTR__@oot:objects/nei_gi_redesign/sages_tunic/gi_dl";
            info.dlistCount=1;info.xluStartIndex=-1;info.scale=1;
            info.neiEffect=int(NeiGi::Kind::Neutral);info.itemShimmer=1;
            info.neiShimmer=int(NeiGi::Kind::SagesTunic)+1;
            MM_DrawNeiGi(info,shop,shop?0:1);
            const auto foreignFit=NeiGi::FrameFit(*bounds,1,shop,shop?0:1);
            SameMedallions(baseline,foreignFit.scale,foreignFit.lift);
            assert(arena.size()==1 && stack.empty() && matrix==1 && matrixY==0);
            for(auto [begin,end]:{std::pair(opa,gfx.polyOpa.p),std::pair(xlu,gfx.polyXlu.p)}) {
                int ownerDepth=0;
                for(auto* p=begin;p!=end;++p) {
                    const auto op=p->words.w0>>24;
                    if(op==G_COMBO_RM_PUSH) {
                        assert(!std::strcmp(reinterpret_cast<const char*>(p->words.w1),"oot"));
                        ++ownerDepth;
                    }
                    if(op==G_COMBO_RM_POP) assert(--ownerDepth>=0);
                }
                assert(ownerDepth==0 && "medallion or model resource owner leaked into the next item");
            }

            prepare(frame,effects,selectedAlt);
            files.erase(model); // Actual legacy callback is retained when the authored pass is absent.
            assert(NeiGi_Draw(&play,&entry));
            SameMedallions(baseline,1,0);
            const auto fallbackPaths=Drawn();
            assert(std::find(fallbackPaths.begin(),fallbackPaths.end(),model)==fallbackPaths.end());
        }
    }
    // Resource absence only suppresses the missing original medals; the new
    // tunic remains drawable with the selected shimmer policy.
    for(int missing=0;missing<7;++missing) {
        prepare(9,0,0);
        files.erase(missing==6 ? gGiMedallionDL : faces[missing]);
        assert(NeiGi_Draw(&play,&entry));
        assert(Medallions().size()==size_t(missing==6?0:5));
        const auto paths=Drawn();
        assert(std::count(paths.begin(),paths.end(),model)==1 && stack.empty());
    }
    // No particle may consume the caller's reserved model or restore tail.
    for(int capacity=0;capacity<160;++capacity) {
        prepare(9,1,0); gfx.polyOpa.d=opa+capacity;
        assert(NeiGi_Draw(&play,&entry));
        assert(gfx.polyOpa.p<=gfx.polyOpa.d && gfx.polyXlu.p<=gfx.polyXlu.d && stack.empty());
    }
    std::cout << "PASS Sage's Tunic: six original medallions retain exact animated poses, fallback, independent shimmer and shelf fit\n";
}
