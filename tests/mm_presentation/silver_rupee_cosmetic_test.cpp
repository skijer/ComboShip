static constexpr const char* kValue = "gCosmetics./* REGISTERED_OPTION */.Value";
static constexpr const char* kChanged = "gCosmetics./* REGISTERED_OPTION */.Changed";
static constexpr const char* kRainbow = "gCosmetics./* REGISTERED_OPTION */.Rainbow";
static void Require(bool value, const char* why) {
    if (!value) { std::cerr << "FAIL " << why << '\n'; std::exit(1); }
}
// Resource lookup/scope are boundaries. Execute the production silver handler
// to verify the owner is selected when segment8 is recorded and the DL runs.
std::string activeOwner = "mm", deferredOwner = "mm", segmentOwner, segmentPath;
int restoredSegments = 0;
namespace Ship {
struct ResourceManager {};
namespace CrossRMRegistry {
std::shared_ptr<ResourceManager> Get(const char* owner) {
    Require(!std::strcmp(owner,"oot"), "silver texture requested the wrong owner RM");
    static auto rm = std::make_shared<ResourceManager>(); return rm;
}
}
struct ResourceManagerScope {
    std::string before;
    ResourceManagerScope(std::shared_ptr<ResourceManager>) : before(activeOwner) { activeOwner="oot"; }
    ~ResourceManagerScope() { activeOwner=before; }
};
}
#define gSPSegment(p,segment,path) do { (void)(p); Require(segment==8,"silver segment changed"); segmentOwner=activeOwner; segmentPath=reinterpret_cast<const char*>(path); } while(0)
#define gSPComboRMPush(p,owner) do { (void)(p); deferredOwner=owner; } while(0)
#define gSPComboRMPop(p) do { (void)(p); deferredOwner="mm"; } while(0)
void MM_RestoreForeignSegs(const int32_t* segments,int count) {
    Require(count==1 && segments[0]==8,"foreign silver segment restoration changed"); ++restoredSegments;
}
/* SILVER_HOST_DRAW */
/* COLOR_HOST_DRAW */

