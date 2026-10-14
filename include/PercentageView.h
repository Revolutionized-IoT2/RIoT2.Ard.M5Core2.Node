#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <riot2/views/PercentageModel.h>

#include "IView.h"

// Single lv_arc (drag-to-adjust) rendering the shared
// riot2::views::PercentageModel: a 0-100% range bound to this device's first
// commandTemplate/reportTemplate (by list order). Dragging and releasing the
// arc publishes the report directly - no deviceParameters, same
// simplification as SliderView (no encoder to need a tap-to-confirm step).
class PercentageView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;
    void onCommand(const Command& command) override;

private:
    riot2::views::PercentageModel _model;

    lv_obj_t* _arc = nullptr;
    lv_obj_t* _valueLabel = nullptr;

    void updateValueLabel();

    static void arcReleasedCb(lv_event_t* event);
};
