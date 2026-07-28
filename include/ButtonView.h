#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <vector>

#include "IView.h"

// 1-4 momentary buttons laid out in a centered, wrapping row. Tapping a
// button publishes a Report (id -> "true") for its reportTemplate; an
// inbound Command addressed to the matching commandTemplate.id (correlated
// by shared `address`, as in M5Dial.Node's ButtonView) sets that button's
// highlighted/on-off visual state. The view's own header/subheader text
// come from the "header"/"subHeader" deviceParameters.
class ButtonView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;
    void onCommand(const Command& command) override;

private:
    struct Slot {
        ReportTemplate report;
        CommandTemplate command;
        bool hasCommand = false;
        bool active = false;
        lv_obj_t* button = nullptr;
        // Back-pointer so the static event/timer callbacks (which only get a
        // Slot*, since that's what's registered as their user_data) can
        // still reach publishReport(), a protected IView member.
        ButtonView* owner = nullptr;
    };

    std::vector<Slot> _slots;
    String _header;
    String _subHeader;

    static void applyVisualState(Slot& slot);
    static void buttonTappedCb(lv_event_t* event);
    static void flashTimerCb(lv_timer_t* timer);
};
