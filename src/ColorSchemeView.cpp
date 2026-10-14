#include "ColorSchemeView.h"

#include <memory>

#include "ViewFactory.h"

namespace {
constexpr lv_coord_t kSwatchWidth = 64;
constexpr lv_coord_t kPadSize = 120;
constexpr lv_coord_t kRowGap = 14;
constexpr lv_coord_t kHueBarWidth = kSwatchWidth + kRowGap + kPadSize;
constexpr lv_coord_t kHueBarHeight = 22;
constexpr lv_coord_t kHueHandleSize = 26;
constexpr lv_coord_t kSvHandleSize = 18;

// The six 60-degree boundary colors of the HSV hue wheel - RGB varies
// perfectly linearly between each consecutive pair, so six 2-stop gradient
// segments reproduce true hue interpolation exactly (no approximation),
// which matters since this project's lv_conf.h leaves LV_GRADIENT_MAX_STOPS
// at its default of 2.
constexpr uint32_t kHueWheelStops[7] = {
    0xFF0000,  // 0   red
    0xFFFF00,  // 60  yellow
    0x00FF00,  // 120 green
    0x00FFFF,  // 180 cyan
    0x0000FF,  // 240 blue
    0xFF00FF,  // 300 magenta
    0xFF0000,  // 360 red (wraps)
};

lv_color_t toLvColor(const riot2::views::ColorSchemeModel::Rgb& rgb) {
    return lv_color_make(rgb.red, rgb.green, rgb.blue);
}
}  // namespace

void ColorSchemeView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
}

