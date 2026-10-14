#include <Arduino.h>
#include <M5Unified.h>

#include <memory>

#include <riot2/NodeRuntime.h>

#include "BoardPeripherals.h"
#include "Buzzer.h"
#include "DiagnosticsView.h"
#include "HapticFeedback.h"
#include "LvglDisplay.h"
#include "Manifest.h"
#include "MatrixRainView.h"
#include "NavigationController.h"
#include "PopupOverlay.h"
#include "QrSettingsView.h"
#include "ScreenPowerPolicy.h"
#include "SplashView.h"
#include "TouchGestures.h"
#include "ViewFactory.h"
#include "ViewManager.h"

// Board setup, the LVGL UI and the gestures. The node lifecycle (Wi-Fi, provisioning, MQTT,
// configuration, OTA, peripherals, BLE) is riot2::NodeRuntime in RIoT2.Ard.Shared.
namespace {

// Hold duration for BtnB (middle virtual button) to toggle the full-screen
// DiagnosticsView overlay - shorter than the factory reset, since normal tab
// navigation only ever clicks BtnB.
constexpr unsigned long kDiagnosticsHoldMs = 3000;

riot2::NodeRuntime runtime;
LvglDisplay lvglDisplay;
NavigationController navigationController;
PopupOverlay popupOverlay;
MatrixRainView matrixRainView;
ScreenPowerPolicy screenPowerPolicy(popupOverlay, matrixRainView);
ViewManager viewManager;
SplashView splashView;
DiagnosticsView diagnosticsView;
TouchGestures gestures;

// True once the first configuration has been applied and the real tabview
// exists. Until then SplashView is all that's shown: without a live
// orchestrator this node can't do anything useful, so there's no cached or
// offline tab UI to fall back to.
bool tabsInitialized = false;

WifiState lastWifiState = WifiState::Disconnected;
MqttState lastMqttState = MqttState::Disconnected;
unsigned long lastDiagnosticsUpdateMs = 0;

// The first configuration replaces SplashView with the real tabview.
bool applyConfiguration(const NodeConfiguration& configuration) {
    if (!tabsInitialized) {
        splashView.destroy();
        navigationController.begin();
        navigationController.setPopupDismissHandler([] { return popupOverlay.dismiss(); });
        tabsInitialized = true;
    }
    viewManager.rebuild(configuration, navigationController);
    return viewManager.hasBleConsumer();
}

riot2::NodeRuntimeHooks runtimeHooks() {
    riot2::NodeRuntimeHooks hooks;
    hooks.viewTemplates = [] { return ViewFactory::instance().configurationTemplates(); };
    hooks.isView = [](const String& classFullName) { return ViewFactory::instance().isRegistered(classFullName); };
    hooks.onConfiguration = applyConfiguration;
    hooks.onCommand = [](const Command& command) { viewManager.onCommand(command.id, command); };
    hooks.onConfigurationRequested = [] {
        if (!tabsInitialized) {
            splashView.setConfigStatus("Config: fetching from Orchestrator...");
        }
    };
    hooks.onOtaStarted = [](const String&) {
        Buzzer::confirm();
        HapticFeedback::instance().confirm();
        popupOverlay.showAlert("Updating...", "Do not power off");
    };
    hooks.onOtaFailed = [] {
        Buzzer::error();
        HapticFeedback::instance().error();
        popupOverlay.showAlert("Update failed", "See serial log");
    };
    hooks.onBleDeviceDiscovered = [](const BleDeviceInfo& device) { viewManager.notifyBleDeviceDiscovered(device); };
    hooks.onBleDeviceLost = [](const String& address) { viewManager.notifyBleDeviceLost(address); };
    hooks.onBleAdvertisement = [](const BleAdvertisement& advertisement) { viewManager.notifyBleAdvertisement(advertisement); };
    return hooks;
}

void factoryReset() {
    Buzzer::error();
    HapticFeedback::instance().error();
    popupOverlay.showAlert("Resetting...", "Clearing config");
    runtime.factoryReset();
}

}  // namespace

