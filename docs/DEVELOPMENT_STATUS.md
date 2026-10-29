# Development Status

Last updated: 2026-10-28

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
- The first F1 tile tap exposed an LVGL crash when dynamically loading a separate screen; pre-creating that screen did not eliminate the issue. The F1 UI now uses a hidden overlay attached to the home screen instead of loading a new LVGL screen. The user subsequently tested the latest firmware on the physical Indicator D1: tapping F1 successfully contacted OpenF1, displayed upcoming races, and showed Refresh and Back Home controls. This confirms the F1 overlay, device-side network request, response parsing, and primary controls work on hardware.
- F1 screen code is implemented in `main/tech_terminal_ui.c`: a hidden home-screen overlay requests race sessions from OpenF1 in a worker task, shows up to five upcoming races, has loading/error states, and falls back to recent races if the season has no future sessions. The user verified on the physical device that tapping F1 retrieves and displays upcoming races and exposes Refresh and Back Home controls.
- F1 retains the last successful display text (schedule plus latest-result section when available) in a fixed 900-byte RAM buffer and can display it as `CACHED RACES (OFFLINE)` if a later request fails within 30 minutes. This cache is volatile and is lost on reboot. The user confirmed the results-enabled screen displays the latest race podium successfully on hardware; explicit offline-cache behavior is not yet verified.
- GitHub Monitor now requests the latest Actions workflow run after fetching repository metrics, and reports clear rate-limit/access messaging for HTTP 403 or 429. The public D1-Dash repository currently returns zero workflow runs in the validation query, so the UI should show a no-runs state rather than a fabricated status. The feature builds and is flashed to verified COM21; the new workflow UI still needs physical verification.
- Web Reader now validates HTTP(S) scheme and requires a nonempty host; it rejects whitespace/control characters, backslashes, and userinfo in the authority before starting a network task. This change builds successfully and was flashed to COM21; malformed-URL behavior still needs touch verification.
- Touch input diagnosis: `components/bsp/src/indev/indev_tp.c` already applies the configured board coordinate swap/mirroring, so the duplicate axis inversion in `main/lv_port.c` was removed. The user confirmed the correct home tile then opened, but taps on the destination page still did not work. A second bug was found in `indev_get_major_value()`: it could leave output untouched after a failed touch-I2C read while still returning `ESP_OK`. The BSP now propagates read failures to LVGL rather than claiming success, and the LVGL callback releases safely on errors. Transition-only press/release logs were added to help distinguish missing touch samples from UI event problems. Build passes; this diagnostic firmware needs physical verification.
- Touch calibration on the physical display recorded center near (230,240), physical top-left near (458,444), and physical bottom-right near (43,44). This establishes that both reported axes are inverted relative to screen position. `main/lv_port.c` now applies the measured X/Y inversion after the BSP read. The corrected firmware built successfully and was flashed to COM21 with esptool hash verification. Physical verification passed: the user confirmed the intended tile opens correctly and Back returns home; the F1 schedule loads, Refresh works without freezing, Back Home works, and F1 can be reopened successfully.
- Latest clean ESP-IDF 5.4.4 build after the GitHub workflow update: PASS, exit code 0; generated `build/indicator_basis.bin`.
- Latest firmware binary size: `0x219000` bytes; app partition is `0x400000` bytes with 48% free. The GitHub workflow-enabled firmware was flashed successfully to verified COM21; esptool verified image hashes and hard-reset the board. The new GitHub workflow view and F1 offline-cache behavior still need physical verification. COM22 remains untouched.
- The two renamed Wi-Fi config options were updated in `sdkconfig.defaults`; reconfigure/build completed without the previous migration warnings.

## Next actions

1. Confirm the other modules and settings/navigation by touch.
2. Physically verify GitHub latest workflow status and the F1 offline-cache fallback.
3. Continue reliability work and the remaining modules.
