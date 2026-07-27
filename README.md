# WeDo Workshop for Cardputer

**WeDo Workshop** is independent firmware for the original **M5Stack
Cardputer** (StampS3 / ESP32-S3). The long-term goal is to provide a complete,
standalone workspace for discovering, configuring, and operating LEGO
Education WeDo 2.0 hubs and their connected motors and sensors.

The current first release is intentionally discovery-only. It scans nearby
Bluetooth Low Energy devices, identifies an original WeDo 2.0 Smart Hub from
official protocol evidence, displays the result, and writes diagnostic details
to the Serial Monitor. It is installed as a separate M5Launcher application;
M5Launcher source code is not part of this project.

## Current release scope

The firmware performs an active, asynchronous BLE scan for about 15 seconds.
It deduplicates results by BLE address, updates RSSI when advertisements repeat,
and presents a confirmed WeDo 2.0 hub without blocking the display or keyboard.

This release does **not** connect, pair, create a GATT client, write to any
characteristic, control motors, change the hub LED, access port modules, or
persist data. It requires no microSD card, SPIFFS, LittleFS, FAT partition, or
external files. `esp32_PoweredUp` is intentionally reserved for a future
control layer.

## Architecture

- `lego_advertisement_parser`: pure C++ parser with no Arduino or hardware
  dependency. It interprets the exact payload returned by
  `NimBLEAdvertisedDevice::getManufacturerData()`.
- `scanner_service`: configures NimBLE, handles asynchronous callbacks,
  deduplicates addresses, refreshes RSSI, and produces safe UI snapshots.
- `ui`: redraws only when the screen state or displayed device changes.
- `main`: initializes USB Serial, Cardputer, keyboard, and UI, then coordinates
  the application without long blocking delays.

The scanner owns its deduplicated result list and uses `setMaxResults(0)` to
avoid retaining a second NimBLE result list. Duplicate callbacks remain enabled
so RSSI can be refreshed. The service never starts overlapping scans.

## Board and dependencies

Versions are pinned in `platformio.ini`:

| Component | Version |
|---|---:|
| PlatformIO Core used for validation | 6.1.19 |
| `espressif32` platform | 6.7.0 |
| PlatformIO board | `m5stack-stamps3` |
| Arduino-ESP32 | 2.0.16 (`3.20016.0`) |
| M5Cardputer | 1.1.1 |
| M5Unified | 0.2.19 |
| M5GFX | 0.2.26 |
| IRremote (M5Cardputer transitive dependency) | 4.7.1 |
| NimBLE-Arduino (h2zero) | 2.5.0 |
| Native test platform | 1.2.1 |
| Unity | 2.6.1 |

