#include "globals.h"

// Define globals declared in globals.h
Config cfg;
Preferences prefs;
RotaryEncoder *encoder = nullptr;
int menuIndex = 0;
Adafruit_AHTX0 *aht20 = nullptr;
Adafruit_BMP280 *bmp280 = nullptr;
unsigned long lastEncoderActivity = 0;
IRrecv *irrecv = nullptr;
