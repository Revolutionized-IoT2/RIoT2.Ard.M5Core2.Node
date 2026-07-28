#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <functional>

// Owns the app's single lv_tabview (bottom tab bar), one tab per active
// DeviceConfiguration view - populated by ViewManager once views exist (see
// Phase 5+); empty until then. Also routes the Core2's three bottom
// touch-zone virtual buttons (BtnA/B/C), the secondary input path alongside
// lv_tabview's own native swipe gesture.
class NavigationController {
public:
    enum class Button { A, B, C };

    void begin();

    // Adds a new tab titled `title` and returns its content area for the
    // caller to build widgets into.
    lv_obj_t* addTab(const String& title);

    // Removes every tab (recreates the underlying lv_tabview from scratch -
    // LVGL has no bulk "remove all tabs" call), e.g. before a
    // NodeConfiguration-driven rebuild repopulates it.
    void clearTabs();

    void previousTab();
    void nextTab();
    void goToTab(uint32_t index);

    // Consulted by onButtonPress(Button::B) before it falls through to
    // "jump to tab 0" - return true if a popup was actually dismissed, so
    // that action isn't also applied on the same press. Injected as a
    // predicate (rather than NavigationController owning a PopupOverlay
    // reference directly) since the two are sibling components, not an
    // owned dependency - same DI pattern as PeripheralManager::rebuild()'s
    // isKnownElsewhere.
    void setPopupDismissHandler(std::function<bool()> handler) { _popupDismissHandler = handler; }

    void onButtonPress(Button button);

private:
    lv_obj_t* _tabview = nullptr;
    std::function<bool()> _popupDismissHandler;
};
