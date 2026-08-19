
Use either the full path or the `platformio` wrapper if it's on your PATH:

- Full path (your environment):
```bash
C:\Users\Peyman\.platformio\penv\Scripts\platformio.exe run --target upload -e pico
```

- Short form (if `platformio` is in PATH):
```bash
platformio run --target upload -e pico
pio run --target upload --upload-port COM5
```

- Add `-v` for verbose output:
```bash
platformio run --target upload -e pico -v
```

Or use the PlatformIO Upload button in VS Code. If auto-detect fails, you can manually copy firmware.uf2 to the RPI-RP2 mass-storage drive (BOOTSEL mode).

Build: pio run
Upload: pio run --target upload
Monitor: pio device monitor

If PlatformIO freezes again in the future, you can run this command to quickly fix it:
taskkill /F /IM pio.exe; taskkill /F /IM python.exe /FI "WINDOWTITLE eq *platformio*"