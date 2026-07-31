#include <Arduino.h>
#include <ArduinoJson.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <time.h>

#include <memory>

#include <riot2/BleScanner.h>
#include <riot2/Command.h>
#include <riot2/ConfigTemplateServer.h>
#include <riot2/GpioPeripheral.h>
#include <riot2/MqttConnection.h>
#include <riot2/NodeConfig.h>
#include <riot2/OrchestratorClient.h>
#include <riot2/OtaUpdater.h>
#include <riot2/PeripheralFactory.h>
#include <riot2/PeripheralManager.h>
#include <riot2/ProvisioningPortal.h>
#include <riot2/Report.h>
#include <riot2/Uuid.h>
#include <riot2/WifiConnection.h>

#include "Buzzer.h"
#include "DiagnosticsView.h"
#include "HapticFeedback.h"
#include "LvglDisplay.h"
#include "Manifest.h"
#include "MatrixRainView.h"
#include "NavigationController.h"
#include "PopupOverlay.h"
#include "QrSettingsView.h"
#include "Rfid2Peripheral.h"
#include "ScreenPowerPolicy.h"
#include "SplashView.h"
#include "ViewFactory.h"
#include "ViewManager.h"

namespace {

// No single physical "boot button" on Core2 (BtnA/B/C are bottom touch
// zones) - factory reset instead requires holding BtnA+BtnC together, which
// is unlikely to happen by accident during normal touch/LVGL navigation
// (added in a later phase).
constexpr unsigned long kFactoryResetHoldMs = 5000;

// Hold duration for BtnB (middle virtual button) to toggle the full-screen
// DiagnosticsView overlay - shorter than kFactoryResetHoldMs since it's a
// single-button gesture with no accidental-activation risk from normal tab
// navigation (which only ever uses BtnB clicks, not holds).
constexpr unsigned long kDiagnosticsHoldMs = 3000;

// Reserved MQTT command id (RIoT2.Ard node-specific extension, not part of
// RIoT2.Core's Command contract) that triggers an OTA update instead of
// being routed to a view: { "id": "system.ota", "value": "<firmware url>" }.
const char* const kOtaCommandId = "system.ota";

// Total touch displacement (px, either axis) from touch-down beyond which a
// gesture is treated as a scroll/swipe rather than a tap - see
// updateScrollSuppression()'s doc comment below for why this is needed.
constexpr int kScrollSuppressDistancePx = 12;

// Core2's Grove PORT.A / PORT.B pin table (GpioPeripheral itself is
// board-agnostic - see riot2/GpioPeripheral.h). B2/GPIO36 is input-only
// (ADC1_CH0) - GpioPeripheral::begin() logs a warning if a commandTemplate
// ever targets it as an OUTPUT.
constexpr GpioPinMap kM5Core2GroveMap{32, 33, 26, 36};

enum class AppMode { Provisioning, Normal };

AppMode mode = AppMode::Normal;
NodeConfig config;
WifiConnection wifi;
MqttConnection mqtt;
ProvisioningPortal provisioning;
ConfigTemplateServer configTemplateServer;
// enableCache=false: this node never falls back to an on-flash cached
// NodeConfiguration (see loadCached()'s removal in setup()) - without a
// live Orchestrator there is nothing this node can meaningfully do, so it
// always shows SplashView (never a stale/offline tab UI) until a real fetch
// succeeds.
OrchestratorClient orchestratorClient{false};
// Grove-port peripherals (PORT.A / PORT.B) - configured the same way as
// Views but never shown on screen, see PeripheralManager.h.
PeripheralManager peripheralManager;

LvglDisplay lvglDisplay;
NavigationController navigationController;
PopupOverlay popupOverlay;
MatrixRainView matrixRainView;
ScreenPowerPolicy screenPowerPolicy(popupOverlay, matrixRainView);
ViewManager viewManager;
SplashView splashView;
DiagnosticsView diagnosticsView;

// True once the first live NodeConfiguration has been applied and
// NavigationController::begin() has created the real tabview - see
// handleConfigurationUpdated(). Until then, SplashView (not a tab UI) is
// all that's shown.
bool tabsInitialized = false;

WifiState lastWifiState = WifiState::Disconnected;
MqttState lastMqttState = MqttState::Disconnected;
unsigned long lastDiagnosticsUpdateMs = 0;
bool factoryResetTriggered = false;
bool factoryResetComboActive = false;
unsigned long factoryResetComboStartMs = 0;

// Touch-down anchor point and drag-detected flag for updateScrollSuppression()
// below - lives across loop() iterations for the duration of one touch.
int touchDownX = 0;
int touchDownY = 0;
bool touchWasScrollLike = false;

// Set once viewManager.hasBleConsumer() first goes true (see
// handleConfigurationUpdated()) - BleScanner::begin() only ever needs to run
// once; there's no "turn it back off" path since a later reconfiguration
// removing BLEView is not expected to happen on a live node.
bool bleActive = false;

// Set by handleConfigurationMessage() when riot2/node/{id}/configuration
// arrives, and consumed once from the top-level loop() rather than being
// acted on immediately inside the MQTT callback - see the identical pattern
// (and its rationale) in RIoT2.Ard.M5Dial.Node/src/main.cpp.
bool pendingConfigFetch = false;
String pendingApiBaseUrl;

void handleCommand(const String& topic, const String& payload) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.printf("[Command] Failed to parse %s: %s\n", topic.c_str(), error.c_str());
        return;
    }

    String id = doc["id"] | "";
    if (id.length() == 0) {
        Serial.printf("[Command] %s missing id, ignoring\n", topic.c_str());
        return;
    }

    if (id == kOtaCommandId) {
        String url = doc["value"] | "";
        Serial.printf("[OTA] Update requested: %s\n", url.c_str());
        Buzzer::confirm();
        HapticFeedback::instance().confirm();
        popupOverlay.showAlert("Updating...", "Do not power off");
        if (!OtaUpdater::performUpdate(url)) {
            Buzzer::error();
            HapticFeedback::instance().error();
            popupOverlay.showAlert("Update failed", "See serial log");
        }
        return;
    }

    Command command{id, doc["value"]};
    // commandTemplate ids are unique per configuration entry, so trying both
    // routers is safe - exactly one of them will ever actually own a given
    // commandId (see ViewManager::rebuild()/PeripheralManager::rebuild()).
    viewManager.onCommand(command.id, command);
    peripheralManager.onCommand(command.id, command);
}

