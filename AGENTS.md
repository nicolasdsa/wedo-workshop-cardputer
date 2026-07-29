# AGENTS.md

## Scope

These instructions apply to the entire repository. They are mandatory for any
agent or automated contributor working on WeDo Workshop for Cardputer.

## Project identity and hardware

- The target is the **original M5Stack Cardputer** with StampS3/ESP32-S3, not
  the Cardputer ADV.
- The development setup currently has the Cardputer connected to the computer
  through a USB data cable. Verify that it is still present before hardware
  operations. USB CDC is enabled and Serial logs use 115200 baud.
- Do not assume a fixed serial device name. Detect the current port first; on
  Linux it will commonly be `/dev/ttyACM0`, but it can change after a reboot.
- This firmware is an independent M5Launcher application. Do not modify or
  build M5Launcher as part of this repository.

## Language

- Keep all repository content in **English**.
- This includes source code, identifiers where practical, comments, UI text,
  Serial logs, tests, documentation, commit messages, release notes, and build
  metadata.
- Before finishing, search all text files and compiled application strings for
  Portuguese text. Translate any project-owned Portuguese text that remains.
- Preserve protocol-defined names, UUIDs, byte layouts, trademarks, and device
  names exactly when translation would make them incorrect.

## Architecture boundaries

Keep the Cardputer application shell separate from WeDo protocol and connection
logic.

### Cardputer application layer

The Cardputer-facing layer owns only:

- device initialization;
- display rendering and screen state;
- keyboard input;
- application navigation;
- USB Serial presentation;
- orchestration through abstract WeDo service interfaces.

Cardputer UI and entry-point code must not contain GATT UUID lookup sequences,
connection state machines, characteristic reads/writes, motor commands, sensor
decoding, or hub-specific packet construction.

### WeDo domain and transport layer

All present and future WeDo-specific code must remain in dedicated modules,
separate from the Cardputer application layer. This includes:

- advertisement parsing and official identification rules;
- BLE scan interpretation;
- hub discovery and selection models;
- connection, disconnection, and reconnection state;
- GATT services and characteristics;
- LEGO Wireless Protocol packet encoding and decoding;
- hub LED operations;
- port and module discovery;
- motor and sensor operations;
- connection errors and protocol timeouts.

For new connection/control work, prefer explicit directories such as
`include/wedo/` and `src/wedo/`. Expose a small interface to the Cardputer layer
instead of including NimBLE connection types throughout `main`, UI, or keyboard
code. Keep pure protocol parsers independent of Arduino, M5Cardputer, display,
keyboard, and filesystem APIs so they can run in the native test environment.

Do not add connection behavior implicitly to discovery callbacks. Connecting
to a hub must be an explicit user action and must not make UI or keyboard
handling blocking.

## Source-of-truth rules

- Use the official LEGO Wireless Protocol and official WeDo 2.0 Communication
  Software Developer Kit as protocol sources of truth.
- Use current official M5Stack Cardputer documentation, current M5Cardputer
  examples, and PlatformIO's official `m5stack-stamps3` definition for hardware
  configuration.
- Confirm how the pinned NimBLE-Arduino version represents advertising and
  Manufacturer Specific Data before using offsets.
- Do not identify a device from its BLE name alone.
- Do not confuse the complete BLE AD structure with the payload returned by
  `getManufacturerData()`.

## Mandatory freshness and verification workflow

Never claim that a binary is current merely because a file already exists in
`dist/`. After every source, dependency, compiler flag, or UI string change,
perform this workflow from the repository root:

1. Inspect `git status` and the relevant diff so the exact source state is
   known. Preserve unrelated user changes.
2. Run the hardware-independent tests:

   ```bash
   pio test -e native
   ```

3. Build the current source without uploading it:

   ```bash
   pio run
   ```

4. Confirm that `.pio/build/cardputer/firmware.bin` was produced by that build
   and is newer than the source changes being released.
5. Copy that exact application image to:

   ```text
   dist/wedo-workshop-cardputer.bin
   ```

6. Calculate its byte size and SHA-256.
7. Verify the ESP application image starts with magic byte `0xE9`, and use
   `esptool.py image_info` when available to validate image structure, checksum,
   segments, target, and embedded validation hash.
8. Update `dist/build-info.txt` with the actual dependency versions, test
   result, memory usage, artifact size, SHA-256, and image validation results
   from the same build.
9. Recalculate the SHA-256 after copying and compare it with
   `dist/build-info.txt`. Do not publish or transfer mismatched artifacts.
10. If the firmware is transferred to hardware, open Serial at 115200 baud and
    verify startup and the changed behavior on the connected Cardputer.

Documentation-only changes do not require recompiling the firmware unless they
alter documented artifact metadata or expose that the current artifact and
`build-info.txt` are inconsistent.

## Build and transfer safety

- The normal build command is `pio run`.
- Do **not** run `pio run --target upload` during the normal workflow. Direct
  PlatformIO USB flashing can overwrite M5Launcher, its boot configuration, or
  the currently installed application layout.
- For an M5Launcher application, build with `pio run`, then install
  `dist/wedo-workshop-cardputer.bin` through M5Launcher WUI as a new application.
- Do not generate or distribute a merged/full-flash dump as the main artifact.
- Do not add a custom partition table, filesystem dependency, or external data
  file unless a future task explicitly requires and justifies it.
- Do not automate WUI upload through guessed endpoints. Automation is allowed
  only when the endpoint contract for the installed M5Launcher version is
  officially known, the device IP is known, authentication is authorized, and
  both the HTTP response and installed application can be verified.
- If the user explicitly intends to remove or replace M5Launcher and accepts
  that consequence, direct USB flashing may be used only after resolving the
  exact serial port and confirming the destructive scope.

## Runtime constraints

- Keep BLE operations asynchronous or non-blocking.
- Keep display and keyboard handling responsive.
- Never start overlapping scans or connections.
- Deduplicate scan results by BLE address and refresh RSSI on repeat
  advertisements.
- Avoid long delays in the main loop and avoid unnecessary full-screen redraws.
- Reuse or release NimBLE scan results correctly.
- Never connect, pair, or write GATT data unless the requested release explicitly
  introduces that feature and supplies corresponding UI, error handling, tests,
  and documentation.

## Tests

- Keep pure advertisement and protocol parsers testable without Cardputer
  hardware.
- Add native tests for every identification rule, packet parser, encoder, and
  connection-state transition that does not intrinsically require hardware.
- Include regression tests for behavior observed on a physical WeDo 2.0 hub.
- Clearly distinguish specification-derived bytes from illustrative test data.

## Completion report

For firmware-changing work, report:

- changed architecture or behavior;
- test command and result;
- build command and result;
- dependency versions;
- application image path, byte size, and SHA-256;
- ESP image validation result;
- physical USB/Serial verification performed;
- assumptions and any remaining manual M5Launcher steps.
