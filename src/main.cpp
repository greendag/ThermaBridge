#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include "config.h"
#include "provisioning.h"
#include "globals.h"
#include "led.h"
#include "system.h"
#include "init.h"

// State objects
// State objects are declared in `globals.h` and defined in `globals.cpp`

void setup()
{
    // Lightweight setup delegating heavy work to init helpers
    initSerial();
    initPins();
    initLedAndSystem();

    // Mount filesystem (attempt format if needed)
    if (!initFileSystem())
    {
        // Filesystem unavailable; fall back to provisioning
        startProvisioning();
        return;
    }

    // Load config; if not present this will start provisioning and return false
    if (!tryLoadConfig())
        return;

    // Try to connect to configured WiFi. If connection fails, provisioning is started.
    tryConnectWifi(15000);
}

void loop()
{
    if (isProvisioningActive())
        loopProvisioning();

    if (checkFactoryReset())
        return;

    loopStatusServer();

    ArduinoOTA.handle();

    ledLoop();
}
