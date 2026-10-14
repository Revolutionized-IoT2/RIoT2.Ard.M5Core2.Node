#include "BLEView.h"

#include <memory>

#include "ViewFactory.h"

void BLEView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
}

void BLEView::buildUi(lv_obj_t* parent) {
    lv_obj_t* content = buildHeaderArea(parent, _model.header(), String());
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    _list = lv_list_create(content);
    lv_obj_set_size(_list, lv_pct(100), lv_pct(100));

    refreshList();
}

void BLEView::refreshList() {
    if (!_list) {
        return;
    }
    lv_obj_clean(_list);

    if (_model.devices().empty()) {
        lv_list_add_text(_list, "No devices found");
        return;
    }

    for (const auto& device : _model.devices()) {
        String label = device.name.length() > 0 ? device.name : device.address;
        label += "  (" + String(device.rssi) + " dBm)";
        lv_list_add_button(_list, nullptr, label.c_str());
    }
}

void BLEView::onBleSnapshot(const std::vector<BleDeviceInfo>& devices) {
    _model.onSnapshot(devices);
    refreshList();
}

void BLEView::onBleDeviceDiscovered(const BleDeviceInfo& device) {
    Report report;
    bool reported = _model.onDeviceDiscovered(device, report);
    refreshList();
    if (reported) {
        publishReport(report);
    }
}

void BLEView::onBleDeviceLost(const String& address) {
    Report report;
    bool reported = _model.onDeviceLost(address, report);
    refreshList();
    if (reported) {
        publishReport(report);
    }
}

void BLEView::onBleAdvertisement(const BleAdvertisement& advertisement) {
    Report report;
    if (_model.onAdvertisement(advertisement, report)) {
        publishReport(report);
    }
}

namespace {
struct BLEViewRegistrar {
    BLEViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Core2.Node.BLEView", []() { return std::make_unique<BLEView>(); },
            []() { return riot2::views::BleModel::buildTemplate("RIoT2.Ard.M5Core2.Node.BLEView"); });
    }
} bleViewRegistrar;
}  // namespace