#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include "IView.h"

// Read-only lv_arc + centered numeric label showing a power/energy reading,
// bound to this device's first commandTemplate (by list order) - updated
// only via onCommand(), like ValueView; never publishes reports (there's
// nothing for the user to adjust here, unlike PercentageView's drag-to-set
// arc).
//
// Optional deviceParameters: "unit" (default "W"), "min"/"max" (arc range,
// default 0/3000) - a typical single-appliance or whole-home power reading
// range.
class EnergyGaugeView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;
    void onCommand(const Command& command) override;

private:
    String _commandId;
    String _unit;
    int32_t _min = 0;
    int32_t _max = 3000;

    lv_obj_t* _arc = nullptr;
    lv_obj_t* _valueLabel = nullptr;

    void updateValueLabel(int32_t value);
};
