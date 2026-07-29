#include "ButtonView.h"

#include <memory>

#include "ViewFactory.h"

namespace {
constexpr uint32_t kFlashMs = 200;
constexpr lv_coord_t kButtonWidth = 120;
constexpr lv_coord_t kButtonHeight = 60;
const lv_color_t kActiveColor = lv_color_hex(0x2060C0);
}  // namespace

void ButtonView::begin(const DeviceConfiguration& config) {
    _slots.clear();
    _header = findParameter(config.deviceParameters, "header", config.name);
    _subHeader = findParameter(config.deviceParameters, "subHeader", "");

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
        if (_slots.size() >= 4) {
            break;
        }
    }
}

void ButtonView::buildUi(lv_obj_t* parent) {
    lv_obj_t* content = buildHeaderArea(parent, _header, _subHeader);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    if (_slots.empty()) {
        lv_obj_t* emptyLabel = lv_label_create(content);
        lv_label_set_text(emptyLabel, "No buttons");
        return;
    }

    // _slots is never resized again after begin(), so these element
    // addresses stay stable for the lifetime of the view - safe to hand out
    // as event/timer user_data below.
    lv_obj_t* grid = lv_obj_create(content);
    lv_obj_set_width(grid, lv_pct(100));
    lv_obj_set_height(grid, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(grid, 0, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (auto& slot : _slots) {
        lv_obj_t* button = lv_button_create(grid);
        lv_obj_set_size(button, kButtonWidth, kButtonHeight);
        lv_obj_set_style_bg_color(button, kActiveColor, LV_STATE_CHECKED);
        lv_obj_add_event_cb(button, buttonTappedCb, LV_EVENT_CLICKED, &slot);

        lv_obj_t* label = lv_label_create(button);
        lv_label_set_text(label, slot.report.name.c_str());
        lv_obj_center(label);

        slot.button = button;
    }
}


void ButtonView::onCommand(const Command& command) {
    for (auto& slot : _slots) {
        if (slot.hasCommand && slot.command.id == command.id) {
            bool value = command.value.is<bool>() ? command.value.as<bool>() : command.value.as<int>() != 0;
            slot.active = value;
            applyVisualState(slot);
            return;
        }
    }
}

void ButtonView::applyVisualState(Slot& slot) {
    if (slot.active) {
        lv_obj_add_state(slot.button, LV_STATE_CHECKED);
    } else {
        lv_obj_remove_state(slot.button, LV_STATE_CHECKED);
    }
}

void ButtonView::buttonTappedCb(lv_event_t* event) {
    auto* slot = static_cast<Slot*>(lv_event_get_user_data(event));
    slot->owner->publishReport(Report{slot->report.id, "true"});

    // Flash on regardless of `active` (a visible tap acknowledgement), then
    // settle back to whatever the last inbound command says it should be.
    lv_obj_add_state(slot->button, LV_STATE_CHECKED);
    lv_timer_t* timer = lv_timer_create(flashTimerCb, kFlashMs, slot);
    lv_timer_set_repeat_count(timer, 1);
}

void ButtonView::flashTimerCb(lv_timer_t* timer) {
    auto* slot = static_cast<Slot*>(lv_timer_get_user_data(timer));
    applyVisualState(*slot);
}

namespace {
struct ButtonViewRegistrar {
    ButtonViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Core2.Node.ButtonView",
                                                 []() { return std::make_unique<ButtonView>(); });
    }
} buttonViewRegistrar;
}  // namespace
