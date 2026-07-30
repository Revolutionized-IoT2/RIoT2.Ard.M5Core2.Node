#include "ScreenPowerPolicy.h"

#include <M5Unified.h>

#include "MatrixRainView.h"
#include "PopupOverlay.h"

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
            }
            break;
        case State::Asleep:
            break;
    }
    return false;
}

void ScreenPowerPolicy::enterIdle() {
    Serial.println("[ScreenPower] Active -> Idle");
    _state = State::Idle;
    M5.Display.setBrightness(kDimBrightness);
    _matrixRainView.start();
}

void ScreenPowerPolicy::enterAsleep() {
    Serial.println("[ScreenPower] Idle -> Asleep");
    _state = State::Asleep;
    _matrixRainView.stop();
    _popupOverlay.dismiss();
    M5.Display.sleep();
}

void ScreenPowerPolicy::wake() {
    Serial.println("[ScreenPower] -> Active (wake)");
    if (_state == State::Asleep) {
        M5.Display.wakeup();
    }
    _matrixRainView.stop();
    _popupOverlay.dismiss();
    M5.Display.setBrightness(kActiveBrightness);
    _state = State::Active;
}

void ScreenPowerPolicy::wakeForAlert() {
    if (_state == State::Asleep) {
        Serial.println("[ScreenPower] -> Active (alert)");
        M5.Display.wakeup();
    }
    _matrixRainView.stop();
    M5.Display.setBrightness(kActiveBrightness);
    _state = State::Active;
    _lastActivityMs = millis();
}
