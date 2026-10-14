#include "TimerView.h"

#include <memory>

#include "Buzzer.h"
#include "HapticFeedback.h"
#include "ViewFactory.h"

using riot2::views::TimerModel;

namespace {
constexpr uint32_t kTickMs = 1000;
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
    _model.begin(config);
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
    lv_spinbox_set_range(_spinbox, _model.stepMinutes(), _model.maxMinutes());
    lv_spinbox_set_step(_spinbox, _model.stepMinutes());
    lv_spinbox_set_digit_format(_spinbox, String(_model.maxMinutes()).length(), 0);
    lv_spinbox_set_value(_spinbox, _model.minutes());
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

    showPhase();
}

void TimerView::showPhase() {
    TimerModel::Phase phase = _model.phase();
    if (_settingContainer) {
        (phase == TimerModel::Phase::Setting ? lv_obj_remove_flag : lv_obj_add_flag)(_settingContainer,
                                                                                     LV_OBJ_FLAG_HIDDEN);
    }
    if (_runningContainer) {
        (phase == TimerModel::Phase::Running ? lv_obj_remove_flag : lv_obj_add_flag)(_runningContainer,
                                                                                     LV_OBJ_FLAG_HIDDEN);
    }
    if (_doneContainer) {
        (phase == TimerModel::Phase::Done ? lv_obj_remove_flag : lv_obj_add_flag)(_doneContainer, LV_OBJ_FLAG_HIDDEN);
    }
}

void TimerView::onCommand(const Command& command) {
    if (_model.onCommand(command) && _spinbox) {
        lv_spinbox_set_value(_spinbox, _model.minutes());
    }
}

void TimerView::adjust(int delta) {
    _model.adjust(delta);
    lv_spinbox_set_value(_spinbox, _model.minutes());
}

void TimerView::start() {
    if (!_model.start()) {
        return;
    }
    lv_bar_set_range(_countdownBar, 0, static_cast<int32_t>(_model.totalSeconds()));
    lv_bar_set_value(_countdownBar, static_cast<int32_t>(_model.totalSeconds()), LV_ANIM_OFF);
    updateCountdown();

    showPhase();
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
    if (!_model.cancel()) {
        return;
    }
    showPhase();
    Buzzer::tap();
    HapticFeedback::instance().tap();
}

void TimerView::finish() {
    if (_tickTimer) {
        lv_timer_delete(_tickTimer);
        _tickTimer = nullptr;
    }
    showPhase();

    if (_model.beepOnComplete()) {
        if (!_ringTimer) {
            _ringTimer = lv_timer_create(ringTimerCb, TimerModel::kRingIntervalMs, this);
        }
    } else {
        Buzzer::confirm();
        HapticFeedback::instance().confirm();
    }

    Report report;
    if (_model.completionReport(report)) {
        publishReport(report);
    }
}

void TimerView::updateCountdown() {
    lv_bar_set_value(_countdownBar, static_cast<int32_t>(_model.remainingSeconds()), LV_ANIM_OFF);
    lv_label_set_text(_countdownLabel, _model.remainingText().c_str());
}

void TimerView::tickTimerCb(lv_timer_t* timer) {
    auto* self = static_cast<TimerView*>(lv_timer_get_user_data(timer));
    self->updateCountdown();
    if (self->_model.finishIfElapsed()) {
        self->finish();  // deletes _tickTimer (the very timer invoking this callback) - safe, lv_timer_delete()
                          // just marks it for removal at the end of the current lv_timer_handler() pass.
    }
}

void TimerView::ringTimerCb(lv_timer_t* timer) {
    auto* self = static_cast<TimerView*>(lv_timer_get_user_data(timer));
    if (self->_model.takeRing()) {
        Buzzer::ring();
        HapticFeedback::instance().ring();
    }
    if (self->_model.ringsRemaining() <= 0) {
        lv_timer_delete(timer);
        self->_ringTimer = nullptr;
    }
}

void TimerView::spinboxIncrementTappedCb(lv_event_t* event) {
    static_cast<TimerView*>(lv_event_get_user_data(event))->adjust(1);
}

void TimerView::spinboxDecrementTappedCb(lv_event_t* event) {
    static_cast<TimerView*>(lv_event_get_user_data(event))->adjust(-1);
}

void TimerView::startButtonTappedCb(lv_event_t* event) {
    static_cast<TimerView*>(lv_event_get_user_data(event))->start();
}

void TimerView::cancelButtonTappedCb(lv_event_t* event) {
    static_cast<TimerView*>(lv_event_get_user_data(event))->cancel();
}

void TimerView::dismissButtonTappedCb(lv_event_t* event) {
    auto* self = static_cast<TimerView*>(lv_event_get_user_data(event));
    self->_model.dismiss();  // stops any in-progress "egg timer ring" beeps
    if (self->_ringTimer) {
        lv_timer_delete(self->_ringTimer);
        self->_ringTimer = nullptr;
    }
    self->showPhase();
    Buzzer::tap();
    HapticFeedback::instance().tap();
}

namespace {
struct TimerViewRegistrar {
    TimerViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Core2.Node.TimerView", []() { return std::make_unique<TimerView>(); },
            []() { return TimerModel::buildTemplate("RIoT2.Ard.M5Core2.Node.TimerView", true); });
    }
} timerViewRegistrar;
}  // namespace