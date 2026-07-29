#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <functional>
#include <vector>

// Owns the app's single lv_tabview (bottom tab bar), one tab per active
// DeviceConfiguration view - populated by ViewManager once views exist (see
// Phase 5+); empty until then. Also routes the Core2's three bottom
// touch-zone virtual buttons (BtnA/B/C), the secondary input path alongside
// lv_tabview's own native swipe gesture. Also owns the main menu overlay - a
// grid of every view's tab, reachable by pressing the middle virtual button
// (BtnB) - see MenuEntry/setMenuEntries().
class NavigationController {
public:
    enum class Button { A, B, C };

    // One tile in the main menu grid - name is the tile's label,
    // classFullName is used to look up its icon (see iconForClassFullName()
    // in the .cpp), tabIndex is what goToTab() jumps to when the tile is
    // selected.
    struct MenuEntry {
        String name;
        String classFullName;
        uint32_t tabIndex;
    };

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

    // Rebuilds the main menu grid's tiles from `entries` - called by
    // ViewManager::rebuild() once every tab has been added, in the same
    // order/count as addTab() calls so each entry's tabIndex lines up.
    void setMenuEntries(std::vector<MenuEntry> entries);

    // Consulted by onButtonPress(Button::B) before it falls through to
    // opening/confirming the main menu - return true if a popup was
    // actually dismissed, so that action isn't also applied on the same
    // press. Injected as a predicate (rather than NavigationController
    // owning a PopupOverlay reference directly) since the two are sibling
    // components, not an owned dependency - same DI pattern as
    // PeripheralManager::rebuild()'s isKnownElsewhere.
    void setPopupDismissHandler(std::function<bool()> handler) { _popupDismissHandler = handler; }

    void onButtonPress(Button button);

private:
    // Pairs a tile's lv_obj_t* with the NavigationController instance and
    // tab index it should jump to - stable heap addresses for event
    // user_data (see SceneSelectorView's identical rationale for _slots).
    struct MenuTile {
        NavigationController* self = nullptr;
        uint32_t tabIndex = 0;
        lv_obj_t* tile = nullptr;
    };

    lv_obj_t* _tabview = nullptr;
    std::function<bool()> _popupDismissHandler;

    lv_obj_t* _menuOverlay = nullptr;  // full-screen background on lv_layer_top(), tap = close
    lv_obj_t* _menuGrid = nullptr;     // flex-wrap grid of tiles inside _menuOverlay
    std::vector<MenuEntry> _menuEntries;
    std::vector<MenuTile> _menuTiles;
    int _menuSelectedIndex = -1;

    void buildMenuOverlay();
    void showMenu();
    void hideMenu();
    bool isMenuVisible() const;
    void moveMenuSelection(int delta);
    void confirmMenuSelection();
    void updateMenuHighlight();

    static void menuBackgroundTappedCb(lv_event_t* event);
    static void menuTileTappedCb(lv_event_t* event);
};
