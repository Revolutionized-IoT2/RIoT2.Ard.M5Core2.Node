#include "BoardPeripherals.h"

#include <memory>

#include <riot2/GpioPeripheral.h>
#include <riot2/PeripheralFactory.h>
#include <riot2/Uuid.h>
#include <riot2/WiegandI2CPeripheral.h>

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
    // A RIoT2.Ard.WiegandI2C module on Grove PORT.A (A1 SDA, A2 SCL), optionally with its IRQ on
    // B1 or B2 - see riot2/WiegandI2CPeripheral.h. Like Rfid2Peripheral, it
    // can't share PORT.A with another peripheral.
    PeripheralFactory::instance().registerCreator(
        "RIoT2.Ard.M5Core2.Node.WiegandI2CPeripheral", [] { return std::make_unique<WiegandI2CPeripheral>(kM5Core2GroveMap); },
        [] {
            DeviceConfiguration config;
            config.id = riot2::newId();
            config.name = "Wiegand Reader";
            config.classFullName = "RIoT2.Ard.M5Core2.Node.WiegandI2CPeripheral";
            config.deviceParameters = {{"address", "0x26"}, {"irq", ""}, {"format", "hex"}};

            ReportTemplate report;
            report.id = riot2::newId();
            report.type = riot2::ValueType::Text;
            report.name = "Card";
            report.address = "code";
            config.reportTemplates.push_back(report);
            return config;
        });
}
