#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include <functional>

#include <riot2/BleTypes.h>
#include <riot2/Command.h>
#include <riot2/DeviceConfiguration.h>
#include <riot2/Report.h>

// A request to show a transient popup via PopupOverlay - see IView::showPopup()
// and ViewManager::onPopup(). autoDismiss=false (AlertView) means "stays until
// tapped"; autoDismiss=true (NotificationView) means "auto-dismisses after
// autoDismissMs", mirroring PopupOverlay::showAlert()/showNotification().
struct PopupRequest {
    String title;
    String message;
    bool autoDismiss = false;
    uint32_t autoDismissMs = 0;
};

// Common interface every on-device View implements. Unlike M5Dial.Node's
// carousel-driven IView, this Core2 flavor has no render(M5Canvas&)/
// onEncoderChange()/focus-lifecycle hooks - ViewManager just gives each
// non-alert view one NavigationController tab to build LVGL widgets into
// once; those widgets' own event callbacks (registered inside buildUi())
// handle all touch input from then on. RFID hooks are omitted entirely
// (Core2 has no on-device RFID reader - see Rfid2Peripheral, a peripheral,
// not a view).
class IView {
public:
    using ReportCallback = std::function<void(const Report&)>;
    using PopupCallback = std::function<void(const PopupRequest&)>;

    virtual ~IView() = default;

    // One-time setup from the orchestrator-provided configuration for this view.
    virtual void begin(const DeviceConfiguration& config) = 0;

    // Builds this view's LVGL widgets once inside `parent` (a tab's content
    // area). Never called for isAlert() views - see ViewManager::rebuild().
    virtual void buildUi(lv_obj_t* parent) = 0;

    // Apply an inbound command addressed to one of this view's commandTemplates.
    virtual void onCommand(const Command& command) { (void)command; }

    // Views like AlertView/NotificationView that must interrupt whatever's
    // currently on screen as soon as their command arrives - rendered via
    // PopupOverlay instead of a normal tab - return true here.
    virtual bool isAlert() const { return false; }

    // BLEView-style views that consume nearby-device scan results from the
    // node's on-device BLE radio (see BleScanner.h) return true here so
    // ViewManager::notifyBleXxx() knows to route scan events to them,
    // regardless of which tab is currently focused - unlike isAlert(), this
    // never takes over the display.
    virtual bool consumesBleEvents() const { return false; }

    // A previously-unseen nearby BLE device started advertising.
    virtual void onBleDeviceDiscovered(const BleDeviceInfo& device) { (void)device; }
    // A previously-seen nearby BLE device hasn't been heard from recently
    // enough (see BleScanner::kDeviceTimeoutMs) and is considered gone.
    virtual void onBleDeviceLost(const String& address) { (void)address; }
    // Fired for every BLE advertisement received (not just the first time a
    // device is seen), so views can forward its raw contents verbatim.
    virtual void onBleAdvertisement(const BleAdvertisement& advertisement) { (void)advertisement; }

    // Set by the ViewManager before begin(). Views that produce telemetry
    // call publishReport() to emit a Report for one of their reportTemplates.
    void setReportCallback(ReportCallback callback) { _reportCallback = callback; }

    // Set by the ViewManager before begin(). isAlert() views call showPopup()
    // from onCommand() to actually render themselves via PopupOverlay -
    // ViewManager itself never inspects PopupRequest contents.
    void setPopupCallback(PopupCallback callback) { _popupCallback = callback; }

protected:
    void publishReport(const Report& report) {
        if (_reportCallback) {
            _reportCallback(report);
        }
    }

    void showPopup(const PopupRequest& request) {
        if (_popupCallback) {
            _popupCallback(request);
        }
    }

private:
    ReportCallback _reportCallback;
    PopupCallback _popupCallback;
};
