# D1-Dash Architecture

## Design goals

1. Keep the existing SenseCAP Indicator BSP and LVGL port as the hardware boundary.
2. Keep the touchscreen UI responsive: network operations must not block the LVGL/UI task.
3. Make modules independently expandable and allow settings to persist across reboots.
4. Keep secrets (Wi-Fi credentials, API tokens) out of source control.
5. Prefer compact, text-first information displays appropriate for the screen and memory budget.

## Layering

- Startup: main.c and app_main
- Hardware: BSP initialization and LVGL port
- Existing application: indicator model, controller and view
- Tech Terminal UI: home and navigation
- Feature modules: Web Reader, Network Monitor, Raspberry Pi Monitor, GitHub Monitor, F1, Information and Settings
- Services: Wi-Fi, HTTP/HTTPS, persistent configuration and module-specific API clients

## Existing source areas

- main/main.c — startup and main task
- main/tech_terminal_ui.c and main/tech_terminal_ui.h — Tech Terminal UI entry points
- main/tech/ — feature and network-related implementation
- main/model/, main/view/, main/controller/ — existing application separation
- main/ui/ — LVGL-generated UI assets and helpers
- components/bsp/ — board and LCD interfaces
- components/i2c_devices/ — I²C peripheral implementations

## Module contract

A module should expose a small initialization/open/close/update interface and own its own screen content. Network requests should run in a worker task, then pass results to the UI task using a queue, event, or LVGL-safe callback. Avoid creating LVGL objects from arbitrary worker tasks.

## Data and secrets

- Store user preferences and endpoint settings in NVS.
- Never commit real Wi-Fi passwords, access tokens, API keys, or private endpoint credentials.
- Use documented, bounded HTTP timeouts and handle offline, TLS, and malformed-response cases.
- Treat remote API responses as untrusted input and cap response sizes.

## Hardware boundary

D1-Dash initially uses only the built-in SenseCAP Indicator D1 hardware. Keep the RP2040 side unchanged unless a concrete feature requires it; the initial app should run on the ESP32-S3.
