#include "AlertView.h"

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

void AlertView::begin(const DeviceConfiguration& config) {
    (void)config;
}

void AlertView::onCommand(const Command& command) {
    String title = extractField(command.value, "title", "Alert");
    String message = extractMessage(command.value, "");
    if (extractSoundEnabled(command.value, false)) {
        Buzzer::alert();
    }
    // Unlike the sound above, vibration isn't gated by the command's own
    // soundEnabled field - it's a separate modality gated only by
    // NodeConfig::vibrateEnabled (see HapticFeedback::setEnabled(), set once
    // in main.cpp), so an alert still gets a physical buzz even when a
    // caller has explicitly silenced its tone.
    HapticFeedback::instance().alert();
    showPopup(PopupRequest{title, message, /*autoDismiss=*/false, 0});
}

namespace {
struct AlertViewRegistrar {
    AlertViewRegistrar() {
        ViewFactory::instance().registerCreator("RIoT2.Ard.M5Core2.Node.AlertView",
                                                 []() { return std::make_unique<AlertView>(); });
    }
} alertViewRegistrar;
}  // namespace
