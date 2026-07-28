#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <riot2/Feedback.h>

// Core2-specific IFeedback implementation using the board's vibration
// motor (M5.Power.setVibration(), 0 = stop) - project-local, not shared,
// since M5Dial.Node has no motor (see riot2/Feedback.h's IFeedback::vibrate()
// no-op default, which M5Dial.Node relies on implicitly by simply never
// implementing this interface). Gated by NodeConfig::vibrateEnabled - see
// setEnabled(), called once from main.cpp right after the config loads.
//
// Each semantic method pulses the motor for a short, distinct duration
// (mirroring Buzzer's distinct tones) via a one-shot LVGL timer
// (lv_timer_set_repeat_count(..., 1), which auto-deletes the timer once it
// has fired - see lv_timer.c) that turns the motor back off without
// blocking the caller, the same non-blocking-timer approach TimerView uses
// for its egg-ring beep.
class HapticFeedback : public IFeedback {
public:
    static HapticFeedback& instance();

    void setEnabled(bool enabled) { _enabled = enabled; }

    void tap() override { vibrate(30); }
    void confirm() override { vibrate(60); }
    void error() override { vibrate(200); }
    void alert() override { vibrate(120); }
    void ring() override { vibrate(80); }
    void vibrate(unsigned long ms) override;

private:
    HapticFeedback() = default;

    static constexpr uint8_t kVibrationLevel = 200;

    bool _enabled = true;
    lv_timer_t* _offTimer = nullptr;

    static void offTimerCb(lv_timer_t* timer);
};
