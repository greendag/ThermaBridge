#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <Preferences.h>
#include "config.h"
#include "provisioning.h"
#include "globals.h"
#include "led.h"
#include "system.h"

// State objects
Config cfg;
Preferences prefs;

void setup()
{
    Serial.begin(115200);
    delay(20);

    pinMode(0, INPUT_PULLUP); // BOOT button is active-low

    // Initialize subsystems
    ledInit();
    systemInit();

    // Initialize LittleFS
    bool lfsMounted = LittleFS.begin();
    if (!lfsMounted)
    {
        Serial.println("Warning: LittleFS failed to mount");
        Serial.println("Attempting LittleFS format to recover filesystem...");
        if (LittleFS.format() && LittleFS.begin())
        {
            Serial.println("LittleFS mounted after format");
        }
        else
        {
            Serial.println("LittleFS mount failed");
        }
    }

    // Load configuration
    if (!loadConfig(cfg))
    {
        Serial.println("No config found, entering provisioning mode...");
        startProvisioning();
        return;
    }

    Serial.println("Loaded config; attempting to connect to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg.ssid.c_str(), cfg.psk.c_str());

    unsigned long start = millis();
    const unsigned long CONNECT_TIMEOUT = 15000; // 15s
    while (millis() - start < CONNECT_TIMEOUT)
    {
        if (WiFi.status() == WL_CONNECTED)
            break;
        if (checkFactoryReset())
            return; // factoryResetAction will restart
        delay(200);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("Connected as STA, IP: ");
        Serial.println(WiFi.localIP());
        startStatusServer();
    }
    else
    {
        Serial.println("Failed to connect, starting provisioning AP...");
        startProvisioning();
    }
}

void loop()
{
    if (isProvisioningActive())
        loopProvisioning();

    if (checkFactoryReset())
        return;

    loopStatusServer();

    ledLoop();
}
