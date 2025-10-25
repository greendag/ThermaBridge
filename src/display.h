#ifndef DISPLAY_H
#define DISPLAY_H

#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "config.h"

// OLED display settings
#define OLED_RESET -1 // No reset pin
extern Adafruit_SSD1306 display;

void displayInit(const Config &cfg);
void displaySplashScreen(const Config &cfg, bool waitForInput = true);
void displayStatus(const Config &cfg);
void displayConfig();
void displayClimate();
void displayMenu();
void displayStatus(const Config &cfg);
void displayLoop();

#endif // DISPLAY_H