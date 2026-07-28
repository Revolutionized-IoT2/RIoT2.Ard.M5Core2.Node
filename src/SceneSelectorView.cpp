#include "SceneSelectorView.h"

#include <memory>

#include "ViewFactory.h"

namespace {
constexpr lv_coord_t kTileWidth = 130;
constexpr lv_coord_t kTileHeight = 70;
}  // namespace

void SceneSelectorView::begin(const DeviceConfiguration& config) {
    _slots.clear();
    for (const auto& report : config.reportTemplates) {
        Slot slot;
        slot.report = report;
        slot.owner = this;
        _slots.push_back(slot);
    }
}

void SceneSelectorView::buildUi(lv_obj_t* parent) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    if (_slots.empty()) {
        lv_obj_t* emptyLabel = lv_label_create(parent);
        lv_label_set_text(emptyLabel, "No scenes");
        return;
    }

    // _slots is never resized again after begin(), so these element
    // addresses stay stable for the lifetime of the view - safe to hand out
    // as event user_data below (mirrors ButtonView's pattern).
    for (auto& slot : _slots) {
        lv_obj_t* tile = lv_button_create(parent);
        lv_obj_set_size(tile, kTileWidth, kTileHeight);
        lv_obj_add_event_cb(tile, tileTappedCb, LV_EVENT_CLICKED, &slot);

        lv_obj_t* label = lv_label_create(tile);
        lv_label_set_text(label, slot.report.name.c_str());
        lv_obj_center(label);
    }
}

void SceneSelectorView::tileTappedCb(lv_event_t* event) {
    auto* slot = static_cast<Slot*>(lv_event_get_user_data(event));
    slot->owner->publishReport(Report{slot->report.id, "true"});
}

namespace {
struct SceneSelectorViewRegistrar {
    SceneSelectorViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Core2.Node.SceneSelectorView",
                                                 []() { return std::make_unique<SceneSelectorView>(); });
    }
} sceneSelectorViewRegistrar;
}  // namespace
