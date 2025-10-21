# ThermaBridge

ThermaBridge is a small firmware project for ESP32-based development boards that
provides an easy captive-portal provisioning UX, persistent device configuration
(using LittleFS), and a simple HTTP status API for diagnostics.

Note: developer & AI-agent guidance lives in `.github/copilot-instructions.md` — please update it when changing behavior or developer workflows.

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
- Alive blink is dim (controlled by `LED_ON_BRIGHTNESS`) and set to a relaxed cadence.
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

## Network discovery (mDNS)

When the device is connected to your LAN it advertises an mDNS name (e.g. `ThermaBridge.local`). You can access the status endpoint directly:

```text
http://<devname>.local/status
```

On macOS use `dns-sd` and on Linux use `avahi-browse` to discover services; Windows may require Bonjour for `.local` resolution.

## Build info and badge

The canonical firmware version is embedded in the firmware header `src/build_info.h` as
`FW_VERSION` and `FW_BUILD_NUMBER`. Build/upload scripts will update `src/build_info.h` at
upload time. `data/config.json` no longer contains `fw_version`, `fw_base`, or `build`.

Local bump-on-build
- The project still increments a local build counter stored in `.build_count` when the bump
	script runs (typically during upload). The header `src/build_info.h` is the single source of
	truth for the firmware version embedded in builds.
- Generate or update the build info locally with:

```powershell
python scripts/bump_build.py
```

Inspect the current generated info quickly with:

```powershell
python scripts/print_build_info.py
```

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
