#include "EnergyGaugeView.h"

#include <memory>

#include <riot2/Uuid.h>

#include "AppColors.h"
#include "ViewFactory.h"

namespace {
constexpr lv_coord_t kGaugeSize = 160;
constexpr uint32_t kAngleRange = 270;
constexpr int32_t kRotation = 135;  // 3-o'clock offset for the gauge's start tick
// Negative needle length = "radius - |needle_length|" (see lv_scale_set_line_needle_value()),
// so the needle scales with kGaugeSize instead of needing a hardcoded absolute length.
constexpr int32_t kNeedleLength = -30;
const lv_color_t kNeedleColor = AppColors::indigoDarken2();
}  // namespace

void EnergyGaugeView::begin(const DeviceConfiguration& config) {
    _commandId = config.commandTemplates.empty() ? String() : config.commandTemplates.front().id;
    _unit = findParameter(config.deviceParameters, "unit", "W");
    _min = findParameter(config.deviceParameters, "min", "0").toInt();
    _max = findParameter(config.deviceParameters, "max", "3000").toInt();
    if (_max <= _min) {
        _max = _min + 1;
    }
}

void EnergyGaugeView::buildUi(lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    _scale = lv_scale_create(parent);
    lv_obj_set_size(_scale, kGaugeSize, kGaugeSize);
    lv_scale_set_mode(_scale, LV_SCALE_MODE_ROUND_INNER);
    lv_scale_set_range(_scale, _min, _max);
    lv_scale_set_angle_range(_scale, kAngleRange);
    lv_scale_set_rotation(_scale, kRotation);
    lv_scale_set_total_tick_count(_scale, 11);
    lv_scale_set_major_tick_every(_scale, 5);
    lv_obj_set_style_length(_scale, 6, LV_PART_ITEMS);
    lv_obj_set_style_length(_scale, 10, LV_PART_INDICATOR);

    _needle = lv_line_create(_scale);
    lv_obj_set_style_line_color(_needle, kNeedleColor, LV_PART_MAIN);
    lv_obj_set_style_line_width(_needle, 6, LV_PART_MAIN);
    lv_obj_set_style_line_rounded(_needle, true, LV_PART_MAIN);
    // Read-only - this gauge only ever moves in response to onCommand(),
    // there's no user interaction on a scale/needle widget to guard against
    // (unlike PercentageView's near-identical but draggable arc).
    lv_scale_set_line_needle_value(_scale, _needle, kNeedleLength, _min);

    _valueLabel = lv_label_create(_scale);
    lv_obj_center(_valueLabel);
    updateValueLabel(_min);
}

void EnergyGaugeView::updateValueLabel(int32_t value) {
    if (!_valueLabel) {
        return;
    }
    lv_label_set_text(_valueLabel, (String(value) + _unit).c_str());
}

void EnergyGaugeView::onCommand(const Command& command) {
    if (_commandId.length() == 0 || command.id != _commandId) {
        return;
    }
    int32_t value = command.value.as<int32_t>();
    if (value < _min) value = _min;
    if (value > _max) value = _max;
    if (_scale && _needle) {
        lv_scale_set_line_needle_value(_scale, _needle, kNeedleLength, value);
    }
    updateValueLabel(value);
}

namespace {
DeviceConfiguration buildEnergyGaugeViewTemplate() {
    DeviceConfiguration config;
    config.id = riot2::newId();
    config.name = "Energy Gauge View";
    config.classFullName = "RIoT2.Ard.M5Core2.Node.EnergyGaugeView";
    config.deviceParameters = {{"unit", "W"}, {"min", "0"}, {"max", "3000"}};

    CommandTemplate cmd;
    cmd.id = riot2::newId();
    cmd.type = "2";
    cmd.name = "Power";
    cmd.address = "power-1";
    cmd.valueType = 2;
    config.commandTemplates.push_back(cmd);
    return config;
}

struct EnergyGaugeViewRegistrar {
    EnergyGaugeViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Core2.Node.EnergyGaugeView",
                                                 []() { return std::make_unique<EnergyGaugeView>(); },
                                                 buildEnergyGaugeViewTemplate);
    }
} energyGaugeViewRegistrar;
}  // namespace
