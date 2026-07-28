#include "ToggleView.h"

#include <memory>

#include "ViewFactory.h"

void ToggleView::begin(const DeviceConfiguration& config) {
    _slots.clear();

    for (const auto& report : config.reportTemplates) {
        Slot slot;
        slot.report = report;
        slot.owner = this;

        for (const auto& cmd : config.commandTemplates) {
            if (report.address.length() > 0 && cmd.address == report.address) {
                slot.command = cmd;
                slot.hasCommand = true;
                break;
            }
        }

        _slots.push_back(slot);
        if (_slots.size() >= 2) {
            break;
        }
    }
}

void ToggleView::buildUi(lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    if (_slots.empty()) {
        lv_obj_t* emptyLabel = lv_label_create(parent);
        lv_label_set_text(emptyLabel, "No switches");
        return;
    }

    // _slots is never resized again after begin(), so these element
    // addresses stay stable for the lifetime of the view - safe to hand out
    // as event callback user_data below.
    for (auto& slot : _slots) {
        lv_obj_t* row = lv_obj_create(parent);
        lv_obj_set_width(row, lv_pct(90));
        lv_obj_set_height(row, LV_SIZE_CONTENT);
        lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(row, 0, 0);
        lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

        lv_obj_t* label = lv_label_create(row);
        lv_label_set_text(label, slot.report.name.c_str());

        lv_obj_t* switchObj = lv_switch_create(row);
        lv_obj_add_event_cb(switchObj, switchToggledCb, LV_EVENT_VALUE_CHANGED, &slot);

        slot.switchObj = switchObj;
    }
}

void ToggleView::onCommand(const Command& command) {
    for (auto& slot : _slots) {
        if (slot.hasCommand && slot.command.id == command.id) {
            bool value = command.value.is<bool>() ? command.value.as<bool>() : command.value.as<int>() != 0;
            if (value) {
                lv_obj_add_state(slot.switchObj, LV_STATE_CHECKED);
            } else {
                lv_obj_remove_state(slot.switchObj, LV_STATE_CHECKED);
            }
            return;
        }
    }
}

void ToggleView::switchToggledCb(lv_event_t* event) {
    auto* slot = static_cast<Slot*>(lv_event_get_user_data(event));
    bool active = lv_obj_has_state(slot->switchObj, LV_STATE_CHECKED);
    slot->owner->publishReport(Report{slot->report.id, active ? "true" : "false"});
}

namespace {
struct ToggleViewRegistrar {
    ToggleViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Core2.Node.ToggleView",
                                                 []() { return std::make_unique<ToggleView>(); });
    }
} toggleViewRegistrar;
}  // namespace
