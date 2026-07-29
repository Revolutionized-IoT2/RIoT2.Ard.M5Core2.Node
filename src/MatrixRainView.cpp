#include "MatrixRainView.h"

#include <M5Unified.h>
#include <esp_heap_caps.h>

#include "lv_font_montserrat_18_bpp8.h"

namespace {
// Matches MatrixRain.cs's AvailableLetterChars.
const char kAvailableChars[] = "abcdefghijklmnopqrstuvwxyz1234567890";

// ~24% - low enough that a glyph takes several frames to fully fade into
// the background, matching MatrixRain.cs's DrawRectangle(..., opacity: 25)
// fade-trail effect.
constexpr lv_opa_t kFadeOpa = 60;
}  // namespace

void MatrixRainView::begin() {
    _screenWidth = M5.Display.width();
    _screenHeight = M5.Display.height();

    _container = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(_container);
    lv_obj_set_size(_container, _screenWidth, _screenHeight);
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_set_style_bg_color(_container, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_COVER, 0);
    // Swallows taps so they don't reach the (dimmed but still live)
    // tabview underneath while this idle screen is up - ScreenPowerPolicy
    // detects the wake gesture itself by polling M5.Touch/M5.BtnA/B/C
    // directly, not via this object's own LVGL events.
    lv_obj_add_flag(_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(_container, LV_OBJ_FLAG_HIDDEN);

    size_t bufBytes = static_cast<size_t>(_screenWidth) * static_cast<size_t>(_screenHeight) * sizeof(uint16_t);
    _canvasBuf = static_cast<uint16_t*>(heap_caps_malloc(bufBytes, MALLOC_CAP_SPIRAM));

    _canvas = lv_canvas_create(_container);
    // Plain (non-swapped) RGB565, same as the embedded icon images - LVGL's
    // blend layer converts to the display's LV_COLOR_FORMAT_RGB565_SWAPPED
    // automatically when compositing the canvas onto the screen (see
    // /memories/repo/lvgl-icon-embedding-rgb565a8.md).
    lv_canvas_set_buffer(_canvas, _canvasBuf, _screenWidth, _screenHeight, LV_COLOR_FORMAT_RGB565);
    lv_obj_set_pos(_canvas, 0, 0);

    _cellWidth = lv_font_get_glyph_width(&lv_font_montserrat_18_bpp8, 'm', 0);
    _cellHeight = lv_font_get_line_height(&lv_font_montserrat_18_bpp8);
    if (_cellWidth < 1) {
        _cellWidth = 12;
    }
    if (_cellHeight < 1) {
        _cellHeight = 22;
    }

    _timer = lv_timer_create(timerTickCb, kTimerPeriodMs, this);
    lv_timer_pause(_timer);
}

void MatrixRainView::start() {
    if (!_container) {
        return;
    }

    int32_t columns = _screenWidth / _cellWidth;
    if (columns < 1) {
        columns = 1;
    }
    _drops.assign(columns, 0);

    lv_canvas_fill_bg(_canvas, lv_color_black(), LV_OPA_COVER);
    lv_obj_remove_flag(_container, LV_OBJ_FLAG_HIDDEN);
    lv_timer_reset(_timer);
    lv_timer_resume(_timer);
}

void MatrixRainView::stop() {
    if (!_container) {
        return;
    }
    lv_timer_pause(_timer);
    lv_obj_add_flag(_container, LV_OBJ_FLAG_HIDDEN);
}

bool MatrixRainView::isVisible() const {
    return _container && !lv_obj_has_flag(_container, LV_OBJ_FLAG_HIDDEN);
}

void MatrixRainView::timerTickCb(lv_timer_t* timer) {
    static_cast<MatrixRainView*>(lv_timer_get_user_data(timer))->tick();
}

void MatrixRainView::tick() {
    lv_layer_t layer;
    lv_canvas_init_layer(_canvas, &layer);

    lv_draw_rect_dsc_t fadeDsc;
    lv_draw_rect_dsc_init(&fadeDsc);
    fadeDsc.bg_color = lv_color_black();
    fadeDsc.bg_opa = kFadeOpa;
    lv_area_t fullArea = {0, 0, static_cast<lv_coord_t>(_screenWidth - 1), static_cast<lv_coord_t>(_screenHeight - 1)};
    lv_draw_rect(&layer, &fadeDsc, &fullArea);

    lv_draw_label_dsc_t labelDsc;
    lv_draw_label_dsc_init(&labelDsc);
    labelDsc.font = &lv_font_montserrat_18_bpp8;
    labelDsc.color = lv_color_hex(0x66FF00);

    char glyph[2] = {0, 0};
    for (size_t i = 0; i < _drops.size(); i++) {
        int32_t x = static_cast<int32_t>(i) * _cellWidth;
        int32_t y = _drops[i] * _cellHeight;

        if (y + _cellHeight < _screenHeight) {
            glyph[0] = kAvailableChars[random(sizeof(kAvailableChars) - 1)];
            labelDsc.text = glyph;
            lv_area_t textArea = {static_cast<lv_coord_t>(x), static_cast<lv_coord_t>(y),
                                   static_cast<lv_coord_t>(x + _cellWidth - 1), static_cast<lv_coord_t>(y + _cellHeight - 1)};
            lv_draw_label(&layer, &labelDsc, &textArea);
        }

        // Sends the drop back to the top at a random chance once it has
        // scrolled past the bottom edge, so drops don't all reset in
        // lockstep - mirrors MatrixRain.cs's identical reset condition.
        if (_drops[i] * _cellHeight > _screenHeight && random(1000) > 850) {
            _drops[i] = 0;
        }
        _drops[i]++;
    }

    lv_canvas_finish_layer(_canvas, &layer);
}
