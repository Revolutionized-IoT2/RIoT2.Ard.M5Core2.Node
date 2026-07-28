#include "LvglDisplay.h"

#include <M5Unified.h>
#include <esp_heap_caps.h>

void LvglDisplay::begin() {
    lv_init();

    const int32_t width = M5.Display.width();
    const int32_t height = M5.Display.height();

    // Two full-frame 16bpp buffers in PSRAM (~150KB each on Core2's 320x240
    // panel) - trivial against the 8MB of onboard PSRAM. Registered for
    // LV_DISPLAY_RENDER_MODE_PARTIAL: LVGL only redraws/pushes the area(s)
    // that actually changed each frame, alternating between the two buffers
    // so rendering the next frame can start before the previous flush (the
    // SPI push to the panel) finishes.
    size_t bufBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * sizeof(uint16_t);
    _buf1 = static_cast<uint16_t*>(heap_caps_malloc(bufBytes, MALLOC_CAP_SPIRAM));
    _buf2 = static_cast<uint16_t*>(heap_caps_malloc(bufBytes, MALLOC_CAP_SPIRAM));

    _display = lv_display_create(width, height);
    lv_display_set_flush_cb(_display, flushCb);
    lv_display_set_buffers(_display, _buf1, _buf2, bufBytes, LV_DISPLAY_RENDER_MODE_PARTIAL);

    _indev = lv_indev_create();
    lv_indev_set_type(_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(_indev, touchReadCb);
}

void LvglDisplay::flushCb(lv_display_t* display, const lv_area_t* area, uint8_t* pixelMap) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    M5.Display.startWrite();
    M5.Display.setAddrWindow(area->x1, area->y1, w, h);
    M5.Display.pushPixels(reinterpret_cast<uint16_t*>(pixelMap), w * h);
    M5.Display.endWrite();

    lv_display_flush_ready(display);
}

void LvglDisplay::touchReadCb(lv_indev_t* indev, lv_indev_data_t* data) {
    // M5.update() is called exactly once per loop() from main.cpp, not here
    // - lv_timer_handler() may invoke this read callback more than once per
    // loop() iteration, and re-running M5.update() on each of those calls
    // would let it observe the same physical touch event more than once as
    // separate press/release transitions.
    if (M5.Touch.getCount() > 0) {
        auto detail = M5.Touch.getDetail(0);
        data->point.x = detail.x;
        data->point.y = detail.y;
        data->state = detail.isPressed() ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}
