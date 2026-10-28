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
- Project checkout: `C:\Projects\AndrewRayR\D1-Dash`.
- ESP-IDF 5.4.4 installed at `C:\Espressif\frameworks\esp-idf-v5.4.4`.
- ESP32-S3 toolchain and Python environment installed under `C:\Users\andre\.espressif`.
- COM21 was verified non-destructively with esptool: ESP32-S3 revision v0.2, Wi-Fi/BLE, 8 MB flash and 8 MB PSRAM. COM22 enumerates separately and has not been flashed.
- Firmware was flashed successfully to COM21. Serial logs identify the board as `SenseCAP_Indicator`, confirm PSRAM initialization, Wi-Fi association, DHCP, internet connectivity and NTP time synchronization.
- The first hardware boot exposed a main-task stack overflow during UI initialization. `CONFIG_ESP_MAIN_TASK_STACK_SIZE=8192` was added to `sdkconfig.defaults`; after rebuilding and reflashing, startup proceeds past UI initialization without that stack-overflow error.
- The first F1 tile tap exposed an LVGL null-screen crash. The F1 page is now built during UI startup and guarded against a failed root-screen allocation. The latest firmware boots and connects to Wi-Fi, but touchscreen navigation into F1 still needs a physical retest.
- F1 screen code is implemented in `main/tech_terminal_ui.c`: it requests race sessions from OpenF1 in a worker task, shows up to five upcoming races, has loading/error states, and falls back to recent races if the season has no future sessions. The endpoint has been tested from the development PC; the device-side F1 request and displayed result are not yet verified.
- Clean ESP-IDF 5.4.4 build result: PASS, exit code 0; generated `build/indicator_basis.bin`.
- Latest firmware binary size: `0x218360` bytes; app partition is `0x400000` bytes with 48% free.
- The two renamed Wi-Fi config options were updated in `sdkconfig.defaults`; reconfigure/build completed without the previous migration warnings.

## Next actions

1. Physically tap the F1 tile and confirm the F1 page opens without a reset; inspect the serial log for the OpenF1 response.
2. Confirm the home screen, back navigation and other modules by touch.
3. If OpenF1 TLS or memory errors appear on-device, inspect the exact error before changing TLS or heap settings.
4. Add F1 results and caching, then continue reliability and remaining module work.
