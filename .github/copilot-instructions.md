This repository is a small ESP32-S3 firmware (PlatformIO + Arduino) for a headless device that either
- starts a provisioning AP + captive UI served from LittleFS, or
- connects as STA and exposes a minimal status HTTP server.

Keep instructions compact, prescriptive, and tied to concrete files and patterns below.

Quick orientation (read these first)
- `src/main.cpp` — boot flow, decides between provisioning and STA modes.
- `src/provisioning.cpp` — AP/captive portal, DNSServer usage, serves `data/` on LittleFS, handles `/save` to write `/config.json`.
- `src/config.cpp`/`src/config.h` — config load/save and `CONFIG_PATH` constant (always `/config.json`).
- `src/system.cpp` — factory reset logic and Preferences namespace handling.
- `src/led.cpp` — LED alive indicator (NeoPixel preferred, PWM fallback).

Big-picture architecture
- Modes: provisioning (AP + captive portal) vs. normal STA mode.
- Persistent storage: LittleFS holds the web UI and `/config.json` (CONFIG_PATH). Code often mounts LittleFS locally; don't assume global mount.
- Preferences: `Preferences` namespace is `"thermabridge"` and is cleared on factory reset (see `system.cpp`).
- Networking: provisioning uses `DNSServer` to capture DNS and serve the captive portal; STA mode starts a minimal `WebServer` exposing `/status`, `/health`, and `/config`.

Key developer workflows
- Build firmware (local):
  - python -m platformio run
- Upload LittleFS `data/` files to the device (required after editing `data/`):
  - python -m platformio run -t uploadfs
- Upload firmware to board (runs the build hook that may bump build number on upload):
  - python -m platformio run -t upload
- Serial monitor (default in platformio.ini is COM9 @ 115200):
  - python -m platformio device monitor -p COM9 -b 115200

Project-specific conventions and gotchas (do not change silently)
- Config path and semantics: `CONFIG_PATH` is `/config.json`. An empty or whitespace-only `ssid` in that file is treated as "no config" and causes provisioning to start.
- LittleFS mounting: Many modules call `LittleFS.begin()` locally. Either preserve this behavior or consolidate carefully and ensure callers still work.
- Wi‑Fi timeouts: Connection attempts are blocking with a 15s timeout in `src/main.cpp` and `src/provisioning.cpp`. Preserve UX semantics when refactoring.
- Preferences namespace: Use the namespace `thermabridge` (see `system.cpp`). Factory reset clears this namespace and erases `/config.json`.
- PSK handling: Mask PSKs before logging or returning them (provisioning masks PSK characters with `*` when returning `/status`).
- BOOT button: GPIO0 is active-low and used for long-press factory reset. Duration is configurable by `reset_hold_seconds` in `data/config.json` or `/config.json` on LittleFS.

Integration points & dependencies
- Libraries referenced in `platformio.ini`: `ArduinoJson`, `Adafruit NeoPixel`, `LittleFS`, `WebServer`, `DNSServer`, `Preferences`.
- Captive portal: `DNSServer` redirects all DNS queries to the AP IP while provisioning is active.
 - LittleFS `data/` files (UI): `data/index.html`, `data/app.js`, `data/style.css`, `data/config.json`.

Files that show important patterns (use as references when making edits)
- `src/provisioning.cpp` — demonstrates captive portal routing and PSK masking.
- `src/config.cpp` — shows config load/save semantics and whitespace SSID logic.
- `src/system.cpp` — factory reset flow and Preferences clearing.
- `src/led.cpp` — shows NeoPixel+PWM fallback and alive cadence toggling.

Testing and validation guidance for code changes
- After editing `data/` files, always run `uploadfs` so LittleFS matches the build artifacts.
- Verify build success locally before PR: `python -m platformio run` (the upload hook increments builds only when running upload target).
- Optional hardware check: open the serial monitor and confirm boot messages, provisioning AP (if no config), or STA endpoints `/status` and `/health` when connected.

Small, low-risk PR additions that are welcome
- Add small unit-style tests or CI steps that run `platformio run` to catch compile errors. (This repo currently has no test suite.)
- Improve inline comments documenting why specific timeouts and mounts are chosen (not just what they do).

When changing behavior, document in `README.md` and the PR description
- Any modification that affects LittleFS mounting, Wi‑Fi timeouts, or reset handling must be clearly described in the PR and validated on hardware where practical.

If something is unclear
- Ask for the intended device behavior (e.g., change to non-blocking Wi‑Fi connect) and whether the LittleFS mount semantics may be altered. Provide suggested small changes rather than large refactors.

Contact points
- For firmware behavior questions, inspect `src/main.cpp` and `src/provisioning.cpp` first.

-- end --
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

- Release/versioning: when preparing a release, increment the `minor` number used by the bump script (typically set via `FW_BASE_VERSION` in `src/build_info.h` or configured by the bump script). The build script will append the build count to produce `vMAJOR.MINOR.<BUILD>`.

Note about the build bump hook:

- The project runs the build bump via the post-build script `scripts/bump_on_build.py`. This hook runs after builds (including the build step run as part of upload), so the bump will occur for both normal builds and uploads.
- To run the bump locally without uploading, execute:

```powershell
python scripts/bump_build.py
```

This keeps `data/config.json` stable in the repo; generated output appears in `src/build_info.h`.

Examples from this repo:

- Place config loading/saving logic in `src/config.cpp` and document `CONFIG_PATH` and the whitespace-SSID semantics.
- Start a provisioning AP only from a small helper function (see `startProvisioning()` in `src/provisioning.cpp`) rather than in large monolithic setup code.
- Mask sensitive values before logging or returning (see PSK masking in `provisioning.cpp::handleStatus()`).

When in doubt, prefer readability and testability over clever micro-optimizations. If a change affects device behavior (mounting LittleFS, Wi‑Fi timeouts, reset handling), mention it explicitly in the PR description and test on-device where reasonable.
