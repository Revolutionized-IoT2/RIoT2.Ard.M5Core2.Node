#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <vector>

#include "IView.h"

// Displays up to 4 read-only values with unit labels, stacked in a centered
// column. Values are only ever updated via an inbound Command matching one
// of this view's commandTemplates - ValueView never publishes reports.
class ValueView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;
    void onCommand(const Command& command) override;

private:
    struct Slot {
        String id;
        String name;
        String unit;
        lv_obj_t* valueLabel = nullptr;
    };

    std::vector<Slot> _slots;
};
