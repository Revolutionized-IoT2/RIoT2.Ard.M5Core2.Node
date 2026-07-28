#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include "IView.h"

// Hue-only color picker: LVGL v9 dropped lv_colorwheel (present in v8, absent
// from this project's installed v9.5.0 - confirmed by searching the whole
// installed lvgl/src tree), so this substitutes an lv_arc as a 0-359 hue
// selector (fixed full saturation/value) + a swatch preview + an explicit
// "Set" button, converting via lv_color_hsv_to_rgb()/lv_color_rgb_to_hsv().
// Report/Command value format matches M5Dial.Node's ColorSchemeView: a
// "#RRGGBB" JSON string. The explicit Set button (rather than reporting on
// every drag tick) avoids flooding MQTT with a report per pixel of arc
// movement.
class ColorSchemeView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;
    void onCommand(const Command& command) override;

private:
    String _commandId;
    String _reportId;

    lv_obj_t* _arc = nullptr;
    lv_obj_t* _swatch = nullptr;

    void applyHue(int32_t hue);

    static void arcChangedCb(lv_event_t* event);
    static void setButtonTappedCb(lv_event_t* event);
};
