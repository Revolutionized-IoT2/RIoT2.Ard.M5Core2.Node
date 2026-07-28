#pragma once

#include <Arduino.h>
#include <IPAddress.h>
#include <lvgl.h>

// Renders the provisioning captive-portal's AP SSID and setup URL as an
// lv_qrcode, plus plain-text fallbacks - built once directly into the
// "Status" tab's content area during AppMode::Provisioning (see main.cpp),
// not registered with ViewFactory since it isn't a NodeConfiguration-driven
// IView; there's no orchestrator configuration to speak of yet at this point
// in the boot sequence.
namespace QrSettingsView {

void build(lv_obj_t* parent, const String& apSsid, const IPAddress& apIp);

}  // namespace QrSettingsView
