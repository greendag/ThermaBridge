## Quick orientation

This is a small ESP32-S3 firmware (PlatformIO + Arduino). Its scope and runtime are intentionally tiny:

- Purpose: run headless device that either (a) starts a provisioning AP + captive UI served from LittleFS, or (b) connects as STA and exposes a minimal status HTTP server.
- Key responsibilities: AP provisioning (captive portal), persistent config at `/config.json` on LittleFS, tiny STA `/status`/`/health` endpoints, and a single LED alive indicator (NeoPixel or PWM).

Read these files first to understand the architecture: `src/main.cpp`, `src/provisioning.cpp`, `src/config.cpp`, `src/led.cpp`, `src/system.cpp` and `src/globals.h`.

Essential developer workflows (PlatformIO / PowerShell examples):

```powershell
# build firmware
python -m platformio run

# upload files in `data/` to LittleFS (must be done after edits to `data/`)
python -m platformio run -t uploadfs

# upload firmware to board
python -m platformio run -t upload

# open serial monitor (platformio.ini defaults to COM9 @ 115200)
python -m platformio device monitor -p COM9 -b 115200
```

Project-specific conventions and patterns (do not change silently):

- Config is stored at `/config.json` (see `CONFIG_PATH` in `src/config.cpp`). Always use that path.
- An empty or whitespace-only `ssid` in `/config.json` is considered "no config"; code will enter provisioning instead of attempting Wi‑Fi. See `src/config.cpp` is_blank lambda.
- Many modules call `LittleFS.begin()` locally. Don't assume LittleFS is globally mounted; either keep the behavior or consolidate carefully and ensure callers still work.
- Wi‑Fi connection attempts are blocking with a 15s timeout in `src/main.cpp` and `src/provisioning.cpp`. Preserve UX semantics when refactoring unless tests and README are updated.
- Preferences namespace is `"thermabridge"` and is cleared on factory reset in `src/system.cpp`.

HTTP endpoints (explicit examples):

- AP (provisioning) server exposes `GET /` (serves `index.html`), `GET /app.js`, `GET /style.css`, `POST /save` (form handler that saves `ssid/psk/devname` into `/config.json` and attempts to connect), and `GET /config` or `/config.json` (raw file download).
- STA-mode status server exposes `GET /status` (JSON with masked PSK), `GET /health` (uptime and free heap), and `GET /config` or `/config.json`.

Small code patterns to reference when editing:

- Mask PSK before returning it on `/status`: see `provisioning.cpp::handleStatus()` where each PSK char is replaced with '*' for safety.
- Factory reset flow: `system.cpp::factoryResetAction()` clears `Preferences` namespace `thermabridge`, calls `eraseConfig()` and shows `ledFactoryResetVisual()` before restarting.
- LED handling: `led.cpp` prefers an `Adafruit_NeoPixel` instance and falls back to `ledc` PWM. Alive cadence is toggled in `ledLoop()` every ~750ms.

Dependencies / integration notes:

- `platformio.ini` lists `ArduinoJson` and `Adafruit NeoPixel` as `lib_deps`. The code also uses `LittleFS`, `WebServer`, `DNSServer`, and `Preferences`.
- Captive portal uses `DNSServer` to redirect all DNS queries to the AP IP while in provisioning mode.

Gotchas and quick tests to run on-device:

- After editing any file in `data/` (UI or `config.json` template), run `uploadfs` so LittleFS reflects changes on the device.
- If LittleFS fails to mount, the code attempts a format (see `main.cpp`). That behavior is noisy on serial; tests should account for it.
- The BOOT button (GPIO0) is active-low and used for long-press factory reset. Duration configured by `reset_hold_seconds` in `/data/config.json` or `config.json` on LittleFS.

When making changes / pull requests:

- Keep changes small and focused. This is firmware for a constrained device; behaviour changes (connect timing, FS assumptions, reset flow) should be documented in the PR description and validated on hardware.
- Run these minimal checks locally before PR:
  - Build success: `python -m platformio run`
  - LittleFS upload (if `data/` changed): `python -m platformio run -t uploadfs`
  - Optional: open the serial monitor and verify boot messages and endpoint availability.


Development guidelines (for humans and AI agents):

- Keep the README accurate: before committing changes that affect behavior, docs, or features, update `README.md` so it reflects the project's current state (build steps, required tools, and runtime behavior).
- Prefer small, class-based modules: organize new logic into classes or small components. Keep `setup()` and `loop()` lightweight and easy to follow; complex initialization or long-running logic should live in testable classes.
- Document public API and data: provide header comments for every class, struct, public field/property and function — including expected inputs, outputs, side-effects, and error modes.
- Comment complex logic: add concise inline comments that explain *why* code does something non-obvious (not just *what*). Use short examples or references to related files when helpful.

Examples from this repo:

- Place config loading/saving logic in `src/config.cpp` and document `CONFIG_PATH` and the whitespace-SSID semantics.
- Start a provisioning AP only from a small helper function (see `startProvisioning()` in `src/provisioning.cpp`) rather than in large monolithic setup code.
- Mask sensitive values before logging or returning (see PSK masking in `provisioning.cpp::handleStatus()`).

When in doubt, prefer readability and testability over clever micro-optimizations. If a change affects device behavior (mounting LittleFS, Wi‑Fi timeouts, reset handling), mention it explicitly in the PR description and test on-device where reasonable.
