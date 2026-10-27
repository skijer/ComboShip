#include "combo/BlackMarketFrame.h"
bool ReportOtherTranslationUnit() {
    const bool active = BlackMarketVisual::Active();
    BlackMarketVisual::Report({ 2, 3, 4 }, { 5, 6, 7 }, [](BlackMarketVisual::Point p) { return p; });
    return active;
}
