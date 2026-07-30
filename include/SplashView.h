#pragma once

#include <Arduino.h>
#include <lvgl.h>

// Full-screen boot splash shown from setup() until this node's first
// NodeConfiguration is actually fetched from the Orchestrator (see
// main.cpp's handleConfigurationUpdated()) - built directly on
// lv_screen_active(), before NavigationController::begin() ever creates a
// tabview. There is no cached/offline fallback UI anymore (see
// OrchestratorClient's enableCache=false in main.cpp): without a reachable
// Orchestrator this node can't do anything useful, so the splash (with live
// Wi-Fi/MQTT/configuration status) is what stays on screen for as long as
// that takes, rather than an empty or stale tab UI.
class SplashView {
public:
    void build(const String& nodeId);

    void setWifiStatus(const String& text);
    void setMqttStatus(const String& text);
    void setConfigStatus(const String& text);

    // Tears down the splash screen's LVGL objects - called once the first
    // real NodeConfiguration arrives and the tab UI takes over.
    void destroy();

private:
    lv_obj_t* _root = nullptr;
    lv_obj_t* _wifiLabel = nullptr;
    lv_obj_t* _mqttLabel = nullptr;
    lv_obj_t* _configLabel = nullptr;
};
