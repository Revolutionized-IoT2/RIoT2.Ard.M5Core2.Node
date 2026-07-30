#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include "IView.h"

// Countdown egg-timer with three phases (mirrors M5Dial.Node's TimerView's
// state machine, minus the encoder): Setting (lv_spinbox minute picker +/-
// buttons + Start button) -> Running (read-only lv_bar counting down,
// ticked by an lv_timer every second, + Cancel button) -> Done ("Time's up!"
// + Dismiss button, optionally beeping a non-blocking "egg ring" pattern).
// Publishes Report{id, "0"} exactly once on completion.
//
// deviceParameters: "defaultMinutes" (default "5"), "stepMinutes" (default
// "1"), "maxMinutes" (default "60"), "beepOnComplete" (default "false").
class TimerView : public IView {
public:
    ~TimerView() override;

    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;
    void onCommand(const Command& command) override;

private:
    enum class Phase { Setting, Running, Done };

    String _name;
    String _commandId;
    String _reportId;
    int _stepMinutes = 1;
    int _maxMinutes = 60;
    int _minutes = 5;
    bool _beepOnComplete = false;

    Phase _phase = Phase::Setting;
    long _totalSeconds = 0;
    unsigned long _startMs = 0;
    int _ringsRemaining = 0;

    lv_obj_t* _settingContainer = nullptr;
    lv_obj_t* _runningContainer = nullptr;
    lv_obj_t* _doneContainer = nullptr;
    lv_obj_t* _spinbox = nullptr;
    lv_obj_t* _countdownBar = nullptr;
    lv_obj_t* _countdownLabel = nullptr;

    lv_timer_t* _tickTimer = nullptr;
    lv_timer_t* _ringTimer = nullptr;

    void showPhase(Phase phase);
    void start();
    void cancel();
    void finish();
    void updateCountdown();

    static void tickTimerCb(lv_timer_t* timer);
    static void ringTimerCb(lv_timer_t* timer);
    static void spinboxIncrementTappedCb(lv_event_t* event);
    static void spinboxDecrementTappedCb(lv_event_t* event);
    static void startButtonTappedCb(lv_event_t* event);
    static void cancelButtonTappedCb(lv_event_t* event);
    static void dismissButtonTappedCb(lv_event_t* event);
};
