#include "NavigationController.h"

void NavigationController::begin() {
    _tabview = lv_tabview_create(lv_screen_active());
    lv_tabview_set_tab_bar_position(_tabview, LV_DIR_BOTTOM);
    lv_tabview_set_tab_bar_size(_tabview, 40);
}

lv_obj_t* NavigationController::addTab(const String& title) {
    return lv_tabview_add_tab(_tabview, title.c_str());
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
