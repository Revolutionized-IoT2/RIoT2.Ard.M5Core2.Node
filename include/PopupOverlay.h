#pragma once

#include <Arduino.h>
#include <lvgl.h>

// One reusable overlay drawn on lv_layer_top() (always above whatever the
// active tab is showing), used for AlertView/NotificationView inbound-
// command popups and, once Phase 8's idle timeout lands, the ClockView
// takeover. Two dismiss behaviors:
//   - showAlert(): stays until the user taps it (mirrors an incoming
//     command the user must acknowledge).
//   - showNotification(): auto-dismisses after autoDismissMs, with an
//     lv_bar counting down so the remaining time is visible.
// Only one popup can be shown at a time - a new show*() call replaces
// whatever is currently displayed.
class PopupOverlay {
public:
    void begin();

    void showAlert(const String& title, const String& message);
    void showNotification(const String& title, const String& message, uint32_t autoDismissMs);

    // Hides the popup if one is showing. Returns true if it actually was
    // showing (see NavigationController::setPopupDismissHandler).
    bool dismiss();

    bool isVisible() const;

private:
    lv_obj_t* _container = nullptr;
    lv_obj_t* _titleLabel = nullptr;
    lv_obj_t* _messageLabel = nullptr;
    lv_obj_t* _bar = nullptr;
    lv_anim_t _dismissAnim;

    void showCommon(const String& title, const String& message);

    static void tapToDismissCb(lv_event_t* event);
    static void barAnimExecCb(void* bar, int32_t value);
    static void barAnimCompletedCb(lv_anim_t* anim);
};
