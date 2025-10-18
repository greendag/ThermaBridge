#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

// Use the library's JsonDocument symbol explicitly to avoid conflicts or
// deprecation ambiguity between DynamicJsonDocument/StaticJsonDocument.

const char *CONFIG_PATH = "/config.json";

bool loadConfig(Config &cfg)
{
    if (!LittleFS.begin())
        return false;
    if (!LittleFS.exists(CONFIG_PATH))
        return false;
    File f = LittleFS.open(CONFIG_PATH, "r");
    if (!f)
        return false;
    ArduinoJson::JsonDocument doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err)
        return false;
    cfg.ssid = doc["ssid"] | "";
    cfg.psk = doc["psk"] | "";
    cfg.devname = doc["devname"] | "ThermaBridge";
    cfg.reset_hold_seconds = doc["reset_hold_seconds"] | 10;
    // Treat empty (or whitespace-only) SSID as "no config" so
    // the device will enter provisioning mode instead of
    // attempting to call WiFi.begin() with an invalid SSID.
    auto is_blank = [](const String &s)
    {
        for (size_t i = 0; i < s.length(); ++i)
        {
            if (!isWhitespace(s[i]))
                return false;
        }
        return true;
    };

    if (cfg.ssid.length() == 0 || is_blank(cfg.ssid))
    {
        return false;
    }

    return true;
}

bool saveConfig(const Config &cfg)
{
    if (!LittleFS.begin())
        return false;
    File f = LittleFS.open(CONFIG_PATH, "w");
    if (!f)
        return false;
    ArduinoJson::JsonDocument doc;
    doc["ssid"] = cfg.ssid;
    doc["psk"] = cfg.psk;
    doc["devname"] = cfg.devname;
    doc["reset_hold_seconds"] = cfg.reset_hold_seconds;
    if (serializeJson(doc, f) == 0)
    {
        f.close();
        return false;
    }
    f.close();
    return true;
}

bool eraseConfig()
{
    if (!LittleFS.begin())
        return false;
    if (LittleFS.exists(CONFIG_PATH))
    {
        return LittleFS.remove(CONFIG_PATH);
    }
    return true;
}
