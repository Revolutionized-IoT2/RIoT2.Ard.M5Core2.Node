#include "SliderView.h"

#include <memory>

#include "ViewFactory.h"

namespace {
constexpr lv_coord_t kSliderWidth = 220;
}  // namespace

void SliderView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
}

void SliderView::buildUi(lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    _valueLabel = lv_label_create(parent);
    updateValueLabel();

    _slider = lv_slider_create(parent);
    lv_obj_set_width(_slider, kSliderWidth);
    lv_slider_set_range(_slider, _model.minimum(), _model.maximum());
    lv_slider_set_value(_slider, _model.value(), LV_ANIM_OFF);
    lv_obj_add_event_cb(_slider, sliderReleasedCb, LV_EVENT_RELEASED, this);
}

void SliderView::updateValueLabel() {
    if (!_valueLabel) {
        return;
    }
    String text = String(_model.value()) + _model.unit();
    lv_label_set_text(_valueLabel, text.c_str());
}

void SliderView::onCommand(const Command& command) {
    if (!_model.onCommand(command)) {
        return;
    }
    if (_slider) {
        lv_slider_set_value(_slider, _model.value(), LV_ANIM_ON);
    }
    updateValueLabel();
}

void SliderView::sliderReleasedCb(lv_event_t* event) {
    auto* self = static_cast<SliderView*>(lv_event_get_user_data(event));
    int32_t dragged = self->_model.snap(lv_slider_get_value(self->_slider));
    lv_slider_set_value(self->_slider, dragged, LV_ANIM_OFF);
    Report report;
    if (self->_model.set(dragged, report)) {
        self->publishReport(report);
    }
    // Without a report template the label still follows the drag.
    if (!self->_model.canReport() && self->_valueLabel) {
        String text = String(dragged) + self->_model.unit();
        lv_label_set_text(self->_valueLabel, text.c_str());
        return;
    }
    self->updateValueLabel();
}

namespace {
struct SliderViewRegistrar {
    SliderViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Core2.Node.SliderView", []() { return std::make_unique<SliderView>(); },
            []() { return riot2::views::SliderModel::buildTemplate("RIoT2.Ard.M5Core2.Node.SliderView"); });
    }
} sliderViewRegistrar;
}  // namespace
