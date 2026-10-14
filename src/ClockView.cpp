#include "ClockView.h"

#include <memory>

#include "ViewFactory.h"

namespace {
constexpr uint32_t kTickMs = 1000;
}  // namespace

ClockView::~ClockView() {
    if (_timer) {
        lv_timer_delete(_timer);
    }
}

void ClockView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
}

void ClockView::buildUi(lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    _timeLabel = lv_label_create(parent);
    lv_obj_set_style_text_font(_timeLabel, &lv_font_montserrat_48, 0);

    _dateLabel = lv_label_create(parent);

    updateLabels();

    // See the destructor - _timer is deleted explicitly there, since it's
    // not an lv_obj_t child that clearTabs() would otherwise cascade-delete.
    _timer = lv_timer_create(tickCb, kTickMs, this);
}

void ClockView::updateLabels() {
    String timeText;
    String dateText;
    riot2::views::ClockModel::now(timeText, dateText);
    lv_label_set_text(_timeLabel, timeText.c_str());
    lv_label_set_text(_dateLabel, dateText.c_str());
}

void ClockView::tickCb(lv_timer_t* timer) {
    auto* self = static_cast<ClockView*>(lv_timer_get_user_data(timer));
    self->updateLabels();
}

namespace {
struct ClockViewRegistrar {
    ClockViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Core2.Node.ClockView", []() { return std::make_unique<ClockView>(); },
            []() { return riot2::views::ClockModel::buildTemplate("RIoT2.Ard.M5Core2.Node.ClockView"); });
    }
} clockViewRegistrar;
}  // namespace
