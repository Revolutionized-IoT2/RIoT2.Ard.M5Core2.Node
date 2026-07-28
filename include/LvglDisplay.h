#pragma once

#include <lvgl.h>

// Bridges LVGL's display/touch-input model onto M5Unified's M5.Display /
// M5.Touch. Core2 has no LVGL support built into M5GFX/M5Unified itself -
// this manual push/pull bridge is the standard community pattern for this
// hardware (e.g. RileyCornelius/M5Stack-Core2-Lvgl-Example,
// imliubo/lvgl_port_M5Core2).
//
// Call begin() once from setup() (after M5.begin()). Call lv_timer_handler()
// directly from loop() on every iteration to drive rendering/animations/
// input - it is not wrapped here since it's a bare LVGL global function,
// not something this class owns.
class LvglDisplay {
public:
    void begin();

private:
    static void flushCb(lv_display_t* display, const lv_area_t* area, uint8_t* pixelMap);
    static void touchReadCb(lv_indev_t* indev, lv_indev_data_t* data);

    lv_display_t* _display = nullptr;
    lv_indev_t* _indev = nullptr;
    uint16_t* _buf1 = nullptr;
    uint16_t* _buf2 = nullptr;
};
