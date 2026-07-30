#include "NavigationController.h"

#include "Icons.h"
#include "lv_font_montserrat_18_bpp8.h"

namespace {
constexpr lv_coord_t kMenuTileWidth = 88;
constexpr lv_coord_t kMenuTileHeight = 96;

// The bottom tab bar's own background - deliberately darker than every
// view's (theme-default light) background so the bar reads as a distinct
// navigation chrome element instead of blending into whichever view is
// currently showing above it.
const lv_color_t kTabBarBgColor = lv_color_hex(0x16181D);

// Text color for non-active tab buttons - the tab bar's dark background
// (see kTabBarBgColor) makes the theme's default dark-on-light button text
// unreadable, so unchecked tabs get an explicit muted light gray instead.
const lv_color_t kTabInactiveTextColor = lv_color_hex(0xA0A4AC);

// Active-tab highlight color - deliberately a different hue than the blue
// used elsewhere for interactive accents (e.g. ButtonView's kActiveColor,
// the main menu's checked-tile highlight) so the bottom nav's "you are
// here" indicator doesn't read as just another button.
const lv_color_t kTabActiveColor = lv_color_hex(0x00BFA5);

// Maps a DeviceConfiguration's classFullName (e.g.
// "RIoT2.Ard.M5Core2.Node.ButtonView") to the icon asset (see Icons.h,
// converted from RIoT2.Ard.M5Dial.Node/Assets/icons) that best represents
// it. Returns nullptr for view types with no matching icon (e.g.
// EnergyGaugeView) - callers fall back to a plain text tile in that case.
const lv_image_dsc_t* iconForClassFullName(const String& classFullName) {
    // Longer/more-specific suffixes are checked first so e.g. "ButtonView"
    // doesn't accidentally match something like "ToggleButtonView" (not a
    // real class here, but keeps this robust either way).
    if (classFullName.endsWith("ColorSchemeView")) return &icon_colorscheme;
    if (classFullName.endsWith("SceneSelectorView")) return &icon_sceneselector;
    if (classFullName.endsWith("ButtonView")) return &icon_button;
    if (classFullName.endsWith("ClockView")) return &icon_clock;
    if (classFullName.endsWith("PercentageView")) return &icon_percentage;
    if (classFullName.endsWith("SliderView")) return &icon_slider;
    if (classFullName.endsWith("TimerView")) return &icon_timer;
    if (classFullName.endsWith("ToggleView")) return &icon_toggle;
    if (classFullName.endsWith("ValueView")) return &icon_value;
    if (classFullName.endsWith("BLEView")) return &icon_ble;
    if (classFullName.endsWith("AlertView")) return &icon_alert;
    if (classFullName.endsWith("NotificationView")) return &icon_notification;
    return nullptr;
}
}  // namespace

