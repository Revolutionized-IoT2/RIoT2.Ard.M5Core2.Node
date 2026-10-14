#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <vector>

#include <riot2/BleTypes.h>
#include <riot2/views/BleModel.h>

#include "IView.h"

// Read-only tab listing nearby BLE advertisers discovered by the node's
// on-device BLE radio (see BleScanner.h) - populated via the
// consumesBleEvents() hooks below, which ViewManager routes to this view
// regardless of whether its tab is currently focused, since the underlying
// BLE scan runs continuously in the background once BLEView is present in
// the configuration (see ViewManager::hasBleConsumer(), checked in
// main.cpp). Renders the shared riot2::views::BleModel as an lv_list; the
// device list, the "deviceFound"/"deviceLost"/"advertisement" reports and
// their per-report `allowedAddresses` filters are documented there.
class BLEView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;

    bool consumesBleEvents() const override { return true; }
    void onBleSnapshot(const std::vector<BleDeviceInfo>& devices) override;
    void onBleDeviceDiscovered(const BleDeviceInfo& device) override;
    void onBleDeviceLost(const String& address) override;
    void onBleAdvertisement(const BleAdvertisement& advertisement) override;

private:
    riot2::views::BleModel _model;
    lv_obj_t* _list = nullptr;

    void refreshList();
};