void ColorSchemeView::buildUi(lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(parent, kRowGap, 0);

    // --- Row: swatch preview + saturation/value pad -----------------------
    lv_obj_t* row = lv_obj_create(parent);
    lv_obj_remove_style_all(row);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(row, kRowGap, 0);

    _swatch = lv_obj_create(row);
    lv_obj_remove_style_all(_swatch);
    lv_obj_set_size(_swatch, kSwatchWidth, kPadSize);
    lv_obj_set_style_radius(_swatch, 8, 0);
    lv_obj_set_style_bg_opa(_swatch, LV_OPA_COVER, 0);
    lv_obj_remove_flag(_swatch, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(_swatch, LV_OBJ_FLAG_SCROLLABLE);

    _svPad = lv_obj_create(row);
    lv_obj_remove_style_all(_svPad);
    lv_obj_set_size(_svPad, kPadSize, kPadSize);
    lv_obj_set_style_radius(_svPad, 8, 0);
    lv_obj_set_style_clip_corner(_svPad, true, 0);
    lv_obj_set_style_bg_opa(_svPad, LV_OPA_COVER, 0);
    lv_obj_remove_flag(_svPad, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(_svPad, svPadTouchCb, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(_svPad, svPadTouchCb, LV_EVENT_PRESSING, this);
    lv_obj_add_event_cb(_svPad, svPadReleasedCb, LV_EVENT_RELEASED, this);
    lv_obj_add_event_cb(_svPad, svPadReleasedCb, LV_EVENT_PRESS_LOST, this);

    lv_color_t stopColors[2] = {lv_color_white(), lv_color_hex(kHueWheelStops[0])};
    lv_opa_t stopOpas[2] = {LV_OPA_COVER, LV_OPA_COVER};
    lv_grad_init_stops(&_svHueGrad, stopColors, stopOpas, nullptr, 2);
    lv_grad_horizontal_init(&_svHueGrad);
    lv_obj_set_style_bg_grad(_svPad, &_svHueGrad, 0);

    _svShadeOverlay = lv_obj_create(_svPad);
    lv_obj_remove_style_all(_svShadeOverlay);
    lv_obj_set_size(_svShadeOverlay, kPadSize, kPadSize);
    lv_obj_set_pos(_svShadeOverlay, 0, 0);
    lv_obj_set_style_bg_opa(_svShadeOverlay, LV_OPA_COVER, 0);
    lv_obj_remove_flag(_svShadeOverlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(_svShadeOverlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_color_t shadeColors[2] = {lv_color_black(), lv_color_black()};
    lv_opa_t shadeOpas[2] = {LV_OPA_TRANSP, LV_OPA_COVER};
    lv_grad_init_stops(&_svShadeGrad, shadeColors, shadeOpas, nullptr, 2);
    lv_grad_vertical_init(&_svShadeGrad);
    lv_obj_set_style_bg_grad(_svShadeOverlay, &_svShadeGrad, 0);

    _svHandle = lv_obj_create(_svPad);
    lv_obj_remove_style_all(_svHandle);
    lv_obj_set_size(_svHandle, kSvHandleSize, kSvHandleSize);
    lv_obj_set_style_radius(_svHandle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(_svHandle, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_svHandle, 2, 0);
    lv_obj_set_style_border_color(_svHandle, lv_color_white(), 0);
    lv_obj_remove_flag(_svHandle, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(_svHandle, LV_OBJ_FLAG_SCROLLABLE);

    // --- Hue bar ------------------------------------------------------------
    _hueBar = lv_obj_create(parent);
    lv_obj_remove_style_all(_hueBar);
    lv_obj_set_size(_hueBar, kHueBarWidth, kHueBarHeight);
    lv_obj_set_style_radius(_hueBar, kHueBarHeight / 2, 0);
    lv_obj_set_style_clip_corner(_hueBar, true, 0);
    lv_obj_remove_flag(_hueBar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(_hueBar, LV_FLEX_FLOW_ROW);
    lv_obj_add_event_cb(_hueBar, hueBarTouchCb, LV_EVENT_PRESSED, this);
    lv_obj_add_event_cb(_hueBar, hueBarTouchCb, LV_EVENT_PRESSING, this);
    lv_obj_add_event_cb(_hueBar, hueBarReleasedCb, LV_EVENT_RELEASED, this);
    lv_obj_add_event_cb(_hueBar, hueBarReleasedCb, LV_EVENT_PRESS_LOST, this);

    for (int i = 0; i < 6; i++) {
        lv_obj_t* segment = lv_obj_create(_hueBar);
        lv_obj_remove_style_all(segment);
        lv_obj_set_flex_grow(segment, 1);
        lv_obj_set_height(segment, LV_PCT(100));
        lv_obj_set_style_bg_opa(segment, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(segment, lv_color_hex(kHueWheelStops[i]), 0);
        lv_obj_set_style_bg_grad_color(segment, lv_color_hex(kHueWheelStops[i + 1]), 0);
        lv_obj_set_style_bg_grad_dir(segment, LV_GRAD_DIR_HOR, 0);
        lv_obj_remove_flag(segment, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_remove_flag(segment, LV_OBJ_FLAG_SCROLLABLE);
    }

    _hueHandle = lv_obj_create(_hueBar);
    lv_obj_remove_style_all(_hueHandle);
    lv_obj_set_size(_hueHandle, kHueHandleSize, kHueHandleSize);
    lv_obj_set_style_radius(_hueHandle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(_hueHandle, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(_hueHandle, lv_color_white(), 0);
    lv_obj_set_style_border_width(_hueHandle, 2, 0);
    lv_obj_set_style_border_color(_hueHandle, lv_color_white(), 0);
    lv_obj_remove_flag(_hueHandle, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(_hueHandle, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(_hueHandle, LV_OBJ_FLAG_IGNORE_LAYOUT);

    // --- Hex value label ------------------------------------------------
    _hexLabel = lv_label_create(parent);

    showHue();
    showSatVal();
    showCurrentColor();
}

void ColorSchemeView::showHue() {
    _svHueGrad.stops[1].color = toLvColor(riot2::views::ColorSchemeModel::hsvToRgb(_model.hue(), 100, 100));
    if (_svPad) {
        lv_obj_invalidate(_svPad);
    }
    if (!_hueHandle) {
        return;
    }
    int32_t x = (_model.hue() * (kHueBarWidth - 1)) / 359;
    lv_obj_set_pos(_hueHandle, x - kHueHandleSize / 2, (kHueBarHeight - kHueHandleSize) / 2);
}

void ColorSchemeView::showSatVal() {
    if (!_svHandle) {
        return;
    }
    int32_t x = (_model.saturation() * (kPadSize - 1)) / 100;
    int32_t y = ((100 - _model.value()) * (kPadSize - 1)) / 100;
    lv_obj_set_pos(_svHandle, x - kSvHandleSize / 2, y - kSvHandleSize / 2);
}

void ColorSchemeView::showCurrentColor() {
    if (_swatch) {
        lv_obj_set_style_bg_color(_swatch, toLvColor(_model.color()), 0);
    }
    if (_hexLabel) {
        lv_label_set_text(_hexLabel, _model.hex().c_str());
    }
}

void ColorSchemeView::publishCurrentColor() {
    Report report;
    if (_model.reportColor(report)) {
        publishReport(report);
    }
}

void ColorSchemeView::onCommand(const Command& command) {
    if (_model.onCommand(command)) {
        showHue();
        showSatVal();
        showCurrentColor();
    }
}

void ColorSchemeView::svPadTouchCb(lv_event_t* event) {
    auto* self = static_cast<ColorSchemeView*>(lv_event_get_user_data(event));
    lv_indev_t* indev = lv_indev_active();
    if (!indev) {
        return;
    }
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    lv_area_t area;
    lv_obj_get_coords(self->_svPad, &area);
    int32_t width = lv_area_get_width(&area);
    int32_t height = lv_area_get_height(&area);
    int32_t x = LV_CLAMP(0, p.x - area.x1, width - 1);
    int32_t y = LV_CLAMP(0, p.y - area.y1, height - 1);
    int32_t sat = width > 1 ? (x * 100) / (width - 1) : 0;
    int32_t val = height > 1 ? 100 - (y * 100) / (height - 1) : 100;
    self->_model.setSaturationValue(sat, val);
    self->showSatVal();
    self->showCurrentColor();
}

void ColorSchemeView::svPadReleasedCb(lv_event_t* event) {
    auto* self = static_cast<ColorSchemeView*>(lv_event_get_user_data(event));
    self->publishCurrentColor();
}

void ColorSchemeView::hueBarTouchCb(lv_event_t* event) {
    auto* self = static_cast<ColorSchemeView*>(lv_event_get_user_data(event));
    lv_indev_t* indev = lv_indev_active();
    if (!indev) {
        return;
    }
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    lv_area_t area;
    lv_obj_get_coords(self->_hueBar, &area);
    int32_t width = lv_area_get_width(&area);
    int32_t x = LV_CLAMP(0, p.x - area.x1, width - 1);
    int32_t hue = width > 1 ? (x * 359) / (width - 1) : 0;
    self->_model.setHue(hue);
    self->showHue();
    self->showCurrentColor();
}

void ColorSchemeView::hueBarReleasedCb(lv_event_t* event) {
    auto* self = static_cast<ColorSchemeView*>(lv_event_get_user_data(event));
    self->publishCurrentColor();
}

namespace {
struct ColorSchemeViewRegistrar {
    ColorSchemeViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Core2.Node.ColorSchemeView", []() { return std::make_unique<ColorSchemeView>(); },
            []() { return riot2::views::ColorSchemeModel::buildTemplate("RIoT2.Ard.M5Core2.Node.ColorSchemeView"); });
    }
} colorSchemeViewRegistrar;
}  // namespace