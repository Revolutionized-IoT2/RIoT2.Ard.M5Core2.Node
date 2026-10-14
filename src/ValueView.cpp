#include "ValueView.h"

#include <memory>

#include "ViewFactory.h"

void ValueView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
    _valueLabels.assign(_model.size(), nullptr);
}

void ValueView::buildUi(lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    if (_model.size() == 0) {
        lv_obj_t* emptyLabel = lv_label_create(parent);
        lv_label_set_text(emptyLabel, "No values");
        return;
    }

    for (size_t i = 0; i < _model.size(); ++i) {
        lv_obj_t* row = lv_obj_create(parent);
        lv_obj_set_width(row, lv_pct(90));
        lv_obj_set_height(row, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t* nameLabel = lv_label_create(row);
        lv_label_set_text(nameLabel, _model.name(i).c_str());

        lv_obj_t* valueLabel = lv_label_create(row);
        // "--" until the first command, without a unit, as before.
        lv_label_set_text(valueLabel, _model.value(i).c_str());
        lv_obj_set_style_text_font(valueLabel, &lv_font_montserrat_24, 0);
        _valueLabels[i] = valueLabel;
    }
}

void ValueView::onCommand(const Command& command) {
    int index = _model.onCommand(command);
    if (index >= 0 && _valueLabels[index]) {
        lv_label_set_text(_valueLabels[index], _model.text(index).c_str());
    }
}

namespace {
struct ValueViewRegistrar {
    ValueViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Core2.Node.ValueView", []() { return std::make_unique<ValueView>(); },
            []() { return riot2::views::ValueModel::buildTemplate("RIoT2.Ard.M5Core2.Node.ValueView"); });
    }
} valueViewRegistrar;
}  // namespace
