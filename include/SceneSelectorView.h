#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <vector>

#include "IView.h"

// lv_roller listing one row per reportTemplate - spinning to (and settling
// on) a scene is itself the confirm gesture, publishing Report{id, "true"}
// for the selected reportTemplate (M5Dial.Node's SceneSelectorView needed a
// separate browse-then-confirm step since it only had a rotary encoder;
// here the roller's own settle event is already unambiguous).
class SceneSelectorView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;

private:
    struct Slot {
        ReportTemplate report;
    };

    std::vector<Slot> _slots;
    lv_obj_t* _roller = nullptr;

    static void rollerValueChangedCb(lv_event_t* event);
};
