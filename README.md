# RIoT2.Ard.M5Core2.Node

Firmware for the [M5Stack Core2](https://docs.m5stack.com/en/core/core2) acting as a **Node**
in the [RIoT2](../RIoT2.Core) ecosystem. It connects to Wi-Fi and MQTT, announces itself to the
RIoT2 Orchestrator, downloads its device configuration, and renders an [LVGL](https://lvgl.io/)
touchscreen UI for viewing and controlling remote devices (lights, scenes, sensors, etc.) via
MQTT reports/commands.

Almost all non-UI logic (Wi-Fi, MQTT, provisioning, orchestrator handshake, OTA, peripherals, BLE
scanning) lives in the sibling [RIoT2.Ard.Shared](../RIoT2.Ard.Shared) library, shared with
[RIoT2.Ard.M5Dial.Node](../RIoT2.Ard.M5Dial.Node) — see that project's README for the shared
protocol/MQTT contracts in more detail. This project only adds the Core2-specific pieces: LVGL UI,
touch-button navigation, and the vibration motor.

## Prerequisites

- [PlatformIO](https://platformio.org/) — either the [VS Code extension](https://platformio.org/install/ide?install=vscode)
  or the standalone `pio` CLI.
- A USB-C cable and an [M5Stack Core2](https://docs.m5stack.com/en/core/core2) device.
- The sibling [RIoT2.Ard.Shared](../RIoT2.Ard.Shared) directory checked out alongside this one
  (referenced via `lib_extra_dirs = ../RIoT2.Ard.Shared` in [platformio.ini](platformio.ini) —
  no separate build step needed, PlatformIO compiles it as part of this project's build).
- (Windows) USB-serial drivers for the Core2's CP2104 USB-to-UART chip if your OS doesn't detect
  the device automatically — see M5Stack's [driver download page](https://docs.m5stack.com/en/download).

No manual library installation is required — PlatformIO resolves all dependencies
(`m5stack/M5Unified`, `lvgl/lvgl`, `PubSubClient`, `ArduinoJson`, `NimBLE-Arduino`, plus
core-bundled ESP32 libraries) from [platformio.ini](platformio.ini) on first build.

## Build

Using the PlatformIO CLI:

```powershell
# If `pio` isn't on your PATH (common on Windows), use the full path instead:
# & "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run

pio run
```

Or in VS Code with the PlatformIO extension installed: open this folder, then use the
**PlatformIO: Build** command (checkmark icon in the status bar).

## Flash to the Core2

1. Connect the Core2 to your computer via USB-C.
2. Build and upload in one step:

   ```powershell
   pio run -t upload
   ```

   If PlatformIO can't auto-detect the serial port, list available ports and pass one explicitly:

   ```powershell
   pio device list
   pio run -t upload --upload-port COM5
   ```
3. (Optional) Open the serial monitor to watch boot/connection logs (115200 baud):

   ```powershell
   pio device monitor
   ```
4. Or do build + upload + monitor in one step:

   ```powershell
   pio run -t upload -t monitor
   ```

## First boot: provisioning

The firmware ships with no Wi-Fi/MQTT credentials baked in. On first boot (or after a factory
reset), the Core2 starts its own Wi-Fi access point and a captive-portal web form:

1. Power on the Core2. The screen shows a QR code (scan it to jump straight to the setup page)
   along with the AP name (`RIoT2-Setup-XXXX`) and setup URL as text.
2. From a phone or laptop, connect to that Wi-Fi network (or scan the QR code, which encodes the
   setup page's URL directly).
3. Fill in the form:
   - **Id** — a unique node identifier (GUID) for this device.
   - **WifiSsid** / **WifiPassword** — your home/office Wi-Fi credentials.
   - **MqttServerUrl** — address of your MQTT broker.
   - **MqttUsername** / **MqttPassword** — MQTT broker credentials (if required).
   - **Enable TLS for MQTT** — checkbox; connects over `WiFiClientSecure` on port 8883 instead of
     plaintext when checked.
   - **Enable vibration feedback** — checkbox; gates the vibration motor (see below). Checked by
     default.
4. Submit the form. The device saves the configuration to flash (NVS) and restarts into normal
   operation, connecting to your Wi-Fi and MQTT broker and then to the RIoT2 Orchestrator.

To re-enter provisioning later (e.g. to change networks), perform a **factory reset**: press and
hold **BtnA and BtnC together** (the leftmost and rightmost touch zones at the bottom of the
screen — Core2 has no single physical "boot button") for about 5 seconds. This clears the stored
configuration and restarts the device back into the setup flow.

## Navigation

The Core2's 320×240 touchscreen renders one tab per configured device/view, swipeable
left/right (LVGL's native `lv_tabview` gesture) with a bottom tab bar. Three touch zones at the
very bottom of the screen act as virtual buttons (no physical buttons on the Core2 body itself):

| Button | Action |
| --- | --- |
| **BtnA** | Previous tab |
| **BtnB** | Dismiss an active popup (alert/notification), otherwise jump to the first tab |
| **BtnC** | Next tab |

Alerts and notifications (from an inbound `AlertView`/`NotificationView` command) render as a
popup overlay above the current tab rather than taking over navigation.

## Screen power management

After 30 seconds of no touch/button activity, the display dims and shows a full-screen
"digital rain" animation as an idle overlay; after 5 minutes of continued inactivity it goes to
sleep (`M5.Display.sleep()`). Any touch or button press wakes it back up — a wake-only
touch/button press is swallowed (it won't also change tabs).

## Vibration feedback

The Core2's built-in vibration motor pulses alongside the existing buzzer tone for confirm/error/
alert/timer events, gated by the **Enable vibration feedback** setting from provisioning
(`NodeConfig.vibrateEnabled`, default on). Unlike the buzzer tone (which a command can silence per
message via its own `soundEnabled` field), vibration is a separate physical modality and is only
ever gated by this device-level setting.

## Updating firmware over the air (OTA)

Once a node is online, it doesn't need to be re-flashed over USB for future updates — an
operator/orchestrator can publish the following to the node's `riot2/node/{id}/command` topic:

```json
{ "id": "system.ota", "value": "http://host/path/to/firmware.bin" }
```

The node downloads and flashes the binary from that URL and reboots automatically on success.

## Troubleshooting

- **Upload fails / port not found:** confirm the correct COM port with `pio device list`, and
  make sure no other program (serial monitor, another IDE) has the port open.
- **Device boots but stays on the QR/setup screen:** it has no valid stored configuration —
  complete the provisioning flow above.
- **Stuck on "WiFi: connecting..." / "MQTT: connecting...":** double-check the credentials
  entered during provisioning (factory reset and re-provision if needed).
- **Screen stays dark / won't wake:** tap anywhere on the touchscreen, or tap one of the three
  BtnA/B/C zones at the bottom edge.
- **LVGL settings (fonts, QR code support, etc.) don't seem to apply:** confirm
  [platformio.ini](platformio.ini) still has `-Iinclude` in `build_flags` — without it,
  `-DLV_CONF_INCLUDE_SIMPLE` silently fails to find [include/lv_conf.h](include/lv_conf.h) for
  LVGL's own library sources (not just this project's own `src/*.cpp`), and every `LV_*` setting
  falls back to its default.
