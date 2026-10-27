// Appended to the same controlled graphics/resource seam as fallback tests.
// Both actual generic native draw bodies and the actual reporters execute.
static void Reset(float scale) {
    matrix={};matrix.xw=10;matrix.yw=20;matrix.zw=30;
    Matrix_Scale(scale,scale,scale,MTXMODE_APPLY);queries.clear();play.gameplayFrames=42;
}
static void Near(float a,float b) { assert(std::abs(a-b)<.0001f); }
int main() {
    const char* model="__OTR__selected/pokeball";
    Reset(.25f);
    Oot::DrawCustomItemDiamond(&play,const_cast<Gfx*>(model),.18f);
    assert(queries.empty());
    for(float scale:{.25f,.32f}) {
        Reset(scale);BlackMarketVisual::Capture offer;
        Oot::DrawCustomItemDiamond(&play,const_cast<Gfx*>(model),.18f);
        assert(queries.size()==1&&offer.frame.valid&&queries[0].roots[0]==model);
        const float spin=42*2*.01f,size=scale*.18f;
        Near((offer.frame.minimum.x+offer.frame.maximum.x)*.5f,10+size*(20*std::cos(spin)+30*std::sin(spin)));
        Near((offer.frame.minimum.z+offer.frame.maximum.z)*.5f,30+size*(-20*std::sin(spin)+30*std::cos(spin)));
        Near(offer.frame.maximum.y-offer.frame.minimum.y,100*size);
    }
    Reset(.32f);
    {
        BlackMarketVisual::Capture offer;Gfx* cache=nullptr;u8 tried=0;
        assert(Mm::DrawNeiRealOpa(model,&cache,&tried,.18f,false));
        assert(queries.size()==1&&offer.frame.valid&&queries[0].owner=="mm");
        Near((offer.frame.minimum.x+offer.frame.maximum.x)*.5f,10+20*.32f*.18f);
        Near((offer.frame.minimum.y+offer.frame.maximum.y)*.5f,20+30*.32f*.18f);
    }
    Reset(.25f);
    {
        alignas(2) char bytes[64]{};std::strcpy(bytes+1,model);
        BlackMarketVisual::Capture offer;
        NeiGi_ReportBlackMarketModel("oot",bytes+1,nullptr);
        assert(queries.size()==1&&offer.frame.valid);
        BlackMarketVisual::Capture disabled(false);
        NeiGi_ReportBlackMarketModel("oot",bytes+1,nullptr);
        assert(queries.size()==1&&!disabled.frame.valid);
    }
    std::cout<<"PASS generic native capture: final scale/spin, selected roots, byte-addressed paths, inactive zero queries\n";
}
