#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#if __has_include("soh/Enhancements/cosmetics/NativeNpcPoseResources.h")
#include "soh/Enhancements/cosmetics/NativeNpcPoseResources.h"
struct Index {
    uint16_t x, y, z;
};
struct Clip {
    struct {
        struct {
            struct {
                int16_t frameCount;
            } common;
            uint16_t staticIndexMax;
        } animationHeader;
    } animationData;
    std::vector<uint16_t> rotationValues;
    std::vector<Index> rotationIndices;
};
int main() {
    using namespace SOH::NativeNpcPose;
    const std::string animation = "__OTR__objects/object_os_anime/gKokiriSittingCrossedArmsLegsAnim";
    const std::string expected =
        "objects/object_kw1/NativeNpcV1/PoseAnimations/objects/object_os_anime/gKokiriSittingCrossedArmsLegsAnim";
    assert(AnimationPath("objects/object_kw1/NativeNpcV1/Skel", animation) == expected);
    assert(AnimationPath("__OTR__objects/object_kw1/NativeNpcV1/Skel", animation) == expected);
    assert(AnimationPath("objects/object_kw1/gKw1Skel", animation).empty());
    assert(
        AnimationPath("objects/object_fa/YoungFadoNpcPOC1/Skel", animation).find("YoungFadoNpcPOC1/PoseAnimations/") !=
        std::string::npos);
    assert(AnimationPath("objects/object_fa/YoungFadoNpcPOC1/Skel",
                         "objects/object_fa/YoungFadoNpcPOC1/Animations/Seated5") ==
           AnimationPath("objects/object_fa/YoungFadoNpcPOC1/Skel", animation));
    assert(AnimationPath("objects/object_link_child/NativeNpcV1/Skel", animation).empty());
    assert(AnimationPath("objects/object_kw1/NativeNpcV1/Skel", "objects/../gClip").empty());
    Clip native = { { { { 2 }, 3 } }, { 100, 200, 300, 1000, 2000 }, { { 0, 1, 2 }, { 3, 1, 2 } } };
    Clip fitted = native;
    fitted.rotationValues[3] = 5000;
    assert(Validate(fitted, native, 2));
    fitted.animationData.animationHeader.common.frameCount = 3;
    assert(!Validate(fitted, native, 2));
    fitted = native;
    fitted.rotationIndices.pop_back();
    assert(!Validate(fitted, native, 2));
    fitted = native;
    fitted.rotationIndices[1].x = 4;
    assert(!Validate(fitted, native, 2));
    fitted = native;
    fitted.rotationValues[0]++;
    assert(!Validate(fitted, native, 2));
    fitted = native;
    fitted.animationData.animationHeader.staticIndexMax = 40;
    assert(!Validate(fitted, native, 2));
    assert(!Validate(native, native, 0));
    std::cout << "PASS private pose paths, Fado compatibility, actor isolation, frame/joint/channel bounds and "
                 "root-motion preservation.\n";
}
#else
int main() {
    assert(!"Archive-editable NPC pose contract is missing");
}
#endif
