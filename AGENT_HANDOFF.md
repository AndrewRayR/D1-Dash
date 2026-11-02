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

- `HEAD`: `673fcb8` — `fix(config): keep display awake after phone save`.
- `main` and `origin/main` pointed to the same commit at inspection.
- Recent history includes the phone configuration portal, sign-in/header fixes, invalid-PIN feedback, and display-awake-after-save fix.
- The working tree is **not clean**. There is one pre-existing modification:
  - `main/tech/tech_recovery_portal.c`
  - Diff increases `httpd_config_t.stack_size` from 6144 to 12288 bytes and adds a comment explaining that authenticated phone-save form handling overflowed the 6 KiB server task stack.
  - This change is **uncommitted** and must be preserved. Do not reset, overwrite, stage, or commit it unless its owner/user explicitly authorizes that.
- Do not assume the current uncommitted change has been pushed. Verify before making any claims about its delivery.

**User-reported hardware status:** The user has explicitly confirmed that the issue they were following up on is now fixed. In the phone-configuration context, the recent code and local change concern the display/save path and the HTTP server task stack. Treat the user's confirmation as a physical-use report, but do not claim that you personally reproduced or tested it.

**Important:** The detailed `docs/DEVELOPMENT_STATUS.md` and `docs/ROADMAP.md` were last updated 2026-10-29 and contain older pending-test notes. They are useful history, not a fully current status snapshot. Reconcile them with current code and the user's latest confirmation before changing their checkboxes.

## 3. Hardware and development environment

- Board: Seeed Studio SenseCAP Indicator D1, ESP32-S3 plus onboard RP2040.
- Display/touch: onboard hardware using the existing BSP and LVGL port.
- Build target: `esp32s3`.
- Intended/installed toolchain on the development PC: ESP-IDF **5.4.4**.
- Windows development PC: `DESKTOP-415LS7I`.
- Project directory: `C:\Projects\AndrewRayR\D1-Dash`.
- ESP-IDF path previously verified: `C:\Espressif\frameworks\esp-idf-v5.4.4`.
- ESP32-S3 Python/toolchain environment previously found under `C:\Users\andre\.espressif`.
- **COM21** was positively identified as the Indicator D1 ESP32-S3 and used for prior verified flashes.
- **COM22** enumerated separately and was deliberately not flashed. Always re-identify ports before flashing; never assume port assignments persist.
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

1. **Protect and resolve the pre-existing working-tree change.** Inspect the exact diff in `main/tech/tech_recovery_portal.c`, confirm with the user/active task owner whether it is intended to be retained, then build and test it. Do not silently stage or commit it.
2. Verify the current phone-save fix on the physical device if any part of that path remains untested. The user has already said the issue is fixed; avoid asking them to repeat a successful test without a reason.
3. Reconcile `docs/DEVELOPMENT_STATUS.md` and `docs/ROADMAP.md` with the current source and the user's latest physical confirmations. Do not mark unrelated old checks complete just because the phone-save issue is fixed.
4. Continue the final reliability pass: repeated module entry/exit, touch responsiveness, refresh behavior, network failure handling, and prolonged operation.
5. Check the remaining roadmap items, especially BLE HID keyboard feasibility/implementation, USB keyboard-dongle feasibility via the RP2040 path, and optional Information feeds. Do not implement optional expansions without prioritization.
6. For each code change, run `git diff --check` and an ESP-IDF 5.4.4 build when the environment is available. Flash only when necessary and after confirming the correct serial port.

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

### 2026-11-02 — Initial shared AI handoff file
- Added `AGENTS.md` to make the handoff procedure discoverable to repository-aware coding agents.
- Added this `AGENT_HANDOFF.md` as the shared project-state and transition record.
- Verified local repository branch/HEAD and inspected the current uncommitted change in `main/tech/tech_recovery_portal.c`.
- No firmware build or flash was performed as part of this documentation task.
- The existing portal stack-size modification was intentionally left untouched and unstaged.
- Next agent: preserve/review the existing portal change, then reconcile the dated development status and continue reliability testing.