void handleReport(const Report& report) {
    Serial.printf("[Report] id=%s value=%s\n", report.id.c_str(), report.value.c_str());
    mqtt.publishReport(report);
}

void handleOrchestratorOnline(const String& topic, const String& payload) {
    // Corrected handshake: riot2/orchestrator/online carries no payload (or
    // an ignorable one). The node simply re-announces itself; the
    // orchestrator replies with the apiBaseUrl on
    // riot2/node/{id}/configuration (see handleConfigurationMessage).
    Serial.println("[Orchestrator] Orchestrator online, re-announcing node");
    mqtt.publishOnline();
}

void handleConfigurationMessage(const String& topic, const String& payload) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
        Serial.printf("[Orchestrator] Failed to parse configuration message: %s\n", error.c_str());
        return;
    }

    String apiBaseUrl = doc["apiBaseUrl"] | "";
    if (apiBaseUrl.length() == 0) {
        Serial.println("[Orchestrator] configuration message missing apiBaseUrl");
        return;
    }

    // Deferred to loop() for the same reason as RIoT2.Ard.M5Dial.Node: a
    // blocking HTTP GET run synchronously inside PubSubClient's own callback
    // dispatch starves its keepalive/socket processing and can drop the MQTT
    // connection right as the fetch completes.
    pendingApiBaseUrl = apiBaseUrl;
    pendingConfigFetch = true;

    if (!tabsInitialized) {
        splashView.setConfigStatus("Config: fetching from Orchestrator...");
    }
}

void handleConfigurationUpdated(const NodeConfiguration& nodeConfiguration) {
    for (const auto& device : nodeConfiguration.deviceConfigurations) {
        Serial.printf("[Orchestrator]   device id=%s name=%s classFullName=%s (%u commands, %u reports)\n",
                      device.id.c_str(), device.name.c_str(), device.classFullName.c_str(),
                      static_cast<unsigned>(device.commandTemplates.size()),
                      static_cast<unsigned>(device.reportTemplates.size()));
    }
    // First live configuration: SplashView has done its job (no cached/
    // offline UI is ever shown - see OrchestratorClient's enableCache=false
    // above), so tear it down and only now create the real tabview.
    if (!tabsInitialized) {
        splashView.destroy();
        navigationController.begin();
        navigationController.setPopupDismissHandler([] { return popupOverlay.dismiss(); });
        tabsInitialized = true;
    }

    // ViewManager rebuilds first so its tabs exist by the time
    // PeripheralManager's isKnownElsewhere predicate below consults
    // ViewFactory - order doesn't strictly matter here (the predicate only
    // checks registration, not the live entries), but matches the
    // dependency direction conceptually.
    viewManager.rebuild(nodeConfiguration, navigationController);

    peripheralManager.rebuild(nodeConfiguration,
                               [](const String& classFullName) { return ViewFactory::instance().isRegistered(classFullName); });

    // Must happen BEFORE BleScanner::instance().begin() below (and before
    // any (re)connect WifiConnection::loop() might do concurrently): ESP32's
    // WiFi/BT coexistence layer hard-aborts (crash/reboot, not just a failed
    // connection) if WiFi modem sleep is disabled while a BLE radio is
    // active - see WifiConnection::setModemSleepEnabled()'s doc comment.
    if (viewManager.hasBleConsumer()) {
        wifi.setModemSleepEnabled(true);
    }

    // BLEView's presence is the only thing that decides whether the BLE
    // radio should be running at all - see BleScanner.h's rationale for why
    // it's not enabled unconditionally at boot.
    if (!bleActive && viewManager.hasBleConsumer()) {
        bleActive = true;
        BleScanner::instance().begin();
    }
}

