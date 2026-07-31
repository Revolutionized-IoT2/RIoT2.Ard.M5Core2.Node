#include "TimerView.h"

#include <memory>

#include <riot2/Uuid.h>

#include "Buzzer.h"
#include "HapticFeedback.h"
#include "ViewFactory.h"

namespace {
constexpr uint32_t kTickMs = 1000;
constexpr int kRingCount = 6;                  // number of beeps in the "egg timer ring" pattern
constexpr uint32_t kRingIntervalMs = 260;       // spacing between those beeps
constexpr lv_coord_t kBarWidth = 220;
constexpr lv_coord_t kBarHeight = 24;
constexpr lv_coord_t kSpinboxButtonSize = 40;
}  // namespace

TimerView::~TimerView() {
    if (_tickTimer) {
        lv_timer_delete(_tickTimer);
    }
    if (_ringTimer) {
        lv_timer_delete(_ringTimer);
    }
}

void TimerView::begin(const DeviceConfiguration& config) {
    _name = config.name;
    _commandId = config.commandTemplates.empty() ? String() : config.commandTemplates.front().id;
    _reportId = config.reportTemplates.empty() ? String() : config.reportTemplates.front().id;

    _stepMinutes = findParameter(config.deviceParameters, "stepMinutes", "1").toInt();
    if (_stepMinutes <= 0) {
        _stepMinutes = 1;
    }
    _maxMinutes = findParameter(config.deviceParameters, "maxMinutes", "60").toInt();
    if (_maxMinutes < _stepMinutes) {
        _maxMinutes = _stepMinutes;
    }
    _minutes = findParameter(config.deviceParameters, "defaultMinutes", "5").toInt();
    if (_minutes < _stepMinutes) {
        _minutes = _stepMinutes;
    }
    if (_minutes > _maxMinutes) {
        _minutes = _maxMinutes;
    }
    _beepOnComplete = findParameter(config.deviceParameters, "beepOnComplete", "false").equalsIgnoreCase("true");

    _phase = Phase::Setting;
    _totalSeconds = 0;
}

