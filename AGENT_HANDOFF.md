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

- Last pushed baseline before the current hardware verification: `73dbb63` — `docs(agents): refresh D1-Dash handoff` (includes implementation commit `98d9987`).
- The phone portal HTTP server stack increase from 6,144 to 12,288 bytes is committed in `98d9987`.
- ESP-IDF 5.4.4 build, COM21 flash, and serial boot verification passed on 2026-11-02; details are recorded in `docs/DEVELOPMENT_STATUS.md` and the handoff log below.
- Re-check Git status and remote synchronization before making changes.

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

1. Perform a regression check of the normal-network phone portal on the newly flashed firmware: sign in, save a settings-only change, confirm the display stays responsive, and close phone access. The user has already reported the latest issue fixed.
2. Complete the physical reliability pass: GitHub workflow/no-runs and rate-limit states, F1 offline-cache fallback, Web Reader redirects/HTTP errors/long pages, repeated module entry/exit, and touch responsiveness.
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

### 2026-11-02 — COM21 reconnected, firmware flashed, and boot verified
- COM21 reappeared as `USB-SERIAL CH340`; COM22 remained a separate `USB Serial Device` and was not used.
- esptool read-only identification confirmed ESP32-S3 revision v0.2, MAC `74:4d:bd:95:b0:28`, Wi-Fi/BLE, and 8 MB embedded PSRAM.
- Flashed the current ESP-IDF 5.4.4 firmware, bootloader, and partition table to COM21. All write hashes verified; esptool hard-reset the board and returned exit code 0.
- Serial boot logs confirmed app version `73dbb63`, SenseCAP_Indicator board detection, 8 MB PSRAM initialization and memory test, LVGL touch input initialization, successful city lookup/internet access, and NTP time synchronization. No immediate crash or stack-overflow was observed.
- No physical touch navigation or phone-portal save test was performed during this remote serial check; those remain the next user/device verification steps.
- Next agent: ask the user to confirm the display reaches the home screen, then test PHONE SETUP login/save/close and proceed through the remaining module checks.

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