void factoryReset() {
    Serial.println("[Config] Factory reset requested, clearing NodeConfig");
    Buzzer::error();
    HapticFeedback::instance().error();
    popupOverlay.showAlert("Resetting...", "Clearing config");
    mqtt.publishOfflineAndDisconnect();
    NodeConfigStore::clear();
    delay(500);
    ESP.restart();
}

// M5Unified's virtual BtnA/B/C touch zones (see M5.setTouchButtonHeight() in
// setup()) only look at each frame's *instantaneous* touch position/motion
// (see M5Unified.cpp's update(): a touch only counts toward a button if it's
// currently inside the bottom strip AND not moving *that exact frame*) -
// it never considers the touch's overall gesture history. A vertical scroll
// (a tab's scrollable content, or the main menu grid) or a horizontal tab
// swipe that happens to end with the finger lifting inside that same bottom
// strip therefore still fires a virtual button click (most disruptively
// BtnB, which opens the main menu) even though the user never intended to
// press a button - touch controllers commonly report one or two near-zero-
// delta samples right before liftoff, which is enough to satisfy "not
// moving this frame". Tracked here instead, using the touch's total
// displacement from its touch-down point (which M5Unified's own
// touch_detail_t conveniently keeps as a fixed anchor for the whole
// gesture, see base_x/base_y in Touch_Class.hpp) - a real tap never moves
// far from where it started, so anything past kScrollSuppressDistancePx is
// a scroll/swipe, not a tap, and every BtnA/B/C click following it this
// touch is suppressed below.
void updateScrollSuppression() {
    if (M5.Touch.getCount() == 0) {
        return;
    }
    auto detail = M5.Touch.getDetail(0);
    if (detail.wasPressed()) {
        touchDownX = detail.x;
        touchDownY = detail.y;
        touchWasScrollLike = false;
    } else if (detail.isPressed() &&
               (abs(detail.x - touchDownX) > kScrollSuppressDistancePx ||
                abs(detail.y - touchDownY) > kScrollSuppressDistancePx)) {
        touchWasScrollLike = true;
    }
}

}  // namespace

