#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include "IView.h"

// Single lv_slider bound to this device's first commandTemplate/reportTemplate
// (by list order, like M5Dial.Node's SliderView) - dragging and releasing the
// slider snaps to "step" and publishes the report directly, dropping
// M5Dial's tap-to-adjust/encoder/tap-to-confirm state machine (there's no
// encoder here, and a drag gesture already is the confirmation). An inbound
// Command addressed to the commandTemplate sets the slider's position
// without publishing a report back.
//
// deviceParameters: "min" (default "0"), "max" (default "100"), "step"
// (default "1"), "unit" (default "", appended to the value label).
class SliderView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;
    void onCommand(const Command& command) override;

private:
    String _commandId;
    String _reportId;
    String _unit;
    int32_t _min = 0;
    int32_t _max = 100;
    int32_t _step = 1;

    lv_obj_t* _slider = nullptr;
    lv_obj_t* _valueLabel = nullptr;

    void updateValueLabel(int32_t value);
    int32_t snapToStep(int32_t value) const;

    static void sliderReleasedCb(lv_event_t* event);
};
