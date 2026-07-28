#include "EnergyGaugeView.h"

#include <memory>

#include "ViewFactory.h"

namespace {
constexpr lv_coord_t kArcSize = 160;
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

    _arc = lv_arc_create(parent);
    lv_obj_set_size(_arc, kArcSize, kArcSize);
    lv_arc_set_range(_arc, _min, _max);
    lv_arc_set_value(_arc, _min);
    // Read-only - this arc only ever moves in response to onCommand(), never
    // dragged by the user (unlike PercentageView's near-identical arc).
    lv_obj_remove_flag(_arc, LV_OBJ_FLAG_CLICKABLE);

    _valueLabel = lv_label_create(_arc);
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
    if (_arc) {
        lv_arc_set_value(_arc, value);
    }
    updateValueLabel(value);
}

namespace {
struct EnergyGaugeViewRegistrar {
    EnergyGaugeViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Core2.Node.EnergyGaugeView",
                                                 []() { return std::make_unique<EnergyGaugeView>(); });
    }
} energyGaugeViewRegistrar;
}  // namespace
