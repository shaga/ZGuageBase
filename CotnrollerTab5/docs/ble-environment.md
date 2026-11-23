# Tab5 BLE environment

## Configuration

- pioarduino platform: `55.03.39` (Arduino-ESP32 `3.3.9`).
- Board: `m5stack-tab5-p4` (ESP32-P4 ES / pre-revision-300 variant).
- Dedicated tool/Python environment: `C:/pio-tab5`. The short path avoids
  Windows path-length failures while extracting the SDK's Matter headers.
- BLE uses the Arduino core's bundled BLE library and ESP-Hosted NimBLE,
  with the onboard ESP32-C6 as the radio. Do not add a second NimBLE-Arduino
  library merely because the peripheral uses it.
- The Tab5 variant supplies SDIO CLK=12, CMD=13, D0=11, D1=10,
  D2=9, D3=8, RESET=15.

## Build

The first installation downloads and extracts a large multi-chip SDK and can
take several minutes. Do not run multiple installation/build commands at once;
VS Code's automatic project initialization can also hold the package lock.
Keep `core_dir` short: placing it under this project produced a 260-character
temporary path and a `FileNotFoundError` while extracting a Matter header.

Run from the project root:

```powershell
pio run -e esp32p4_pioarduino
pio run -e ble_check
```

The default environment keeps the existing track UI. `ble_check` is a separate
diagnostic firmware: initialize M5Unified and BLE, create a central client,
scan for five seconds, and print results on screen and serial (115200 baud).
It does not connect to or send commands to the railway controller.

After explicitly choosing to flash the diagnostic:

```powershell
pio run -e ble_check -t upload
pio device monitor -b 115200
```

Restore the normal UI with `pio run -e esp32p4_pioarduino -t upload`.

## Hardware verification

### Source layout

- `main.cpp`: M5/serial startup, UI startup, BLE startup, and the update loop.
- `AppUi`: owns the PSRAM canvas, detects new touches, converts coordinates,
  routes taps to the visible screen, decides full/partial redraw, presents
  completed frames, logs memory, and polls screenshot commands.
- `DeviceSelectScreen`: device-list snapshot, selection/connection screen,
  device-row bounds and taps. A successful selection asks AppUi to reset controls.
- `ControlScreen`: turnout selection state, track and control artwork, speed
  indicator, and control hit testing/commands. Speed comes from BleCentral.
- `UiDrawing`: shared logical coordinates, colors, rectangles, and drawing
  helpers. It draws only into the canvas and does not access BLE or touch.
- `BleCentral`: BLE worker and commands; `Screenshot`: capture and SD writer.

Screen classes never push the canvas to the physical display. AppUi presents
once after drawing finishes, then lets Screenshot capture that completed frame.
Speed-only changes redraw the speed/direction area; screen/turnout changes
redraw the layout. Both still use a single full-screen sprite transfer.

### Screenshots

Insert a formatted microSD card into Tab5. In the serial monitor at 115200 baud,
send `screenshot` followed by LF or CR/LF (Enter). The completed canvas is saved
as `/screenshots/shot-00001.bmp`, then the next unused number. Existing files
are not overwritten and the card is never automatically formatted.
Wait for `[Screenshot] Saved ...` before removing the card. Mount/write failures
are reported in the serial log; correct the card issue and resend the command.

BMP output is top-down 24-bit BGR, about 2.64 MiB at 720 x 1280. A temporary
PSRAM copy holds the captured frame while a separate task writes it to SD,
then releases it. Further captures are rejected while a save is in progress.
The brief capture runs on the UI task; SD writes do not block touch processing.
Check orientation/colors on a PC and confirm repeated captures get new names.

### Display and BLE

