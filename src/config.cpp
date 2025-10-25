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
    cfg.ota_password = doc["ota_password"] | "";
    cfg.mdns_enable = doc["mdns_enable"] | true;
    cfg.display_enabled = doc["display_enabled"] | true;
    cfg.display_width = doc["display_width"] | 128;
    cfg.display_height = doc["display_height"] | 64;
    cfg.display_sda_pin = doc["display_sda_pin"] | 21;
    cfg.display_scl_pin = doc["display_scl_pin"] | 4;
    cfg.encoder_enabled = doc["encoder_enabled"] | true;
    cfg.encoder_clk_pin = doc["encoder_clk_pin"] | 16;
    cfg.encoder_dt_pin = doc["encoder_dt_pin"] | 17;
    cfg.encoder_sw_pin = doc["encoder_sw_pin"] | 18;
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
    doc["ota_password"] = cfg.ota_password;
    doc["mdns_enable"] = cfg.mdns_enable;
    doc["display_enabled"] = cfg.display_enabled;
    doc["display_width"] = cfg.display_width;
    doc["display_height"] = cfg.display_height;
    doc["display_sda_pin"] = cfg.display_sda_pin;
    doc["display_scl_pin"] = cfg.display_scl_pin;
    doc["encoder_enabled"] = cfg.encoder_enabled;
    doc["encoder_clk_pin"] = cfg.encoder_clk_pin;
    doc["encoder_dt_pin"] = cfg.encoder_dt_pin;
    doc["encoder_sw_pin"] = cfg.encoder_sw_pin;
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
