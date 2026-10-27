#define main RewardRendererMain
#include "renderer_test.cpp"
#undef main
// Execute the two actual z_draw.c recipes and their real native table field
// types. Only legacy resource allocation/graphics boundaries are substituted.
static struct { Gfx* dlists[8]; } sDrawItemTable[10];
static u8 primXluColor[3], envXluColor[3], primOpaColor[3], envOpaColor[3];
extern "C" {
void gSPSegment(void* p, int segment, uintptr_t base) { __gSPSegment(static_cast<Gfx*>(p),segment,base); }
Gfx* Gfx_TexScrollEx(GraphicsContext*,u32,u32,s32,s32,s32,s32) { return &Fixture::setupDl; }
Gfx* Gfx_TwoTexScrollEx(GraphicsContext*,s32,u32,u32,s32,s32,s32,u32,u32,s32,s32,s32,s32,s32,s32) { return &Fixture::setupDl; }
}
// Native C recipes pass raw display-list pointers to the uintptr_t bridge.
static void gSPSegment(void* p, int segment, Gfx* base) {
    gSPSegment(p,segment,reinterpret_cast<uintptr_t>(base));
}
extern "C" {
#include "reward_native_recipes.inc"
}
int main() {
    using namespace Fixture;
    const char* const surface[] = {
        "__OTR__objects/object_gi_medal/gGiForestMedallionFaceDL", "__OTR__objects/object_gi_medal/gGiFireMedallionFaceDL",
        "__OTR__objects/object_gi_medal/gGiWaterMedallionFaceDL", "__OTR__objects/object_gi_medal/gGiSpiritMedallionFaceDL",
        "__OTR__objects/object_gi_medal/gGiShadowMedallionFaceDL", "__OTR__objects/object_gi_medal/gGiLightMedallionFaceDL",
        "__OTR__objects/object_gi_jewel/gGiKokiriEmeraldGemDL", "__OTR__objects/object_gi_jewel/gGiGoronRubyGemDL",
        "__OTR__objects/object_gi_jewel/gGiZoraSapphireGemDL"
    };
    const char* const setting[] = {
        "__OTR__objects/object_gi_medal/gGiMedallionDL", "__OTR__objects/object_gi_medal/gGiMedallionDL",
        "__OTR__objects/object_gi_medal/gGiMedallionDL", "__OTR__objects/object_gi_medal/gGiMedallionDL",
        "__OTR__objects/object_gi_medal/gGiMedallionDL", "__OTR__objects/object_gi_medal/gGiMedallionDL",
        "__OTR__objects/object_gi_jewel/gGiKokiriEmeraldSettingDL", "__OTR__objects/object_gi_jewel/gGiGoronRubySettingDL",
        "__OTR__objects/object_gi_jewel/gGiZoraSapphireSettingDL"
    };
    for (int i=0;i<9;++i) {
        Reset(); portableSongLists=true;
        files.insert(RewardGi_Texture(i+1)); files.insert("__OTR__objects/nei_reward_gi/metal");
        sDrawItemTable[i].dlists[0]=reinterpret_cast<Gfx*>(const_cast<char*>(surface[i]));
        sDrawItemTable[i].dlists[1]=reinterpret_cast<Gfx*>(const_cast<char*>(setting[i]));
        if (i<6) GetItem_DrawEggOrMedallion(&play,i); else GetItem_DrawJewel(&play,i);
        assert(arena.size()==3 && stack.empty() && matrix==1 && matrixY==0);
        const auto paths=Drawn();
        assert(paths.size()==2 && std::find(paths.begin(),paths.end(),surface[i])!=paths.end() &&
               std::find(paths.begin(),paths.end(),setting[i])!=paths.end());
    }
    Reset(); portableSongLists=true;
    sDrawItemTable[9].dlists[0]=reinterpret_cast<Gfx*>(const_cast<char*>("__OTR__objects/object_gi_egg/gGiEggMaterialDL"));
    sDrawItemTable[9].dlists[1]=reinterpret_cast<Gfx*>(const_cast<char*>("__OTR__objects/object_gi_egg/gGiEggDL"));
    GetItem_DrawEggOrMedallion(&play,9);
    assert(arena.empty() && stack.empty());
    std::cout << "PASS production native reward recipes: all six medallions, all three rotated stones, retained base draws, egg exclusion\n";
}
