#include "EponaCosmeticMasks.h"
#include "test_require.h"
#include <cstdio>

int main() {
    using EponaMasks::BuildInverseMask;
    using EponaMasks::Part;
    REQUIRE(BuildInverseMask("unrelated/texture", Part::Coat).empty());
    REQUIRE(BuildInverseMask("gEponaSaddleTex", Part::Coat).empty());
    REQUIRE(BuildInverseMask("gEponaHorseshoeTex", Part::WhiteHair).empty());

    auto coat = BuildInverseMask("objects/object_horse_link_child/object_horse_link_child_Tex_00DCF0", Part::Coat);
    REQUIRE(coat.size() == 32 * 32);
    for (auto pixel : coat) {
        REQUIRE(pixel == 0);
    }
    REQUIRE(coat == BuildInverseMask("object_horse_link_childTex_008320", Part::Coat));

    auto hair = BuildInverseMask("object_horse_link_child_Tex_001F68", Part::WhiteHair);
    REQUIRE(hair.size() == 16 * 16);
    REQUIRE(hair[0] == 1);           // dark leg/hoof above the feathering is protected
    REQUIRE(hair[15 * 16 + 8] == 0); // white hoof feathering is selected
    REQUIRE(hair == BuildInverseMask("object_horse_link_childTex_001F68", Part::WhiteHair));

    auto eye = BuildInverseMask("objects/object_horse_link_child/gEponaEyeOpenTex", Part::Eyes);
    auto eyelid = BuildInverseMask("objects/object_horse/gEponaEyeOpenTex", Part::Coat);
    REQUIRE(eye.size() == 32 * 16 && eyelid.size() == eye.size());
    REQUIRE(eye[0] == 1 && eyelid[0] == 0); // iris tint excludes the orange surrounding coat
    REQUIRE(eye[6 * 32 + 12] == 1);         // white reflection
    REQUIRE(eye[10 * 32 + 5] == 1);         // near-black pupil/eye edge
    REQUIRE(eye[7 * 32 + 10] == 0);         // visible gray iris retains its shade under tint
    unsigned irisPixels = 0;
    for (size_t i = 0; i < eye.size(); ++i) {
        irisPixels += eye[i] == 0;
        REQUIRE(eye[i] || eyelid[i]); // coat and iris masks never overlap
    }
    REQUIRE(irisPixels > 0 && irisPixels < 100);
    REQUIRE(eye == BuildInverseMask("gChildEponaEyeOpenTex", Part::Eyes));
    REQUIRE(BuildInverseMask("gEponaEyeClosedTex", Part::Eyes).empty());
    REQUIRE(BuildInverseMask("gChildEponaEyeCloseTex", Part::Eyes).empty());
    REQUIRE(BuildInverseMask("gEponaForeheadTex", Part::WhiteHair).empty());
    REQUIRE(BuildInverseMask("object_horse_link_child_Tex_002588", Part::WhiteHair).empty());
    std::puts("PASS Epona native semantic masks: coat, iris/blink, mane/feathering, protected tack and aliases");
}
