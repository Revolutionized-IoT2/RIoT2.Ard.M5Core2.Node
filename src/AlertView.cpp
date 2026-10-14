#include "AlertView.h"

#include <memory>

#include "Buzzer.h"
#include "HapticFeedback.h"
#include "ViewFactory.h"

void AlertView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
}

void AlertView::onCommand(const Command& command) {
    _model.onCommand(command);
    if (_model.soundEnabled()) {
        Buzzer::alert();
    }
    // Unlike the sound above, vibration isn't gated by the command's own
    // soundEnabled field - it's a separate modality gated only by
    // NodeConfig::vibrateEnabled (see HapticFeedback::setEnabled(), set once
    // in main.cpp), so an alert still gets a physical buzz even when a
    // caller has explicitly silenced its tone.
    HapticFeedback::instance().alert();
    showPopup(PopupRequest{_model.title(), _model.message(), /*autoDismiss=*/false, 0});
}

namespace {
struct AlertViewRegistrar {
    AlertViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Core2.Node.AlertView", []() { return std::make_unique<AlertView>(); },
            []() { return riot2::views::AlertModel::buildTemplate("RIoT2.Ard.M5Core2.Node.AlertView"); });
    }
} alertViewRegistrar;
}  // namespace
