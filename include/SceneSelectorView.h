#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <vector>

#include "IView.h"

// Flex-wrap grid of tappable tiles, one per reportTemplate - tap = select
// AND confirm in one gesture (M5Dial.Node's SceneSelectorView needed a
// browse-then-tap-to-confirm two-step since it only had a rotary encoder;
// a touchscreen tap is already unambiguous). Publishes Report{id, "true"}
// for the tapped tile's reportTemplate.
class SceneSelectorView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;

private:
    struct Slot {
        ReportTemplate report;
        SceneSelectorView* owner = nullptr;
    };

    std::vector<Slot> _slots;

    static void tileTappedCb(lv_event_t* event);
};
