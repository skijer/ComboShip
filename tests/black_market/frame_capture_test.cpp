#include "combo/BlackMarketFrame.h"
#include <cassert>
#include <cmath>
#include <limits>

using namespace BlackMarketVisual;
bool ReportOtherTranslationUnit();
static void Near(float a, float b) { assert(std::abs(a - b) < .0001f); }

int main() {
    assert(!Active() && !ReportOtherTranslationUnit());
    int transforms = 0;
    auto pose = [&](Point p) {
        ++transforms;
        return Point{ 10.f + .32f * p.z, 20.f + .32f * (p.y + 14.f), 30.f - .32f * p.x };
    };
    Report({ -3, -10, -2 }, { 9, 30, 6 }, pose);
    assert(transforms == 0);
    {
        Capture offer;
        assert(Active() && !offer.frame.valid);
        Report({ -3, -10, -2 }, { 9, 30, 6 }, pose);
        assert(transforms == 8 && offer.frame.valid);
        Near(offer.frame.minimum.x, 9.36f); Near(offer.frame.maximum.x, 11.92f);
        Near(offer.frame.minimum.y, 21.28f); Near(offer.frame.maximum.y, 34.08f);
        Near(offer.frame.minimum.z, 27.12f); Near(offer.frame.maximum.z, 30.96f);
        {
            Capture next;
            Report({ 100, 200, 300 }, { 110, 210, 310 }, [](Point p) { return p; });
            assert(next.frame.valid && next.frame.minimum.x == 100);
            assert(offer.frame.minimum.x < 10);
        }
        {
            Capture unlocked(false);
            assert(!Active());
            Report({ -999, 0, 0 }, { 999, 1, 1 }, [](Point p) { return p; });
            assert(!unlocked.frame.valid && offer.frame.minimum.x > 9);
        }
        assert(Active());
        Report({ -10, 0, 0 }, { 0, 1, 1 }, [](Point p) { return p; });
        assert(offer.frame.minimum.x == -10 && offer.frame.maximum.y == 34.08f);
    }
    assert(!Active());
    {
        Capture nextOffer;
        assert(!nextOffer.frame.valid);
        Report({ 0, 0, 0 }, { 1, 1, 1 }, [](Point p) {
            p.x = std::numeric_limits<float>::infinity(); return p;
        });
        assert(!nextOffer.frame.valid);
        assert(ReportOtherTranslationUnit() && nextOffer.frame.valid && nextOffer.frame.minimum.x == 2 &&
               nextOffer.frame.maximum.z == 7);
    }
}