void TimerView::buildUi(lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // --- Setting phase ---
    _settingContainer = lv_obj_create(parent);
    lv_obj_set_size(_settingContainer, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(_settingContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_settingContainer, 0, 0);
    lv_obj_set_flex_flow(_settingContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_settingContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* spinboxRow = lv_obj_create(_settingContainer);
    lv_obj_set_size(spinboxRow, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(spinboxRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spinboxRow, 0, 0);
    lv_obj_set_flex_flow(spinboxRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(spinboxRow, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* decrementButton = lv_button_create(spinboxRow);
    lv_obj_set_size(decrementButton, kSpinboxButtonSize, kSpinboxButtonSize);
    lv_obj_set_style_bg_image_src(decrementButton, LV_SYMBOL_MINUS, 0);
    lv_obj_add_event_cb(decrementButton, spinboxDecrementTappedCb, LV_EVENT_CLICKED, this);

    _spinbox = lv_spinbox_create(spinboxRow);
    lv_spinbox_set_range(_spinbox, _stepMinutes, _maxMinutes);
    lv_spinbox_set_step(_spinbox, _stepMinutes);
    lv_spinbox_set_digit_format(_spinbox, String(_maxMinutes).length(), 0);
    lv_spinbox_set_value(_spinbox, _minutes);
    lv_obj_set_width(_spinbox, 90);

    lv_obj_t* incrementButton = lv_button_create(spinboxRow);
    lv_obj_set_size(incrementButton, kSpinboxButtonSize, kSpinboxButtonSize);
    lv_obj_set_style_bg_image_src(incrementButton, LV_SYMBOL_PLUS, 0);
    lv_obj_add_event_cb(incrementButton, spinboxIncrementTappedCb, LV_EVENT_CLICKED, this);

    lv_obj_t* startButton = lv_button_create(_settingContainer);
    lv_obj_t* startLabel = lv_label_create(startButton);
    lv_label_set_text(startLabel, "Start");
    lv_obj_center(startLabel);
    lv_obj_add_event_cb(startButton, startButtonTappedCb, LV_EVENT_CLICKED, this);

    // --- Running phase ---
    _runningContainer = lv_obj_create(parent);
    lv_obj_set_size(_runningContainer, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(_runningContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_runningContainer, 0, 0);
    lv_obj_set_flex_flow(_runningContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_runningContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    _countdownBar = lv_bar_create(_runningContainer);
    lv_obj_set_size(_countdownBar, kBarWidth, kBarHeight);
    lv_obj_remove_flag(_countdownBar, LV_OBJ_FLAG_CLICKABLE);
    _countdownLabel = lv_label_create(_runningContainer);

    lv_obj_t* cancelButton = lv_button_create(_runningContainer);
    lv_obj_t* cancelLabel = lv_label_create(cancelButton);
    lv_label_set_text(cancelLabel, "Cancel");
    lv_obj_center(cancelLabel);
    lv_obj_add_event_cb(cancelButton, cancelButtonTappedCb, LV_EVENT_CLICKED, this);

    // --- Done phase ---
    _doneContainer = lv_obj_create(parent);
    lv_obj_set_size(_doneContainer, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(_doneContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(_doneContainer, 0, 0);
    lv_obj_set_flex_flow(_doneContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_doneContainer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t* doneLabel = lv_label_create(_doneContainer);
    lv_label_set_text(doneLabel, "Time's up!");

    lv_obj_t* dismissButton = lv_button_create(_doneContainer);
    lv_obj_t* dismissLabel = lv_label_create(dismissButton);
    lv_label_set_text(dismissLabel, "Dismiss");
    lv_obj_center(dismissLabel);
    lv_obj_add_event_cb(dismissButton, dismissButtonTappedCb, LV_EVENT_CLICKED, this);

    showPhase(Phase::Setting);
}

void TimerView::showPhase(Phase phase) {
    _phase = phase;
    if (_settingContainer) {
        (phase == Phase::Setting ? lv_obj_remove_flag : lv_obj_add_flag)(_settingContainer, LV_OBJ_FLAG_HIDDEN);
    }
    if (_runningContainer) {
        (phase == Phase::Running ? lv_obj_remove_flag : lv_obj_add_flag)(_runningContainer, LV_OBJ_FLAG_HIDDEN);
    }
    if (_doneContainer) {
        (phase == Phase::Done ? lv_obj_remove_flag : lv_obj_add_flag)(_doneContainer, LV_OBJ_FLAG_HIDDEN);
    }
}

void TimerView::onCommand(const Command& command) {
    if (_phase != Phase::Setting || _commandId.length() == 0 || command.id != _commandId) {
        return;
    }
    int minutes = command.value.as<int>();
    if (minutes < _stepMinutes) {
        minutes = _stepMinutes;
    }
    if (minutes > _maxMinutes) {
        minutes = _maxMinutes;
    }
    _minutes = minutes;

    if (_spinbox) {
        lv_spinbox_set_value(_spinbox, _minutes);
    }
}

void TimerView::start() {
    _minutes = lv_spinbox_get_value(_spinbox);

    _totalSeconds = _minutes * 60;
    _startMs = millis();
    _ringsRemaining = 0;
    lv_bar_set_range(_countdownBar, 0, _totalSeconds);
    lv_bar_set_value(_countdownBar, _totalSeconds, LV_ANIM_OFF);
    updateCountdown();

    showPhase(Phase::Running);
    Buzzer::confirm();
    HapticFeedback::instance().confirm();

    if (!_tickTimer) {
        _tickTimer = lv_timer_create(tickTimerCb, kTickMs, this);
    }
}

void TimerView::cancel() {
    if (_tickTimer) {
        lv_timer_delete(_tickTimer);
        _tickTimer = nullptr;
    }
    showPhase(Phase::Setting);
    Buzzer::tap();
    HapticFeedback::instance().tap();
}

void TimerView::finish() {
    if (_tickTimer) {
        lv_timer_delete(_tickTimer);
        _tickTimer = nullptr;
    }
    showPhase(Phase::Done);

    if (_beepOnComplete) {
        _ringsRemaining = kRingCount;
        if (!_ringTimer) {
            _ringTimer = lv_timer_create(ringTimerCb, kRingIntervalMs, this);
        }
    } else {
        Buzzer::confirm();
        HapticFeedback::instance().confirm();
    }

    if (_reportId.length() > 0) {
        publishReport(Report{_reportId, "0"});
    }
}

void TimerView::updateCountdown() {
    long elapsedSeconds = static_cast<long>(millis() - _startMs) / 1000;
    long remaining = _totalSeconds - elapsedSeconds;
    if (remaining < 0) {
        remaining = 0;
    }

    lv_bar_set_value(_countdownBar, static_cast<int32_t>(remaining), LV_ANIM_OFF);

    int mm = static_cast<int>(remaining / 60);
    int ss = static_cast<int>(remaining % 60);
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", mm, ss);
    lv_label_set_text(_countdownLabel, buf);
}

void TimerView::tickTimerCb(lv_timer_t* timer) {
    auto* self = static_cast<TimerView*>(lv_timer_get_user_data(timer));
    long elapsedSeconds = static_cast<long>(millis() - self->_startMs) / 1000;
    if (elapsedSeconds >= self->_totalSeconds) {
        self->updateCountdown();
        self->finish();  // deletes _tickTimer (the very timer invoking this callback) - safe, lv_timer_delete()
                          // just marks it for removal at the end of the current lv_timer_handler() pass.
        return;
    }
    self->updateCountdown();
}

void TimerView::ringTimerCb(lv_timer_t* timer) {
    auto* self = static_cast<TimerView*>(lv_timer_get_user_data(timer));
    Buzzer::ring();
    HapticFeedback::instance().ring();
    self->_ringsRemaining--;
    if (self->_ringsRemaining <= 0) {
        lv_timer_delete(timer);
        self->_ringTimer = nullptr;
    }
}

void TimerView::spinboxIncrementTappedCb(lv_event_t* event) {
    auto* self = static_cast<TimerView*>(lv_event_get_user_data(event));
    lv_spinbox_increment(self->_spinbox);
}

void TimerView::spinboxDecrementTappedCb(lv_event_t* event) {
    auto* self = static_cast<TimerView*>(lv_event_get_user_data(event));
    lv_spinbox_decrement(self->_spinbox);
}

void TimerView::startButtonTappedCb(lv_event_t* event) {
    static_cast<TimerView*>(lv_event_get_user_data(event))->start();
}

void TimerView::cancelButtonTappedCb(lv_event_t* event) {
    static_cast<TimerView*>(lv_event_get_user_data(event))->cancel();
}

void TimerView::dismissButtonTappedCb(lv_event_t* event) {
    auto* self = static_cast<TimerView*>(lv_event_get_user_data(event));
    self->_ringsRemaining = 0;  // stop any in-progress "egg timer ring" beeps
    if (self->_ringTimer) {
        lv_timer_delete(self->_ringTimer);
        self->_ringTimer = nullptr;
    }
    self->showPhase(Phase::Setting);
    Buzzer::tap();
    HapticFeedback::instance().tap();
}

namespace {
struct TimerViewRegistrar {
    TimerViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Core2.Node.TimerView", []() { return std::make_unique<TimerView>(); }, [] {
                DeviceConfiguration config;
                config.id = riot2::newId();
                config.name = "Timer View";
                config.classFullName = "RIoT2.Ard.M5Core2.Node.TimerView";
                config.deviceParameters = {
                    {"defaultMinutes", "5"}, {"stepMinutes", "1"}, {"maxMinutes", "60"}, {"beepOnComplete", "false"}};

                CommandTemplate cmd;
                cmd.id = riot2::newId();
                cmd.type = "2";
                cmd.name = "Timer";
                cmd.address = "timer-1";
                cmd.valueType = 0;
                config.commandTemplates.push_back(cmd);

                ReportTemplate report;
                report.id = riot2::newId();
                report.type = "2";
                report.name = "Timer";
                report.address = "timer-1";
                config.reportTemplates.push_back(report);
                return config;
            });
    }
} timerViewRegistrar;
}  // namespace
