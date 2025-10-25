#include "init.h"
#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>
#include "led.h"
#include "system.h"
#include "config.h"
#include "globals.h"
#include "provisioning.h"
#include "display.h"

bool initSerial()
{
    Serial.begin(115200);
    delay(20);
    return true;
}

bool initPins()
{
    pinMode(0, INPUT_PULLUP); // BOOT button is active-low
    return true;
}

bool initLedAndSystem()
{
    ledInit();
    systemInit();
    return true;
}

bool initFileSystem()
{
    bool lfsMounted = LittleFS.begin();
    if (!lfsMounted)
    {
        Serial.println("Warning: LittleFS failed to mount");
        Serial.println("Attempting LittleFS format to recover filesystem...");
        if (LittleFS.format() && LittleFS.begin())
        {
            Serial.println("LittleFS mounted after format");
            return true;
        }
        else
        {
            Serial.println("LittleFS mount failed");
            return false;
        }
    }
    return true;
}

bool tryLoadConfig()
{
    if (!loadConfig(cfg))
    {
        Serial.println("No config found, entering provisioning mode...");
        startProvisioning();
        return false;
    }
    return true;
}

bool tryConnectWifi(unsigned long timeoutMs)
{
    Serial.println("Loaded config; attempting to connect to WiFi...");
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg.ssid.c_str(), cfg.psk.c_str());

    unsigned long start = millis();
    while (millis() - start < timeoutMs)
    {
        if (WiFi.status() == WL_CONNECTED)
            break;
        if (checkFactoryReset())
            return false; // factoryResetAction will restart
        delay(200);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("Connected as STA, IP: ");
        Serial.println(WiFi.localIP());
        startStatusServer();
        // Initialize ArduinoOTA
        ArduinoOTA.setHostname(cfg.devname.c_str());
        if (!cfg.ota_password.isEmpty())
        {
            ArduinoOTA.setPassword(cfg.ota_password.c_str());
        }
        ArduinoOTA.begin();
        Serial.println("ArduinoOTA started");
        if (cfg.encoder_enabled)
        {
            encoder = new RotaryEncoder(cfg.encoder_clk_pin, cfg.encoder_dt_pin, cfg.encoder_sw_pin);
            encoder->begin();
            Serial.println("Encoder initialized");
        }
        displayInit(cfg);
        displaySplashScreen(cfg);
        if (cfg.mdns_enable)
        {
            if (MDNS.begin(cfg.devname.c_str()))
            {
                MDNS.addService("http", "tcp", 80);
                Serial.println("mDNS started");
            }
            else
            {
                Serial.println("mDNS failed to start");
            }
        }
        return true;
    }
    else
    {
        Serial.println("Failed to connect, starting provisioning AP...");
        startProvisioning();
        return false;
    }
}