void setup() {
    Serial.begin(115200);
    // First, so a new OTA image's trial time starts at boot (see riot2/OtaRollback.h).
    riot2::NodeRuntime::startOtaTrial();
    registerPeripherals();

    auto cfg = M5.config();
    M5.begin(cfg);

    // M5Unified's virtual BtnA/B/C touch zones are off by default (a
    // zero-height strip below the 240px panel no touch can reach), so the
    // buttons never fired. 40px makes y=200-239 the button strip (BtnA/B/C =
    // its left/middle/right thirds).
    M5.setTouchButtonHeight(40);

    // Default hold threshold is 500ms - raised so BtnB's normal click (tab
    // nav/menu) still works for anything shorter, and wasHold() only fires
    // once the diagnostics-toggle hold is reached.
    M5.BtnB.setHoldThresh(kDiagnosticsHoldMs);

    Buzzer::begin();

    // After M5.begin() (it reads the display size), and before anything that
    // draws: SplashView and the provisioning screen need LVGL to own the display.
    lvglDisplay.begin();
    popupOverlay.begin();
    matrixRainView.begin();
    screenPowerPolicy.begin();

    runtime.begin(Manifest::json, runtimeHooks());
    HapticFeedback::instance().setEnabled(runtime.config().vibrateEnabled);

    if (runtime.isProvisioning()) {
        QrSettingsView::build(lv_screen_active(), runtime.provisioning().apSsid(), runtime.provisioning().apIp());
        return;
    }

    splashView.build(runtime.config().id);
    // Node id/name never change at runtime, so this is built once (see the
    // BtnB hold in loop()).
    diagnosticsView.build(runtime.config().id, runtime.config().name);

    viewManager.onReport([](const Report& report) { runtime.publishReport(report); });
    // Every view's showPopup() goes to the one PopupOverlay. Alerts and
    // notifications must be seen, so wakeForAlert() forces the display on and
    // hides the idle MatrixRainView first (separate from ScreenPowerPolicy's
    // input-driven wake()).
    viewManager.onPopup([](const PopupRequest& request) {
        screenPowerPolicy.wakeForAlert();
        if (request.autoDismiss) {
            popupOverlay.showNotification(request.title, request.message, request.autoDismissMs);
        } else {
            popupOverlay.showAlert(request.title, request.message);
        }
    });
}

void loop() {
    M5.update();
    gestures.update();
    lv_timer_handler();

    runtime.loop();
    if (runtime.isProvisioning()) {
        return;
    }

    // True when this frame's touch/button activity only woke the display; it
    // then doesn't also press a button (see ScreenPowerPolicy::loop()).
    bool wokeFromIdle = screenPowerPolicy.loop();

    if (!wokeFromIdle && tabsInitialized && !gestures.wasScroll()) {
        if (M5.BtnA.wasClicked()) {
            navigationController.onButtonPress(NavigationController::Button::A);
        }
        if (M5.BtnB.wasClicked()) {
            navigationController.onButtonPress(NavigationController::Button::B);
        }
        if (M5.BtnC.wasClicked()) {
            navigationController.onButtonPress(NavigationController::Button::C);
        }
    }

    // wasHold() fires once per hold, also while the splash screen shows.
    if (M5.BtnB.wasHold()) {
        if (diagnosticsView.isVisible()) {
            diagnosticsView.hide();
        } else {
            diagnosticsView.show();
        }
    }

    if (gestures.factoryResetRequested()) {
        factoryReset();
        return;
    }

    WifiConnection& wifi = runtime.wifi();
    MqttConnection& mqtt = runtime.mqtt();
    if (wifi.state() != lastWifiState || mqtt.state() != lastMqttState) {
        lastWifiState = wifi.state();
        lastMqttState = mqtt.state();
        if (!tabsInitialized) {
            splashView.setWifiStatus(String("WiFi: ") + (wifi.isConnected() ? "connected" : "connecting..."));
            splashView.setMqttStatus(String("MQTT: ") + (mqtt.isConnected() ? "connected" : "connecting..."));
        }
    }

    // DiagnosticsView has no timer of its own: refreshed about once a second
    // while visible.
    if (diagnosticsView.isVisible() && (millis() - lastDiagnosticsUpdateMs) >= 1000) {
        lastDiagnosticsUpdateMs = millis();
        diagnosticsView.update(wifi, mqtt);
    }
}
