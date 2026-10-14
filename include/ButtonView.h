#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <vector>

#include <riot2/views/ButtonModel.h>

#include "IView.h"

// 1-4 momentary buttons laid out as a single lv_buttonmatrix row (keeps
// them evenly stacked left-to-right, unlike an lv_button flex-wrap layout),
// rendering the shared riot2::views::ButtonModel. Tapping a button
// publishes a Report (id -> "true") for its reportTemplate and flashes it;
// an inbound Command addressed to the matching commandTemplate.id
// (correlated by shared `address`) sets that button's highlighted/on-off
// visual state. The view's own header/subheader text come from the
// "header"/"subHeader" deviceParameters.
class ButtonView : public IView {
public:
    ~ButtonView() override;
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;
    void onCommand(const Command& command) override;

private:
    struct Slot {
        // Back-pointer so the static timer callback (which only gets a
        // Slot*, since that's what's registered as its user_data) can still
        // reach the view.
        ButtonView* owner = nullptr;
        uint32_t btnId = 0;  // index of this slot's button within _matrix and _model
    };

    riot2::views::ButtonModel _model;
    std::vector<Slot> _slots;
    std::vector<lv_timer_t*> _flashTimers;
    std::vector<String> _buttonLabels;  // owns the text each _map entry points into
    std::vector<const char*> _map;      // lv_buttonmatrix_set_map()'s map, "" terminated
    lv_obj_t* _matrix = nullptr;

    void applyVisualState(uint32_t btnId, bool active);
    void cancelFlashTimers();
    // lv_buttonmatrix has no built-in text wrapping - long labels are broken
    // onto a second line in place so they don't spill past their button's edges.
    void wrapLongButtonLabels();
    static void matrixEventCb(lv_event_t* event);
    static void flashTimerCb(lv_timer_t* timer);
};
