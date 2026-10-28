# Building D1-Dash on Windows

## Required environment

- Windows 10 or 11
- ESP-IDF 5.4.4 installed with the ESP32-S3 toolchain
- Git for Windows
- USB data cable for the SenseCAP Indicator D1

The project lock file currently records ESP-IDF 5.4.4 and target esp32s3. Avoid mixing ESP-IDF releases in the same shell or build directory.

## Configure and build

Open ESP-IDF 5.4.4 PowerShell and run:

    cd C:\Projects\AndrewRayR\D1-Dash
    idf.py set-target esp32s3
    idf.py build

The target-setting command may clean the existing build and regenerate sdkconfig. After the first successful build, ordinary rebuilds generally need only idf.py build.

If the build directory was created by a different ESP-IDF version, clean it before rebuilding:

    idf.py fullclean
    idf.py set-target esp32s3
    idf.py build

## Flash and monitor

Only after a successful build and with the device physically connected:

    idf.py -p COMx flash monitor

Replace COMx with the actual port from Device Manager or the ESP-IDF port selector. Do not guess the port. Exit the serial monitor with Ctrl+].

## Troubleshooting notes

- Confirm the shell is using ESP-IDF 5.4.4, not another installed release.
- Confirm idf.py set-target esp32s3 and dependencies.lock both indicate ESP32-S3.
- If CMake/Ninja reports API signature errors inside components/bsp/, capture the complete first compiler error before making changes.
- The BSP compatibility edits currently present in this repository must be build-tested on the replacement machine before flashing.
- Do not edit the TCA9535 driver merely to silence a function-pointer warning; its hardware input register is 16-bit. Adapt it at the BSP boundary.
- Keep sdkconfig and build output local unless there is a specific reason to track a reproducible configuration; sdkconfig.defaults and dependencies.lock are the shared baseline.
