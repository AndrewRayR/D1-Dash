# Contributing to D1-Dash

## Working rules

- Make changes in small, reviewable commits.
- Keep the ESP-IDF version and target explicit: ESP-IDF 5.4.4, ESP32-S3.
- Before changing BSP function signatures, inspect the typedef and every call site.
- Do not silence incompatible function-pointer diagnostics with casts.
- Keep API credentials and Wi-Fi secrets out of Git.
- After a source change, run a build when the toolchain is available and report the actual result.
- Do not claim a build, flash, or device test passed unless it was run and verified.

## Suggested commit format

type(scope): concise description

Examples:
- docs(build): document ESP-IDF 5.4.4 setup
- fix(bsp): adapt TCA9535 input pins to BSP interface
- feat(home): add module navigation
