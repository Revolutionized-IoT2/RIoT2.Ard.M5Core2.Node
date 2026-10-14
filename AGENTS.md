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
- CI: `.github/workflows/ci.yml` builds every firmware project and runs the Ard.Shared native
  tests on every push ([CI guide](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/guides/ci.md)).
- Release: bump `VERSION`, commit, then push the same `x.y.z` tag. `release.yml` attaches
  `firmware-m5stack-core2-<tag>.bin`, the manifest and `SHA256SUMS` to a GitHub release; serve the
  `.bin` to OTA from there.

## Layout

| Path | Contents |
|---|---|
| `platformio.ini` | `m5stack-core2` environment (platform `espressif32` pinned for OTA rollback), C++14 flags, LVGL include flags and shared-library path |
| `MANIFEST_NAME`, `VERSION` | Inputs to the shared manifest generation script |
| `include/lv_conf.h` | LVGL configuration used by library sources through `-Iinclude` |
| `include/*View.h`, `src/*View.cpp` | Core2 LVGL views and templates; the shared view types render a `riot2::views` model (RIoT2.Ard.Shared), which also builds their template |
| `include/NavigationController.h`, `src/NavigationController.cpp` | Tab view and bottom virtual-button navigation |
| `include/ScreenPowerPolicy.h`, `src/ScreenPowerPolicy.cpp` | Dim, Matrix Rain idle and display sleep behavior |
| `include/HapticFeedback.h`, `src/HapticFeedback.cpp` | Vibration feedback implementation |
| `include/Rfid2Peripheral.h`, `src/Rfid2Peripheral.cpp` | Core2 RFID-to-peripheral bridge |
| `src/main.cpp` | Board setup, splash/provisioning/diagnostics UI, `NodeRuntimeHooks` and the main loop |
| `include/TouchGestures.h`, `src/TouchGestures.cpp` | Scroll-vs-tap tracking for the virtual buttons, BtnA+BtnC factory reset |
| `include/BoardPeripherals.h`, `src/BoardPeripherals.cpp` | Grove pin map and peripheral registration (GPIO, RFID2) |

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
- The node lifecycle (NVS settings, provisioning, Wi-Fi, MQTT, configuration, OTA, peripherals,
  BLE start) is `riot2::NodeRuntime` in RIoT2.Ard.Shared. `main.cpp` sets up the board, builds the
  UI and passes `NodeRuntimeHooks`; don't add lifecycle code here, extend the runtime instead.
- Keep `riot2::NodeRuntime::startOtaTrial()` first in `setup()`; `NodeRuntime` marks the image
  healthy after the first successful configuration fetch (see RIoT2.Ard.Shared). Don't change the pinned
  `espressif32` platform without checking that its bootloader still has app rollback enabled.
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
- A new OTA image rolls back unless it fetches its configuration within 3 minutes
  ([README](README.md#automatic-rollback)). Restarting or factory-resetting the device during that
  time rolls it back as well.
- Template `type` is a number (`riot2::ValueType::Number`, not `"2"`); there is no `valueType`.

## Related work

- Backlog item [15](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/backlog/open-issues.md):
  duplicated firmware views and runtime wiring.
- Optional hardening [S9](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/backlog/optional-hardening.md):
  provisioning security, NVS encryption, signed OTA and fail-closed TLS.
- Accepted architecture item ([ADR 0011](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/adr/0011-firmware-noderuntime.md)) [A8](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/architecture/target.md#a8-share-a-firmware-noderuntime):
  shared firmware `NodeRuntime`.
- Plan [M5](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/plans/m05-firmware-node-runtime.md):
  shared firmware view logic and node runtime.
- [Feature ideas](https://github.com/Revolutionized-IoT2/.github/blob/main/docs/features.md#firmware):
  Wiegand peripheral integration and OTA channel improvements.
