#include "SliderView.h"

#include <memory>

#include "ViewFactory.h"

namespace {
constexpr lv_coord_t kSliderWidth = 220;
}  // namespace

void SliderView::begin(const DeviceConfiguration& config) {
    _commandId = config.commandTemplates.empty() ? String() : config.commandTemplates.front().id;
    _reportId = config.reportTemplates.empty() ? String() : config.reportTemplates.front().id;

    _min = findParameter(config.deviceParameters, "min", "0").toInt();
    _max = findParameter(config.deviceParameters, "max", "100").toInt();
    _step = findParameter(config.deviceParameters, "step", "1").toInt();
    if (_step <= 0) {
        _step = 1;
    }
    _unit = findParameter(config.deviceParameters, "unit", "");
}

int32_t SliderView::snapToStep(int32_t value) const {
    int32_t snapped = _min + static_cast<int32_t>(lroundf(static_cast<float>(value - _min) / _step)) * _step;
    if (snapped < _min) snapped = _min;
    if (snapped > _max) snapped = _max;
    return snapped;
}

void SliderView::buildUi(lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    _valueLabel = lv_label_create(parent);
    updateValueLabel(_min);

    _slider = lv_slider_create(parent);
    lv_obj_set_width(_slider, kSliderWidth);
    lv_slider_set_range(_slider, _min, _max);
    lv_slider_set_value(_slider, _min, LV_ANIM_OFF);
    lv_obj_add_event_cb(_slider, sliderReleasedCb, LV_EVENT_RELEASED, this);
}

void SliderView::updateValueLabel(int32_t value) {
    if (!_valueLabel) {
        return;
    }
    String text = String(value) + _unit;
    lv_label_set_text(_valueLabel, text.c_str());
}

void SliderView::onCommand(const Command& command) {
    int32_t value = snapToStep(command.value.as<int32_t>());
    if (_slider) {
        lv_slider_set_value(_slider, value, LV_ANIM_ON);
    }
    updateValueLabel(value);
}

void SliderView::sliderReleasedCb(lv_event_t* event) {
    auto* self = static_cast<SliderView*>(lv_event_get_user_data(event));
    int32_t value = self->snapToStep(lv_slider_get_value(self->_slider));
    lv_slider_set_value(self->_slider, value, LV_ANIM_OFF);
    self->updateValueLabel(value);
    if (self->_reportId.length() > 0) {
        self->publishReport(Report{self->_reportId, String(value)});
    }
}

namespace {
struct SliderViewRegistrar {
    SliderViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Core2.Node.SliderView",
                                                 []() { return std::make_unique<SliderView>(); });
    }
} sliderViewRegistrar;
}  // namespace
