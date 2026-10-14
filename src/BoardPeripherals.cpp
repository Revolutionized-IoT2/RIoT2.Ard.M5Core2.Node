#include "BoardPeripherals.h"

#include <memory>

#include <riot2/GpioPeripheral.h>
#include <riot2/PeripheralFactory.h>
#include <riot2/Uuid.h>

#include "Rfid2Peripheral.h"

namespace {
// Core2's Grove PORT.A / PORT.B pin table (GpioPeripheral itself is
// board-agnostic - see riot2/GpioPeripheral.h). B2/GPIO36 is input-only
// (ADC1_CH0) - GpioPeripheral::begin() logs a warning if a commandTemplate
// ever targets it as an OUTPUT.
constexpr GpioPinMap kM5Core2GroveMap{32, 33, 26, 36, 0x07};
}  // namespace

void registerPeripherals() {
    PeripheralFactory::instance().registerCreator(
        "RIoT2.Ard.M5Core2.Node.GpioPeripheral", [] { return std::make_unique<GpioPeripheral>(kM5Core2GroveMap); },
        [] {
            DeviceConfiguration config;
            config.id = riot2::newId();
            config.name = "GPIO Peripheral";
            config.classFullName = "RIoT2.Ard.M5Core2.Node.GpioPeripheral";
            config.deviceParameters = {{"pullup", "true"}, {"invert", "false"}};

            CommandTemplate cmd;
            cmd.id = riot2::newId();
            cmd.type = riot2::ValueType::Boolean;
            cmd.name = "Porch Relay";
            cmd.address = "B1";
            config.commandTemplates.push_back(cmd);
            return config;
        });
    // Mutually exclusive with the GpioPeripheral A1/A2 slot on real hardware
    // (both use Grove PORT.A / GPIO32+33) - see Rfid2Peripheral.h. Configuring
    // both in the same node's configuration is a config error, not a firmware one.
    PeripheralFactory::instance().registerCreator(
        "RIoT2.Ard.M5Core2.Node.Rfid2Peripheral", [] { return std::make_unique<Rfid2Peripheral>(); },
        [] {
            DeviceConfiguration config;
            config.id = riot2::newId();
            config.name = "RFID2 Peripheral";
            config.classFullName = "RIoT2.Ard.M5Core2.Node.Rfid2Peripheral";

            ReportTemplate report;
            report.id = riot2::newId();
            report.type = riot2::ValueType::Text;
            report.name = "RFID Tag";
            config.reportTemplates.push_back(report);
            return config;
        });
}