void setup() {
    Serial.begin(115200);

    PeripheralFactory::instance().registerCreator(
        "RIoT2.Ard.M5Core2.Node.GpioPeripheral", [] { return std::make_unique<GpioPeripheral>(kM5Core2GroveMap); },
        [] {
            DeviceConfiguration config;
            config.id = riot2::newId();
            config.name = "GPIO Peripheral";
            config.classFullName = "RIoT2.Ard.M5Core2.Node.GpioPeripheral";
            config.deviceParameters = {{"pullup", "true"}, {"invert", "false"}};

            CommandTemplate cmd;
            cmd.id = riot2::newId();
            cmd.type = "0";
            cmd.name = "Porch Relay";
            cmd.address = "B2";
            cmd.valueType = 0;
            config.commandTemplates.push_back(cmd);
            return config;
        });
    // Mutually exclusive with the GpioPeripheral A1/A2 slot on real hardware
    // (both use Grove PORT.A / GPIO32+33) - see Rfid2Peripheral.h. Configuring
    // both in the same node's configuration is a config error, not a firmware one.
    PeripheralFactory::instance().registerCreator(
        "RIoT2.Ard.M5Core2.Node.Rfid2Peripheral", [] { return std::make_unique<Rfid2Peripheral>(); },
        [] {
            DeviceConfiguration config;
            config.id = riot2::newId();
            config.name = "RFID2 Peripheral";
            config.classFullName = "RIoT2.Ard.M5Core2.Node.Rfid2Peripheral";

            ReportTemplate report;
            report.id = riot2::newId();
            report.type = "1";
            report.name = "RFID Tag";
            config.reportTemplates.push_back(report);
            return config;
        });

    auto cfg = M5.config();
    M5.begin(cfg);

    // M5Unified's virtual BtnA/B/C touch zones (bottom edge of the screen)
    // are OFF by default (_touch_button_height == 0), which reserves a
    // zero-height strip starting exactly at the bottom edge of the
    // screen (y >= 240 on Core2's 240px-tall panel) - i.e. a strip that no
    // touch coordinate can ever fall inside. Without this call, BtnA/B/C
    // NEVER register a press no matter where on screen you tap - only
    // lv_tabview's native swipe gesture works for navigation. This was the
    // root cause of "buttons do not react" reports. 40px carves out
    // y=200-239 as the virtual button strip (BtnA/B/C = left/mid/right
    // thirds of that strip's width).
    M5.setTouchButtonHeight(40);

    // Default hold threshold is 500ms (see Button_Class.hpp) - raised so
    // BtnB's normal click (tab nav/menu) still works for anything shorter,
    // and wasHold() only fires once the diagnostics-toggle gesture's own
    // longer hold duration is reached.
    M5.BtnB.setHoldThresh(kDiagnosticsHoldMs);

    Buzzer::begin();

    // M5.Display.width()/height() (read inside LvglDisplay::begin()) are
    // only valid once M5.begin() has run, so this must come after it - and
    // everything below that touches the screen (SplashView, provisioning's
    // own UI) needs LVGL already owning the display.
    lvglDisplay.begin();

    popupOverlay.begin();
    matrixRainView.begin();
    screenPowerPolicy.begin();

    // NavigationController::begin() (the real tabview) is deliberately NOT
    // created here - it's deferred to handleConfigurationUpdated()'s first
    // call, once a live NodeConfiguration has actually been fetched from
    // the Orchestrator. Until then SplashView is the only thing on screen:
    // this node can't do anything useful without the Orchestrator, so
    // there's no cached/offline tab UI to fall back to (see
    // OrchestratorClient's enableCache=false above).

    config = NodeConfigStore::load();
    HapticFeedback::instance().setEnabled(config.vibrateEnabled);

    if (!config.isValid()) {
        Serial.println("[Config] No valid NodeConfig in NVS. Starting provisioning portal.");
        mode = AppMode::Provisioning;
        provisioning.begin();
        QrSettingsView::build(lv_screen_active(), provisioning.apSsid(), provisioning.apIp());
        return;
    }

    Serial.printf("[Config] Loaded node id=%s name=%s\n", config.id.c_str(), config.name.c_str());
    splashView.build(config.id);

    // Node id/name never change at runtime, so unlike the other
    // NodeConfiguration-driven views this only needs to be built once (see
    // the BtnB hold-to-toggle gesture in loop()).
    diagnosticsView.build(config.id, config.name);

    // Callbacks must be wired before requestConfiguration() can ever run
    // (triggered from loop() once the orchestrator handshake completes), so
    // a fetch immediately builds the real tab UI instead of firing into an
    // unset std::function.
    viewManager.onReport(handleReport);
    peripheralManager.onReport(handleReport);
    orchestratorClient.onConfigurationUpdated(handleConfigurationUpdated);

    // Bridges every view's showPopup() call into the single shared
    // PopupOverlay - ViewManager itself never needs to know PopupOverlay
    // exists (see ViewManager::onPopup()'s doc comment). Alerts/
    // notifications must always be seen, so wakeForAlert() forces the
    // display on and hides the idle MatrixRainView overlay first - see its
    // doc comment for why that's separate from ScreenPowerPolicy's own
    // input-driven wake().
    viewManager.onPopup([](const PopupRequest& request) {
        screenPowerPolicy.wakeForAlert();
        if (request.autoDismiss) {
            popupOverlay.showNotification(request.title, request.message, request.autoDismissMs);
        } else {
            popupOverlay.showAlert(request.title, request.message);
        }
    });

    // Wired unconditionally (cheap - just sets std::function members); the
    // BLE radio itself only actually starts once hasBleConsumer() goes true,
    // see handleConfigurationUpdated().
    BleScanner::instance().onDeviceDiscovered(
        [](const BleDeviceInfo& device) { viewManager.notifyBleDeviceDiscovered(device); });
    BleScanner::instance().onDeviceLost([](const String& address) { viewManager.notifyBleDeviceLost(address); });
    BleScanner::instance().onAdvertisement(
        [](const BleAdvertisement& advertisement) { viewManager.notifyBleAdvertisement(advertisement); });

    wifi.begin(config.wifiSsid, config.wifiPassword);

    // Available as soon as Wi-Fi comes up (served unconditionally, unlike
    // MQTT/orchestrator handshake state) so the orchestrator can fetch this
    // node's device configuration templates via the nodeBaseUrl advertised
    // in NodeOnlineMessage as soon as it sees this node online.
    configTemplateServer.begin([]() {
        std::vector<DeviceConfiguration> templates = ViewFactory::instance().configurationTemplates();
        std::vector<DeviceConfiguration> peripheralTemplates = PeripheralFactory::instance().configurationTemplates();
        templates.insert(templates.end(), peripheralTemplates.begin(), peripheralTemplates.end());
        return templates;
    });

    // SNTP sync so Report.timeStamp is a real Unix epoch; opportunistic, runs
    // once Wi-Fi comes up. Reports published before the first sync completes
    // will carry a small/incorrect timestamp - acceptable for now.
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    mqtt.onCommand(handleCommand);
    mqtt.onOrchestratorOnline(handleOrchestratorOnline);
    mqtt.onConfigurationMessage(handleConfigurationMessage);
    mqtt.setManifestJson(Manifest::json);
    mqtt.begin(config);
}

