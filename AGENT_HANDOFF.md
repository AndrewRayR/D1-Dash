# D1-Dash — AI Agent Handoff

> **Mandatory:** Read this file before making changes. Update it during work and before handing the repository to another agent. Pair it with the repository instructions in `AGENTS.md`.

Last verified against the local checkout: **2026-11-02**
Repository: [AndrewRayR/D1-Dash](https://github.com/AndrewRayR/D1-Dash)
Local checkout: `C:\Projects\AndrewRayR\D1-Dash`
Primary branch: `main`
Remote: `origin = https://github.com/AndrewRayR/D1-Dash.git`

## 1. Project purpose

D1-Dash is an expandable, touchscreen-first Tech Terminal for the Seeed Studio SenseCAP Indicator D1. It uses the onboard ESP32-S3, ESP-IDF, and LVGL. The goal is a useful personal status terminal, not a smart-home controller. The initial project is designed to use the built-in hardware only; do not add hardware requirements without the user's approval.

Core design requirements:
- Keep the existing board support package (BSP), LVGL integration, and application structure intact.
- Keep network I/O and parsing out of the LVGL/UI task; marshal results safely back to the UI.
- Use bounded HTTP timeouts, response sizes, redirects, and robust error handling.
- Keep settings persistent through the existing NVS settings layer.
- Keep credentials and secrets out of Git.
- Prefer compact, readable, text-first UI suitable for the Indicator display.
- Keep the architecture modular and expandable.

## 2. Current verified repository state (2026-11-02)

The following was checked on `DESKTOP-415LS7I`:

- Latest pushed repository state before this documentation update: 3bf1f57 — docs: record F1 offline cache regression pass; branch main was clean and synchronized with origin/main before recording the Web Reader test.
- The phone portal HTTP server stack increase from 6,144 to 12,288 bytes is committed in `98d9987`.
- ESP-IDF 5.4.4 build, COM21 flash, and serial boot verification passed on 2026-11-02; details are recorded in `docs/DEVELOPMENT_STATUS.md` and the handoff log below.
- The user has now physically confirmed the GitHub Monitor normal workflow on the current firmware: repository information and Actions status, Refresh, Back Home, and reopening all behaved as designed. Rate-limit/403/429 presentation was not deliberately triggered.

**User-reported hardware status:** The user has explicitly confirmed that the issue they were following up on is now fixed. In the phone-configuration context, the recent code and local change concern the display/save path and the HTTP server task stack. Treat the user's confirmation as a physical-use report, but do not claim that you personally reproduced or tested it.

**Documentation status:** `docs/DEVELOPMENT_STATUS.md` and `docs/ROADMAP.md` were reconciled on 2026-11-02. They distinguish user-confirmed normal-network phone portal behavior from hardware tests still pending, especially recovery captive-portal testing and final module reliability checks.

## 3. Hardware and development environment

- Board: Seeed Studio SenseCAP Indicator D1, ESP32-S3 plus onboard RP2040.
- Display/touch: onboard hardware using the existing BSP and LVGL port.
- Build target: `esp32s3`.
- Intended/installed toolchain on the development PC: ESP-IDF **5.4.4**.
- Windows development PC: `DESKTOP-415LS7I`.
- Project directory: `C:\Projects\AndrewRayR\D1-Dash`.
- ESP-IDF path previously verified: `C:\Espressif\frameworks\esp-idf-v5.4.4`.
- ESP32-S3 Python/toolchain environment previously found under `C:\Users\andre\.espressif`.
- **COM21** was re-identified on 2026-11-02 as `USB-SERIAL CH340` and positively queried with esptool as the Indicator D1 ESP32-S3 (revision v0.2, MAC `74:4d:bd:95:b0:28`, 8 MB PSRAM). Firmware was successfully flashed to COM21 with all image hashes verified; boot logs confirmed SenseCAP_Indicator, PSRAM test, LVGL touch input, internet access, and NTP sync.
- **COM22** (`USB Serial Device`, VID `2E8A`, PID `000A`) remains separate and was not flashed. Always re-identify ports before flashing; never assume port assignments persist.
- Previous hardware logs confirmed SenseCAP_Indicator detection, 8 MB PSRAM initialization, Wi-Fi association, DHCP, internet connectivity, and NTP synchronization.

## 4. Implemented product areas

The current product has five core modules:

1. **Web Reader** — text-first HTTP/HTTPS reader, URL validation, bounded fetching, readable text extraction, and session-only Back/Forward history. It is not a full JavaScript-capable browser.
2. **Network Monitor / Wi-Fi Manager** — connection state, SSID, RSSI, IP/gateway information, nearby 2.4 GHz scans, password entry, and network connection flow.
3. **Raspberry Pi Monitor** — configurable JSON endpoint and parsed host/CPU/memory/disk/temperature/uptime information.
4. **GitHub Monitor** — repository metrics and latest Actions workflow state when available, with explicit rate-limit/access and no-runs states.
5. **F1** — OpenF1 race schedule and latest completed race/podium information, with refresh and a bounded volatile RAM fallback cache.

Settings include brightness, screen timeout, module refresh interval, browser home URL, GitHub owner/repository, Raspberry Pi endpoint, and information location. The refresh interval is persisted in NVS and used by active module timers; public GitHub/OpenF1 polling has a five-minute minimum, while Web Reader remains user-triggered.

An authenticated phone configuration service is implemented:
- Recovery mode provides a temporary setup Wi-Fi network, on-screen credentials/code, local DNS/HTTP captive portal, login throttling, and a sliding authenticated session.
- Normal-network setup can be started from the device's **PHONE SETUP** control; the D1 displays its LAN address and a fresh six-digit code.
- The phone page updates supported settings; Wi-Fi credential changes are reserved for recovery mode.
- The user has previously reported successful access to the configuration webpage and that a location change appeared on the device.

## 5. Current next actions

1. Normal-network phone portal regression is complete: the user confirmed sign-in, settings-only save, and continued display responsiveness on the newly flashed firmware.
2. F1 offline-cache fallback and Web Reader redirects/HTTP errors/long pages/Back-Forward navigation are now user-confirmed passes on the current firmware. Continue with repeated entry/exit across all five modules, touch responsiveness, automatic refresh timing, and prolonged operation. GitHub normal workflow/refresh/navigation is also user-confirmed pass; do not deliberately induce rate limits.
3. Verify automatic refresh timing and prolonged operation; record the tested firmware build and release version.
4. Separately test recovery AP/captive-portal behavior on Android/iOS, including manual URL, wrong-password retry, and Wi-Fi recovery. This remains distinct from normal-network configuration.
5. Revisit BLE HID and USB keyboard-dongle feasibility only after the core reliability pass; do not implement optional expansions without prioritization.
6. For each code change, run `git diff --check` and an ESP-IDF 5.4.4 build. Flash only when necessary and after positively identifying the ESP32-S3 port.

## 6. Build and test quick reference

Open an ESP-IDF 5.4.4 PowerShell environment, then:

```powershell
Set-Location 'C:\Projects\AndrewRayR\D1-Dash'
idf.py build
```

If the build directory was generated with another ESP-IDF release, follow `docs/BUILDING.md` and perform the documented clean/reconfigure procedure. Do not casually delete build state.

Before a device flash:
1. Confirm a successful build.
2. Inspect Device Manager / esptool output to identify the ESP32-S3 port.
3. Confirm it is the Indicator D1, not just an assumed COM number.
4. Flash only the confirmed port and verify the esptool hash/reset result.
5. Record the actual result and firmware identity.

Useful commands:

```powershell
git status --short --branch
git log -8 --oneline --decorate
git diff --check
git diff
idf.py build
```

## 7. Architecture and code navigation

- `main/main.c` — application startup.
- `main/tech_terminal_ui.c`, `main/tech_terminal_ui.h` — Tech Terminal UI, navigation, module views, and refresh timers.
- `main/tech/` — HTTP, settings, recovery/configuration portal, and feature services.
- `main/model/`, `main/view/`, `main/controller/` — existing application separation.
- `main/lv_port.c` — LVGL display/input integration.
- `components/bsp/` — board support, display, touch, and peripheral interfaces.
- `components/i2c_devices/` — onboard I²C device drivers.
- `sdkconfig.defaults`, `dependencies.lock` — shared build configuration/dependency baseline.

Read these before architectural changes:
- `docs/ARCHITECTURE.md`
- `docs/BUILDING.md`
- `docs/DEVELOPMENT_STATUS.md`
- `docs/ROADMAP.md`
- `CONTRIBUTING.md`

## 8. Mandatory handoff update format

Every agent must update this file before handing off. Add a dated entry at the top of the handoff log and refresh the current-state/next-actions sections when needed.

For each handoff, record:
- **Agent/task:** what you were asked to do.
- **Changes:** exact files and concise descriptions.
- **Validation:** commands/tests actually run and pass/fail results.
- **Hardware:** whether flashed; exact verified port; user-confirmed versus agent-run tests.
- **Git:** branch, commit SHA, whether pushed, and remaining uncommitted changes.
- **Outstanding:** known bugs, unverified paths, and risks.
- **Next action:** one concrete step the next agent should take.

Never delete older handoff entries; append new entries so the project retains a durable transition history.

## 9. Handoff log

### 2026-11-02 � Speed display / recovery AP fixes flashed and boot-verified
- Re-identified COM21 as USB-SERIAL CH340 (VID 1A86, PID 7523); read-only esptool identification confirmed ESP32-S3 revision v0.2, MAC 74:4d:bd:95:b0:28, 8 MB PSRAM. COM22 remained USB Serial Device (VID 2E8A, PID 000A) and was not accessed.
- User authorized flashing. ESP-IDF 5.4.4 idf.py -p COM21 flash completed successfully; bootloader, application, and partition table hashes verified; hard reset succeeded.
- Serial boot log confirmed app version 7d551d5, SenseCAP Indicator detection, PSRAM memory test OK, and normal startup. Saved station credentials did not authenticate; firmware then successfully switched to softAP, started the recovery configuration portal with captive DNS, and started DHCP on 192.168.4.1.
- Serial log reported recovery SSID D1-Dash-Setup-48D3. This confirms the AP startup code reached softAP/beacon initialization and the portal/DHCP started. Actual phone discovery and connection remain user-verification items.
- Flash and boot are agent-verified; numeric speed display, phone SSID visibility, and portal connection are not yet physically verified.
- Next: user should check the phone Wi-Fi list for the recovery SSID shown on the Indicator, connect using the password shown on the device, and report whether the portal appears. Then test Network Monitor speed results after restoring Wi-Fi.

### 2026-11-02 � Network Monitor speed display and recovery AP fix
- User-reported regression on the expanded Network Monitor firmware: speed test reached completion but displayed literal f characters (Down f Mbps Up f Mbps); the Wi-Fi Disconnect action worked, but reconnecting failed and entered Recovery Mode while the temporary SSID was not visible to the phone.
- Root cause of speed display: LVGL's compact lv_label_set_text_fmt formatter does not support floating-point conversions. Updated main/tech_terminal_ui.c to round Mbps to hundredths and render integer fields.
- Recovery AP investigation found the existing code switched the running Wi-Fi driver directly from STA to AP without an explicit stop/start cycle. Updated main/model/indicator_wifi.c to stop the driver, set AP mode/configuration, start the AP radio, and then start the recovery portal; failure cleanup restores STA mode.
- ESP-IDF 5.4.4 idf.py build: PASS. App image 0x223D20 bytes; 47% of the 4 MiB app partition remains. git diff --check: PASS.
- Hardware: no flash performed for these fixes. Physical AP visibility and speed display remain unverified until the user authorizes and tests the updated firmware. COM22 remains untouched.
- Next: flash only after re-identifying COM21 as the Indicator ESP32-S3, then test numeric speed output and recovery SSID visibility / reconnect.

### 2026-11-02 — COM21 reconnected, firmware flashed, and boot verified
- COM21 reappeared as `USB-SERIAL CH340`; COM22 remained a separate `USB Serial Device` and was not used.
- esptool read-only identification confirmed ESP32-S3 revision v0.2, MAC `74:4d:bd:95:b0:28`, Wi-Fi/BLE, and 8 MB embedded PSRAM.
- Flashed the current ESP-IDF 5.4.4 firmware, bootloader, and partition table to COM21. All write hashes verified; esptool hard-reset the board and returned exit code 0.
- Serial boot logs confirmed app version `73dbb63`, SenseCAP_Indicator board detection, 8 MB PSRAM initialization and memory test, LVGL touch input initialization, successful city lookup/internet access, and NTP time synchronization. No immediate crash or stack-overflow was observed.
- The serial check itself did not exercise touch navigation or phone-portal save. Subsequent user-confirmed physical checks passed for the home screen, phone-portal save, and GitHub Monitor workflow/navigation.
- Next agent: continue with F1 offline-cache fallback, then Web Reader edge cases and full navigation/refresh soak testing.

### 2026-11-02 — Portal stack verification and documentation refresh
- Reviewed the pre-existing `main/tech/tech_recovery_portal.c` change: HTTP server task stack increased from 6,144 to 12,288 bytes to accommodate authenticated form parsing and settings-save handling.
- Reinitialized ESP-IDF 5.4.4 from `C:\Espressif\frameworks\esp-idf-v5.4.4` (Python 3.13.2) and ran `idf.py build`; all 1,337 Ninja steps completed successfully.
- Generated `build/indicator_basis.bin` at `0x222AF0` bytes; 47% of the 4 MiB app partition remains. `git diff --check` passed.
- Updated `docs/DEVELOPMENT_STATUS.md` and `docs/ROADMAP.md` with the build result and the user's reported phone-save fix. Recovery captive-portal and other module tests remain pending.
- Hardware: no flash performed. Only COM22 (`USB Serial Device`, VID `2E8A`, PID `000A`) was enumerated; COM21 was absent, so COM22 was left untouched.
- Commit `98d9987` (`fix(config): increase phone portal server stack`) contains the portal stack change and status/roadmap updates; pushed to `origin/main`.
- Working tree was clean after push. No physical regression test was possible during this session.
- Next agent: continue the final physical reliability pass, beginning with the normal-network phone portal regression test when COM21/ESP32-S3 is available.

### 2026-11-02 — Initial shared AI handoff file
- Added `AGENTS.md` to make the handoff procedure discoverable to repository-aware coding agents.
- Added this `AGENT_HANDOFF.md` as the shared project-state and transition record.
- Verified local repository branch/HEAD and inspected the current uncommitted change in `main/tech/tech_recovery_portal.c`.
- No firmware build or flash was performed as part of this documentation task.
- The existing portal stack-size modification was intentionally left untouched and unstaged.
- Next agent: continue the final physical reliability pass, beginning with the normal-network phone portal regression test when COM21/ESP32-S3 is available.

### 2026-11-02 — Normal-network phone save regression passed
- User confirmed the newly flashed COM21 firmware booted to the normal D1-Dash home screen.
- User then completed the normal-network PHONE SETUP regression: authenticated from a phone on the same Wi-Fi, saved a settings-only change, and confirmed the Indicator remained usable/responded afterward.
- This is a user-confirmed physical test; the assistant did not independently operate the touchscreen or phone.
- No source-code changes were required. COM22 remains untouched.
- Next action: test F1 cached-data fallback after a successful online load, then continue Web Reader edge cases, repeated navigation, refresh timing, and recovery portal tests.

### 2026-11-02 — GitHub Monitor physical regression passed
- User confirmed the GitHub Monitor behaved as designed on the current firmware: repository information and Actions status appeared, Refresh worked, Back Home returned correctly, and reopening the module worked.
- Result: PASS (user-confirmed physical test). No source-code change, build, or flash was needed for this verification.
- The 403/429 rate-limit/access state was not deliberately triggered; avoid generating excessive API requests merely to force this state.
- COM22 remains untouched. No hardware action was performed during this documentation update.
- Next action: run a repeated entry/exit and touch-responsiveness pass across all five modules, then verify automatic-refresh timing and prolonged operation.

### 2026-11-02 — F1 offline-cache physical regression passed
- User confirmed the F1 schedule loaded successfully online, then the previously loaded schedule was displayed after a subsequent refresh request failed while the device remained powered on.
- The cached/offline UI state appeared as designed. This validates the bounded 900-byte, 30-minute RAM fallback on the current firmware.
- Result: PASS (user-confirmed physical test). No source-code change, build, or flash was required. The cache is intentionally RAM-only and was not expected to survive a reboot.
- GitHub Monitor normal workflow/navigation and F1 offline fallback are both user-confirmed passes. GitHub 403/429 error messaging remains unverified and was not deliberately triggered.
- COM22 remains untouched. No hardware action was performed during this documentation update.
- Next action: run a repeated entry/exit and touch-responsiveness pass across all five modules, then verify automatic-refresh timing and prolonged operation.

### 2026-11-02 — Web Reader physical regression passed
- User confirmed a one-hop HTTP redirect, HTTP 404 handling, long plain-text page readability/scrolling, and Back/Forward history across multiple pages all behaved as designed.
- Returning Home and reopening Web Reader also worked.
- Result: PASS (user-confirmed physical test). No source-code change, build, or flash was required.
- The user has now confirmed normal phone setup/save, GitHub Monitor workflow/navigation, F1 offline-cache fallback, and Web Reader edge cases/navigation on the current firmware. GitHub 403/429 messaging and recovery AP/captive portal remain unverified.
- COM22 remains untouched. No hardware action was performed during this documentation update.
- Next action: run a repeated entry/exit and touch-responsiveness pass across all five modules, then verify automatic-refresh timing and prolonged operation.


### 2026-11-02 ? Network Monitor and GitHub Monitor feature expansion
- User requested download/upload speed testing and an optional Wi-Fi disconnect action in Network Monitor; GitHub repository-wide commit count, age of latest commit, changed filenames only, fork count, watcher count, and removal of OPEN IN WEB.
- Network Monitor implementation: added a bounded Cloudflare speed test using 500,000 download bytes and 200,000 upload bytes; runs in a worker task with TLS certificate verification and a 20-second per-request timeout. Added Wi-Fi disconnect action and explicit suppression of both event-handler and background-task automatic reconnect. User can reconnect through the existing Wi-Fi Manager.
- GitHub Monitor implementation: reads forks_count and subscribers_count, obtains repository-wide commit count from the commits API pagination Link header, calculates latest-commit age from the UTC committer timestamp, and fetches latest commit metadata to display changed filenames only. File contents are not rendered. Existing Actions summary and manual refresh remain. Removed the OPEN IN WEB control.
- Extended the shared HTTP response to capture the Link response header. New speed-test service is in main/tech/tech_speed_test.c and main/tech/tech_speed_test.h.
- Validation: git diff --check passed. ESP-IDF 5.4.4 build passed; generated build/indicator_basis.bin size 0x223CA0, leaving 47% of the 4 MiB app partition free. A live GitHub REST check on the development PC confirmed subscribers_count, a pagination Link header with rel="last" and page=48, latest commit timestamp inputs, and commit detail files[].filename. The GitHub API check was read-only.
- Hardware: no flash performed; no physical feature test yet. COM21/COM22 were not accessed.
- Git: commit c827c35 (feat: expand network and GitHub monitors) was committed on main and pushed successfully to origin/main.
- Outstanding: verify the Cloudflare upload/download endpoints and reported throughput on the D1; verify explicit disconnect remains disconnected and Wi-Fi Manager reconnect works; physically verify all new GitHub metrics and filename-only list. The speed test is an estimate of throughput to Cloudflare's test endpoint, not a guaranteed ISP line-rate certification.
- Next action: ask the user to authorize a COM21 firmware flash, then run physical regression tests for the new Network Monitor and GitHub Monitor features.


### 2026-11-02 ? Monitor feature firmware flashed and boot verified
- User authorized flashing the updated Network Monitor/GitHub Monitor firmware.
- Re-enumerated serial devices: COM21 = USB-SERIAL CH340 (VID 1A86, PID 7523); COM22 = separate USB Serial Device (VID 2E8A, PID 000A).
- Read-only esptool identification on COM21 confirmed ESP32-S3 revision v0.2, MAC 74:4d:bd:95:b0:28, Wi-Fi/BLE, and 8 MB PSRAM. COM22 was not accessed.
- Ran ESP-IDF 5.4.4 idf.py -p COM21 flash. Bootloader, application, and partition table were written; all three hashes verified and hard reset completed successfully. Application image size 0x223CA0 bytes; 47% of the 4 MiB app partition remains.
- Attached serial monitor and observed boot: app version 47d0c8c, ESP-IDF 5.4.4, SenseCAP_Indicator detected, 8 MB PSRAM memory test passed, Wi-Fi associated to saved network, DHCP address 192.168.0.85, ping 8.8.8.8 succeeded (4/4), city lookup succeeded, and NTP synchronized. No immediate crash or stack overflow was observed.
- Hardware status: flash and boot are agent-verified. Touchscreen feature behavior remains unverified; the user must exercise the new speed test, disconnect/reconnect, and GitHub metrics controls.
- Git: source changes remain at c827c35, documentation delivery commit 47d0c8c; a further documentation-only update is pending.
- Next action: user physically tests the new Network Monitor and GitHub Monitor features.

### 2026-11-02 - Recovery reconnect honors explicit user reconnect
- User confirmed that after submitting Wi-Fi details through the recovery portal, the Indicator remains on the Wi-Fi Recovery screen.
- Source inspection found a state bug: the Network Monitor explicit Disconnect action sets s_user_disconnect=true to suppress automatic reconnects, but the recovery-portal credential submission did not clear that flag. A failed station attempt could therefore be treated as another intentional disconnect, bypassing the recovery retry branch.
- Updated main/model/indicator_wifi.c to clear user-disconnect suppression before starting the explicit recovery reconnect attempt. Added a station-disconnect log containing the ESP-IDF reason code and relevant reconnect state (no credentials are logged).
- Validation: ESP-IDF 5.4.4 idf.py build passed; app image 0x223D60 bytes with 47% of the 4 MiB app partition free. git diff --check passed.
- Hardware: this fix has not been flashed or physically tested. COM21 must be re-identified as the ESP32-S3 before any flash; COM22 remains untouched.
- Next: commit/push the source fix, obtain user authorization to flash the verified COM21 port, then test recovery reconnect. If it still fails, the new serial reason code will distinguish authentication, AP-not-found, and other station failures.
