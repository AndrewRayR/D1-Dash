# D1-Dash Roadmap

## Phase 0 — Recoverable project baseline

- [x] Put the current SenseCAP Indicator source tree under the D1-Dash repository.
- [x] Document target, architecture, build steps and known environment limits.
- [x] Provision ESP-IDF 5.4.4 on the replacement computer.
- [x] Run a clean ESP32-S3 build; the recovered BSP compatibility changes compile successfully.
- [ ] Identify the connected USB serial device conclusively, then flash the Indicator D1 and verify display, touch and Wi-Fi.

## Phase 1 — Stable home and navigation

- [ ] Confirm the home screen fits the D1 display and supports touch.
- [ ] Add a consistent back/navigation pattern for all modules.
- [ ] Keep long operations off the UI task.
- [ ] Verify settings and navigation survive screen changes.

## Phase 2 — Core modules

- [ ] Network Monitor: Wi-Fi state, IP, RSSI and basic diagnostics.
- [ ] Settings: saved endpoints, refresh intervals and user preferences.
- [ ] Raspberry Pi Monitor: configurable JSON endpoint and clear offline state.
- [ ] GitHub Monitor: configurable repository, recent workflow status and API rate-limit handling.
- [ ] F1: schedule and results with caching and graceful API failure.
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
