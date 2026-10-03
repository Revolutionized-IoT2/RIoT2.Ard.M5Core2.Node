# AGENTS.md — RIoT2.Ard.M5Core2.Node

Applies to: this repository. Read the platform guide first:
[.github/AGENTS.md](https://github.com/Revolutionized-IoT2/.github/blob/main/AGENTS.md). It covers the
workspace map, platform-wide rules and the documentation rules. In the local workspace, every
`https://github.com/Revolutionized-IoT2/<Repo>/blob/main/<path>` link is the file
`C:\Src\RIoT2\<Repo>\<path>`; read the local file instead of fetching the URL.

## What this is

PlatformIO/Arduino firmware for the M5Stack Core2 RIoT2 node. It uses
`RIoT2.Ard.Shared` for Wi-Fi, MQTT, provisioning, configuration fetch, OTA, BLE and peripherals,
and adds the Core2 LVGL touchscreen UI, virtual buttons, vibration motor, QR setup screen and
board pin map.

## Commands

Run from the workspace root (`C:\Src\RIoT2`), in PowerShell:

```powershell
$pio = "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe"
& $pio run -d .\RIoT2.Ard.M5Core2.Node
```

Run shared native regressions from `C:\Src\RIoT2\RIoT2.Ard.Shared` when a C++14 compiler is
available:

```powershell
python .\tests\test_firmware_p1.py
python .\tests\test_firmware_p2.py
python .\tests\test_firmware_architecture.py
```

Device commands, only when a Core2 is connected and flashing/monitoring was requested:

```powershell
& $pio run -d .\RIoT2.Ard.M5Core2.Node -t upload
& $pio device monitor -d .\RIoT2.Ard.M5Core2.Node
```

- Build artifact: `.pio\build\m5stack-core2\firmware.bin`.
- Run: there is no host runtime. Do not run upload/monitor unless the task explicitly involves a
  connected device.
- Release: no CI workflow or git tags are present. Update `VERSION`, build the `.bin`, and serve it
  to OTA if doing a manual release.

## Layout

| Path | Contents |
|---|---|
| `platformio.ini` | `m5stack-core2` environment, C++14 flags, LVGL include flags and shared-library path |
| `MANIFEST_NAME`, `VERSION` | Inputs to the shared manifest generation script |
| `include/lv_conf.h` | LVGL configuration used by library sources through `-Iinclude` |
| `include/*View.h`, `src/*View.cpp` | Core2 LVGL views and templates |
| `include/NavigationController.h`, `src/NavigationController.cpp` | Tab view and bottom virtual-button navigation |
| `include/ScreenPowerPolicy.h`, `src/ScreenPowerPolicy.cpp` | Dim, Matrix Rain idle and display sleep behavior |
| `include/HapticFeedback.h`, `src/HapticFeedback.cpp` | Vibration feedback implementation |
| `include/Rfid2Peripheral.h`, `src/Rfid2Peripheral.cpp` | Core2 RFID-to-peripheral bridge |
| `src/main.cpp` | Board setup, provisioning UI, MQTT/configuration/OTA wiring and main loop |

## Contracts consumed here

- [mqtt-topics.md](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/mqtt-topics.md):
  consumed through `RIoT2.Ard.Shared/RIoT2Shared/include/riot2/Topics.h` and `MqttConnection`.
- [configuration.md](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/configuration.md):
  view and peripheral templates in `src/*View.cpp`, `Rfid2Peripheral.cpp` and the shared parser.
- [http-api.md](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/http-api.md):
  shared configuration fetch, provisioning portal and template server.
- [env-vars.md firmware section](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/contracts/env-vars.md#firmware-m5core2-m5dial):
  provisioning values stored in NVS by shared code.

## Rules

- Reusable logic must go in `RIoT2.Ard.Shared`, not only in this repository. If shared code changes,
  build both Core2 and M5Dial.
- Keep `-DLV_CONF_INCLUDE_SIMPLE` and `-Iinclude` together in `platformio.ini`; LVGL library
  sources need the include path to see `include/lv_conf.h`.
- Do not use B2/GPIO36 as an output. `GpioPinMap` marks only A1, A2 and B1 output-capable.
- Do not assign `system.ota` to a view command template.
- Do not edit vendored `.pio\libdeps` content.
- Do not commit real Wi-Fi, MQTT, node id, broker or certificate values. Use placeholders in docs.
- Do not flash a device unless the user asks for upload/device validation.

## Pitfalls

- `OrchestratorClient` is constructed with `enableCache=false`; the Core2 shows the splash screen
  until a live orchestrator fetch succeeds and does not load cached configuration.
- `BtnA`/`BtnB`/`BtnC` are M5Unified virtual touch zones. `M5.setTouchButtonHeight(40)` is required
  before those buttons register touches.
- `BtnB` hold threshold is raised to the diagnostics hold duration so normal clicks are not
  mistaken for diagnostics toggles.
- Screen wake consumes the first touch/button input when waking from idle or sleep.
- Firmware template `type` values downloaded from the orchestrator are lost because shared parsing
  reads `type` as a string while the contract sends a number (configuration divergence C2).

## Related work

- Backlog item [6](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/backlog/open-issues.md):
  OTA rollback policy.
- Backlog item [15](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/backlog/open-issues.md):
  duplicated firmware views and runtime wiring.
- Optional hardening [S9](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/backlog/optional-hardening.md):
  provisioning security, NVS encryption, signed OTA and fail-closed TLS.
- Architecture proposal [A8](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/architecture/target.md#a8-share-a-firmware-noderuntime):
  shared firmware `NodeRuntime`.
- Plan [M5](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/plans/m05-firmware-node-runtime.md):
  shared firmware view logic and node runtime.
- [Feature ideas](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/features.md#firmware):
  Wiegand peripheral integration and OTA channel improvements.
