#include "HapticFeedback.h"

#include <M5Unified.h>

HapticFeedback& HapticFeedback::instance() {
    static HapticFeedback instance;
    return instance;
}

void HapticFeedback::vibrate(unsigned long ms) {
    if (!_enabled || ms == 0) {
        return;
    }
    if (_offTimer) {
        // A pulse is already running - restart it rather than stacking
        // timers; M5.Power.setVibration() is idempotent to call again.
        lv_timer_delete(_offTimer);
        _offTimer = nullptr;
    }

    M5.Power.setVibration(kVibrationLevel);
    _offTimer = lv_timer_create(offTimerCb, ms, this);
    lv_timer_set_repeat_count(_offTimer, 1);
}

void HapticFeedback::offTimerCb(lv_timer_t* timer) {
    // repeat_count=1 means LVGL auto-deletes `timer` right after this
    // callback returns - don't lv_timer_delete() it here too.
    auto* self = static_cast<HapticFeedback*>(lv_timer_get_user_data(timer));
    M5.Power.setVibration(0);
    self->_offTimer = nullptr;
}
