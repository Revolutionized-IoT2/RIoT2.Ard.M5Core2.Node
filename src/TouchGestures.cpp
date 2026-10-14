#include "TouchGestures.h"

#include <M5Unified.h>

void TouchGestures::update() {
    if (M5.Touch.getCount() == 0) {
        return;
    }
    auto detail = M5.Touch.getDetail(0);
    if (detail.wasPressed()) {
        _downX = detail.x;
        _downY = detail.y;
        _scrolled = false;
    } else if (detail.isPressed() &&
               (abs(detail.x - _downX) > kScrollDistancePx || abs(detail.y - _downY) > kScrollDistancePx)) {
        _scrolled = true;
    }
}

bool TouchGestures::factoryResetRequested() {
    bool comboPressed = M5.BtnA.isPressed() && M5.BtnC.isPressed();
    if (!comboPressed) {
        _comboActive = false;
        _resetFired = false;
        return false;
    }
    if (!_comboActive) {
        _comboActive = true;
        _comboStartMs = millis();
    }
    if (!_resetFired && millis() - _comboStartMs >= kFactoryResetHoldMs) {
        _resetFired = true;
        return true;
    }
    return false;
}
