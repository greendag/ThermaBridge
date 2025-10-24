#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

struct Config
{
    String ssid;
    String psk;
    String devname;
    uint16_t reset_hold_seconds; // seconds to hold BOOT button to factory reset
    String ota_password;         // optional password for ArduinoOTA
    bool mdns_enable;
};

bool loadConfig(Config &cfg);
bool saveConfig(const Config &cfg);
bool eraseConfig();

#endif // CONFIG_H
