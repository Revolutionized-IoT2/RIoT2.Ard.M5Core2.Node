#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <riot2/views/NotificationModel.h>

#include "IView.h"

// Transient popup pushed by an inbound Command (e.g. a heads-up status
// update) - like AlertView, isAlert() means it never gets a buildUi() tab;
// onCommand() renders via showPopup() (PopupOverlay's auto-dismissing
// "notification" kind, with a countdown bar) instead of requiring a
// deliberate tap-to-acknowledge.
//
// The Command's `value` is parsed by the shared
// riot2::views::NotificationModel: an object with "title" and "message"
// fields, or a plain string used directly as the message. Falls back to a
// generic "Notification" title / empty message. The auto-dismiss timeout
// defaults to 4 seconds, overridable via this view's "durationMs"
// deviceParameter.
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
    riot2::views::NotificationModel _model;
};
