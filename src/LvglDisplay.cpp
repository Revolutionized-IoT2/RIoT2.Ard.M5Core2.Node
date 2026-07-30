#include "LvglDisplay.h"

#include <M5Unified.h>
#include <esp_heap_caps.h>

namespace {
// LVGL 9.x no longer honors the old v8-style LV_TICK_CUSTOM /
// LV_TICK_CUSTOM_INCLUDE / LV_TICK_CUSTOM_SYS_TIME_EXPR macros in lv_conf.h
// (lv_tick.c has no reference to them at all in 9.x) - lv_tick_get() now
// always returns 0 unless a callback is registered at runtime via
// lv_tick_set_cb(), or lv_tick_inc() is called manually. Without this,
// LVGL's internal tick is permanently frozen at 0, so every period-based
// lv_timer (including the display's own refresh timer) computes an elapsed
// time of 0 forever and never re-fires after its very first execution -
// the display renders exactly one frame at boot and then appears "frozen"
// even though loop()/lv_timer_handler() keep running normally and touch
// input keeps working (see /memories/repo/lvgl-refresh-stops-after-first-frame.md).
uint32_t lvglTickGetCb() {
    return millis();
}

// Registered below via lv_log_register_print_cb() so LV_LOG_WARN/ERROR
// (including the message LV_ASSERT_MALLOC prints right before the default
// LV_ASSERT_HANDLER's silent `while(1);` halt) actually reaches the serial
// monitor - Serial.printf() (our own app code), not raw printf() from
// vendored library code, matches this project's proven-reliable logging
// channel.
void lvglLogPrintCb(lv_log_level_t level, const char* buf) {
    Serial.printf("[LVGL] %s", buf);
}
}  // namespace

void LvglDisplay::begin() {
    lv_init();
    lv_tick_set_cb(lvglTickGetCb);
    lv_log_register_print_cb(lvglLogPrintCb);

    const int32_t width = M5.Display.width();
    const int32_t height = M5.Display.height();

    // Two full-frame 16bpp buffers in PSRAM (~150KB each on Core2's 320x240
    // panel). Reverted back from small (40-line), reused internal-RAM
    // partial buffers: those were tried as a fix for reported "grainy"/
    // color-fringed anti-aliased text, on the theory that PSRAM + a
    // CPU-driven SPI write loop was the culprit, but the fringing persisted
    // - and a *reused* small buffer is itself a real, distinct risk for
    // exactly that symptom: solid/opaque pixels always get directly
    // overwritten with a fully-computed color, so they're immune to stale
    // buffer content, but blended (anti-aliased) pixels can depend on
    // whatever backdrop is already sitting in the destination buffer at
    // blend time - a small buffer gets reused across many unrelated screen
    // regions every frame, so if any redraw path ever composites text
    // without first re-filling the full background in that reused chunk,
    // the blend reads leftover pixel data from whatever *other* region was
    // last rendered into that chunk, which looks exactly like random,
    // rainbow-colored noise concentrated at blended edges while flat fills
    // stay clean. A full-frame buffer has no such cross-region reuse within
    // a frame, sidestepping that risk entirely.
    size_t bufBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * sizeof(uint16_t);
    _buf1 = static_cast<uint16_t*>(heap_caps_malloc(bufBytes, MALLOC_CAP_SPIRAM));
    _buf2 = static_cast<uint16_t*>(heap_caps_malloc(bufBytes, MALLOC_CAP_SPIRAM));

    _display = lv_display_create(width, height);
    lv_display_set_flush_cb(_display, flushCb);
    // LVGL's internal RGB565 pixel format is packed in native CPU byte
    // order (little-endian on ESP32), but M5GFX/LovyanGFX's pushPixels()
    // expects the wire byte order the SPI panel itself uses - the two
    // don't match. Pure black/white (and other byte-swap-palindromic
    // values) are unaffected either way, so flat fills always looked
    // correct, but alpha-blended anti-aliased edge pixels are almost never
    // palindromic - handing them to the panel un-swapped scrambles each one
    // into an unrelated color, which looks exactly like scattered
    // multicolored/"rainbow" noise concentrated at blended edges while
    // solid fills stay clean (confirmed: LVGL's own official LovyanGFX
    // driver, src/drivers/display/lovyan_gfx/lv_lovyan_gfx.cpp, sets this
    // exact same color format for this exact display family). Telling LVGL
    // to render directly in the swapped format avoids needing a separate
    // lv_draw_sw_rgb565_swap() pass over the buffer in flushCb().
    lv_display_set_color_format(_display, LV_COLOR_FORMAT_RGB565_SWAPPED);
    // FULL, not PARTIAL: with two full-frame buffers already available,
    // FULL mode makes LVGL recompute the *entire* screen into one buffer
    // every refresh (alternating buf1/buf2), rather than only the
    // invalidated area(s) - removing any possibility of a redraw path
    // compositing against stale/partial buffer content (see the comment
    // above on why that's a real risk for the reported color fringing).
    lv_display_set_buffers(_display, _buf1, _buf2, bufBytes, LV_DISPLAY_RENDER_MODE_FULL);

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
