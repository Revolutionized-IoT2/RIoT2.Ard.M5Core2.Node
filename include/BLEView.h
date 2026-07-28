#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <vector>

#include <riot2/BleTypes.h>

#include "IView.h"

// Read-only tab listing nearby BLE advertisers discovered by the node's
// on-device BLE radio (see BleScanner.h) - populated via the
// consumesBleEvents() hooks below, which ViewManager routes to this view
// regardless of whether its tab is currently focused, since the underlying
// BLE scan runs continuously in the background once BLEView is present in
// the configuration (see ViewManager::hasBleConsumer(), checked in
// main.cpp). Rendering swap only vs. M5Dial.Node's BLEView (lv_list instead
// of a hand-drawn scrollable row list) - the report/filter logic below
// mirrors it exactly.
//
// Reports are published for three distinct scenarios, each addressed by a
// separate reportTemplate matched by its `address` field (falling back to
// positional order 0/1/2 if `address` isn't set):
//   - address "deviceFound"   -> a previously-unseen device started advertising
//   - address "deviceLost"    -> a previously-seen device is no longer present
//   - address "advertisement" -> forwarded for every advertisement received
// Any of the three may be omitted from the configuration; that scenario is
// then simply not reported.
//
// Optional per-reportTemplate `parameters` entry `allowedAddresses`: a
// comma-separated list of BLE MAC addresses (matched case-insensitively)
// that THIS report should be restricted to. Devices excluded by a given
// report's filter are still discovered/tracked and shown on screen, they
// just don't publish that particular report.
class BLEView : public IView {
public:
    void begin(const DeviceConfiguration& config) override;
    void buildUi(lv_obj_t* parent) override;

    bool consumesBleEvents() const override { return true; }
    void onBleDeviceDiscovered(const BleDeviceInfo& device) override;
    void onBleDeviceLost(const String& address) override;
    void onBleAdvertisement(const BleAdvertisement& advertisement) override;

private:
    String _header;
    String _deviceFoundReportId;
    String _deviceLostReportId;
    String _advertisementReportId;
    std::vector<String> _deviceFoundAllowedAddresses;
    std::vector<String> _deviceLostAllowedAddresses;
    std::vector<String> _advertisementAllowedAddresses;

    std::vector<BleDeviceInfo> _devices;
    lv_obj_t* _list = nullptr;

    void refreshList();
    static bool isAddressAllowed(const std::vector<String>& allowedAddresses, const String& address);
};
