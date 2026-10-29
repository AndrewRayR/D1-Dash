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


## GitHub Monitor crash investigation (2026-10-28)

- After the user opened GitHub Monitor and tapped additional screen locations, the display went black and the UI stopped responding.
- COM21 serial output captured a `LoadProhibited` Guru Meditation in `lv_scr_load_anim`, called from `open_github_page()` at `main/tech_terminal_ui.c:1121` during the `open_module()` touch event.
- Root cause identified in `show_home()`: GitHub Monitor's `github_page` is cached in a static pointer, but unlike the other cached pages it was omitted from the persistent-page exclusions before `lv_obj_del_async(active_page)`. Returning home could delete the page while leaving `github_page` non-NULL; opening it again then attempted to load a deleted LVGL screen.
- Fix: exclude `github_page` from `show_home()`'s deletion condition, matching the lifecycle of the other cached screens.
- Rebuild passed; firmware flashed to COM21 with esptool image hash verification and hard reset. COM22 remains untouched. Physical retest of GitHub Monitor, Back Home, and repeated entry is still required.

## GitHub Monitor task stack overflow (2026-10-28)

- Reproduced the black-screen failure by opening GitHub Monitor and captured the serial output on COM21.
- Confirmed ESP-IDF reported `A stack overflow in task github_repo has been detected` and halted the CPU.
- Increased the `github_repo` task stack in `main/tech_terminal_ui.c` from 6,144 to 12,288 bytes to accommodate HTTPS/TLS and JSON parsing.
- `idf.py build` succeeded with ESP-IDF v5.4.4. The firmware was flashed only to the Indicator D1 on COM21; esptool verified the written image hash and the device rebooted.
- The user confirmed that opening GitHub Monitor works after this change, so the immediate black-screen reproduction is resolved in their hardware test. Continue monitoring for stability during refresh and repeated navigation.

## Settings keyboard visibility improvement (2026-10-28)

- Made the settings form vertically scrollable and resize it to the visible area when the on-screen keyboard opens, then scroll the selected text field into view.
- The keyboard close/cancel and Save actions now hide the keyboard, restore the full form height, and return the form to its top position.
- `git diff --check` and ESP-IDF 5.4.4 build passed. The updated firmware was flashed only to verified COM21; esptool verified all written image hashes and reset the board. The user physically tested Settings after flashing and confirmed the test passed.

## Raspberry Pi Monitor and Web Reader task stacks (2026-10-28)

- Increased the `pi_status` task stack from 6,144 to 12,288 bytes to provide headroom for HTTPS/TLS and response parsing. Commit `ff9dfa9` was pushed to `origin/main`, and the firmware was successfully flashed to COM21 with image hash verification and hard reset.
- Increased the `web_reader` task stack from 6,144 to 12,288 bytes for the same HTTPS/TLS and response-processing risk. ESP-IDF 5.4.4 build passed, and this firmware was flashed to COM21 with image hash verification and hard reset.
- Physical validation of Raspberry Pi Monitor and Web Reader on this newest firmware is still pending; verify successful requests, invalid URLs, back navigation, and repeated entry/exit before marking these paths hardware-verified.

## Network Monitor and Wi-Fi Manager hardware verification (2026-10-29)

- The Network Monitor is complete: it reports Wi-Fi state, IP address, signal strength (RSSI), and basic network diagnostics.
- Added a dedicated Tech Terminal Wi-Fi Manager with nearby 2.4 GHz scan results, SSID/security/signal display, password entry, connection requests, scan refresh, and return navigation to Network Monitor.
- Wi-Fi scan and connection events are handled through the existing view event system. The stock Wi-Fi list handler ignores scan results while the Tech Terminal Wi-Fi Manager is the active screen.
- ESP-IDF 5.4.4 build and COM21 firmware flash succeeded. The user physically verified the Wi-Fi Manager workflow, including network list, password dialog/keyboard, cancel/reopen, and return/re-entry navigation.
- COM22 remains untouched.


## Settings refresh-interval control (2026-10-29)

- Added a touchscreen slider for the default module refresh interval, adjustable from 1 to 60 minutes. The selected value is saved through the existing versioned settings/NVS path as `refresh_seconds`.
- Expanded the scrollable settings form to accommodate the control while retaining the existing brightness, screen-timeout and endpoint fields.
- ESP-IDF 5.4.4 build and COM21 flash passed; serial boot confirmed PSRAM initialization and Wi-Fi reconnection.
- The user physically verified changing the interval, saving, leaving/reopening Settings, and persistence after reboot. The UI control is verified; the broader Settings milestone remains open until refresh scheduling is consistently applied by the modules.
- COM22 remains untouched.
