#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <vector>

#include <riot2/views/ValueModel.h>

#include "IView.h"

// Displays up to 4 read-only values with unit labels (the shared
// riot2::views::ValueModel), stacked in a centered column. Values are only
// ever updated via an inbound Command - ValueView never publishes reports.
class ValueView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;
    void onCommand(const Command& command) override;

private:
    riot2::views::ValueModel _model;
    std::vector<lv_obj_t*> _valueLabels;
};
