#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include "IView.h"

// HSV color picker: LVGL v9 dropped lv_colorwheel (present in v8, absent
// from this project's installed v9.5.0 - confirmed by searching the whole
// installed lvgl/src tree), so this rebuilds the classic "hue slider + SV
// pad" picker layout from plain lv_obj widgets/gradients instead:
//   1. A rainbow hue bar (6 segments, each a 2-stop lv_obj bg gradient
//      between the standard 60-degree HSV hue wheel colors - exactly
//      reproduces true hue interpolation, no approximation) - tapping/
//      dragging it picks the hue.
//   2. A square saturation/value pad whose background is a horizontal
//      white -> current-hue-color gradient with a vertical transparent ->
//      black gradient overlay on top (the standard HSV picker square:
//      color(s, v) = lerp(white, hueColor, s) * v) - tapping/dragging it
//      picks saturation (x) and value (y).
//   3. A swatch previewing the fully composed color live, and a label
//      showing its "#RRGGBB" hex value.
// Report/Command value format matches M5Dial.Node's ColorSchemeView: a
// "#RRGGBB" JSON string. Reports are only published on release (see
// SliderView/PercentageView's identical pattern), not on every drag tick,
// to avoid flooding MQTT with a report per pixel of movement.
class ColorSchemeView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;
    void onCommand(const Command& command) override;

private:
    String _commandId;
    String _reportId;

    int32_t _hue = 0;    // 0-359
    int32_t _sat = 100;  // 0-100
    int32_t _val = 100;  // 0-100

    lv_obj_t* _swatch = nullptr;
    lv_obj_t* _svPad = nullptr;
    lv_obj_t* _svShadeOverlay = nullptr;
    lv_obj_t* _svHandle = nullptr;
    lv_obj_t* _hueBar = nullptr;
    lv_obj_t* _hueHandle = nullptr;
    lv_obj_t* _hexLabel = nullptr;

    // Persisted gradient descriptors - lv_obj_set_style_bg_grad() stores the
    // pointer handed to it, not a copy, so these must outlive (and be
    // mutated in place instead of replaced by) the styles referencing them.
    lv_grad_dsc_t _svHueGrad{};
    lv_grad_dsc_t _svShadeGrad{};

    void setHue(int32_t hue);
    void setSatVal(int32_t sat, int32_t val);
    void updateCurrentColor();
    void repositionSvHandle();
    void repositionHueHandle();
    void publishCurrentColor();

    static void svPadTouchCb(lv_event_t* event);
    static void svPadReleasedCb(lv_event_t* event);
    static void hueBarTouchCb(lv_event_t* event);
    static void hueBarReleasedCb(lv_event_t* event);
};
