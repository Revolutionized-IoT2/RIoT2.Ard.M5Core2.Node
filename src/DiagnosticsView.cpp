#include "DiagnosticsView.h"

#include <WiFi.h>

#include <riot2/MqttConnection.h>
#include <riot2/WifiConnection.h>

#include "AppColors.h"
#include "lv_font_montserrat_18_bpp8.h"

void DiagnosticsView::tapToDismissCb(lv_event_t* event) {
    static_cast<DiagnosticsView*>(lv_event_get_user_data(event))->hide();
}

void DiagnosticsView::build(const String& nodeId, const String& nodeName) {
    _container = lv_obj_create(lv_layer_top());
    lv_obj_remove_style_all(_container);
    lv_obj_set_size(_container, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(_container, 0, 0);
    lv_obj_set_style_bg_color(_container, AppColors::indigoLighten5(), 0);
    lv_obj_set_style_bg_opa(_container, LV_OPA_COVER, 0);
    lv_obj_add_flag(_container, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(_container, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(_container, tapToDismissCb, LV_EVENT_CLICKED, this);

    lv_obj_set_flex_flow(_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_all(_container, 12, 0);
    lv_obj_set_style_pad_row(_container, 4, 0);

    lv_obj_t* title = lv_label_create(_container);
    lv_label_set_text(title, "Diagnostics");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18_bpp8, 0);
    lv_obj_set_style_text_color(title, lv_color_black(), 0);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_pad_bottom(title, 6, 0);

    lv_obj_t* idLabel = lv_label_create(_container);
    lv_label_set_text(idLabel, (String("Node ID: ") + nodeId).c_str());
    lv_obj_set_style_text_color(idLabel, lv_color_black(), 0);
    lv_obj_set_style_text_align(idLabel, LV_TEXT_ALIGN_LEFT, 0);
    lv_label_set_long_mode(idLabel, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_width(idLabel, lv_pct(100));

    lv_obj_t* nameLabel = lv_label_create(_container);
    lv_label_set_text(nameLabel, (String("Name: ") + nodeName).c_str());
    lv_obj_set_style_text_color(nameLabel, lv_color_black(), 0);
    lv_obj_set_style_text_align(nameLabel, LV_TEXT_ALIGN_LEFT, 0);

    _wifiLabel = lv_label_create(_container);
    lv_obj_set_style_text_color(_wifiLabel, lv_color_black(), 0);
    lv_obj_set_style_text_align(_wifiLabel, LV_TEXT_ALIGN_LEFT, 0);

    _mqttLabel = lv_label_create(_container);
    lv_obj_set_style_text_color(_mqttLabel, lv_color_black(), 0);
    lv_obj_set_style_text_align(_mqttLabel, LV_TEXT_ALIGN_LEFT, 0);

    _heapLabel = lv_label_create(_container);
    lv_obj_set_style_text_color(_heapLabel, lv_color_black(), 0);
    lv_obj_set_style_text_align(_heapLabel, LV_TEXT_ALIGN_LEFT, 0);

    _uptimeLabel = lv_label_create(_container);
    lv_obj_set_style_text_color(_uptimeLabel, lv_color_black(), 0);
    lv_obj_set_style_text_align(_uptimeLabel, LV_TEXT_ALIGN_LEFT, 0);
}

void DiagnosticsView::show() {
    lv_obj_remove_flag(_container, LV_OBJ_FLAG_HIDDEN);
}

void DiagnosticsView::hide() {
    lv_obj_add_flag(_container, LV_OBJ_FLAG_HIDDEN);
}

bool DiagnosticsView::isVisible() const {
    return _container != nullptr && !lv_obj_has_flag(_container, LV_OBJ_FLAG_HIDDEN);
}

void DiagnosticsView::update(WifiConnection& wifi, MqttConnection& mqtt) {
    if (_wifiLabel) {
        String text = String("WiFi: ") + (wifi.isConnected() ? "connected" : "connecting...");
        if (wifi.isConnected()) {
            text += " (" + WiFi.SSID() + ", " + String(WiFi.RSSI()) + " dBm)";
        }
        lv_label_set_text(_wifiLabel, text.c_str());
    }
    if (_mqttLabel) {
        String text = String("MQTT: ") + (mqtt.isConnected() ? "connected" : "connecting...");
        lv_label_set_text(_mqttLabel, text.c_str());
    }
    if (_heapLabel) {
        lv_label_set_text(_heapLabel, (String("Free heap: ") + String(ESP.getFreeHeap() / 1024) + " KB").c_str());
    }
    if (_uptimeLabel) {
        lv_label_set_text(_uptimeLabel, (String("Uptime: ") + String(millis() / 1000) + " s").c_str());
    }
}