UI drawing uses one persistent 16-bit full-screen M5Canvas in PSRAM
(720 x 1280: about 1.76 MiB). Drawing routines update this canvas; a completed
frame is pushed once at the end of an update, only when the UI changed.
Touch input still comes from the physical display. If allocation fails, an
error is shown and BLE/control startup is skipped until restart.
Serial `[UI]` logs report internal/PSRAM free bytes and largest blocks before
and after allocation and on BLE state changes, plus frame transfer time.
Check flicker and responsiveness on hardware when selecting a device, changing
turnouts, adjusting speed, changing direction, and pressing STOP. Sprite drawing
hides erase/redraw steps; display scan-out tearing must be assessed separately.

The default firmware scans for the service UUID below and lists up to eight
matching devices by name (no signal strength or address in the UI). Devices
without an advertised name use `ZGuageBase` as the label. Tap a row to connect;
no connection or railway commands are issued before selection. Devices with
identical names appear as separate rows. Scans last five seconds, separated by
two seconds; a selection is handled when its device is found in a scan.

After protocol validation (one speed channel, two turnouts), the operation
screen appears. On disconnect it returns to a reconnection screen and searches
only for the selected address. `Select another device` cancels that selection.
Choosing another device clears pending turnout requests and resets speed.
BLE work runs on a separate task; serial at 115200 baud logs state and address.

Manual checks after uploading `esp32p4_pioarduino`:
- Start with the base off, then power it on; a row appears without connecting.
- Tap its name; operation controls appear only after connection succeeds.
- Turn it off/on; reconnect to the same base with power reset to zero.
- Cancel a pending connection and select another base; never connect to an
  unselected device. Test with two bases if available.
- Check indicator steps at 0, 2, 10, 12, 80, 82, 90, 98, 100, direction at zero, and STOP.

The track toggles send turnout commands using Write Without Response:
two bytes `[lower/near, upper/far]`, each `0` for straight or `1` for branch.
The display shows the requested position, not physical position feedback.
Rapid taps are coalesced to the latest pair. Controls are hidden while
disconnected; the latest requested pair is resent on reconnection to the same base. Connecting before any user operation sends no turnout command.
Speed controls below the track use one tap per step: minus/plus changes the
requested power by two, clamped to 0..100. Ten outlined horizontal bars fill
from bottom to top; each represents 10 units. Partial fill uses five intensity
steps, with full fill matching the outline. Bars grow wider toward the top.
The bottom eight bars are white and the top two are red. Plus sits at upper
right and minus at lower right; both use white backgrounds and black text. STOP is always visible
across the full width beneath them, and preserves direction when stopping.
The track-center folded arrow matches the base display: its upper tip points
right for CW, left for CCW. At zero it becomes a wider white direction button with a black arrow;
while moving it is a white arrow only. Controls are enabled only while
connected. Speed is encoded as one byte (low seven bits power, bit 7 CCW) and
sent asynchronously with Write Without Response. Disconnect/reconnect resets
the request to zero/CW; a previous running speed is never replayed on reconnect.
The indicator shows requested power, not measured train speed.
The base's
existing connection handler may stop the train and disable manual controls.
Physical connection and reconnection still require verification on hardware.

A successful build does not verify the onboard C6 firmware. Check the serial
log for ESP-Hosted transport and BLE initialization, then look for
`ZGuageBase service found`. Detection is by service UUID
`8a5e1020-f39d-42a4-ad5c-34f68745e643`, not the advertised name alone.
If initialization fails, inspect the reported C6 firmware version and SDIO
errors before considering a C6 firmware update. This project does not
automatically update the C6 firmware.

## References

- https://github.com/pioarduino/platform-espressif32/releases/tag/55.03.39
- https://github.com/pioarduino/platform-espressif32/blob/55.03.39/boards/m5stack-tab5-p4.json
- https://github.com/espressif/arduino-esp32/blob/3.3.9/variants/m5stack_tab5/pins_arduino.h
- https://github.com/espressif/arduino-esp32/blob/3.3.9/cores/esp32/esp32-hal-hosted.c
