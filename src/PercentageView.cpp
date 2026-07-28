#include "PercentageView.h"

#include <memory>

#include "ViewFactory.h"

namespace {
constexpr lv_coord_t kArcSize = 160;
}  // namespace

void PercentageView::begin(const DeviceConfiguration& config) {
    _commandId = config.commandTemplates.empty() ? String() : config.commandTemplates.front().id;
    _reportId = config.reportTemplates.empty() ? String() : config.reportTemplates.front().id;
}

void PercentageView::buildUi(lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    _arc = lv_arc_create(parent);
    lv_obj_set_size(_arc, kArcSize, kArcSize);
    lv_arc_set_range(_arc, 0, 100);
    lv_arc_set_value(_arc, 0);
    lv_obj_add_event_cb(_arc, arcReleasedCb, LV_EVENT_RELEASED, this);

    _valueLabel = lv_label_create(_arc);
    lv_obj_center(_valueLabel);
    updateValueLabel(0);
}

void PercentageView::updateValueLabel(int32_t value) {
    if (!_valueLabel) {
        return;
    }
    lv_label_set_text(_valueLabel, (String(value) + "%").c_str());
}

void PercentageView::onCommand(const Command& command) {
    int32_t value = command.value.as<int32_t>();
    if (value < 0) value = 0;
    if (value > 100) value = 100;
    if (_arc) {
        lv_arc_set_value(_arc, value);
    }
    updateValueLabel(value);
}

void PercentageView::arcReleasedCb(lv_event_t* event) {
    auto* self = static_cast<PercentageView*>(lv_event_get_user_data(event));
    int32_t value = lv_arc_get_value(self->_arc);
    self->updateValueLabel(value);
    if (self->_reportId.length() > 0) {
        self->publishReport(Report{self->_reportId, String(value)});
    }
}

namespace {
struct PercentageViewRegistrar {
    PercentageViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Core2.Node.PercentageView",
                                                 []() { return std::make_unique<PercentageView>(); });
    }
} percentageViewRegistrar;
}  // namespace