void loop() {
    M5.update();
    updateScrollSuppression();
    lv_timer_handler();

    if (mode == AppMode::Provisioning) {
        provisioning.loop();
        return;
    }

    wifi.loop();
    mqtt.loop();
    configTemplateServer.loop();

    // Runs the (blocking) configuration fetch requested by
    // handleConfigurationMessage(), outside of mqtt.loop()'s own callback
    // dispatch - see the pendingConfigFetch comment above.
    if (pendingConfigFetch) {
        pendingConfigFetch = false;
        orchestratorClient.requestConfiguration(pendingApiBaseUrl, config.id);
    }

    peripheralManager.loop();

    if (bleActive) {
        BleScanner::instance().loop();
    }

    // Returns true when this call's touch/button activity was consumed
    // purely as a wake gesture (display was dimmed/idle/asleep) - see
    // ScreenPowerPolicy::loop()'s doc comment for why that's used to skip
    // the normal BtnA/B/C dispatch below for this one frame.
    bool wokeFromIdle = screenPowerPolicy.loop();

    if (!wokeFromIdle && tabsInitialized && !touchWasScrollLike) {
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

    // Holding BtnB for kDiagnosticsHoldMs toggles the full-screen
    // DiagnosticsView overlay - wasHold() fires exactly once per hold (see
    // Button_Class::setRawState()), independent of tabsInitialized/
    // wokeFromIdle so it works even while the splash screen is showing.
    if (M5.BtnB.wasHold()) {
        if (diagnosticsView.isVisible()) {
            diagnosticsView.hide();
        } else {
            diagnosticsView.show();
        }
    }

    bool comboPressed = M5.BtnA.isPressed() && M5.BtnC.isPressed();
    if (comboPressed && !factoryResetComboActive) {
        factoryResetComboActive = true;
        factoryResetComboStartMs = millis();
    } else if (!comboPressed) {
        factoryResetComboActive = false;
        factoryResetTriggered = false;
    }
    if (factoryResetComboActive && !factoryResetTriggered &&
        (millis() - factoryResetComboStartMs) >= kFactoryResetHoldMs) {
        factoryResetTriggered = true;
        factoryReset();
        return;
    }

    bool statusChanged = wifi.state() != lastWifiState || mqtt.state() != lastMqttState;
    if (statusChanged) {
        lastWifiState = wifi.state();
        lastMqttState = mqtt.state();
        if (!tabsInitialized) {
            splashView.setWifiStatus(String("WiFi: ") + (wifi.isConnected() ? "connected" : "connecting..."));
            splashView.setMqttStatus(String("MQTT: ") + (mqtt.isConnected() ? "connected" : "connecting..."));
        }
    }

    // DiagnosticsView has no timer of its own - throttled to ~1/s here so
    // its heap/uptime labels stay reasonably fresh without churning LVGL
    // label text on every single loop() iteration. Only bothers refreshing
    // while actually visible.
    if (diagnosticsView.isVisible() && (millis() - lastDiagnosticsUpdateMs) >= 1000) {
        lastDiagnosticsUpdateMs = millis();
        diagnosticsView.update(wifi, mqtt);
    }
}
