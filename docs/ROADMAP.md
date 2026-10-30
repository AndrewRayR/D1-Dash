# D1-Dash Roadmap

Last reviewed: 2026-10-29

This roadmap separates features already implemented from checks that still require the physical device. The five home-screen modules are the current core product; optional feeds and alternate input/configuration methods are future expansions.

## 1. Platform and navigation

- [x] Recover the SenseCAP Indicator D1 project baseline and document the ESP32-S3/RP2040 architecture.
- [x] Build with ESP-IDF 5.4.4 for ESP32-S3.
- [x] Verify the Indicator D1 on COM21 (8 MB flash and 8 MB PSRAM); COM22 is not used.
- [x] Initialize the display, touch input, Wi-Fi, and persistent settings.
- [x] Resolve touch coordinate inversion and the LVGL screen-lifecycle issues found during testing.
- [x] Provide a consistent Home action on the implemented modules.
- [x] Physically verify core tile navigation, F1 entry/exit, repeated F1 entry, refresh, Wi-Fi Manager navigation, and Settings access.
- [ ] Run a final full-module navigation and soak test on the physical display.

## 2. Core modules

### Network Monitor and Wi-Fi Manager
- [x] Show Wi-Fi connection state, SSID, signal strength, IP address, and gateway.
- [x] Scan nearby 2.4 GHz networks and show signal/security information.
- [x] Join a selected network using the on-screen password keyboard.
- [x] Handle scan/connect results through the view event system.
- [x] User physically verified the Wi-Fi Manager workflow and navigation.

### Settings
- [x] Persist settings in NVS with schema validation and migration.
- [x] Configure display brightness and screen timeout.
- [x] Configure browser home URL, GitHub owner/repository, and Raspberry Pi status endpoint.
- [x] Persist the default module refresh interval (1–60 minutes).
- [x] Apply refresh timers to Network Monitor, Raspberry Pi Monitor, GitHub, and F1; timers pause their work while their module is not active.
- [x] Reconfigure existing timers when the saved interval changes.
- [x] User physically verified saving, reopening, and reboot persistence of the refresh interval.
- [ ] Physically verify automatic refresh timing and confirm it does not interfere with touch/navigation.

Note: public API modules (GitHub and OpenF1) enforce a five-minute minimum polling interval to reduce unnecessary requests and avoid excessive API use. Local network and Raspberry Pi checks use the selected interval. The Web Reader remains user-triggered rather than automatically reloading the current page.

### Raspberry Pi Monitor
- [x] Read a configurable JSON status endpoint from a worker task.
- [x] Parse hostname, CPU, memory, disk, temperature, and uptime.
- [x] Show clear loading, success, and failure states.
- [x] Increase the HTTPS/JSON worker stack after diagnosing the earlier task-stack issue.
- [x] User previously confirmed the Raspberry Pi Monitor works.
- [ ] Include it in the final repeated-entry/offline soak test.

### GitHub Monitor
- [x] Read configured repository metrics.
- [x] Show the latest GitHub Actions workflow run when available.
- [x] Display explicit HTTP 403/429 rate-limit/access messaging and no-runs states.
- [x] Move HTTPS and JSON work off the LVGL task.
- [x] Fix the stale-screen lifecycle bug and increase the worker stack after diagnosing black-screen failures.
- [x] User confirmed the module opens after the crash fix.
- [ ] Physically verify the current workflow/no-runs and rate-limit states on the device.

### Formula 1
- [x] Fetch race sessions from OpenF1 in a worker task.
- [x] Display upcoming races or recent races when the season has no future sessions.
- [x] Display the latest completed race podium when data is available.
- [x] Keep a bounded 900-byte RAM cache for up to 30 minutes for request-failure fallback.
- [x] User physically verified the schedule and latest-race podium display.
- [ ] Physically verify offline-cache fallback and repeated refresh/navigation on the current firmware.

## 3. Web Reader

- [x] Accept HTTP and HTTPS URLs and validate the scheme, host, port, and authority.
- [x] Reject malformed hosts, whitespace/control characters, userinfo, and backslashes.
- [x] Fetch pages using TLS certificate verification, bounded timeouts, bounded response size, and limited redirects.
- [x] Extract readable text from static HTML and plain-text responses.
- [x] Provide session-only Back/Forward history.
- [x] User confirmed readable page text and malformed-URL rejection.
- [ ] Final physical test of long pages, redirects, HTTP errors, and repeated navigation.

