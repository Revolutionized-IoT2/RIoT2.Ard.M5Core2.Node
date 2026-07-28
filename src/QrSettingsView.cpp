#include "QrSettingsView.h"

namespace {
constexpr int32_t kQrSize = 160;
}  // namespace

void QrSettingsView::build(lv_obj_t* parent, const String& apSsid, const IPAddress& apIp) {
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    String url = String("http://") + apIp.toString() + "/";

    lv_obj_t* headerLabel = lv_label_create(parent);
    lv_label_set_text(headerLabel, "Scan to set up this node");

    lv_obj_t* qr = lv_qrcode_create(parent);
    lv_qrcode_set_size(qr, kQrSize);
    lv_qrcode_set_dark_color(qr, lv_color_black());
    lv_qrcode_set_light_color(qr, lv_color_white());
    lv_qrcode_set_quiet_zone(qr, true);
    // The AP has no internet uplink of its own to encode a WIFI: join
    // string usefully (this QR code's only job is getting the user to the
    // setup page once they've joined `apSsid` manually) - just the URL.
    lv_qrcode_set_data(qr, url.c_str());

    lv_obj_t* ssidLabel = lv_label_create(parent);
    lv_label_set_text(ssidLabel, (String("WiFi: ") + apSsid).c_str());

    lv_obj_t* urlLabel = lv_label_create(parent);
    lv_label_set_text(urlLabel, url.c_str());
}
