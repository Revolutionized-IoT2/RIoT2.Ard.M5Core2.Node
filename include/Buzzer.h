#pragma once

#include <M5Unified.h>

// Simple audible feedback via the Core2's onboard speaker for confirm/error
// actions across views and the app shell. Non-blocking (M5Unified's Speaker
// mixes tones on a background task), safe to call from render/input handlers.
namespace Buzzer {

// M5Unified's default speaker master volume is only 64/255 - quite quiet on
// the Core2's built-in speaker. Call once from setup() to turn it up to max;
// all tone()s below share this master volume.
inline void begin() {
    M5.Speaker.setVolume(255);
}

// A light click, for momentary/toggle actions (ButtonView, ToggleView).
inline void tap() {
    M5.Speaker.tone(1200, 40);
}

// A short rising tone, for a confirmed value/selection (Percentage/Slider/
// ColorScheme/SceneSelectorView "confirm" press, diagnostics toggle, OTA start).
inline void confirm() {
    M5.Speaker.tone(1800, 80);
}

// A low buzz, for destructive/failure actions (factory reset, OTA failure).
inline void error() {
    M5.Speaker.tone(300, 250);
}

// A brief two-tone "alarm" chord for AlertView's incoming-alert sound.
inline void alert() {
    M5.Speaker.tone(2200, 200, 0, true);
    M5.Speaker.tone(2800, 200, 1, true);
}

// A short bright "bell" tone, for a longer attention-grabbing pattern (e.g.
// TimerView's optional "egg timer ring" on completion) called repeatedly
// from the caller's own non-blocking timer/state machine.
inline void ring() {
    M5.Speaker.tone(2600, 120);
}

}  // namespace Buzzer
