#pragma once

#include <Arduino.h>

#include <memory>
#include <vector>

#include <riot2/BleTypes.h>
#include <riot2/Command.h>
#include <riot2/DeviceConfiguration.h>

#include "IView.h"
#include "NavigationController.h"

// Populates one NavigationController tab per non-alert DeviceConfiguration
// entry the orchestrator sends - the Core2 tab-based analog of M5Dial.Node's
// carousel-driven ViewManager. There's no carousel/idle-timer/encoder logic
// here: LVGL's own lv_tabview (owned by NavigationController) plus each
// view's buildUi() event callbacks handle navigation and input.
class ViewManager {
public:
    // (Re)builds every tab from a fresh NodeConfiguration. Safe to call
    // again for re-configuration pushes: clears every existing tab first
    // (via NavigationController::clearTabs()).
    void rebuild(const NodeConfiguration& nodeConfiguration, NavigationController& navigationController);

    void onReport(IView::ReportCallback callback) { _reportCallback = callback; }

    // Wired to a PopupOverlay-backed bridge in main.cpp: every view's
    // showPopup() call (see IView.h) ends up here, regardless of which view
    // made it - PopupOverlay only ever shows one popup at a time.
    void onPopup(IView::PopupCallback callback) { _popupCallback = callback; }

    // Routes an inbound command to whichever view owns a commandTemplate
    // matching commandId. Returns true if a view handled it (callers can try
    // PeripheralManager::onCommand() too - since commandTemplate ids are
    // unique per configuration entry, exactly one of the two will ever
    // actually match).
    bool onCommand(const String& commandId, const Command& command);

    // True if at least one current view wants BLE scan events (see
    // IView::consumesBleEvents()) - main.cpp uses this to lazily activate
    // BleScanner only when a configuration actually needs it.
    bool hasBleConsumer() const;

    // Forwarded from main.cpp's BleScanner callbacks to every view with
    // consumesBleEvents() == true, regardless of which tab is focused.
    void notifyBleDeviceDiscovered(const BleDeviceInfo& device);
    void notifyBleDeviceLost(const String& address);
    void notifyBleAdvertisement(const BleAdvertisement& advertisement);

private:
    struct Entry {
        DeviceConfiguration config;
        std::unique_ptr<IView> view;
    };

    std::vector<Entry> _entries;
    IView::ReportCallback _reportCallback;
    IView::PopupCallback _popupCallback;
};
