#include "NotificationView.h"

#include <memory>

#include "Buzzer.h"
#include "HapticFeedback.h"
#include "ViewFactory.h"

void NotificationView::begin(const DeviceConfiguration& config) {
    _model.begin(config);
}

void NotificationView::onCommand(const Command& command) {
    _model.onCommand(command);
    if (_model.soundEnabled()) {
        Buzzer::confirm();
    }
    // See AlertView::onCommand()'s identical comment - vibration is a
    // separate modality from the command's own soundEnabled field.
    HapticFeedback::instance().confirm();
    showPopup(PopupRequest{_model.title(), _model.message(), /*autoDismiss=*/true,
                           static_cast<uint32_t>(_model.durationMs())});
}

namespace {
struct NotificationViewRegistrar {
    NotificationViewRegistrar() {
        ViewFactory::instance().registerCreator(
            "RIoT2.Ard.M5Core2.Node.NotificationView", []() { return std::make_unique<NotificationView>(); },
            []() { return riot2::views::NotificationModel::buildTemplate("RIoT2.Ard.M5Core2.Node.NotificationView"); });
    }
} notificationViewRegistrar;
}  // namespace
