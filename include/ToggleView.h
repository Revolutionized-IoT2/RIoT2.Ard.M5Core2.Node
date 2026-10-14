#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <vector>

#include <riot2/views/ToggleModel.h>

#include "IView.h"

// 1-2 on/off lv_switch widgets, stacked vertically (a single switch is
// centered), rendering the shared riot2::views::ToggleModel. Toggling a
// switch publishes its new state as a Report; an inbound Command sets that
// switch's state without publishing a Report for it.
class ToggleView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;
    void onCommand(const Command& command) override;

private:
    struct Row {
        // Back-pointer so the static event callback (which only gets a
        // Row*, its registered user_data) can reach the model and
        // publishReport(), a protected IView member.
        ToggleView* owner = nullptr;
        size_t index = 0;
        lv_obj_t* switchObj = nullptr;
    };

    riot2::views::ToggleModel _model;
    std::vector<Row> _rows;

    void showState(size_t index);
    static void switchToggledCb(lv_event_t* event);
};
