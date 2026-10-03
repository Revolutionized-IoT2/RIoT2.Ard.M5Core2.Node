# RIoT2.Ard.M5Core2.Node

PlatformIO/Arduino firmware for the [M5Stack Core2](https://docs.m5stack.com/en/core/core2) as a
RIoT2 device node. It connects to Wi-Fi and MQTT, announces itself to the orchestrator, fetches
device configuration and renders an LVGL touchscreen UI for views such as buttons, sliders,
timers, scenes, BLE and RFID-derived peripherals.

Most connectivity and protocol code lives in
[RIoT2.Ard.Shared](https://github.com/Revolutionized-IoT2/RIoT2.Ard.Shared). This repository owns
the Core2-specific LVGL UI, touchscreen navigation, screen power policy, QR setup screen,
vibration feedback and Grove pin map.

## Hardware and prerequisites

- M5Stack Core2 and USB-C cable.
- PlatformIO, either the VS Code extension or CLI.
- The sibling [RIoT2.Ard.Shared](https://github.com/Revolutionized-IoT2/RIoT2.Ard.Shared)
  repository checked out next to this one.
- Windows USB-serial drivers if the Core2 is not detected automatically.

PlatformIO restores libraries from [platformio.ini](platformio.ini): `M5Unified`, `lvgl`,
`PubSubClient`, `ArduinoJson` and `NimBLE-Arduino`.

## Build

Run from the workspace root (`C:\Src\RIoT2`) in PowerShell:

```powershell
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
& $pio run -d .\RIoT2.Ard.M5Core2.Node
```

The firmware image is produced under `.pio\build\m5stack-core2\firmware.bin`.

To upload only when a device is connected and you intend to flash it:

```powershell
& $pio run -d .\RIoT2.Ard.M5Core2.Node -t upload
& $pio device monitor -d .\RIoT2.Ard.M5Core2.Node
```

Host regressions are in the shared repository:

```powershell
Set-Location C:\Src\RIoT2\RIoT2.Ard.Shared
python .\tests\test_firmware_p1.py
python .\tests\test_firmware_p2.py
python .\tests\test_firmware_architecture.py
```

## First boot provisioning

The firmware ships without Wi-Fi or MQTT credentials. On first boot, or after factory reset, it
starts an open setup access point named `RIoT2-Setup-XXXX`. The Core2 screen shows the setup URL
and a QR code.

Connect to the setup AP and open `http://192.168.4.1/`. The portal stores:

- node id and name;
- Wi-Fi SSID and password;
- MQTT URL, username and password;
- MQTT TLS flag;
- vibration feedback flag.

Settings are stored in ESP32 NVS namespace `riot2node`; see the hub
[firmware settings contract](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/env-vars.md#firmware-m5core2-m5dial).

To factory-reset provisioning, hold **BtnA and BtnC together** for about five seconds.

## UI and operation

- LVGL renders one tab per configured view with a bottom virtual button strip.
- `BtnA` selects the previous tab, `BtnC` selects the next tab and `BtnB` dismisses a popup or
  returns to the first tab.
- Hold `BtnB` for about three seconds to toggle diagnostics.
- After one minute of no touch/button input, the screen dims and shows Matrix Rain. After four
  minutes, the panel sleeps. The first wake input is swallowed.
- The vibration motor is enabled by the provisioning setting and is separate from per-command sound.
- The Core2 Grove map is A1/GPIO32, A2/GPIO33, B1/GPIO26 and B2/GPIO36. B2 is input-only; output
  attempts are logged and ignored.

## OTA

Firmware OTA is triggered by an MQTT command to `riot2/node/{id}/command`:

```json
{ "id": "system.ota", "value": "https://host/path/to/firmware.bin" }
```

`system.ota` is reserved for firmware and is handled before view dispatch. HTTPS downloads use the
compiled `RIOT2_ROOT_CA_PEM` when present; otherwise the firmware logs a warning and uses insecure
TLS.

## Troubleshooting

- **Upload fails or the port isn't found:** find the COM port with `& $pio device list`. Make
  sure no other program (a serial monitor or another IDE) has the port open.
- **The device stays on the QR/setup screen:** it has no valid stored configuration. Complete the
  provisioning above.
- **Stuck on "WiFi: connecting..." or "MQTT: connecting...":** check the credentials entered
  during provisioning. If needed, factory-reset and provision again.
- **The screen stays dark:** tap the touchscreen, or one of the three button zones at the
  bottom edge.
- **LVGL settings (fonts, QR support, …) don't apply:** `platformio.ini` must keep `-Iinclude`
  in `build_flags`. Without it, `-DLV_CONF_INCLUDE_SIMPLE` can't find `include/lv_conf.h` for
  LVGL's own sources, and every `LV_*` setting silently falls back to its default.
## Contracts and links

- [MQTT topics and payloads](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/mqtt-topics.md)
- [Firmware configuration subset](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/configuration.md#firmware-subset)
- [Firmware HTTP behavior](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/http-api.md#firmware-riot2ardshared)
- [Architecture overview](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/architecture/overview.md)
- AI coding instructions: [AGENTS.md](AGENTS.md)
- Release notes: [CHANGELOG.md](CHANGELOG.md)

## License

See [LICENSE](LICENSE).