void NavigationController::begin() {
    _tabview = lv_tabview_create(lv_screen_active());
    lv_tabview_set_tab_bar_position(_tabview, LV_DIR_BOTTOM);
    lv_tabview_set_tab_bar_size(_tabview, 40);

    // Give the tab bar itself a solid, distinctly darker background than
    // any view's (theme-default light) content background - otherwise the
    // bar's default theme bg is close enough to the active view's own bg
    // that the two visually merge into one another (see kTabBarBgColor).
    lv_obj_t* tabBar = lv_tabview_get_tab_bar(_tabview);
    lv_obj_set_style_bg_color(tabBar, kTabBarBgColor, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(tabBar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(tabBar, 0, LV_PART_MAIN);

    // lv_tabview_set_active() (used by previousTab()/nextTab()/goToTab())
    // only scrolls the *content* area and updates each button's checked
    // state - it never scrolls the tab bar, so the active tab's highlight
    // can end up off-screen after navigating. LV_EVENT_VALUE_CHANGED also
    // fires on this object for swipe-driven tab changes (see
    // cont_scroll_end_event_cb in lv_tabview.c), so hooking it here covers
    // swipe navigation too - the button-driven paths call
    // centerActiveTabButton() directly (see previousTab()/nextTab()/
    // goToTab() below).
    lv_obj_add_event_cb(_tabview, tabviewValueChangedCb, LV_EVENT_VALUE_CHANGED, this);

    // The menu overlay lives on lv_layer_top(), independent of _tabview, so
    // it must only ever be built once - begin() itself is re-run every time
    // clearTabs() recreates _tabview (see clearTabs() below).
    if (!_menuOverlay) {
        buildMenuOverlay();
    }
}

lv_obj_t* NavigationController::addTab(const String& title) {
    lv_obj_t* content = lv_tabview_add_tab(_tabview, title.c_str());

    // LVGL's default theme draws the *active* tab button as a light tint of
    // the primary color with label text in that same primary color - i.e.
    // low-contrast color-on-its-own-tint (confirmed in lvgl's
    // lv_theme_default.c: LV_STATE_CHECKED gets styles.bg_color_primary_muted,
    // whose bg is the primary color at 20% opacity and whose text color is
    // the *same* primary color at full opacity). That's barely readable
    // regardless of which hue the theme picks, so override the checked tab
    // button here with a solid background and high-contrast text for
    // guaranteed contrast instead of relying on the theme's built-in checked
    // style.
    uint32_t index = lv_tabview_get_tab_count(_tabview) - 1;
    lv_obj_t* tabBar = lv_tabview_get_tab_bar(_tabview);
    lv_obj_t* tabButton = lv_obj_get_child(tabBar, index);
    if (tabButton) {
        // Non-active tabs: transparent (so the bar's own dark
        // kTabBarBgColor shows through) with muted light text - the
        // theme's default dark-on-light button text is unreadable against
        // that dark bar otherwise.
        lv_obj_set_style_bg_opa(tabButton, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_set_style_text_color(tabButton, kTabInactiveTextColor, LV_PART_MAIN);

        // Active tab: solid, distinct accent color (kTabActiveColor) rather
        // than the same blue used elsewhere for buttons/menu highlights, so
        // the "current view" indicator doesn't read as just another
        // button.
        lv_obj_set_style_bg_opa(tabButton, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(tabButton, kTabActiveColor, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(tabButton, lv_color_black(), LV_PART_MAIN | LV_STATE_CHECKED);

        // The theme's default LV_FONT_MONTSERRAT_14 had a broken/missing
        // pixel in certain glyphs (e.g. the "u" in "Status"); this custom
        // bpp8 Montserrat font (already used by QrSettingsView) doesn't
        // have that defect, so it's used here for the tab bar labels too.
        lv_obj_set_style_text_font(tabButton, &lv_font_montserrat_18_bpp8, LV_PART_MAIN);

        // lv_tabview_add_tab() gives every tab button flex_grow=1, which
        // evenly divides the tab bar's fixed width across however many tabs
        // exist - fine for 3-4 tabs, but a NodeConfiguration with 10+ views
        // (all sharing one bottom tab bar) squishes every label down to an
        // unreadable sliver ("menu is a mess"). Give each button its
        // natural (label + padding) width instead and let the tab bar
        // scroll horizontally once tabs overflow it - it's already a
        // scrollable flex-row container (lv_tabview_set_tab_bar_position()),
        // this just stops forcing every button to shrink to fit.
        lv_obj_set_flex_grow(tabButton, 0);
        lv_obj_set_width(tabButton, LV_SIZE_CONTENT);
        lv_obj_set_style_pad_left(tabButton, 14, LV_PART_MAIN);
        lv_obj_set_style_pad_right(tabButton, 14, LV_PART_MAIN);
    }

    // If this is the first tab added, it's implicitly the active one -
    // make sure it starts centered rather than pinned at the bar's left
    // edge (matters once there are enough tabs for the bar to scroll).
    centerActiveTabButton();

    return content;
}

void NavigationController::clearTabs() {
    if (_tabview) {
        lv_obj_delete(_tabview);
    }
    begin();
}

void NavigationController::previousTab() {
    uint32_t count = lv_tabview_get_tab_count(_tabview);
    if (count == 0) {
        return;
    }
    uint32_t current = lv_tabview_get_tab_active(_tabview);
    uint32_t target = (current == 0) ? (count - 1) : (current - 1);
    Serial.printf("[Nav] previousTab: %u -> %u (count=%u)\n", current, target, count);
    lv_tabview_set_active(_tabview, target, LV_ANIM_OFF);
    centerActiveTabButton();
}

void NavigationController::nextTab() {
    uint32_t count = lv_tabview_get_tab_count(_tabview);
    if (count == 0) {
        return;
    }
    uint32_t current = lv_tabview_get_tab_active(_tabview);
    uint32_t target = (current + 1) % count;
    Serial.printf("[Nav] nextTab: %u -> %u (count=%u)\n", current, target, count);
    lv_tabview_set_active(_tabview, target, LV_ANIM_OFF);
    centerActiveTabButton();
}

void NavigationController::goToTab(uint32_t index) {
    if (index >= lv_tabview_get_tab_count(_tabview)) {
        return;
    }
    Serial.printf("[Nav] goToTab: -> %u\n", index);
    lv_tabview_set_active(_tabview, index, LV_ANIM_OFF);
    centerActiveTabButton();
}

void NavigationController::onButtonPress(Button button) {
    Serial.printf("[Nav] onButtonPress: %d\n", static_cast<int>(button));
    switch (button) {
        case Button::A:
            if (isMenuVisible()) {
                moveMenuSelection(-1);
            } else {
                previousTab();
            }
            break;
        case Button::B:
            if (_popupDismissHandler && _popupDismissHandler()) {
                return;
            }
            if (isMenuVisible()) {
                confirmMenuSelection();
            } else {
                showMenu();
            }
            break;
        case Button::C:
            if (isMenuVisible()) {
                moveMenuSelection(1);
            } else {
                nextTab();
            }
            break;
    }
}

void NavigationController::buildMenuOverlay() {
    // Full-screen translucent background on lv_layer_top() - same layer
    // PopupOverlay uses, so an inbound alert popup still wins visually if
    // it happens to arrive while the menu is open (isMenuVisible() is never
    // true at the same time in practice - see onButtonPress()'s popup
    // dismiss check above, which runs before the menu is ever touched).
    _menuOverlay = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(_menuOverlay);
    lv_obj_set_size(_menuOverlay, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(_menuOverlay, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(_menuOverlay, LV_OPA_COVER, 0);
    lv_obj_add_flag(_menuOverlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(_menuOverlay, menuBackgroundTappedCb, LV_EVENT_CLICKED, this);

    lv_obj_t* title = lv_label_create(_menuOverlay);
    lv_label_set_text(title, "Menu");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18_bpp8, 0);
    lv_obj_set_style_text_color(title, lv_color_black(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 6);

    _menuGrid = lv_obj_create(_menuOverlay);
    lv_obj_remove_style_all(_menuGrid);
    lv_obj_set_size(_menuGrid, LV_PCT(100), LV_PCT(85));
    lv_obj_align(_menuGrid, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_flex_flow(_menuGrid, LV_FLEX_FLOW_ROW_WRAP);
    // Track (3rd) alignment must be START, not CENTER: with ROW_WRAP the
    // track axis is vertical - the same axis this container scrolls on
    // once tiles overflow it. CENTER there makes LVGL's flex layout vertically
    // center the rows around the rest scroll position, so scrolling toward
    // the top fights the layout's own re-centering and snaps back to the
    // middle instead of reaching the first row.
    lv_obj_set_flex_align(_menuGrid, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_row(_menuGrid, 6, 0);
    lv_obj_set_style_pad_column(_menuGrid, 6, 0);
    // Tiles are direct children handling their own LV_EVENT_CLICKED (see
    // menuTileTappedCb) - since that event doesn't bubble by default, taps
    // on a tile never also fire _menuOverlay's own background-tap handler.
    lv_obj_add_flag(_menuGrid, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_add_flag(_menuOverlay, LV_OBJ_FLAG_HIDDEN);
}

void NavigationController::setMenuEntries(std::vector<MenuEntry> entries) {
    _menuEntries = std::move(entries);
    _menuTiles.clear();
    lv_obj_clean(_menuGrid);

    // _menuTiles is fully rebuilt (not resized incrementally) below and
    // never touched again until the next setMenuEntries() call, so its
    // element addresses stay stable for event user_data - same pattern as
    // SceneSelectorView's _slots.
    _menuTiles.resize(_menuEntries.size());
    for (size_t i = 0; i < _menuEntries.size(); i++) {
        const MenuEntry& entry = _menuEntries[i];

        lv_obj_t* tile = lv_button_create(_menuGrid);
        lv_obj_set_size(tile, kMenuTileWidth, kMenuTileHeight);
        lv_obj_set_flex_flow(tile, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(tile, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_bg_color(tile, lv_color_hex(0xF2F2F2), 0);
        lv_obj_set_style_border_color(tile, lv_color_hex(0xC0C0C0), 0);
        lv_obj_set_style_border_width(tile, 1, 0);
        // Highlight the virtual-button-selected tile with a solid blue
        // border/background instead of the theme's low-contrast default
        // checked style - same rationale as addTab()'s tab-button override
        // above.
        lv_obj_set_style_border_color(tile, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_CHECKED);
        lv_obj_set_style_border_width(tile, 3, LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(tile, lv_palette_lighten(LV_PALETTE_BLUE, 4), LV_STATE_CHECKED);

        const lv_image_dsc_t* icon = iconForClassFullName(entry.classFullName);
        if (icon) {
            lv_obj_t* image = lv_image_create(tile);
            lv_image_set_src(image, icon);
        }

        lv_obj_t* label = lv_label_create(tile);
        lv_label_set_text(label, entry.name.c_str());
        lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_width(label, kMenuTileWidth - 8);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_color(label, lv_color_black(), 0);

        _menuTiles[i].self = this;
        _menuTiles[i].tabIndex = entry.tabIndex;
        _menuTiles[i].tile = tile;
        lv_obj_add_event_cb(tile, menuTileTappedCb, LV_EVENT_CLICKED, &_menuTiles[i]);
    }

    _menuSelectedIndex = -1;
}

void NavigationController::showMenu() {
    if (_menuEntries.empty()) {
        return;
    }
    Serial.println("[Nav] showMenu");

    uint32_t activeTab = lv_tabview_get_tab_count(_tabview) ? lv_tabview_get_tab_active(_tabview) : 0;
    _menuSelectedIndex = 0;
    for (size_t i = 0; i < _menuEntries.size(); i++) {
        if (_menuEntries[i].tabIndex == activeTab) {
            _menuSelectedIndex = static_cast<int>(i);
            break;
        }
    }
    updateMenuHighlight();

    lv_obj_remove_flag(_menuOverlay, LV_OBJ_FLAG_HIDDEN);
}

void NavigationController::hideMenu() {
    lv_obj_add_flag(_menuOverlay, LV_OBJ_FLAG_HIDDEN);
}

bool NavigationController::isMenuVisible() const {
    return _menuOverlay && !lv_obj_has_flag(_menuOverlay, LV_OBJ_FLAG_HIDDEN);
}

void NavigationController::moveMenuSelection(int delta) {
    if (_menuEntries.empty()) {
        return;
    }
    int count = static_cast<int>(_menuEntries.size());
    _menuSelectedIndex = ((_menuSelectedIndex + delta) % count + count) % count;
    updateMenuHighlight();
}

void NavigationController::confirmMenuSelection() {
    if (_menuSelectedIndex < 0 || _menuSelectedIndex >= static_cast<int>(_menuEntries.size())) {
        hideMenu();
        return;
    }
    goToTab(_menuEntries[_menuSelectedIndex].tabIndex);
    hideMenu();
}

void NavigationController::updateMenuHighlight() {
    for (size_t i = 0; i < _menuTiles.size(); i++) {
        if (static_cast<int>(i) == _menuSelectedIndex) {
            lv_obj_add_state(_menuTiles[i].tile, LV_STATE_CHECKED);
        } else {
            lv_obj_remove_state(_menuTiles[i].tile, LV_STATE_CHECKED);
        }
    }

    // Virtual-button navigation (moveMenuSelection()) has no pointer/touch
    // event of its own to trigger LVGL's usual scroll-on-focus behavior, so
    // the grid never follows the selection unless done explicitly here.
    if (_menuSelectedIndex >= 0 && _menuSelectedIndex < static_cast<int>(_menuTiles.size())) {
        lv_obj_scroll_to_view(_menuTiles[_menuSelectedIndex].tile, LV_ANIM_ON);
    }
}

void NavigationController::centerActiveTabButton() {
    if (!_tabview) {
        return;
    }
    uint32_t count = lv_tabview_get_tab_count(_tabview);
    if (count == 0) {
        return;
    }

    lv_obj_t* tabBar = lv_tabview_get_tab_bar(_tabview);
    uint32_t active = lv_tabview_get_tab_active(_tabview);
    lv_obj_t* button = lv_obj_get_child(tabBar, active);
    if (!button) {
        return;
    }

    // lv_obj_get_x() returns a button's position within the tab bar's
    // *unscrolled* content (it already folds the current scroll offset
    // back out - see lv_obj_get_x() in lv_obj_pos.c), so this is stable
    // regardless of the bar's current scroll position.
    lv_obj_update_layout(tabBar);
    int32_t buttonCenter = lv_obj_get_x(button) + lv_obj_get_width(button) / 2;
    int32_t barContentWidth = lv_obj_get_content_width(tabBar);
    lv_obj_scroll_to_x(tabBar, buttonCenter - barContentWidth / 2, LV_ANIM_ON);
}

void NavigationController::menuBackgroundTappedCb(lv_event_t* event) {
    auto* self = static_cast<NavigationController*>(lv_event_get_user_data(event));
    self->hideMenu();
}

void NavigationController::menuTileTappedCb(lv_event_t* event) {
    auto* menuTile = static_cast<MenuTile*>(lv_event_get_user_data(event));
    menuTile->self->goToTab(menuTile->tabIndex);
    menuTile->self->hideMenu();
}

void NavigationController::tabviewValueChangedCb(lv_event_t* event) {
    auto* self = static_cast<NavigationController*>(lv_event_get_user_data(event));
    self->centerActiveTabButton();
}

