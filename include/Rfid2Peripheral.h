#pragma once

#include <Arduino.h>
#include <Wire.h>

#include <riot2/IPeripheral.h>

// Polls M5Stack's optional RFID2 Grove unit (WS1850S chip - a register- and
// protocol-compatible clone of NXP's MFRC522, wired over I2C instead of
// SPI) and publishes each newly-seen tag's UID (hex string, e.g.
// "04A3B2C1") as a Report for this device's first reportTemplate. No
// display takeover: IPeripheral has no rendering hooks by design (see
// plan.md Section 6) - an on-screen popup-on-tag-read would need the
// orchestrator to react to the Report and push an AlertView/NotificationView
// Command back instead.
//
// Hardware notes (both unverified against real hardware - flag if either
// turns out wrong):
//   - The RFID2 unit plugs into Grove PORT.A, whose pins (G32/G33 on Core2)
//     are the exact same physical pins GpioPeripheral's "A1"/"A2" addresses
//     use for plain digital GPIO (see kM5Core2GroveMap in main.cpp) - the
//     two peripherals are mutually exclusive on real hardware, since only
//     one device can be plugged into a single Grove port at a time. This
//     isn't a firmware bug to fix, just a real constraint to document: don't
//     configure both an Rfid2Peripheral and a GpioPeripheral A1/A2 slot in
//     the same node's configuration.
//   - Only single-cascade-level anticollision is implemented, i.e. only
//     4-byte-UID cards (MIFARE Classic/Ultralight) are supported. 7-byte UID
//     cards (NTAG21x, DESFire, etc.) reply to anticollision with a 0x88
//     cascade-tag byte that this driver doesn't follow into cascade level 2
//     - such cards are silently ignored (requestA()/anticollision() below
//     return false for them, same as "no card present").
class Rfid2Peripheral : public IPeripheral {
public:
    void begin(const DeviceConfiguration& config) override;
    void loop() override;

private:
    static constexpr uint8_t kI2cAddress = 0x28;
    // Same pins as Grove PORT.A's A1/A2 GPIO addresses (see the class
    // comment above) - Core2-specific, matching kM5Core2GroveMap in main.cpp.
    static constexpr int kSdaPin = 32;
    static constexpr int kSclPin = 33;
    // Re-poll interval - fast enough to feel responsive for a tap-a-tag
    // interaction, slow enough not to flood the I2C bus/CPU.
    static constexpr unsigned long kPollIntervalMs = 200;

    String _reportId;
    unsigned long _lastPollMs = 0;
    String _lastUidHex;  // last UID reported, so a tag held in place isn't re-reported every poll

    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    void setBitMask(uint8_t reg, uint8_t mask);
    void clearBitMask(uint8_t reg, uint8_t mask);
    void softReset();
    void antennaOn();

    // Sends a REQA (7-bit short frame) and returns true if a PICC answered
    // (its 2-byte ATQA is discarded - not needed for UID-only reporting).
    bool requestA();
    // Cascade-level-1 anticollision: returns the 4-byte UID (with BCC
    // already verified) in `uid`, true on success.
    bool anticollision(uint8_t uid[4]);
    // Low-level FIFO transceive: writes `sendData` (via BitFramingReg for the
    // trailing `sendBits` count, 0 = whole bytes), starts PCD_TRANSCEIVE,
    // waits for RxIRq/timeout, and reads back into `backData`. Mirrors the
    // minimal subset of the standard MFRC522 PCD_CommunicateWithPICC
    // sequence needed for REQA + anticollision.
    bool transceive(const uint8_t* sendData, uint8_t sendLen, uint8_t sendBits, uint8_t* backData,
                     uint8_t& backLen);
};
