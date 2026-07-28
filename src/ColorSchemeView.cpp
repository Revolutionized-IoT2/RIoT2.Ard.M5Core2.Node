#include "ColorSchemeView.h"

#include <memory>

#include "ViewFactory.h"

namespace {
constexpr lv_coord_t kArcSize = 160;
constexpr lv_coord_t kSwatchSize = 48;

String hueToHex(int32_t hue) {
    lv_color_t rgb = lv_color_hsv_to_rgb(static_cast<uint16_t>(hue), 100, 100);
    char buf[7];
    snprintf(buf, sizeof(buf), "%02X%02X%02X", rgb.red, rgb.green, rgb.blue);
    return String(buf);
}

// Parses a "#RRGGBB" (or "RRGGBB") string into its hue component, ignoring
// saturation/value - this view only ever selects a pure hue.
int32_t hexToHue(const String& hex) {
    String digits = hex.startsWith("#") ? hex.substring(1) : hex;
    if (digits.length() < 6) {
        return 0;
    }
    uint8_t r = static_cast<uint8_t>(strtol(digits.substring(0, 2).c_str(), nullptr, 16));
    uint8_t g = static_cast<uint8_t>(strtol(digits.substring(2, 4).c_str(), nullptr, 16));
    uint8_t b = static_cast<uint8_t>(strtol(digits.substring(4, 6).c_str(), nullptr, 16));
    return lv_color_rgb_to_hsv(r, g, b).h;
}
}  // namespace

void ColorSchemeView::begin(const DeviceConfiguration& config) {
    _commandId = config.commandTemplates.empty() ? String() : config.commandTemplates.front().id;
    _reportId = config.reportTemplates.empty() ? String() : config.reportTemplates.front().id;
}

void ColorSchemeView::buildUi(lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    _arc = lv_arc_create(parent);
    lv_obj_set_size(_arc, kArcSize, kArcSize);
    lv_arc_set_bg_angles(_arc, 0, 359);
    lv_arc_set_range(_arc, 0, 359);
    lv_arc_set_value(_arc, 0);
    lv_obj_add_event_cb(_arc, arcChangedCb, LV_EVENT_VALUE_CHANGED, this);

    _swatch = lv_obj_create(_arc);
    lv_obj_remove_style_all(_swatch);
    lv_obj_set_size(_swatch, kSwatchSize, kSwatchSize);
    lv_obj_set_style_radius(_swatch, kSwatchSize / 2, 0);
    lv_obj_set_style_bg_opa(_swatch, LV_OPA_COVER, 0);
    lv_obj_center(_swatch);

    lv_obj_t* setButton = lv_button_create(parent);
    lv_obj_t* setLabel = lv_label_create(setButton);
    lv_label_set_text(setLabel, "Set");
    lv_obj_center(setLabel);
    lv_obj_add_event_cb(setButton, setButtonTappedCb, LV_EVENT_CLICKED, this);

    applyHue(0);
}

void ColorSchemeView::applyHue(int32_t hue) {
    lv_color_t rgb = lv_color_hsv_to_rgb(static_cast<uint16_t>(hue), 100, 100);
    if (_swatch) {
        lv_obj_set_style_bg_color(_swatch, rgb, 0);
    }
}

void ColorSchemeView::onCommand(const Command& command) {
    int32_t hue = hexToHue(command.value.as<String>());
    if (_arc) {
        lv_arc_set_value(_arc, hue);
    }
    applyHue(hue);
}

void ColorSchemeView::arcChangedCb(lv_event_t* event) {
    auto* self = static_cast<ColorSchemeView*>(lv_event_get_user_data(event));
    self->applyHue(lv_arc_get_value(self->_arc));
}

void ColorSchemeView::setButtonTappedCb(lv_event_t* event) {
    auto* self = static_cast<ColorSchemeView*>(lv_event_get_user_data(event));
    if (self->_reportId.length() == 0) {
        return;
    }
    String hex = "#" + hueToHex(lv_arc_get_value(self->_arc));
    self->publishReport(Report{self->_reportId, String("\"") + hex + "\""});
}

namespace {
struct ColorSchemeViewRegistrar {
    ColorSchemeViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Core2.Node.ColorSchemeView",
                                                 []() { return std::make_unique<ColorSchemeView>(); });
    }
} colorSchemeViewRegistrar;
}  // namespace
