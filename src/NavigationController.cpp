#include "NavigationController.h"

#include "lv_font_montserrat_18_bpp8.h"

void NavigationController::begin() {
    _tabview = lv_tabview_create(lv_screen_active());
    lv_tabview_set_tab_bar_position(_tabview, LV_DIR_BOTTOM);
    lv_tabview_set_tab_bar_size(_tabview, 40);
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
        lv_obj_set_style_bg_opa(tabButton, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_bg_color(tabButton, lv_palette_main(LV_PALETTE_BLUE), LV_PART_MAIN | LV_STATE_CHECKED);
        lv_obj_set_style_text_color(tabButton, lv_color_white(), LV_PART_MAIN | LV_STATE_CHECKED);

        // The theme's default LV_FONT_MONTSERRAT_14 had a broken/missing
        // pixel in certain glyphs (e.g. the "u" in "Status"); this custom
        // bpp8 Montserrat font (already used by QrSettingsView) doesn't
        // have that defect, so it's used here for the tab bar labels too.
        lv_obj_set_style_text_font(tabButton, &lv_font_montserrat_18_bpp8, LV_PART_MAIN);
    }

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
    lv_tabview_set_active(_tabview, target, LV_ANIM_ON);
}

void NavigationController::nextTab() {
    uint32_t count = lv_tabview_get_tab_count(_tabview);
    if (count == 0) {
        return;
    }
    uint32_t current = lv_tabview_get_tab_active(_tabview);
    uint32_t target = (current + 1) % count;
    lv_tabview_set_active(_tabview, target, LV_ANIM_ON);
}

void NavigationController::goToTab(uint32_t index) {
    if (index >= lv_tabview_get_tab_count(_tabview)) {
        return;
    }
    lv_tabview_set_active(_tabview, index, LV_ANIM_ON);
}

void NavigationController::onButtonPress(Button button) {
    switch (button) {
        case Button::A:
            previousTab();
            break;
        case Button::B:
            if (_popupDismissHandler && _popupDismissHandler()) {
                return;
            }
            goToTab(0);
            break;
        case Button::C:
            nextTab();
            break;
    }
}
