#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include "IView.h"

// Transient popup pushed by an inbound Command (e.g. doorbell, alarm) -
// isAlert() tells ViewManager this Command must never route to a buildUi()
// tab, and onCommand() itself renders via showPopup() (PopupOverlay's
// tap-to-dismiss "alert" kind) instead. Never built into a tab -
// ViewManager::rebuild() skips buildUi() for isAlert() views, so this is a
// no-op empty override.
//
// The Command's `value` is expected to be an object with "title" and
// "message" fields (e.g. { "title": "Doorbell", "message": "Someone is at
// the front door" }); a plain string is used directly as the message
// instead. Falls back to a generic "Alert" title / empty message when the
// value doesn't specify them.
//
// The Command's `value` object may also include a "soundEnabled" field
// (default false) controlling whether Buzzer::alert() plays.
class AlertView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override {}
    void onCommand(const Command& command) override;

    bool isAlert() const override { return true; }
};
