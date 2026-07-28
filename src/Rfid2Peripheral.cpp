#include "Rfid2Peripheral.h"

#include <memory>

#include <riot2/PeripheralFactory.h>

namespace {
// MFRC522 register map (subset actually used below) - the WS1850S on
// M5Stack's RFID2 unit is register-compatible with NXP's MFRC522, just
// bridged over I2C instead of SPI, so register numbers are unshifted
// (unlike the SPI datasheet's "address << 1" convention).
constexpr uint8_t kCommandReg = 0x01;
constexpr uint8_t kComIrqReg = 0x04;
constexpr uint8_t kErrorReg = 0x06;
constexpr uint8_t kFIFODataReg = 0x09;
constexpr uint8_t kFIFOLevelReg = 0x0A;
constexpr uint8_t kBitFramingReg = 0x0D;
constexpr uint8_t kCollReg = 0x0E;
constexpr uint8_t kModeReg = 0x11;
constexpr uint8_t kTxControlReg = 0x14;
constexpr uint8_t kTxASKReg = 0x15;
constexpr uint8_t kTModeReg = 0x2A;
constexpr uint8_t kTPrescalerReg = 0x2B;
constexpr uint8_t kTReloadRegH = 0x2C;
constexpr uint8_t kTReloadRegL = 0x2D;

constexpr uint8_t kPcdIdle = 0x00;
constexpr uint8_t kPcdTransceive = 0x0C;
constexpr uint8_t kPcdResetPhase = 0x0F;

constexpr uint8_t kPiccCmdReqA = 0x26;
constexpr uint8_t kPiccCmdSelCl1 = 0x93;

constexpr unsigned long kTransceiveTimeoutMs = 50;
}  // namespace

void Rfid2Peripheral::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(kI2cAddress);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

uint8_t Rfid2Peripheral::readRegister(uint8_t reg) {
    Wire.beginTransmission(kI2cAddress);
    Wire.write(reg);
    Wire.endTransmission(false);  // repeated start, keep the bus for the read below
    Wire.requestFrom(static_cast<int>(kI2cAddress), 1);
    return Wire.available() ? static_cast<uint8_t>(Wire.read()) : 0;
}

void Rfid2Peripheral::setBitMask(uint8_t reg, uint8_t mask) {
    writeRegister(reg, readRegister(reg) | mask);
}

void Rfid2Peripheral::clearBitMask(uint8_t reg, uint8_t mask) {
    writeRegister(reg, readRegister(reg) & ~mask);
}

void Rfid2Peripheral::softReset() {
    writeRegister(kCommandReg, kPcdResetPhase);
    delay(50);
}

void Rfid2Peripheral::antennaOn() {
    setBitMask(kTxControlReg, 0x03);
}

void Rfid2Peripheral::begin(const DeviceConfiguration& config) {
    _reportId = config.reportTemplates.empty() ? String() : config.reportTemplates.front().id;
    _lastUidHex = "";
    _lastPollMs = 0;

    Wire.begin(kSdaPin, kSclPin);

    softReset();
    writeRegister(kTModeReg, 0x8D);
    writeRegister(kTPrescalerReg, 0x3E);
    writeRegister(kTReloadRegL, 30);
    writeRegister(kTReloadRegH, 0);
    writeRegister(kTxASKReg, 0x40);
    writeRegister(kModeReg, 0x3D);
    antennaOn();
}

bool Rfid2Peripheral::transceive(const uint8_t* sendData, uint8_t sendLen, uint8_t sendBits, uint8_t* backData,
                                  uint8_t& backLen) {
    writeRegister(kCommandReg, kPcdIdle);
    writeRegister(kComIrqReg, 0x7F);       // clear all interrupt request bits
    setBitMask(kFIFOLevelReg, 0x80);       // flush the FIFO

    for (uint8_t i = 0; i < sendLen; ++i) {
        writeRegister(kFIFODataReg, sendData[i]);
    }

    writeRegister(kBitFramingReg, sendBits);  // TxLastBits = sendBits (0 = whole bytes), StartSend not set yet
    writeRegister(kCommandReg, kPcdTransceive);
    setBitMask(kBitFramingReg, 0x80);  // StartSend = 1

    unsigned long start = millis();
    bool completed = false;
    while (millis() - start < kTransceiveTimeoutMs) {
        uint8_t irq = readRegister(kComIrqReg);
        if (irq & 0x30) {  // RxIRq or IdleIRq
            completed = true;
            break;
        }
        if (irq & 0x01) {  // TimerIRq - the PICC didn't answer in time
            break;
        }
    }
    clearBitMask(kBitFramingReg, 0x80);

    if (!completed) {
        return false;
    }

    uint8_t error = readRegister(kErrorReg);
    if (error & 0x1B) {  // BufferOvfl | ParityErr | ProtocolErr | CollErr
        return false;
    }

    uint8_t fifoLevel = readRegister(kFIFOLevelReg);
    if (fifoLevel > backLen) {
        fifoLevel = backLen;  // clip to the caller's buffer capacity
    }
    for (uint8_t i = 0; i < fifoLevel; ++i) {
        backData[i] = readRegister(kFIFODataReg);
    }
    backLen = fifoLevel;
    return true;
}

bool Rfid2Peripheral::requestA() {
    clearBitMask(kCollReg, 0x80);  // ValuesAfterColl = 0, so anticollision below can read partial bytes cleanly

    uint8_t reqa = kPiccCmdReqA;
    uint8_t atqa[2];
    uint8_t backLen = sizeof(atqa);
    // REQA is a 7-bit short frame, not a whole byte.
    return transceive(&reqa, 1, 7, atqa, backLen) && backLen == 2;
}

bool Rfid2Peripheral::anticollision(uint8_t uid[4]) {
    writeRegister(kBitFramingReg, 0x00);  // whole bytes from here on

    // SEL (cascade level 1) + NVB=0x20 asks every present PICC to reply with
    // its full UID (cascade level 1 only - 7-byte-UID cards would reply with
    // a leading 0x88 cascade tag, which this driver doesn't chase into level
    // 2, see the class comment).
    uint8_t command[2] = {kPiccCmdSelCl1, 0x20};
    uint8_t response[5];  // 4 UID bytes + BCC
    uint8_t backLen = sizeof(response);

    if (!transceive(command, sizeof(command), 0, response, backLen) || backLen != 5) {
        return false;
    }

    uint8_t bcc = response[0] ^ response[1] ^ response[2] ^ response[3];
    if (bcc != response[4]) {
        return false;
    }
    if (response[0] == 0x88) {
        return false;  // cascade tag -> 7-byte UID, unsupported
    }

    memcpy(uid, response, 4);
    return true;
}

void Rfid2Peripheral::loop() {
    unsigned long now = millis();
    if (now - _lastPollMs < kPollIntervalMs) {
        return;
    }
    _lastPollMs = now;

    if (!requestA()) {
        _lastUidHex = "";  // no tag in range - allow the next tag seen (even the same one) to be reported again
        return;
    }

    uint8_t uid[4];
    if (!anticollision(uid)) {
        return;
    }

    char hex[9];
    snprintf(hex, sizeof(hex), "%02X%02X%02X%02X", uid[0], uid[1], uid[2], uid[3]);
    String uidHex(hex);
    if (uidHex == _lastUidHex) {
        return;  // same tag still sitting on the reader, already reported
    }
    _lastUidHex = uidHex;

    if (_reportId.length() > 0) {
        publishReport(Report{_reportId, String("\"") + uidHex + "\""});
    }
}
