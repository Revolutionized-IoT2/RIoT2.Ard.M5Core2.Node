#include "ButtonView.h"

#include <memory>

#include "ViewFactory.h"

namespace {
constexpr uint32_t kFlashMs = 200;
constexpr lv_coord_t kMatrixHeight = 70;
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

    // The label strings and the map array must outlive the button matrix -
    // it keeps a reference to them, not a copy (see lv_buttonmatrix_set_map()) -
    // so they're stored as members. _slots is never resized again after
    // begin(), so Slot addresses stay stable for the lifetime of the view -
    // safe to hand out as timer user_data below.
    _buttonLabels.clear();
    for (auto& slot : _slots) {
        _buttonLabels.push_back(slot.report.name);
    }

    _map.clear();
    for (auto& label : _buttonLabels) {
        _map.push_back(label.c_str());
    }
    _map.push_back("");

    _matrix = lv_buttonmatrix_create(content);
    lv_obj_set_width(_matrix, lv_pct(100));
    lv_obj_set_height(_matrix, kMatrixHeight);
    lv_buttonmatrix_set_map(_matrix, _map.data());
    // CLICK_TRIG makes each button behave like a tap-to-fire lv_button
    // (event on release-inside), rather than firing as soon as pressed.
    lv_buttonmatrix_set_button_ctrl_all(_matrix, LV_BUTTONMATRIX_CTRL_CLICK_TRIG);
    lv_obj_set_style_bg_color(_matrix, kActiveColor, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_add_event_cb(_matrix, matrixEventCb, LV_EVENT_VALUE_CHANGED, this);

    for (size_t i = 0; i < _slots.size(); ++i) {
        _slots[i].btnId = static_cast<uint32_t>(i);
    }
}

void ButtonView::onCommand(const Command& command) {
    for (auto& slot : _slots) {
        if (slot.hasCommand && slot.command.id == command.id) {
            bool value = command.value.is<bool>() ? command.value.as<bool>() : command.value.as<int>() != 0;
            slot.active = value;
            applyVisualState(slot.btnId, slot.active);
            return;
        }
    }
}

void ButtonView::applyVisualState(uint32_t btnId, bool active) {
    if (!_matrix) {
        return;
    }
    if (active) {
        lv_buttonmatrix_set_button_ctrl(_matrix, btnId, LV_BUTTONMATRIX_CTRL_CHECKED);
    } else {
        lv_buttonmatrix_clear_button_ctrl(_matrix, btnId, LV_BUTTONMATRIX_CTRL_CHECKED);
    }
}

void ButtonView::matrixEventCb(lv_event_t* event) {
    auto* self = static_cast<ButtonView*>(lv_event_get_user_data(event));
    uint32_t btnId = lv_buttonmatrix_get_selected_button(self->_matrix);
    if (btnId == LV_BUTTONMATRIX_BUTTON_NONE || btnId >= self->_slots.size()) {
        return;
    }

    Slot* slot = &self->_slots[btnId];
    slot->owner->publishReport(Report{slot->report.id, "true"});

    // Flash on regardless of `active` (a visible tap acknowledgement), then
    // settle back to whatever the last inbound command says it should be.
    self->applyVisualState(btnId, true);
    lv_timer_t* timer = lv_timer_create(flashTimerCb, kFlashMs, slot);
    lv_timer_set_repeat_count(timer, 1);
}

void ButtonView::flashTimerCb(lv_timer_t* timer) {
    auto* slot = static_cast<Slot*>(lv_timer_get_user_data(timer));
    slot->owner->applyVisualState(slot->btnId, slot->active);
}

namespace {
struct ButtonViewRegistrar {
    ButtonViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Core2.Node.ButtonView",
                                                 []() { return std::make_unique<ButtonView>(); });
    }
} buttonViewRegistrar;
}  // namespace
