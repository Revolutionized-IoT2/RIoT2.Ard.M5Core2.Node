#include "NotificationView.h"

#include <memory>

#include "Buzzer.h"
#include "HapticFeedback.h"
#include "ViewFactory.h"

namespace {
// Pulls a human-readable title/message out of a Command's loosely-typed
// value: a plain string is used as the message directly, an object looks
// for "title"/"message" (falling back to "text") keys, anything else (or a
// missing key) falls back to the supplied default.
String extractField(const JsonVariantConst& value, const char* key, const String& fallback) {
    if (value.is<JsonObjectConst>()) {
        JsonObjectConst obj = value.as<JsonObjectConst>();
        if (obj[key].is<const char*>()) {
            return obj[key].as<String>();
        }
    }
    return fallback;
}

String extractMessage(const JsonVariantConst& value, const String& fallback) {
    if (value.is<const char*>()) {
        return value.as<String>();
    }
    return extractField(value, "message", extractField(value, "text", fallback));
}

// Reads an optional "soundEnabled" field out of the Command's `value` object.
// Accepts a real JSON boolean (expected) or a "true"/"false" string, so it
// tolerates either representation from the orchestrator.
bool extractSoundEnabled(const JsonVariantConst& value, bool fallback) {
    if (value.is<JsonObjectConst>()) {
        JsonVariantConst v = value.as<JsonObjectConst>()["soundEnabled"];
        if (v.is<bool>()) {
            return v.as<bool>();
        }
        if (v.is<const char*>()) {
            return String(v.as<const char*>()).equalsIgnoreCase("true");
        }
    }
    return fallback;
}
}  // namespace

void NotificationView::begin(const DeviceConfiguration& config) {
    String durationParam = findParameter(config.deviceParameters, "durationMs", "");
    _durationMs = durationParam.length() > 0 ? static_cast<unsigned long>(durationParam.toInt()) : 4000;
    if (_durationMs == 0) {
        _durationMs = 4000;
    }
}

void NotificationView::onCommand(const Command& command) {
    String title = extractField(command.value, "title", "Notification");
    String message = extractMessage(command.value, "");
    if (extractSoundEnabled(command.value, false)) {
        Buzzer::confirm();
    }
    // See AlertView::onCommand()'s identical comment - vibration is a
    // separate modality from the command's own soundEnabled field.
    HapticFeedback::instance().confirm();
    showPopup(PopupRequest{title, message, /*autoDismiss=*/true, static_cast<uint32_t>(_durationMs)});
}

namespace {
struct NotificationViewRegistrar {
    NotificationViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Core2.Node.NotificationView",
                                                 []() { return std::make_unique<NotificationView>(); });
    }
} notificationViewRegistrar;
}  // namespace
