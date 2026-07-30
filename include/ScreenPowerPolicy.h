#pragma once

#include <Arduino.h>

class PopupOverlay;
class MatrixRainView;

// Active -> Idle (dim + a full-screen MatrixRainView animation) -> Asleep
// (M5.Display.sleep()) -> wake on any touch/button. Plain compile-time
// constants below, not NodeConfig fields, despite plan.md Section 4's
// "idleTimeoutMs/sleepTimeoutMs NodeConfig field" phrasing - that turned
// out not to match how RIoT2.Ard.M5Dial.Node actually implements the
// equivalent dim/sleep behavior (plain constexpr constants in its
// main.cpp).
//
// Also deliberately doesn't rely on MatrixRainView/PopupOverlay's own
// tap-to-dismiss handling for wake detection: this class polls
// M5.Touch/M5.BtnA/B/C directly every loop() call instead - the same way
// main.cpp already does for its own factory-reset combo detection and
// NavigationController's button routing.
class ScreenPowerPolicy {
public:
    ScreenPowerPolicy(PopupOverlay& popupOverlay, MatrixRainView& matrixRainView)
        : _popupOverlay(popupOverlay), _matrixRainView(matrixRainView) {}

    void begin();

    // Call once per loop() iteration, right after M5.update(). Returns true
    // if this call consumed the current touch/button input purely as a wake
    // gesture (the display was dimmed/idle/asleep) - callers should skip
    // their own normal input handling for this frame when true, so e.g. a
    // BtnA press that wakes the screen doesn't also change tabs underneath
    // the still-visible idle overlay.
    bool loop();

    // Forces the display fully on (waking it up if asleep, undimming it if
    // idle) and hides the MatrixRainView idle overlay, without touching
    // PopupOverlay - call this right before showing an incoming
    // alert/notification popup (see main.cpp's viewManager.onPopup handler)
    // so it's never rendered underneath the idle animation or while the
    // panel itself is powered off. Unlike wake(), deliberately does NOT
    // dismiss whatever PopupOverlay is about to show/is showing, and resets
    // the inactivity timer so the freshly-woken screen doesn't immediately
    // re-idle out from under the still-visible popup.
    void wakeForAlert();

private:
    enum class State { Active, Idle, Asleep };

    static constexpr unsigned long kIdleTimeoutMs = 60000;
    static constexpr unsigned long kSleepTimeoutMs = 240000;
    static constexpr uint8_t kDimBrightness = 15;
    static constexpr uint8_t kActiveBrightness = 255;

    PopupOverlay& _popupOverlay;
    MatrixRainView& _matrixRainView;
    State _state = State::Active;
    unsigned long _lastActivityMs = 0;

    static bool hasInputActivity();
    void enterIdle();
    void enterAsleep();
    void wake();
};