static void Begin(bool drops, bool changed, Color_RGBA8 color) {
    selected=RG_SHADOW_SILVER_BLADES;
    integers["gEnhancements.NewDrops"]=drops;
    integers[kChanged]=changed; colors[kValue]=color;
    colors["gCosmetics.Consumable_SilverRupee.Value"]={1,2,3,255};
    integers["gCosmetics.Consumable_SilverRupee.Changed"]=0;
    for(auto& command:opa) command.stream=0;
    for(auto& command:xlu) command.stream=1;
}
static std::vector<int> ColorOf(const ComboForeignDrawInfoOOT& info) {
    return Rgb(info.drawKind==CW_DRAW_KIND_COLOR_LAYERS ? info.layerPrimColor[0] : info.primColorOpa);
}
static void Palette() {
    for(bool drops:{false,true}) for(bool changed:{false,true}) {
        Begin(drops,changed,{29,147,231,255});
        ComboForeignDrawInfoOOT info;
        Require(ComboFillForeignDrawInfoOOT(1,info)==ComboForeignResolveOOT::Ok,"foreign silver recipe failed");
        if(drops || changed) Require(ColorOf(info)==std::vector<int>({29,147,231}),"registered picker color does not reach foreign recipe");
        Require(info.drawKind==(drops?CW_DRAW_KIND_COLOR_LAYERS:CW_DRAW_KIND_SILVER_RUPEE),"foreign NewDrops model mismatch");
        Require(drops || bool(info.primColorOpa[3])==changed,"registered Changed flag does not control foreign tint");
        if(drops) {
            Require(info.count==2 && info.xluStart==1,"foreign inner/outer split changed");
            Require(Rgb(info.layerEnvColor[0])==std::vector<int>({5,29,46}),"foreign inner shade changed");
            Require(Rgb(info.layerEnvColor[1])==std::vector<int>({21,110,173}),"foreign outer shade changed");
            ResetDraw(); primColors.clear(); MM_DrawForeignColorLayers(&info);
            Require(submitted.size()==2 && primColors.size()==2,"foreign colored models not submitted");
            Require(primColors[0].second==std::vector<int>({29,147,231,255}),"foreign inner submitted tint differs");
        }
    }
    std::cout<<"PASS foreign palette: registered hex/Changed, native shades and OPA/XLU submission\n";
}
static void Receipt() {
    Begin(true,true,{29,147,231,255});
    ComboLatchForeignDrawOOT(2);
    // Acquisition is complete: editing the palette and switching the model must
    // refresh the actual cached recipe, while the award identity stays frozen.
    integers["gEnhancements.NewDrops"]=0; colors[kValue]={213,72,45,255};
    const auto* live=ComboResolveForeignDrawInfoOOT(2);
    Require(live && live->drawKind==CW_DRAW_KIND_SILVER_RUPEE,"NewDrops froze after foreign receipt latch");
    Require(ColorOf(*live)==std::vector<int>({213,72,45}),"palette froze after foreign receipt latch");
    integers[kChanged]=0;
    live=ComboResolveForeignDrawInfoOOT(2);
    Require(live && live->primColorOpa[3]==0,"Changed toggle froze after grant");
    integers["gEnhancements.NewDrops"]=1;
    live=ComboResolveForeignDrawInfoOOT(2);
    Require(live && live->drawKind==CW_DRAW_KIND_COLOR_LAYERS,"NewDrops cannot switch back after grant");
    std::cout<<"PASS foreign receipt: palette/Changed/NewDrops stay live through the production latch and cache\n";
}
static void Rainbow() {
    for(bool drops:{false,true}) {
        Begin(drops,true,{29,147,231,255}); integers[kRainbow]=1;
        const int nativeHue = 7+int(drops)*4;
        hue=nativeHue; CosmeticsUpdateTick(); const auto expected=colors.at(kValue);
        hue=nativeHue; nativePatches=0; OOT_SetGiCosmeticFrame(300);
        CwItemDrawInfo initial{}; Require(DescribeSilverRupee(selected,&initial)==1,"rainbow descriptor missing");
        auto color = [&](const CwItemDrawInfo& info) {return Rgb(drops?info.layerPrimColor[0]:info.primColorOpa);};
        Require(color(initial)==std::vector<int>({expected.r,expected.g,expected.b}),"foreign silver does not sample native editor rainbow phase");
        const auto stored=colors.at(kValue); OOT_SetGiCosmeticFrame(301);
        CwItemDrawInfo next{}; DescribeSilverRupee(selected,&next);
        Require(color(next)!=color(initial),"dormant OoT silver rainbow stopped advancing");
        const auto now=colors.at(kValue);
        Require(now.r==stored.r && now.g==stored.g && now.b==stored.b && nativePatches==0,"foreign rainbow ran dormant editor patches or changed stored color");
        OOT_SetGiCosmeticFrame(301); CwItemDrawInfo same{}; DescribeSilverRupee(selected,&same);
        Require(color(same)==color(next),"same host frame advanced silver rainbow twice");
        play.gameplayFrames=301; ComboLatchForeignDrawOOT(4+int(drops));
        play.gameplayFrames=302;
        const auto* awarded=ComboResolveForeignDrawInfoOOT(4+int(drops));
        Require(awarded && ColorOf(*awarded)!=color(next),"dormant rainbow froze after the award latch");
        integers[kRainbow]=0;
    }
    std::cout<<"PASS dormant rainbow: production editor phase/speed/order, host-frame updates and unchanged owner storage\n";
}
static void Texture() {
    Begin(false,false,{255,255,255,255});
    ComboForeignDrawInfoOOT info;
    Require(ComboFillForeignDrawInfoOOT(3,info)==ComboForeignResolveOOT::Ok,"silver texture recipe failed");
    Require(info.scale==.05f && info.segTexPath && !std::strcmp(info.segTexPath,gRupeeSilverTex),"foreign silver owner texture/scale changed");
    ResetDraw(); restoredSegments=0;
    Require(MM_DrawForeignSilverRupee(&info),"foreign silver host draw failed");
    Require(segmentOwner=="oot" && segmentPath==gRupeeSilverTex,"segment8 was not bound in the OoT resource scope");
    Require(submitted.size()==1 && submitted[0].second==std::string("__OTR__@oot:")+(gRupeeDL+7),"silver DL lost OoT routing");
    Require(activeOwner=="mm" && deferredOwner=="mm" && restoredSegments==1,"foreign silver did not restore owner/segment state");
    std::cout<<"PASS owner texture: production segment8 handler, OoT routed model, scale and restoration\n";
}
int main(int argc,char** argv) {
    Require(argc==2,"case argument missing");
    if(!std::strcmp(argv[1],"foreign_palette")) Palette();
    else if(!std::strcmp(argv[1],"foreign_receipt")) Receipt();
    else if(!std::strcmp(argv[1],"foreign_rainbow")) Rainbow();
    else if(!std::strcmp(argv[1],"foreign_texture")) Texture();
    else Require(false,"unknown case");
}
