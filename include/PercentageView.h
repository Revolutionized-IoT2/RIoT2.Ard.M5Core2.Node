#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include "IView.h"

// Single lv_arc (drag-to-adjust) fixed to a 0-100% range, bound to this
// device's first commandTemplate/reportTemplate (by list order). Dragging
// and releasing the arc publishes the report directly - no deviceParameters,
// same simplification as SliderView (no encoder to need a tap-to-confirm
// step).
class PercentageView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;
    void onCommand(const Command& command) override;

private:
    String _commandId;
    String _reportId;

    lv_obj_t* _arc = nullptr;
    lv_obj_t* _valueLabel = nullptr;

    void updateValueLabel(int32_t value);

    static void arcReleasedCb(lv_event_t* event);
};
