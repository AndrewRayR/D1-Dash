# Development Status

Last updated: 2026-10-27

## Recovery context

The original Windows development computer is unavailable. The replacement machine is DESKTOP-415LS7I. The source tree has been cloned from the owner-maintained repository AndrewRayR/indicator-esp-idf-template into C:\Projects\AndrewRayR\D1-Dash so work can continue from this computer.

The original machine's exact uncommitted working-tree state cannot be recovered through the unavailable computer. This checkout contains the source that was present in the GitHub fork, including the Tech Terminal files and BSP compatibility edits already committed there.

## Known build history

On the original machine, ESP-IDF 5.4.4 and ESP32-S3 were confirmed as the active build target. A previous build log reported these incompatibilities:

- TCA9535 16-bit input-read callback did not match the BSP's 8-bit callback type.
- RGB VSYNC callback event data const qualification differed.
- Framebuffer API expected void **, while the old code supplied uint8_t **.
- LVGL direct-mode copy callback type differed.

The current repository checkout already includes a BSP wrapper named tca9535_read_input_pins_8, void * framebuffer pointers, a const-qualified VSYNC callback, and a void (*)(void) direct-mode copy callback. These source changes are present but have not yet been recompiled on this replacement computer.

## Current replacement-machine findings

- Remote Desktop Commander connection to DESKTOP-415LS7I: online.
- Git for Windows: installed.
- Project checkout: C:\Projects\AndrewRayR\D1-Dash.
- ESP-IDF installation at the old path C:\esp\v5.4.4\esp-idf: not found.
- idf.py: not found in PATH.
- Hardware/USB connection: not yet verified.

## Next actions

1. Confirm D1-Dash is pushed to GitHub and its default branch contains the recovered source and project docs.
2. Install ESP-IDF 5.4.4 and its ESP32-S3 tools on this machine.
3. Run idf.py fullclean, idf.py set-target esp32s3, and idf.py build.
4. Resolve any new build errors based on the first complete compiler diagnostic.
5. Flash only after a successful build and confirming the actual COM port.
