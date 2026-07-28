#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <vector>

#include "IView.h"

// 1-2 on/off lv_switch widgets, stacked vertically (a single switch is
// centered). Toggling a switch publishes its new state as a Report; an
// inbound Command addressed to the matching commandTemplate.id (correlated
// via shared `address`, as in ButtonView) sets that switch's state without
// re-publishing a Report for it.
class ToggleView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;
    void onCommand(const Command& command) override;

private:
    struct Slot {
        ReportTemplate report;
        CommandTemplate command;
        bool hasCommand = false;
        lv_obj_t* switchObj = nullptr;
        // Back-pointer so the static event callback (which only gets a
        // Slot*, its registered user_data) can still reach publishReport(),
        // a protected IView member.
        ToggleView* owner = nullptr;
    };

    std::vector<Slot> _slots;

    static void switchToggledCb(lv_event_t* event);
};
