#include "ViewManager.h"

#include <riot2/PeripheralFactory.h>

#include "ViewFactory.h"

void ViewManager::rebuild(const NodeConfiguration& nodeConfiguration, NavigationController& navigationController) {
    navigationController.clearTabs();
    _entries.clear();

    // Collected alongside the addTab() calls below so each entry's
    // tabIndex matches the tab lv_tabview actually created for it (alert
    // views never get a tab and are excluded here too) - see
    // NavigationController::setMenuEntries()'s doc comment.
    std::vector<NavigationController::MenuEntry> menuEntries;

    for (const auto& deviceConfig : nodeConfiguration.deviceConfigurations) {
        auto view = ViewFactory::instance().create(deviceConfig.classFullName);
        if (!view) {
            // Views and Grove-port peripherals share this same
            // deviceConfigurations array, distinguished purely by
            // classFullName - silently skip any entry PeripheralFactory
            // owns instead of logging it as an unrecognized view.
            if (!PeripheralFactory::instance().isRegistered(deviceConfig.classFullName)) {
                Serial.printf("[ViewManager] No view registered for classFullName=%s (name=%s), skipping\n",
                              deviceConfig.classFullName.c_str(), deviceConfig.name.c_str());
            }
            continue;
        }

        view->setReportCallback(_reportCallback);
        view->setPopupCallback(_popupCallback);
        view->begin(deviceConfig);

        if (!view->isAlert()) {
            lv_obj_t* tab = navigationController.addTab(deviceConfig.name);
            view->buildUi(tab);
            menuEntries.push_back({deviceConfig.name, deviceConfig.classFullName,
                                    static_cast<uint32_t>(menuEntries.size())});
        }

        Entry entry;
        entry.config = deviceConfig;
        entry.view = std::move(view);
        _entries.push_back(std::move(entry));
    }

    navigationController.setMenuEntries(std::move(menuEntries));

    Serial.printf("[ViewManager] Rebuilt with %u view(s)\n", static_cast<unsigned>(_entries.size()));
}

bool ViewManager::onCommand(const String& commandId, const Command& command) {
    for (auto& entry : _entries) {
        for (const auto& cmdTemplate : entry.config.commandTemplates) {
            if (cmdTemplate.id == commandId) {
                entry.view->onCommand(command);
                return true;
            }
        }
    }
    return false;
}

bool ViewManager::hasBleConsumer() const {
    for (const auto& entry : _entries) {
        if (entry.view->consumesBleEvents()) {
            return true;
        }
    }
    return false;
}

void ViewManager::notifyBleDeviceDiscovered(const BleDeviceInfo& device) {
    for (auto& entry : _entries) {
        if (entry.view->consumesBleEvents()) {
            entry.view->onBleDeviceDiscovered(device);
        }
    }
}

void ViewManager::notifyBleDeviceLost(const String& address) {
    for (auto& entry : _entries) {
        if (entry.view->consumesBleEvents()) {
            entry.view->onBleDeviceLost(address);
        }
    }
}

void ViewManager::notifyBleAdvertisement(const BleAdvertisement& advertisement) {
    for (auto& entry : _entries) {
        if (entry.view->consumesBleEvents()) {
            entry.view->onBleAdvertisement(advertisement);
        }
    }
}
