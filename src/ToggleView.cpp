#include "ToggleView.h"

#include <memory>

#include "ViewFactory.h"

void ToggleView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
    _rows.assign(_model.size(), Row());
    for (size_t i = 0; i < _rows.size(); ++i) {
        _rows[i].owner = this;
        _rows[i].index = i;
    }
}

void ToggleView::buildUi(lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    if (_rows.empty()) {
        lv_obj_t* emptyLabel = lv_label_create(parent);
        lv_label_set_text(emptyLabel, "No switches");
        return;
    }

    // _rows is never resized again after begin(), so these element
    // addresses stay stable for the lifetime of the view - safe to hand out
    // as event callback user_data below.
    for (auto& row : _rows) {
        lv_obj_t* container = lv_obj_create(parent);
        lv_obj_set_width(container, lv_pct(90));
        lv_obj_set_height(container, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(container, 0, 0);
        lv_obj_set_flex_flow(container, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(container, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t* label = lv_label_create(container);
        lv_label_set_text(label, _model.name(row.index).c_str());

        row.switchObj = lv_switch_create(container);
        lv_obj_add_event_cb(row.switchObj, switchToggledCb, LV_EVENT_VALUE_CHANGED, &row);
        showState(row.index);
    }
}

void ToggleView::showState(size_t index) {
    lv_obj_t* switchObj = _rows[index].switchObj;
    if (!switchObj) {
        return;
    }
    if (_model.isActive(index)) {
        lv_obj_add_state(switchObj, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(switchObj, LV_STATE_CHECKED);
    }
}

void ToggleView::onCommand(const Command& command) {
    int index = _model.onCommand(command);
    if (index >= 0) {
        showState(static_cast<size_t>(index));
    }
}

void ToggleView::switchToggledCb(lv_event_t* event) {
    auto* row = static_cast<Row*>(lv_event_get_user_data(event));
    Report report;
    if (row->owner->_model.set(row->index, lv_obj_has_state(row->switchObj, LV_STATE_CHECKED), report)) {
        row->owner->publishReport(report);
    }
}

namespace {
struct ToggleViewRegistrar {
    ToggleViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Core2.Node.ToggleView", []() { return std::make_unique<ToggleView>(); },
            []() { return riot2::views::ToggleModel::buildTemplate("RIoT2.Ard.M5Core2.Node.ToggleView", 2); });
    }
} toggleViewRegistrar;
}  // namespace
