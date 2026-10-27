#ifndef BENMENU_H
#define BENMENU_H

#include "UIWidgets.hpp"
#include "Menu.h"
#include "2s2h/Enhancements/Enhancements.h"
#include "2s2h/DeveloperTools/DeveloperTools.h"
#include <fast/backends/gfx_rendering_api.h>
#include "ComboMenuExport.h"

namespace BenGui {

// The Sheikah Sensor rune's five wish slots. Drawn from both the NEI and the Randomizer menus, so
// it lives on its own instead of being written twice. Skijer's NEI
void DrawSensorDesirePicker();

class BenMenu : public Ship::Menu {
  public:
    BenMenu(const std::string& consoleVariable, const std::string& name);

    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;
    void Draw() override;

    void AddSidebarEntry(std::string sectionName, std::string sidbarName, uint32_t columnCount);
    WidgetInfo& AddWidget(WidgetPath& pathInfo, std::string widgetName, WidgetType widgetType);
    void AddMenuElements();
    void AddSettings();
    void AddEnhancements();
    void AddDevTools();
    void AddNetwork();
    void AddNEI();

    // ComboShip: C-ABI menu export (see combo/menu/ComboMenuExport.h). Builds (once, cached) the flat
    // CwMenu describing the whole BenMenu tree and returns a pointer stable for this instance's
    // lifetime. comboui ingests the CwMenu and invokes back by index. MM analog of SohMenu's exports.
    const CwMenu* ExportComboMenu();
    void InvokeCallbackByIndex(int32_t i);                          // runs widget i's .callback(*w)
    int32_t EvalDisabledByIndex(int32_t i, const char** outReason); // runs widget i's preFunc; 1 if disabled
    void DrawCustomByIndex(int32_t i);                              // runs widget i's customFunction(*w)
    int32_t DrawWidgetByIndex(int32_t i, int32_t width); // draws widget i via real MenuDrawItem; 1 if changed

  private:
    void AddFleetComboSection(WidgetPath& path);

    bool mMenuElementsInitialized = false;

    // ComboShip menu-export backing storage (combo-owned serializer; see ComboMenuExport.h).
    // Lives as long as this BenMenu instance so C-ABI pointers stay valid for process life.
    ComboMenuExport::State<WidgetInfo> mComboExport;
};
} // namespace BenGui

#endif // BENMENU_H
