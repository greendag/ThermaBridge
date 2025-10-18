# ThermaBridge

This project runs on an ESP32 DevKit v1 and provides captive-portal Wi-Fi provisioning, persistent configuration stored on LittleFS, and a factory-reset via long-press of the BOOT button.

Features
- Captive-portal provisioning served from LittleFS (open AP named `ThermaBridge-XXXX`)
- Configuration stored in `config.json` on LittleFS
- Factory reset by long-pressing the BOOT button (GPIO0) for the configured number of seconds

Files to know
- `data/` contains the web UI files (`index.html`, `app.js`, `style.css`) and initial `config.json` used to populate LittleFS at upload time.
- `src/config.*` implements reading/writing `config.json` using LittleFS and ArduinoJson.
- `src/provisioning.*` serves the static UI from LittleFS and handles `/save`.
- `src/main.cpp` loads the config, attempts Wi-Fi connection, or starts provisioning. It also monitors the BOOT button for factory reset.

Factory reset behavior
- The reset-hold time is stored as `reset_hold_seconds` in `config.json` on LittleFS. By default it's 30 seconds.
- To perform a factory reset, press and hold the BOOT button (GPIO0) for the configured number of seconds. The device will erase `config.json` and Preferences and reboot into provisioning mode.

Build & upload
Use PlatformIO in the project root. Example (PowerShell):

```powershell
python -m platformio run -t upload -e esp32doit-devkit-v1
```

Notes
- If you enclose the board, ensure the BOOT button is accessible or wire a dedicated GPIO to a button and change `BOOT_PIN` in `src/main.cpp`.
- For a richer web UI, replace `data/index.html`/`app.js`/`style.css` and re-upload using the LittleFS upload tool (PlatformIO has an extension to upload LittleFS contents).
