#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include "IView.h"

// Transient popup pushed by an inbound Command (e.g. a heads-up status
// update) - like AlertView, isAlert() means it never gets a buildUi() tab;
// onCommand() renders via showPopup() (PopupOverlay's auto-dismissing
// "notification" kind, with a countdown bar) instead of requiring a
// deliberate tap-to-acknowledge.
//
// The Command's `value` is expected to be an object with "title" and
// "message" fields; a plain string is used directly as the message instead.
// Falls back to a generic "Notification" title / empty message. The
// auto-dismiss timeout defaults to 4 seconds, overridable via this view's
// "durationMs" deviceParameter.
//
// The Command's `value` object may also include a "soundEnabled" field
// (default false) controlling whether Buzzer::confirm() plays.
class NotificationView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override {}
    void onCommand(const Command& command) override;

    bool isAlert() const override { return true; }

private:
    unsigned long _durationMs = 4000;
};