Future enhancements (not part of the current implemented reader): persistent bookmarks/history, clickable links, stronger URL canonicalization, and optional limited image/CSS support. Pages that depend on JavaScript will not render like a full desktop browser.

## 4. Reliability and release readiness

- [x] Keep network fetches off the LVGL/UI task for Pi, GitHub, F1, and Web Reader.
- [x] Bound HTTP response size and request timeout; verify TLS certificates.
- [x] Add module loading/error states and task-creation failure handling.
- [x] Document the Windows/ESP-IDF build process.
- [ ] Complete final physical checks listed above.
- [ ] Review memory/stack behavior during repeated module switching and prolonged operation.
- [ ] Record a release version and tested firmware build details.

## 5. Phone-based recovery configuration

The first implementation is a D1-hosted recovery portal. It does not depend on the development PC, a cloud service, or an internet connection.

- [x] Start a recovery access point on first boot when no Wi-Fi credentials exist.
- [x] Start recovery mode after the saved Wi-Fi network fails its connection retry limit.
- [x] Generate a unique setup SSID, WPA2 password, and six-digit configuration login code.
- [x] Display the SSID, network password, login code, and manual portal address on the D1 touchscreen.
- [x] Run a local DNS catch-all and HTTP captive portal so Android/iOS captive-network detection can offer the sign-in page automatically; retain a manual http://192.168.4.1 fallback.
- [x] Require the on-screen login code, limit repeated failed attempts, and expire authenticated sessions after inactivity.
- [x] Provide a responsive phone page for Wi-Fi credentials, brightness, screen timeout, refresh interval, browser home, GitHub owner/repository, Pi status URL, and information location.
- [x] Validate supplied values and save settings through the existing NVS settings layer.
- [x] Attempt the new station connection while keeping the setup access point available; stop the temporary portal only after the D1 receives an IP address.
- [ ] Physically verify captive-portal auto-open behavior on Android and iOS, manual URL access, wrong-password retry, and successful Wi-Fi recovery.
- [x] Add an optional authenticated configuration page reachable over the normal home Wi-Fi network, started from the home-screen PHONE SETUP button; it displays the D1's current LAN address and a six-digit login code.
- [x] Reuse the recovery portal form on the normal network, allowing settings-only saves without forcing a Wi-Fi reconnect; Wi-Fi credentials are optional in this mode.
- [ ] Physically verify phone login and settings-only saves over the normal home Wi-Fi network, then verify recovery captive-portal behavior separately.

### BLE keyboard
- [ ] Add BLE HID host support so a standard Bluetooth LE keyboard can be paired.
- [ ] Route keyboard events into LVGL text fields and navigation.
- [ ] Add a pairing screen, connection status, and reconnect behavior.
- [ ] Verify coexistence with Wi-Fi and the touchscreen.

### USB keyboard dongle
- [ ] Inspect the exact D1 USB routing and available port power before selecting an implementation.
- [ ] Evaluate using the RP2040 as a USB HID host and forwarding key events to the ESP32-S3 over the existing inter-MCU UART.
- [ ] Only pursue direct ESP32-S3 USB-host support if the board exposes the required USB-OTG data lines and power path; do not assume the external USB-C connectors are wired directly to the ESP32-S3 USB peripheral.

Hardware feasibility notes: the D1 is a dual-MCU ESP32-S3/RP2040 design. Its published system diagram shows the two USB-C connectors routed through an onboard USB 2.0 hub to the RP2040 and USB-TTL path. The ESP32-S3 supports BLE, and ESP-IDF provides a Bluetooth HID Host example. USB HID host support exists for ESP32-S3 in ESP-IDF, but requires a board with USB-OTG host wiring; the D1 connector routing must be confirmed before implementation.

## 6. Optional future modules

- [ ] Information dashboard: configurable weather, news, and NASA feeds.
- [ ] Expand Raspberry Pi Monitor to multiple configured devices if needed.
- [ ] Expand GitHub Monitor to multiple repositories if needed.
