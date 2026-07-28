#pragma once

#include <Arduino.h>

class PopupOverlay;

// Active -> Idle (dim + a live clock shown via PopupOverlay's existing
// showAlert() API, refreshed once a second) -> Asleep (M5.Display.sleep())
// -> wake on any touch/button. Plain compile-time constants below, not
// NodeConfig fields, despite plan.md Section 4's "idleTimeoutMs/
// sleepTimeoutMs NodeConfig field" phrasing - that turned out not to match
// how RIoT2.Ard.M5Dial.Node actually implements the equivalent dim/sleep
// behavior (plain constexpr constants in its main.cpp).
//
// Also deliberately doesn't rely on PopupOverlay's own tap-to-dismiss
// handler for wake detection: the popup only covers a small centered box,
// not the full screen, so a touch elsewhere on the tab underneath wouldn't
// reach it. Instead this class polls M5.Touch/M5.BtnA/B/C directly every
// loop() call - the same way main.cpp already does for its own
// factory-reset combo detection and NavigationController's button routing.
class ScreenPowerPolicy {
public:
    explicit ScreenPowerPolicy(PopupOverlay& popupOverlay) : _popupOverlay(popupOverlay) {}

    void begin();

    // Call once per loop() iteration, right after M5.update(). Returns true
    // if this call consumed the current touch/button input purely as a wake
    // gesture (the display was dimmed/idle/asleep) - callers should skip
    // their own normal input handling for this frame when true, so e.g. a
    // BtnA press that wakes the screen doesn't also change tabs underneath
    // the still-visible idle overlay.
    bool loop();

private:
    enum class State { Active, Idle, Asleep };

    static constexpr unsigned long kIdleTimeoutMs = 30000;
    static constexpr unsigned long kSleepTimeoutMs = 300000;
    static constexpr unsigned long kIdleClockRefreshMs = 1000;
    static constexpr uint8_t kDimBrightness = 15;
    static constexpr uint8_t kActiveBrightness = 255;

    PopupOverlay& _popupOverlay;
    State _state = State::Active;
    unsigned long _lastActivityMs = 0;
    unsigned long _lastIdleClockRefreshMs = 0;

    static bool hasInputActivity();
    void enterIdle();
    void enterAsleep();
    void wake();
    void refreshIdleClock();
};
