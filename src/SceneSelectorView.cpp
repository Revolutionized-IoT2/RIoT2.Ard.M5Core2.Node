#include "SceneSelectorView.h"

#include <memory>

#include "ViewFactory.h"

namespace {
constexpr uint8_t kVisibleRowCount = 4;
}  // namespace

void SceneSelectorView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
}

void SceneSelectorView::buildUi(lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    if (_model.empty()) {
        lv_obj_t* emptyLabel = lv_label_create(parent);
        lv_label_set_text(emptyLabel, "No scenes");
        return;
    }

    // lv_roller_set_options() copies into its internal label (see
    // lv_roller.c), so this local `options` string doesn't need to outlive
    // the call.
    String options;
    for (size_t i = 0; i < _model.size(); ++i) {
        if (i > 0) {
            options += "\n";
        }
        options += _model.name(i);
    }

    _roller = lv_roller_create(parent);
    lv_roller_set_options(_roller, options.c_str(), LV_ROLLER_MODE_NORMAL);
    lv_roller_set_visible_row_count(_roller, kVisibleRowCount);
    lv_obj_add_event_cb(_roller, rollerValueChangedCb, LV_EVENT_VALUE_CHANGED, this);
}

void SceneSelectorView::rollerValueChangedCb(lv_event_t* event) {
    auto* self = static_cast<SceneSelectorView*>(lv_event_get_user_data(event));
    Report report;
    if (self->_model.select(lv_roller_get_selected(self->_roller), report)) {
        self->publishReport(report);
    }
}

namespace {
struct SceneSelectorViewRegistrar {
    SceneSelectorViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Core2.Node.SceneSelectorView", []() { return std::make_unique<SceneSelectorView>(); },
            []() { return riot2::views::SceneSelectorModel::buildTemplate("RIoT2.Ard.M5Core2.Node.SceneSelectorView"); });
    }
} sceneSelectorViewRegistrar;
}  // namespace
