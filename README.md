# D1-Dash

D1-Dash is a touchscreen-first Tech Terminal for the Seeed Studio SenseCAP Indicator D1, built with ESP-IDF and LVGL.

The project starts from AndrewRayR's SenseCAP Indicator ESP-IDF template fork and preserves its board support package, peripheral drivers, LVGL port, and existing application code. The goal is an expandable personal status terminal rather than a smart-home controller.

## Planned home modules

- **Home** — concise dashboard and module navigation
- **Web Reader** — lightweight text-first HTTP/HTTPS browsing, not a full desktop browser
- **Network Monitor** — Wi-Fi state and network diagnostics
- **Raspberry Pi Monitor** — status from a small JSON endpoint hosted on a Pi
- **GitHub Monitor** — repository and workflow summaries using GitHub APIs
- **F1** — race schedule and motorsport information from an API
- **Information** — weather, news, NASA and other configurable feeds
- **Settings** — Wi-Fi, endpoints, display and module configuration

Developer tools are intentionally not a home-screen module; those tools already live on the project's GitHub site.

## Hardware and software

- Seeed Studio SenseCAP Indicator D1 / ESP32-S3
- Onboard display and touch hardware; no extra hardware required for the initial milestone
- Intended toolchain: ESP-IDF 5.4.4
- Target: esp32s3
- LVGL UI

## Repository layout

- main/ — app entry point, UI, models, views, controllers and Tech Terminal modules
- components/bsp/ — board support and display integration
- components/i2c_devices/ — onboard I²C peripheral drivers
- docs/ — architecture, build notes, roadmap and status
- sdkconfig.defaults — baseline project configuration
- dependencies.lock — resolved ESP-IDF component dependency lock

## Current status

The source tree is synced to this repository and has completed a clean ESP-IDF 5.4.4 / ESP32-S3 build on DESKTOP-415LS7I. Firmware has not yet been flashed or tested on the physical Indicator D1. See docs/DEVELOPMENT_STATUS.md before attempting a device flash.

## Build overview

1. Install ESP-IDF 5.4.4 for Windows and the ESP32-S3 toolchain.
2. Open an ESP-IDF PowerShell terminal.
3. From the repository root, run: idf.py set-target esp32s3
4. Run: idf.py build
5. Connect the Indicator D1 over USB and identify its serial port before using idf.py -p COMx flash monitor.

Full details and cautions are in docs/BUILDING.md.

## Project documents

- docs/ARCHITECTURE.md
- docs/BUILDING.md
- docs/ROADMAP.md
- docs/DEVELOPMENT_STATUS.md
- CONTRIBUTING.md

## Source attribution

The baseline derives from Seeed Studio's indicator-esp-idf-template and the owner-maintained fork AndrewRayR/indicator-esp-idf-template. Preserve upstream license headers and component licenses when modifying or redistributing code.
