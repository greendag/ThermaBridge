// globals.h
// Shared global declarations
#pragma once

#include "config.h"
#include "encoder.h"
#include <Preferences.h>
#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

class IRrecv;
extern Config cfg;
extern Preferences prefs;
extern RotaryEncoder *encoder;
extern int menuIndex;
extern Adafruit_AHTX0 *aht20;
extern Adafruit_BMP280 *bmp280;
extern unsigned long lastEncoderActivity;
extern IRrecv *irrecv;
