#include "ValueView.h"

#include <memory>

#include <riot2/Uuid.h>

#include "ViewFactory.h"

namespace {
constexpr size_t kMaxSlots = 4;

DeviceConfiguration buildValueViewTemplate() {
    DeviceConfiguration config;
    config.id = riot2::newId();
    config.name = "Value View";
    config.classFullName = "RIoT2.Ard.M5Core2.Node.ValueView";
    config.deviceParameters = {{"unit1", "\u00b0C"}, {"unit2", "%"}};

    for (const char* label : {"Value 1", "Value 2"}) {
        CommandTemplate cmd;
        cmd.id = riot2::newId();
        cmd.type = "2";
        cmd.name = label;
        cmd.valueType = 2;
        config.commandTemplates.push_back(cmd);
    }
    return config;
}
}  // namespace

void ValueView::begin(const DeviceConfiguration& config) {
    _slots.clear();

    for (size_t i = 0; i < config.commandTemplates.size() && i < kMaxSlots; ++i) {
        const auto& cmd = config.commandTemplates[i];
        Slot slot;
        slot.id = cmd.id;
        slot.name = cmd.name;
        slot.unit = findParameter(config.deviceParameters, "unit" + String(i + 1));
        _slots.push_back(slot);
    }
}

void ValueView::buildUi(lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    if (_slots.empty()) {
        lv_obj_t* emptyLabel = lv_label_create(parent);
        lv_label_set_text(emptyLabel, "No values");
        return;
    }

    for (auto& slot : _slots) {
        lv_obj_t* row = lv_obj_create(parent);
        lv_obj_set_width(row, lv_pct(90));
        lv_obj_set_height(row, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t* nameLabel = lv_label_create(row);
        lv_label_set_text(nameLabel, slot.name.c_str());

        lv_obj_t* valueLabel = lv_label_create(row);
        lv_label_set_text(valueLabel, "--");
        lv_obj_set_style_text_font(valueLabel, &lv_font_montserrat_24, 0);

        slot.valueLabel = valueLabel;
    }
}

void ValueView::onCommand(const Command& command) {
    for (auto& slot : _slots) {
        if (slot.id == command.id) {
            String text = command.value.as<String>() + slot.unit;
            lv_label_set_text(slot.valueLabel, text.c_str());
            return;
        }
    }
}

namespace {
struct ValueViewRegistrar {
    ValueViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Core2.Node.ValueView",
                                                 []() { return std::make_unique<ValueView>(); }, buildValueViewTemplate);
    }
} valueViewRegistrar;
}  // namespace
