# Development Status

Last updated: 2026-10-27

## Recovery context

The original Windows development computer is unavailable. The replacement machine is DESKTOP-415LS7I. The source tree has been cloned from the owner-maintained repository AndrewRayR/indicator-esp-idf-template into C:\Projects\AndrewRayR\D1-Dash so work can continue from this computer.

The original machine's exact uncommitted working-tree state cannot be recovered through the unavailable computer. This checkout contains the source that was present in the GitHub fork, including the Tech Terminal files and BSP compatibility edits already committed there.

## Known build history

On the original machine, ESP-IDF 5.4.4 and ESP32-S3 were confirmed as the active build target. A previous build log reported these incompatibilities:

- TCA9535 16-bit input-read callback did not match the BSP's 8-bit callback type.
- RGB VSYNC callback event data const qualification differed.
- Framebuffer API expected void **, while the old code supplied uint8_t **.
- LVGL direct-mode copy callback type differed.

The current repository checkout includes a BSP wrapper named tca9535_read_input_pins_8, void * framebuffer pointers, a const-qualified VSYNC callback, and a void (*)(void) direct-mode copy callback. These compatibility changes were recompiled successfully on the replacement computer as part of the clean ESP-IDF 5.4.4 build.

## Current replacement-machine findings

- Remote Desktop Commander connection to DESKTOP-415LS7I: online.
- Project checkout: C:\Projects\AndrewRayR\D1-Dash.
- ESP-IDF 5.4.4 installed at C:\Espressif\frameworks\esp-idf-v5.4.4.
- ESP32-S3 toolchain and Python environment installed under C:\Users\andre\.espressif.
- Clean build command: idf.py set-target esp32s3 followed by idf.py build.
- Build result: PASS, exit code 0; generated build/indicator_basis.bin.
- Firmware binary size: 0x217730 bytes; app partition is 0x400000 bytes with 48% free.
- The build reported two renamed Wi-Fi config options; sdkconfig.defaults has now been updated to the current ESP-IDF names.
- USB serial scan showed COM22 as “USB Serial Device” with VID 2E8A / PID 000A. This identifies a USB serial device but does not prove it is the Indicator D1 ESP32-S3 interface, so it must not be flashed based on this scan alone.
- Firmware has not been flashed; display, touch, Wi-Fi and API behavior remain unverified on hardware.

## Next actions

1. Rebuild after the sdkconfig.defaults cleanup and confirm there are no configuration-migration warnings.
2. Identify the exact USB serial interface for the Indicator D1 before flashing.
3. Flash only after the device identity and COM port are confirmed.
4. Verify the touchscreen home screen and Wi-Fi behavior on hardware.
5. Implement and test the remaining module work, starting with the F1 tile, which is currently a placeholder.
