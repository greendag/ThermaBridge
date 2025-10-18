# ThermaBridge — Decisions & Resume

This file records the confirmed hardware, software, and configuration decisions for the ThermaBridge project so you (or I) can pick up quickly later. Keep this file with the project and update it when decisions change.

Project path
- C:\Users\gary\OneDrive\Documents\PlatformIO\Projects\ThermaBridge

---

## Confirmed decisions

- Board / framework: ESP32 DevKit V1 — PlatformIO + Arduino framework
- I2C (sensors + OLED): SDA = GPIO21, SCL = GPIO22
- IR RX (VS1838B OUT): GPIO27
- IR TX (IR LED via 2N2222): GPIO18 (PWM/carrier)
- KY-040 rotary encoder: CLK = GPIO32, DT = GPIO33, SW = GPIO25
- OLED: SSD1306 on I2C (shared with sensors)
- Status LED: use internal multicolor LED
- IR drive: Option A — 2N2222 + 220 Ω for low-range testing (IRLZ44N available later)
- IR capture format: raw mark/space timings (JSON) + attempted protocol+code metadata
- Initial buttons to learn: `power_on`, `power_off`, `temp_up`, `temp_down`
- Sensors: AHT20 (temp/humidity) + BMP280 (pressure) on I2C
- Sensor read interval: 60 s
- Reaction / hysteresis: wait 2 minutes between adjustments OR altitude change > 10 m deadband
- Persistence: LittleFS + JSON (data files under `data/`)
- Safety: conservative defaults; do NOT auto-power-off on sensor failure; on persistent sensor failure stop auto-adjust and keep last safe setpoint
- UI: OLED + rotary + Serial initially; MQTT/Home Assistant deferred to later
- Thermostat integration: simple relay (deferred)
- Units: internal = Celsius; display = Fahrenheit

### Default min/max (conservative)
- Pump speed scale: 0..9 (0=off)
- Default min pump = 1, default max pump = 5
- Default min fan = 1, default max fan = 5

### Initial altitude compensation table (editable)
- 0–500 m: pump +0, fan +0
- 500–1,500 m: pump +1, fan +0
- 1,500–3,000 m: pump +2, fan +1
- > 3,000 m: pump +3, fan +1

---

## Files to back up / include in repository
- `platformio.ini`
- `src/` (all .cpp/.h files)
- `include/` (headers)
- `lib/` (project libraries)
- `data/` (LittleFS files such as `learned_ir.json`, `config.json`)
- `docs/` (this file and any images / wiring photos)
- `.vscode/` (optional workspace settings)

---

## Resume this work — paste into a new chat to continue

Resume: ThermaBridge project snapshot

Project path: C:\Users\gary\OneDrive\Documents\PlatformIO\Projects\ThermaBridge

Decisions & defaults:
- Board: ESP32 DevKit V1, PlatformIO + Arduino framework
- Pins: SDA=GPIO21, SCL=GPIO22, IR_RX=GPIO27, IR_TX=GPIO18, ENC CLK=GPIO32, DT=GPIO33, SW=GPIO25
- IR: learn raw mark/space JSON + protocol metadata; initial hardware: 2N2222 + 220Ω (IRLZ44N available later)
- First buttons: power_on, power_off, temp_up, temp_down
- Sensors: AHT20 + BMP280 on I2C; read every 60s
- Hysteresis: 2 min between adjustments or altitude change > 10 m
- Persistence: LittleFS + JSON; initial files: `data/learned_ir.json`, `data/config.json`
- Safety: conservative defaults; min/max pump & fan = 1..5 (scale 0..9)
- UI: OLED + rotary + Serial; MQTT/Home Assistant deferred
- Units: display Fahrenheit; internal C

Current todos (high-level):
- Init PlatformIO project (not started)
- IR learning & transmit module (not started)
- Sensor drivers (AHT20/BMP280) (not started)
- Altitude & compensation (not started)
- Persistence, safety, UI, tests (planned)

Files to restore/inspect: `platformio.ini`, `src/`, `include/`, `data/`, `.vscode/`

Notes/attachments: wiring photos and part images saved under `docs/images/` if present.

---

## Suggested PowerShell ZIP command (optional)
Run from PowerShell to make a quick archive of the project folder:

```powershell
cd "C:\Users\gary\OneDrive\Documents\PlatformIO\Projects\ThermaBridge"
Compress-Archive -Path . -DestinationPath ..\ThermaBridge_snapshot.zip -Force
```

---

## Change log
- 2025-10-09: Created `docs/decisions.md` with confirmed pins, defaults and resume block.

---

If you want this committed to git or adjusted (different defaults, added images), tell me which changes and I will apply them.
