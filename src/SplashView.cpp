#include "SplashView.h"

#include "AppColors.h"
#include "lv_font_montserrat_18_bpp8.h"

void SplashView::build(const String& nodeId) {
    _root = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(_root);
    lv_obj_set_size(_root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(_root, AppColors::indigo(), 0);
    lv_obj_set_style_bg_opa(_root, LV_OPA_COVER, 0);
    lv_obj_set_scrollbar_mode(_root, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_flex_flow(_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(_root, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(_root, 6, 0);

    lv_obj_t* title = lv_label_create(_root);
    lv_label_set_text(title, "RIoT2");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);

    lv_obj_t* nodeIdLabel = lv_label_create(_root);
    lv_label_set_text(nodeIdLabel, (String("Node: ") + nodeId).c_str());
    lv_obj_set_style_text_font(nodeIdLabel, &lv_font_montserrat_18_bpp8, 0);
    lv_obj_set_style_text_color(nodeIdLabel, AppColors::indigoLighten4(), 0);
    lv_obj_set_style_pad_bottom(nodeIdLabel, 10, 0);

    _wifiLabel = lv_label_create(_root);
    lv_obj_set_style_text_font(_wifiLabel, &lv_font_montserrat_18_bpp8, 0);
    lv_obj_set_style_text_color(_wifiLabel, lv_color_white(), 0);

    _mqttLabel = lv_label_create(_root);
    lv_obj_set_style_text_font(_mqttLabel, &lv_font_montserrat_18_bpp8, 0);
    lv_obj_set_style_text_color(_mqttLabel, lv_color_white(), 0);

    _configLabel = lv_label_create(_root);
    lv_obj_set_style_text_font(_configLabel, &lv_font_montserrat_18_bpp8, 0);
    lv_obj_set_style_text_color(_configLabel, lv_color_white(), 0);

    setWifiStatus("WiFi: connecting...");
    setMqttStatus("MQTT: waiting for WiFi");
    setConfigStatus("Config: waiting for Orchestrator");
}

void SplashView::setWifiStatus(const String& text) {
    if (_wifiLabel) {
        lv_label_set_text(_wifiLabel, text.c_str());
    }
}

void SplashView::setMqttStatus(const String& text) {
    if (_mqttLabel) {
        lv_label_set_text(_mqttLabel, text.c_str());
    }
}

void SplashView::setConfigStatus(const String& text) {
    if (_configLabel) {
        lv_label_set_text(_configLabel, text.c_str());
    }
}

void SplashView::destroy() {
    if (_root) {
        lv_obj_delete(_root);
        _root = nullptr;
        _wifiLabel = nullptr;
        _mqttLabel = nullptr;
        _configLabel = nullptr;
    }
}
