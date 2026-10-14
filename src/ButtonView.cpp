#include "ButtonView.h"

#include <algorithm>
#include <memory>

#include "AppColors.h"
#include "ViewFactory.h"

namespace {
const lv_color_t kActiveColor = AppColors::indigo();
}  // namespace

ButtonView::~ButtonView() {
    cancelFlashTimers();
}

void ButtonView::cancelFlashTimers() {
    for (auto* timer : _flashTimers) {
        lv_timer_delete(timer);
    }
    _flashTimers.clear();
}

void ButtonView::begin(const DeviceConfiguration& config) {
    cancelFlashTimers();
    _model.begin(config);
    _slots.assign(_model.size(), Slot());
    for (size_t i = 0; i < _slots.size(); ++i) {
        _slots[i].owner = this;
        _slots[i].btnId = static_cast<uint32_t>(i);
    }
}

void ButtonView::buildUi(lv_obj_t* parent) {
    lv_obj_t* content = buildHeaderArea(parent, _model.header(), _model.subHeader());
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
    for (size_t i = 0; i < _model.size(); ++i) {
        _buttonLabels.push_back(_model.name(i));
    }

    _map.clear();
    for (auto& label : _buttonLabels) {
        _map.push_back(label.c_str());
    }
    _map.push_back("");

    _matrix = lv_buttonmatrix_create(content);
    lv_obj_set_width(_matrix, lv_pct(100));
    // Grow to fill whatever vertical space content (the header-less area
    // below the header block) has left, instead of a fixed height.
    lv_obj_set_flex_grow(_matrix, 1);
    lv_buttonmatrix_set_map(_matrix, _map.data());
    // CLICK_TRIG makes each button behave like a tap-to-fire lv_button
    // (event on release-inside), rather than firing as soon as pressed.
    lv_buttonmatrix_set_button_ctrl_all(_matrix, LV_BUTTONMATRIX_CTRL_CLICK_TRIG);
    lv_obj_set_style_bg_color(_matrix, kActiveColor, LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_add_event_cb(_matrix, matrixEventCb, LV_EVENT_VALUE_CHANGED, this);

    // Needs real (pct-resolved) pixel sizes to know each button's share of
    // the matrix width - lv_obj_update_layout() forces that resolution now
    // rather than waiting for the next lv_timer_handler() pass.
    lv_obj_update_layout(_matrix);
    wrapLongButtonLabels();
}

void ButtonView::onCommand(const Command& command) {
    int index = _model.onCommand(command);
    if (index >= 0) {
        applyVisualState(static_cast<uint32_t>(index), _model.isActive(static_cast<size_t>(index)));
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

void ButtonView::wrapLongButtonLabels() {
    if (_matrix == nullptr || _slots.empty()) {
        return;
    }
    // Rough per-button share of the matrix's content width - ignores the
    // small inter-button gap, which is fine for a "does this overflow"
    // estimate.
    int32_t buttonWidth = lv_obj_get_content_width(_matrix) / static_cast<int32_t>(_slots.size());
    const lv_font_t* font = lv_obj_get_style_text_font(_matrix, LV_PART_ITEMS);
    constexpr int32_t kHorizontalPad = 8;  // items' left+right text padding, approximated

    for (auto& label : _buttonLabels) {
        lv_point_t size;
        lv_text_get_size(&size, label.c_str(), font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
        if (size.x <= buttonWidth - kHorizontalPad) {
            continue;
        }

        // Break at the space closest to the middle of the label - this
        // rewrites a single character in place (no length change), so the
        // char* already cached in _map by lv_buttonmatrix_set_map() stays
        // valid; a literal '\n' here is just a line break within one
        // button's text, unlike a "\n" map entry (which starts a new row).
        int mid = static_cast<int>(label.length()) / 2;
        int breakAt = -1;
        for (int offset = 0; offset <= mid; ++offset) {
            if (mid - offset >= 0 && label[mid - offset] == ' ') {
                breakAt = mid - offset;
                break;
            }
            if (mid + offset < static_cast<int>(label.length()) && label[mid + offset] == ' ') {
                breakAt = mid + offset;
                break;
            }
        }
        if (breakAt > 0) {
            label.setCharAt(breakAt, '\n');
        }
    }
}

void ButtonView::matrixEventCb(lv_event_t* event) {
    auto* self = static_cast<ButtonView*>(lv_event_get_user_data(event));
    uint32_t btnId = lv_buttonmatrix_get_selected_button(self->_matrix);
    if (btnId == LV_BUTTONMATRIX_BUTTON_NONE || btnId >= self->_slots.size()) {
        return;
    }

    Report report;
    if (!self->_model.press(btnId, report)) {
        return;
    }
    self->publishReport(report);

    // Flash on regardless of `active` (a visible tap acknowledgement), then
    // settle back to whatever the last inbound command says it should be.
    self->applyVisualState(btnId, true);
    lv_timer_t* timer = lv_timer_create(flashTimerCb, riot2::views::ButtonModel::kFlashMs, &self->_slots[btnId]);
    self->_flashTimers.push_back(timer);
    lv_timer_set_repeat_count(timer, 1);
}

void ButtonView::flashTimerCb(lv_timer_t* timer) {
    auto* slot = static_cast<Slot*>(lv_timer_get_user_data(timer));
    auto& timers = slot->owner->_flashTimers;
    timers.erase(std::remove(timers.begin(), timers.end(), timer), timers.end());
    slot->owner->applyVisualState(slot->btnId, slot->owner->_model.isActive(slot->btnId));
}

namespace {
struct ButtonViewRegistrar {
    ButtonViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Core2.Node.ButtonView", []() { return std::make_unique<ButtonView>(); },
            []() { return riot2::views::ButtonModel::buildTemplate("RIoT2.Ard.M5Core2.Node.ButtonView"); });
    }
} buttonViewRegistrar;
}  // namespace