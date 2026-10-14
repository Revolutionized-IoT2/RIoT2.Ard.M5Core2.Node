#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <riot2/views/SliderModel.h>

#include "IView.h"

// Single lv_slider rendering the shared riot2::views::SliderModel (first
// commandTemplate/reportTemplate; deviceParameters "min", "max", "step",
// "unit"). Dragging and releasing the slider snaps to "step" and publishes
// the report directly - there's no encoder here, and a drag gesture already
// is the confirmation. An inbound Command sets the slider's position
// without publishing a report back.
class SliderView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;
    void onCommand(const Command& command) override;

private:
    riot2::views::SliderModel _model;

    lv_obj_t* _slider = nullptr;
    lv_obj_t* _valueLabel = nullptr;

    void updateValueLabel();

    static void sliderReleasedCb(lv_event_t* event);
};
