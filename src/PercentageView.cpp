#include "PercentageView.h"

#include <memory>

#include "ViewFactory.h"

namespace {
constexpr lv_coord_t kArcSize = 160;
}  // namespace

void PercentageView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
}

void PercentageView::buildUi(lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    _arc = lv_arc_create(parent);
    lv_obj_set_size(_arc, kArcSize, kArcSize);
    lv_arc_set_range(_arc, riot2::views::PercentageModel::kMinimum, riot2::views::PercentageModel::kMaximum);
    lv_arc_set_value(_arc, _model.value());
    lv_obj_add_event_cb(_arc, arcReleasedCb, LV_EVENT_RELEASED, this);

    _valueLabel = lv_label_create(_arc);
    lv_obj_center(_valueLabel);
    updateValueLabel();
}

void PercentageView::updateValueLabel() {
    if (!_valueLabel) {
        return;
    }
    lv_label_set_text(_valueLabel, (String(_model.value()) + "%").c_str());
}

void PercentageView::onCommand(const Command& command) {
    if (!_model.onCommand(command)) {
        return;
    }
    if (_arc) {
        lv_arc_set_value(_arc, _model.value());
    }
    updateValueLabel();
}

void PercentageView::arcReleasedCb(lv_event_t* event) {
    auto* self = static_cast<PercentageView*>(lv_event_get_user_data(event));
    Report report;
    bool hasReport = self->_model.set(lv_arc_get_value(self->_arc), report);
    self->updateValueLabel();
    if (hasReport) {
        self->publishReport(report);
    }
}

namespace {
struct PercentageViewRegistrar {
    PercentageViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Core2.Node.PercentageView", []() { return std::make_unique<PercentageView>(); },
            []() { return riot2::views::PercentageModel::buildTemplate("RIoT2.Ard.M5Core2.Node.PercentageView"); });
    }
} percentageViewRegistrar;
}  // namespace
