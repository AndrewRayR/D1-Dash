# D1-Dash Roadmap

## Phase 0 — Recoverable project baseline

- [x] Put the current SenseCAP Indicator source tree under the D1-Dash repository.
- [x] Document target, architecture, build steps and known environment limits.
- [x] Provision ESP-IDF 5.4.4 on the replacement computer.
- [x] Run a clean ESP32-S3 build; the recovered BSP compatibility changes compile successfully.
- [x] Identify COM21 conclusively with esptool as the Indicator D1 ESP32-S3 (8 MB flash, 8 MB PSRAM); leave COM22 untouched.
- [x] Flash the ESP32-S3 through COM21 and confirm the SenseCAP Indicator board initializes and joins Wi-Fi.
- [x] Verify touchscreen navigation into F1 on the physical display; the user confirmed upcoming races load and Refresh/Back Home controls appear.
- [ ] Verify Back Home, repeated F1 entry and Refresh by touch.

## Phase 1 — Stable home and navigation

- [ ] Confirm the home screen fits the D1 display and supports touch.
- [ ] Add a consistent back/navigation pattern for all modules.
- [x] Keep the F1 network request off the UI task.
- [ ] Review all other module operations for UI-task blocking.
- [ ] Verify settings and navigation survive screen changes.

## Phase 2 — Core modules

- [ ] Network Monitor: Wi-Fi state, IP, RSSI and basic diagnostics.
- [ ] Settings: saved endpoints, refresh intervals and user preferences.
- [ ] Raspberry Pi Monitor: configurable JSON endpoint and clear offline state.
- [ ] GitHub Monitor: configurable repository, recent workflow status and API rate-limit handling.
- [x] F1: basic schedule screen, OpenF1 request task, loading/error states, and recent-race fallback.
- [x] Validate the OpenF1 schedule request and display on the physical device.
- [ ] F1: add race results and a bounded response cache.
- [ ] Information: configurable weather/news/NASA feeds.

## Phase 3 — Web Reader

- [ ] Address entry and URL validation.
- [ ] HTTP/HTTPS retrieval with TLS verification and strict response limits.
- [ ] Text-first HTML parsing, links and basic navigation.
- [ ] History and bookmarks stored persistently.
- [ ] Later: limited images/CSS only if memory and performance allow.

## Phase 4 — Reliability and documentation

- [ ] Add module-level error and loading states.
- [ ] Document API providers, rate limits and privacy implications.
- [ ] Add reproducible build notes and troubleshooting.
- [ ] Record tested ESP-IDF version and firmware version with each release.
