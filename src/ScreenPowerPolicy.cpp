#include "ScreenPowerPolicy.h"

#include <M5Unified.h>
#include <time.h>

#include "PopupOverlay.h"

namespace {
// time(nullptr) returns small values (seconds since boot-ish) before SNTP
// has synced; treat anything before 2020-01-01 as "not yet synced" - same
// threshold ClockView uses.
constexpr time_t kMinValidEpoch = 1577836800;
}  // namespace

void ScreenPowerPolicy::begin() {
    _state = State::Active;
    _lastActivityMs = millis();
    M5.Display.setBrightness(kActiveBrightness);
}

bool ScreenPowerPolicy::hasInputActivity() {
    return M5.Touch.getCount() > 0 || M5.BtnA.isPressed() || M5.BtnB.isPressed() || M5.BtnC.isPressed();
}

bool ScreenPowerPolicy::loop() {
    unsigned long now = millis();

    if (hasInputActivity()) {
        bool wasNotActive = (_state != State::Active);
        _lastActivityMs = now;
        if (wasNotActive) {
            wake();
            return true;
        }
        return false;
    }

    switch (_state) {
        case State::Active:
            if (now - _lastActivityMs >= kIdleTimeoutMs) {
                enterIdle();
            }
            break;
        case State::Idle:
            if (now - _lastActivityMs >= kSleepTimeoutMs) {
                enterAsleep();
            } else if (now - _lastIdleClockRefreshMs >= kIdleClockRefreshMs) {
                refreshIdleClock();
            }
            break;
        case State::Asleep:
            break;
    }
    return false;
}

void ScreenPowerPolicy::enterIdle() {
    _state = State::Idle;
    M5.Display.setBrightness(kDimBrightness);
    _lastIdleClockRefreshMs = 0;  // force an immediate refresh below
    refreshIdleClock();
}

void ScreenPowerPolicy::enterAsleep() {
    _state = State::Asleep;
    _popupOverlay.dismiss();
    M5.Display.sleep();
}

void ScreenPowerPolicy::wake() {
    if (_state == State::Asleep) {
        M5.Display.wakeup();
    }
    _popupOverlay.dismiss();
    M5.Display.setBrightness(kActiveBrightness);
    _state = State::Active;
}

void ScreenPowerPolicy::refreshIdleClock() {
    _lastIdleClockRefreshMs = millis();

    time_t now = time(nullptr);
    struct tm timeInfo;
    char buf[9] = "--:--:--";
    if (now >= kMinValidEpoch && localtime_r(&now, &timeInfo) != nullptr) {
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
    }
    _popupOverlay.showAlert(buf, "Tap or press a button to wake");
}
