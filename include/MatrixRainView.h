#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <vector>

// Full-screen "digital rain" idle-screen animation, shown by
// ScreenPowerPolicy once the display dims after kIdleTimeoutMs of
// inactivity - replaces the earlier "show the current time via
// PopupOverlay::showAlert()" idle screen. Ported from the sibling
// nanoFramework project's UI/MatrixRain.cs: one falling "drop" per
// character column, each frame paints a low-opacity black rectangle over
// the whole canvas (fades previously-drawn glyphs a little every tick,
// producing a trailing effect over several frames) then draws one random
// glyph per column at its current row and advances the row; a column
// resets back to the top at a random chance once it has fully scrolled
// off the bottom edge, so drops don't restart in lockstep.
//
// Draws into a persistent lv_canvas buffer (allocated in PSRAM, like
// LvglDisplay's own frame buffers) rather than plain lv_label objects,
// because the fade-trail effect depends on the previous frame's pixels
// still being present to blend against - individual labels would need to
// be re-created/recolored every tick and have no way to "partially fade"
// their own previously-rendered glyph.
class MatrixRainView {
public:
    // Creates the (hidden) overlay on lv_layer_top(). Call once from
    // setup(), after LvglDisplay::begin().
    void begin();

    // Clears the canvas, resets every column's drop position, and starts
    // animating. Safe to call again while already visible (restarts).
    void start();

    // Stops the animation timer and hides the overlay. Safe to call when
    // not currently visible (no-op).
    void stop();

    bool isVisible() const;

private:
    static constexpr uint32_t kTimerPeriodMs = 50;

    lv_obj_t* _container = nullptr;
    lv_obj_t* _canvas = nullptr;
    uint16_t* _canvasBuf = nullptr;
    lv_timer_t* _timer = nullptr;

    int32_t _screenWidth = 0;
    int32_t _screenHeight = 0;
    int32_t _cellWidth = 0;
    int32_t _cellHeight = 0;
    std::vector<int32_t> _drops;

    void tick();

    static void timerTickCb(lv_timer_t* timer);
};