The original Cardputer uses the M5Stack StampS3 module, so this project uses
PlatformIO's official [`m5stack-stamps3` board definition](https://docs.platformio.org/en/stable/boards/espressif32/m5stack-stamps3.html)
with 8 MB flash. `ARDUINO_USB_CDC_ON_BOOT=1` and `ARDUINO_USB_MODE=1` follow the
[official original Cardputer PlatformIO configuration](https://docs.m5stack.com/en/core/Cardputer?hidden=true)
and expose `Serial` through USB CDC. The target is not the Cardputer ADV.

GNU C++17 is selected explicitly with `-std=gnu++17`.

## Build and tests

From the project directory in a VS Code terminal:

```bash
pio run
```

The ESP32-S3 application image is produced at:

```text
.pio/build/cardputer/firmware.bin
```

The release artifact for M5Launcher is:

```text
dist/wedo-workshop-cardputer.bin
```

Do not run `pio run --target upload`: a direct USB upload may overwrite
M5Launcher. The distributed artifact is a normal ESP application image, not a
merged or full-flash dump.

Run the hardware-independent parser tests with:

```bash
pio test -e native
```

In the test vectors, `97 03`, the `SSS DDDDD` byte position, and hub type values
come from the LEGO specification. The complete vectors use illustrative button,
capability, network, status, and option values because those fields do not
participate in this release's classification.

## BLE identification criteria

The implementation combines the modern
[LEGO Wireless Protocol 3.0.00](https://lego.github.io/lego-ble-wireless-protocol-docs/#advertising)
with the official
[WeDo 2.0 Communication Software Developer Kit](https://education.lego.com/en-gb/product-resources/wedo-2/downloads/developer-kits/).
It considers these official signals:

1. the original WeDo 2.0 dedicated service
   `00001523-1212-EFDE-1523-785FEABCD123`;
2. the LWP 3.x service `00001623-1212-EFDE-1623-785FEABCD123`, when advertised;
3. the LEGO Company/Manufacturer ID `0x0397`;
4. System Type `000` and Device Number `00000` in the `SSS DDDDD` byte.

The complete LEGO advertising structure is:

```text
09 FF 97 03 <button> <SSS DDDDD> <capabilities> <network> <status> <option>
```

The current [NimBLE API](https://h2zero.github.io/NimBLE-Arduino/class_nim_b_l_e_advertised_device.html)
returns only the Manufacturer Specific Data contents. It removes `09` (length)
and `FF` (AD type), so the parser receives:

```text
97 03 <button> <SSS DDDDD> ...
```

The Company ID is little-endian: `0x0397` appears as `97 03`. There are two
confirmation paths: the original hub's full dedicated `1523` UUID, or the LEGO
Manufacturer ID together with LWP System/Device byte `0x00`. The generic `1623`
service without Manufacturer Data indicates only a possible LEGO device. A
name containing “LEGO”, an empty name, and public/random address type never
count as identification evidence.

## Controls

- On startup, turn on the WeDo 2.0 and press the hub button.
- `R`: stops the current scan if necessary, clears volatile results, and starts
  a new scan.
- `ESC` when provided by the keyboard map, or `Backspace/Del`: resets the
  application state and scan.
- `Enter`: intentionally does nothing and never initiates a connection in this
  release.

The side indicator turns green only after confirmed WeDo 2.0 identification.
Random/private BLE addresses are displayed as observed, may change between
sessions, and are never treated as persistent identity.

## Serial logs

Open the Serial Monitor at 115200 baud:

```bash
pio device monitor -b 115200
```

Logs include initialization, scan start/end, unique address count, device name,
address and address type, RSSI, advertised UUIDs, hexadecimal Manufacturer Data,
classification criteria, and parser errors. No keys or passwords are collected.

If USB CDC does not appear, use a USB data cable, close any program holding the
port, and restart the Cardputer. The port number may change after a reboot.

## Install through M5Launcher without microSD

1. Restart the Cardputer.
2. Enter the M5Launcher menu during its startup screen.
3. Open `WUI`.
4. On your computer, open the address displayed by the Cardputer.
5. Authenticate with the credentials configured in that Launcher installation.
6. Upload `dist/wedo-workshop-cardputer.bin`.
7. Request installation as a **new application**, not a Launcher update.
8. If prompted, allow Launcher to create a new application partition.
9. Name the shortcut `WeDo Workshop`.
10. Confirm that M5Launcher itself remains installed.
11. Start `WeDo Workshop` from the menu.
12. To return, restart the device and enter the Launcher menu during its startup
    screen using the key/button indicated by the installed Launcher version.

Do not automate WUI upload through guessed endpoints. Automation is appropriate
only when the installed version's official endpoint contract is known, the IP
address is known, and both the HTTP response and installation result can be
verified.

## Troubleshooting

- **No hub found:** verify that the hub is powered, press its button, move it
  closer, and disconnect phones or computers that may already be using it.
  Press `R` to scan again.
- **LEGO device, but not WeDo:** inspect Manufacturer Data in Serial. A lone
  `1623` service or another `SSS DDDDD` value is insufficient evidence.
- **Empty name:** this is valid. The screen displays `unknown` and classification
  relies only on official protocol data.
- **MAC changed:** random/private BLE addresses may rotate. This is expected.
- **WUI rejects the image:** choose the file from `dist/`, compare its SHA-256
  with `dist/build-info.txt`, and install it as an OTA application.

## Roadmap and limitations

This release has been physically validated with an original M5Stack Cardputer
and an original WeDo 2.0 Smart Hub. Detection of the observed hub uses its
official dedicated `1523` service even when Manufacturer Data is absent.

Future releases may add `esp32_PoweredUp` behind a dedicated connection layer,
followed by explicit hub selection, connection status, LED configuration, port
discovery, motor controls, sensor monitoring, and reusable module profiles.
Those capabilities should remain user-initiated and should preserve the pure,
hardware-independent advertisement parser.

## Official references

- [M5Stack: original Cardputer](https://docs.m5stack.com/en/core/Cardputer?hidden=true)
- [M5Stack: official keyboard example](https://github.com/m5stack/M5Cardputer/blob/master/examples/Basic/keyboard/singlePress/singlePress.ino)
- [PlatformIO: M5Stack StampS3](https://docs.platformio.org/en/stable/boards/espressif32/m5stack-stamps3.html)
- [LEGO Wireless Protocol](https://lego.github.io/lego-ble-wireless-protocol-docs/)
- [LEGO Education: WeDo 2.0 Communication Software Developer Kit](https://education.lego.com/en-gb/product-resources/wedo-2/downloads/developer-kits/)
- [NimBLE-Arduino: non-blocking scan](https://h2zero.github.io/NimBLE-Arduino/class_nim_b_l_e_scan.html)
