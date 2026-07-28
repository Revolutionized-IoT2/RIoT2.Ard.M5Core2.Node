#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include "IView.h"

// Shows the current time (and date once SNTP has synced; see
// main.cpp's configTime() call) as a normal tab. Takes no
// commandTemplates/reportTemplates - registered like any other view so the
// orchestrator can place it explicitly in deviceConfigurations.
//
// Unlike M5Dial.Node's ClockView, this is not (yet) wired up as an
// idle-timeout overlay - that needs ScreenPowerPolicy, which is Phase 8
// scope (see plan.md Section 4/8); until then it's just a tab like any
// other view.
//
// Supports an optional "timezone" deviceParameter: a POSIX TZ string (e.g.
// "EET-2EEST,M3.5.0/3,M10.5.0/4" for Helsinki, with full DST support)
// applied via setenv("TZ", ...)/tzset() so localtime_r() shows local time
// instead of UTC. If absent, time is shown in UTC (main.cpp's configTime()
// call has no gmtOffset/DST applied).
class ClockView : public IView {
public:
    // Deletes _timer explicitly - it's a standalone LVGL timer, not an
    // lv_obj_t child, so it isn't cascade-deleted when
    // NavigationController::clearTabs() destroys the tab this view built
    // into (see ViewManager::rebuild()). Without this, a stale timer would
    // keep firing against a freed ClockView on every re-configuration push.
    ~ClockView() override;

    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;

private:
    lv_obj_t* _timeLabel = nullptr;
    lv_obj_t* _dateLabel = nullptr;
    lv_timer_t* _timer = nullptr;

    void updateLabels();
    static void tickCb(lv_timer_t* timer);
};
