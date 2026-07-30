#include "PopupOverlay.h"

#include "AppColors.h"

namespace {
constexpr int32_t kWidth = 260;
constexpr int32_t kHeight = 160;
constexpr uint32_t kDismissAnimTag = 1;
}  // namespace

void PopupOverlay::begin() {
    _container = lv_obj_create(lv_layer_top());
    lv_obj_set_size(_container, kWidth, kHeight);
    lv_obj_center(_container);
    lv_obj_set_style_bg_color(_container, lv_color_hex(0x202020), 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(_container, AppColors::indigoAccent2(), 0);
    lv_obj_set_style_border_width(_container, 2, 0);
    lv_obj_set_style_radius(_container, 8, 0);

    _titleLabel = lv_label_create(_container);
    lv_obj_set_style_text_align(_titleLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(_titleLabel, LV_ALIGN_TOP_MID, 0, 10);

    _messageLabel = lv_label_create(_container);
    lv_label_set_long_mode(_messageLabel, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(_messageLabel, kWidth - 40);
    lv_obj_set_style_text_align(_messageLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(_messageLabel, LV_ALIGN_CENTER, 0, 10);

    _bar = lv_bar_create(_container);
    lv_bar_set_range(_bar, 0, 100);
    lv_obj_set_size(_bar, kWidth - 40, 10);
    lv_obj_align(_bar, LV_ALIGN_BOTTOM_MID, 0, -10);

    lv_obj_add_flag(_container, LV_OBJ_FLAG_HIDDEN);
}

void PopupOverlay::showCommon(const String& title, const String& message) {
    lv_anim_delete(_bar, barAnimExecCb);
    lv_obj_remove_event_cb(_container, tapToDismissCb);
    lv_obj_add_flag(_bar, LV_OBJ_FLAG_HIDDEN);

    lv_label_set_text(_titleLabel, title.c_str());
    lv_label_set_text(_messageLabel, message.c_str());
    lv_obj_remove_flag(_container, LV_OBJ_FLAG_HIDDEN);
}

void PopupOverlay::showAlert(const String& title, const String& message) {
    showCommon(title, message);
    lv_obj_add_event_cb(_container, tapToDismissCb, LV_EVENT_CLICKED, this);
}

void PopupOverlay::showNotification(const String& title, const String& message, uint32_t autoDismissMs) {
    showCommon(title, message);

    lv_obj_remove_flag(_bar, LV_OBJ_FLAG_HIDDEN);
    lv_bar_set_value(_bar, 100, LV_ANIM_OFF);

    lv_anim_init(&_dismissAnim);
    lv_anim_set_var(&_dismissAnim, _bar);
    lv_anim_set_exec_cb(&_dismissAnim, barAnimExecCb);
    lv_anim_set_values(&_dismissAnim, 100, 0);
    lv_anim_set_duration(&_dismissAnim, autoDismissMs);
    lv_anim_set_completed_cb(&_dismissAnim, barAnimCompletedCb);
    lv_anim_set_user_data(&_dismissAnim, this);
    lv_anim_start(&_dismissAnim);
}

bool PopupOverlay::dismiss() {
    if (!isVisible()) {
        return false;
    }
    lv_anim_delete(_bar, barAnimExecCb);
    lv_obj_remove_event_cb(_container, tapToDismissCb);
    lv_obj_add_flag(_container, LV_OBJ_FLAG_HIDDEN);
    return true;
}

bool PopupOverlay::isVisible() const {
    return !lv_obj_has_flag(_container, LV_OBJ_FLAG_HIDDEN);
}

void PopupOverlay::tapToDismissCb(lv_event_t* event) {
    auto* self = static_cast<PopupOverlay*>(lv_event_get_user_data(event));
    self->dismiss();
}

void PopupOverlay::barAnimExecCb(void* bar, int32_t value) {
    lv_bar_set_value(static_cast<lv_obj_t*>(bar), value, LV_ANIM_OFF);
}

void PopupOverlay::barAnimCompletedCb(lv_anim_t* anim) {
    auto* self = static_cast<PopupOverlay*>(lv_anim_get_user_data(anim));
    self->dismiss();
}
