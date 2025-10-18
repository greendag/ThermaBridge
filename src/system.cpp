#include "system.h"
#include "globals.h"
#include "led.h"
#include <Arduino.h>

void systemInit()
{
    // placeholder for future system-wide initialization
}

void factoryResetAction()
{
    Serial.println("Factory reset: clearing stored configuration...");

    // Clear preferences namespace
    prefs.begin("thermabridge", false);
    prefs.clear();
    prefs.end();

    // Erase config.json from LittleFS
    eraseConfig();

    // Visual feedback
    ledFactoryResetVisual();

    Serial.println("Factory reset complete, rebooting...");
    delay(200);
    ESP.restart();
}

bool checkFactoryReset()
{
    uint16_t holdSecs = (cfg.reset_hold_seconds > 0) ? cfg.reset_hold_seconds : 10;
    if (digitalRead(0) == LOW)
    {
        unsigned long start = millis();
        while (digitalRead(0) == LOW)
        {
            if (millis() - start >= (unsigned long)holdSecs * 1000UL)
            {
                factoryResetAction();
                return true;
            }
            delay(200);
        }
    }
    return false;
}
