#include "BLEView.h"

#include <ArduinoJson.h>

#include <memory>

#include <riot2/Uuid.h>

#include "ViewFactory.h"

namespace {
// Resolves which reportTemplate (if any) should be used for one of BLEView's
// three report scenarios: prefers an exact match on `address`, falling back
// to the given positional index so a minimal configuration that just lists
// up to three reportTemplates in order (without bothering to set `address`)
// still works. Returns a pointer into `templates` (not a copy) so callers
// can also read that specific reportTemplate's own `parameters` (e.g. its
// `allowedAddresses` filter); nullptr if no match/fallback exists.
const ReportTemplate* resolveReportTemplate(const std::vector<ReportTemplate>& templates, const String& address,
                                             size_t fallbackIndex) {
    for (const auto& tmpl : templates) {
        if (tmpl.address == address) {
            return &tmpl;
        }
    }
    return fallbackIndex < templates.size() ? &templates[fallbackIndex] : nullptr;
}

// Builds the JSON report value for a discovered/updated device. Uses
// ArduinoJson's serializer (rather than hand-built string concatenation) so
// the advertised `name` - untrusted data broadcast over the air by whatever
// nearby device - is always properly JSON-escaped.
String deviceJson(const String& address, const String& name, int rssi) {
    JsonDocument doc;
    doc["address"] = address;
    doc["name"] = name;
    doc["rssi"] = rssi;
    String out;
    serializeJson(doc, out);
    return out;
}

String advertisementJson(const BleAdvertisement& advertisement) {
    JsonDocument doc;
    doc["address"] = advertisement.address;
    doc["name"] = advertisement.name;
    doc["rssi"] = advertisement.rssi;
    doc["manufacturerData"] = advertisement.manufacturerDataHex;
    String out;
    serializeJson(doc, out);
    return out;
}

// Splits the `allowedAddresses` deviceParameter (comma-separated MAC
// addresses) into a list of trimmed, non-empty tokens. An empty/blank input
// yields an empty vector.
std::vector<String> splitAddressList(const String& raw) {
    std::vector<String> result;
    int start = 0;
    while (start <= static_cast<int>(raw.length())) {
        int comma = raw.indexOf(',', start);
        String token = (comma == -1) ? raw.substring(start) : raw.substring(start, comma);
        token.trim();
        if (token.length() > 0) {
            result.push_back(token);
        }
        if (comma == -1) {
            break;
        }
        start = comma + 1;
    }
    return result;
}
}  // namespace

void BLEView::begin(const DeviceConfiguration& config) {
    _header = findParameter(config.deviceParameters, "header", "Nearby BLE");

    const ReportTemplate* deviceFoundTmpl = resolveReportTemplate(config.reportTemplates, "deviceFound", 0);
    const ReportTemplate* deviceLostTmpl = resolveReportTemplate(config.reportTemplates, "deviceLost", 1);
    const ReportTemplate* advertisementTmpl = resolveReportTemplate(config.reportTemplates, "advertisement", 2);

    _deviceFoundReportId = deviceFoundTmpl ? deviceFoundTmpl->id : String();
    _deviceLostReportId = deviceLostTmpl ? deviceLostTmpl->id : String();
    _advertisementReportId = advertisementTmpl ? advertisementTmpl->id : String();

    _deviceFoundAllowedAddresses =
        deviceFoundTmpl ? splitAddressList(findParameter(deviceFoundTmpl->parameters, "allowedAddresses", ""))
                        : std::vector<String>();
    _deviceLostAllowedAddresses =
        deviceLostTmpl ? splitAddressList(findParameter(deviceLostTmpl->parameters, "allowedAddresses", ""))
                       : std::vector<String>();
    _advertisementAllowedAddresses =
        advertisementTmpl ? splitAddressList(findParameter(advertisementTmpl->parameters, "allowedAddresses", ""))
                          : std::vector<String>();

    _devices.clear();
}

bool BLEView::isAddressAllowed(const std::vector<String>& allowedAddresses, const String& address) {
    if (allowedAddresses.empty()) {
        return true;
    }
    for (const auto& allowed : allowedAddresses) {
        if (allowed.equalsIgnoreCase(address)) {
            return true;
        }
    }
    return false;
}

void BLEView::buildUi(lv_obj_t* parent) {
    lv_obj_t* content = buildHeaderArea(parent, _header, String());
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    _list = lv_list_create(content);
    lv_obj_set_size(_list, lv_pct(100), lv_pct(100));

    refreshList();
}

void BLEView::refreshList() {
    if (!_list) {
        return;
    }
    lv_obj_clean(_list);

    if (_devices.empty()) {
        lv_list_add_text(_list, "No devices found");
        return;
    }

    for (const auto& device : _devices) {
        String label = device.name.length() > 0 ? device.name : device.address;
        label += "  (" + String(device.rssi) + " dBm)";
        lv_list_add_button(_list, nullptr, label.c_str());
    }
}

void BLEView::onBleDeviceDiscovered(const BleDeviceInfo& device) {
    _devices.push_back(device);
    refreshList();

    if (_deviceFoundReportId.length() > 0 && isAddressAllowed(_deviceFoundAllowedAddresses, device.address)) {
        publishReport(Report{_deviceFoundReportId, deviceJson(device.address, device.name, device.rssi)});
    }
}

void BLEView::onBleDeviceLost(const String& address) {
    for (size_t i = 0; i < _devices.size(); ++i) {
        if (_devices[i].address == address) {
            _devices.erase(_devices.begin() + i);
            break;
        }
    }
    refreshList();

    if (_deviceLostReportId.length() > 0 && isAddressAllowed(_deviceLostAllowedAddresses, address)) {
        // `address` is always a colon-separated hex MAC (from BLEAddress::toString()),
        // so it's safe to embed directly as a quoted JSON string literal.
        publishReport(Report{_deviceLostReportId, String("\"") + address + "\""});
    }
}

void BLEView::onBleAdvertisement(const BleAdvertisement& advertisement) {
    if (_advertisementReportId.length() > 0 &&
        isAddressAllowed(_advertisementAllowedAddresses, advertisement.address)) {
        publishReport(Report{_advertisementReportId, advertisementJson(advertisement)});
    }
}

namespace {
DeviceConfiguration buildBLEViewTemplate() {
    DeviceConfiguration config;
    config.id = riot2::newId();
    config.name = "BLE View";
    config.classFullName = "RIoT2.Ard.M5Core2.Node.BLEView";
    config.deviceParameters = {{"header", "Nearby BLE"}};

    for (const auto& entry :
         {std::make_pair("deviceFound", "BLE Device Found"), std::make_pair("deviceLost", "BLE Device Lost"),
          std::make_pair("advertisement", "BLE Advertisement")}) {
        ReportTemplate report;
        report.id = riot2::newId();
        report.type = "1";
        report.name = entry.second;
        report.address = entry.first;
        report.parameters = {{"allowedAddresses", ""}};
        config.reportTemplates.push_back(report);
    }
    return config;
}

struct BLEViewRegistrar {
    BLEViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Core2.Node.BLEView",
                                                 []() { return std::make_unique<BLEView>(); }, buildBLEViewTemplate);
    }
} bleViewRegistrar;
}  // namespace
