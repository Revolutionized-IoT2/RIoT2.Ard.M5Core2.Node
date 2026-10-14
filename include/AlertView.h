#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <riot2/views/AlertModel.h>

#include "IView.h"

// Transient popup pushed by an inbound Command (e.g. doorbell, alarm) -
// isAlert() tells ViewManager this Command must never route to a buildUi()
// tab, and onCommand() itself renders via showPopup() (PopupOverlay's
// tap-to-dismiss "alert" kind) instead. Never built into a tab -
// ViewManager::rebuild() skips buildUi() for isAlert() views, so this is a
// no-op empty override.
//
// The Command's `value` is parsed by the shared riot2::views::AlertModel:
// an object with "title" and "message" fields (e.g. { "title": "Doorbell",
// "message": "Someone is at the front door" }), or a plain string used
// directly as the message. Falls back to a generic "Alert" title / empty
// message when the value doesn't specify them; "subHeader" isn't shown here.
//
// The Command's `value` object may also include a "soundEnabled" field
// (default false) controlling whether Buzzer::alert() plays.
class AlertView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override {}
    void onCommand(const Command& command) override;

    bool isAlert() const override { return true; }

private:
    riot2::views::AlertModel _model;
};
