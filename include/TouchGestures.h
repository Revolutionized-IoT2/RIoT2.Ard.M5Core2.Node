#pragma once

#include <Arduino.h>

// Gestures M5Unified doesn't tell apart on the Core2's touch panel:
//
// - Scrolls ending on the virtual buttons. M5Unified's virtual BtnA/B/C touch
//   zones only look at each frame's instantaneous touch position and motion
//   (M5Unified.cpp's update(): a touch counts toward a button if it's inside
//   the bottom strip and not moving that frame), never at the gesture's
//   history. A vertical scroll or a horizontal tab swipe that ends with the
//   finger lifting inside that strip therefore still fires a button click
//   (most disruptively BtnB, which opens the main menu): touch controllers
//   commonly report one or two near-zero-delta samples right before liftoff.
//   wasScroll() tracks the touch's total displacement from its touch-down
//   point (touch_detail_t keeps it as base_x/base_y) - a real tap never moves
//   far - and is true for the rest of a touch that moved further than
//   kScrollDistancePx, so the board suppresses button clicks then.
// - Factory reset. There's no single physical "boot button" on Core2 (BtnA/B/C
//   are touch zones), so it takes holding BtnA+BtnC together for
//   kFactoryResetHoldMs, which normal navigation never does.
class TouchGestures {
public:
    static constexpr int kScrollDistancePx = 12;
    static constexpr unsigned long kFactoryResetHoldMs = 5000;

    // Once per loop(), after M5.update().
    void update();

    bool wasScroll() const { return _scrolled; }

    // True once per BtnA+BtnC hold that reaches kFactoryResetHoldMs.
    bool factoryResetRequested();

private:
    int _downX = 0;
    int _downY = 0;
    bool _scrolled = false;
    bool _comboActive = false;
    bool _resetFired = false;
    unsigned long _comboStartMs = 0;
};
