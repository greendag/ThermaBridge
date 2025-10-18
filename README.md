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
# ThermaBridge

ThermaBridge is a small firmware project for ESP32-based development boards that
provides an easy captive-portal provisioning UX, persistent device configuration
(using LittleFS), and a simple HTTP status API for diagnostics.

This repository contains the firmware, the web UI files (served from LittleFS),
and tools to build/upload the project using PlatformIO.

## Supported hardware
- ESP32-S3-DevKitC-1 (primary development target in this repo)
- Other ESP32/ESP32-S3 boards may work with small `platformio.ini` updates.

## Features
- Captive-portal provisioning (AP mode) with UI served from LittleFS
- Persistent configuration in `config.json` on LittleFS (SSID, PSK, device name,
	reset timeout)
- Factory-reset via long-press of the BOOT button (clears Preferences and
	`config.json`)
- Small STA-mode status server exposing `/status`, `/health`, and `/config`
- LED alive indicator (NeoPixel when present) with different color for AP vs
	STA modes and dim brightness for low power

## HTTP endpoints
- AP (provisioning) server (default at 192.168.4.1 when in AP mode):
	- `/` - UI (index.html)
	- `/app.js`, `/style.css` - UI assets
	- `POST /save` - save Wi‑Fi credentials and device name (starts STA connect)
	- `GET /config` and `GET /config.json` - download the current LittleFS `config.json`

- STA (status) server (when connected to your network, on device IP):
	- `GET /status` - returns JSON with masked SSID/PSK, devname, wifi status, and IP
	- `GET /health` - basic health metrics (uptime, free heap)
	- `GET /config` and `GET /config.json` - download the LittleFS `config.json`

## LED behaviour
- The onboard RGB NeoPixel (data pin `GPIO48` on the tested S3 board) is used when
	present. When not present, the firmware falls back to PWM on the LED pin or plain
	digital toggling.
- Alive blink is dim (controlled by `LED_ON_BRIGHTNESS`) and slower than before.
- Colors:
	- AP/provisioning mode: amber (low brightness)
	- STA/connected mode: green (low brightness)
	- Factory reset sequence: amber flashes then solid green at the same low brightness

## Configuration
- Default configuration template is located in `data/config.json` and is copied to
	LittleFS during the uploadfs step (if you use the PlatformIO LittleFS uploader).
- The important field is `reset_hold_seconds` (how long to hold BOOT/GPIO0 to
	trigger factory reset). Default is set in `data/config.json`.

## Building and uploading
From the project root, using PowerShell (Windows):

```powershell
# Build
python -m platformio run

# Upload LittleFS image (files under data/)
python -m platformio run -t uploadfs

# Upload firmware
python -m platformio run -t upload

# Open serial monitor
python -m platformio device monitor -p COM9 -b 115200
```

Adjust `COM9` and the board/env in `platformio.ini` as needed.

## Development notes
- Edit the web UI files in `data/` and re-run `uploadfs` to update LittleFS contents.
- Config is parsed with ArduinoJson v7; `src/config.*` contains helpers `loadConfig()` and `saveConfig()`.
- The webservers are implemented using `WebServer` and `DNSServer` (for captive portal).

## CI
A basic CI workflow is included in `.github/workflows/ci.yml` which runs a PlatformIO build.

## Contributing
Contributions welcome. Please open issues for bugs and pull requests for fixes or features.

## License
Add your preferred license here.
