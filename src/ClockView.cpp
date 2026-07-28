#include "ClockView.h"

#include <memory>
#include <stdlib.h>
#include <time.h>

#include "ViewFactory.h"

namespace {
// time(nullptr) returns small values (seconds since boot-ish) before SNTP
// has synced; treat anything before 2020-01-01 as "not yet synced".
constexpr time_t kMinValidEpoch = 1577836800;
constexpr uint32_t kTickMs = 1000;
}  // namespace

ClockView::~ClockView() {
    if (_timer) {
        lv_timer_delete(_timer);
    }
}

void ClockView::begin(const DeviceConfiguration& config) {
    // A POSIX TZ string (e.g. "EET-2EEST,M3.5.0/3,M10.5.0/4") so
    // localtime_r() below reflects the configured timezone, DST included,
    // instead of the UTC that main.cpp's configTime() call establishes by
    // default. Left alone (no setenv/tzset call) when unset, preserving
    // UTC-only behavior.
    String timezone = findParameter(config.deviceParameters, "timezone", "");
    if (timezone.length() > 0) {
        setenv("TZ", timezone.c_str(), 1);
        tzset();
    }
}

void ClockView::buildUi(lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    _timeLabel = lv_label_create(parent);
    lv_obj_set_style_text_font(_timeLabel, &lv_font_montserrat_24, 0);

    _dateLabel = lv_label_create(parent);

    updateLabels();

    // See the destructor - _timer is deleted explicitly there, since it's
    // not an lv_obj_t child that clearTabs() would otherwise cascade-delete.
    _timer = lv_timer_create(tickCb, kTickMs, this);
}

void ClockView::updateLabels() {
    time_t now = time(nullptr);
    struct tm timeInfo;
    if (now >= kMinValidEpoch && localtime_r(&now, &timeInfo) != nullptr) {
        char timeBuf[9];
        snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
        lv_label_set_text(_timeLabel, timeBuf);

        char dateBuf[16];
        snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d", timeInfo.tm_year + 1900, timeInfo.tm_mon + 1,
                  timeInfo.tm_mday);
        lv_label_set_text(_dateLabel, dateBuf);
    } else {
        lv_label_set_text(_timeLabel, "--:--:--");
        lv_label_set_text(_dateLabel, "waiting for time sync");
    }
}

void ClockView::tickCb(lv_timer_t* timer) {
    auto* self = static_cast<ClockView*>(lv_timer_get_user_data(timer));
    self->updateLabels();
}

namespace {
struct ClockViewRegistrar {
    ClockViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Core2.Node.ClockView",
                                                 []() { return std::make_unique<ClockView>(); });
    }
} clockViewRegistrar;
}  // namespace
