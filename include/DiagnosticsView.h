#pragma once

#include <Arduino.h>
#include <lvgl.h>

class WifiConnection;
class MqttConnection;

// Full-screen overlay on lv_layer_top() (not a tab, not in the main menu
// grid), shown by holding the middle virtual button (BtnB) for
// kDiagnosticsHoldMs - see main.cpp - and dismissed by tapping it. Core2's
// analog of RIoT2.Ard.M5Dial.Node's hold-button diagnostics overlay. Shows
// live node/connectivity status: node id, node name, Wi-Fi state/SSID/RSSI,
// MQTT state, free heap and uptime.
class DiagnosticsView {
public:
    // Creates the (hidden) overlay. Call once from setup(), after
    // LvglDisplay::begin() - node id/name don't change at runtime so this
    // doesn't need rebuilding on (re)configuration pushes.
    void build(const String& nodeId, const String& nodeName);

    void show();
    void hide();
    bool isVisible() const;

    // Refreshes the live status labels - call periodically from loop()
    // while visible (this class has no timer of its own). Takes non-const
    // references since MqttConnection::isConnected() itself isn't const.
    void update(WifiConnection& wifi, MqttConnection& mqtt);

private:
    lv_obj_t* _container = nullptr;
    lv_obj_t* _wifiLabel = nullptr;
    lv_obj_t* _mqttLabel = nullptr;
    lv_obj_t* _heapLabel = nullptr;
    lv_obj_t* _uptimeLabel = nullptr;

    static void tapToDismissCb(lv_event_t* event);
};
