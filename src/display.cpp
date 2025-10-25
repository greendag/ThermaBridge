#include "display.h"
#include <Wire.h>
#include <WiFi.h>
#include "config.h"
#include "build_info.h"
#include "globals.h"

Adafruit_SSD1306 display(128, 64, &Wire, OLED_RESET);

const char *menuItems[] = {"Status", "Config", "Climate", "Reboot"};
const int MENU_ITEMS_COUNT = 4;
const int MAX_VISIBLE_MENU_ITEMS = 6;

int getOptimalTextSize(String text, int maxWidth)
{
    int len = text.length();
    for (int size = 3; size >= 1; size--)
    {
        int charWidth = size * 6; // approximate character width
        if (len * charWidth <= maxWidth)
            return size;
    }
    return 1;
}

void displayInit(const Config &cfg)
{
    if (!cfg.display_enabled)
        return;

    Wire.begin(cfg.display_sda_pin, cfg.display_scl_pin);

    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("SSD1306 allocation failed");
        return;
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.display();
    Serial.println("SSD1306 initialized");
}

void displaySplashScreen(const Config &cfg, bool waitForInput)
{
    display.clearDisplay();

    // Device name
    int nameSize = getOptimalTextSize(cfg.devname, 128);
    display.setTextSize(nameSize);
    int charWidth = nameSize * 6;
    int textWidth = cfg.devname.length() * charWidth;
    int x = (128 - textWidth) / 2;
    display.setCursor(x, 10);
    display.println(cfg.devname);

    // Version
    String version = String(FW_BASE_VERSION).substring(1); // remove 'v'
    String verText = "Ver " + version;
    display.setTextSize(1); // smaller
    charWidth = 6;
    textWidth = verText.length() * charWidth;
    x = (128 - textWidth) / 2;
    display.setCursor(x, 40);
    display.println(verText);

    display.display();

    if (!waitForInput)
        return;

    // Wait for encoder input
    if (encoder)
    {
        encoder->update(); // Update before getting initial position
        int initialPos = encoder->getPosition();
        while (true)
        {
            encoder->update();
            if (encoder->getPosition() != initialPos || encoder->isButtonPressed())
            {
                if (encoder->isButtonPressed())
                {
                    // Wait for button release to avoid immediate select in menu
                    while (encoder->isButtonPressed())
                    {
                        encoder->update();
                        delay(10);
                    }
                }
                break;
            }
            delay(10); // small delay to avoid busy loop
        }
    }
    else
    {
        delay(3000); // fallback if no encoder
    }
}
String formatUptime()
{
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    unsigned long days = hours / 24;
    if (days > 0)
    {
        return String(days) + "d " + String(hours % 24) + "h " + String(minutes % 60) + "m";
    }
    else if (hours > 0)
    {
        return String(hours) + "h " + String(minutes % 60) + "m";
    }
    else
    {
        return String(minutes) + "m " + String(seconds % 60) + "s";
    }
}

void displayStatus(const Config &cfg)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.printf("IP: %s\n", WiFi.localIP().toString().c_str());
    display.printf("Uptime: %s\n", formatUptime().c_str());
    display.display();
}

void displayConfig()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.printf("SSID: %s\n", cfg.ssid.c_str());
    String masked = "";
    for (size_t i = 0; i < cfg.psk.length(); ++i)
        masked += '*';
    display.printf("PSK: %s\n", masked.c_str());
    display.printf("Dev: %s\n", cfg.devname.c_str());
    display.display();
}

void displayClimate()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    if (aht20)
    {
        sensors_event_t humidity, temp;
        aht20->getEvent(&humidity, &temp);
        if (!isnan(temp.temperature))
        {
            display.printf("Temp: %.1f F\n", (temp.temperature * 9.0 / 5.0) + 32.0);
        }
        else
        {
            display.println("Temp: -- F");
        }
        if (!isnan(humidity.relative_humidity))
        {
            display.printf("Hum: %.1f %%\n", humidity.relative_humidity);
        }
        else
        {
            display.println("Hum: -- %");
        }
    }
    if (bmp280)
    {
        float pres = bmp280->readPressure() / 100.0F;
        if (!isnan(pres))
        {
            display.printf("Pres: %.1f hPa\n", pres);
        }
        else
        {
            display.println("Pres: -- hPa");
        }
        float alt = bmp280->readAltitude(1013.25F); // sea level pressure
        if (!isnan(alt))
        {
            display.printf("Alt: %.1f ft\n", alt * 3.28084F);
        }
        else
        {
            display.println("Alt: -- ft");
        }
    }
    display.display();
}

void displayMenu()
{
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);

    int start = 0;
    if (MENU_ITEMS_COUNT > MAX_VISIBLE_MENU_ITEMS)
    {
        start = menuIndex - MAX_VISIBLE_MENU_ITEMS / 2;
        if (start < 0)
            start = 0;
        if (start + MAX_VISIBLE_MENU_ITEMS > MENU_ITEMS_COUNT)
            start = MENU_ITEMS_COUNT - MAX_VISIBLE_MENU_ITEMS;
    }

    for (int i = start; i < min(start + MAX_VISIBLE_MENU_ITEMS, MENU_ITEMS_COUNT); i++)
    {
        if (i == menuIndex)
            display.print("> ");
        else
            display.print("  ");
        display.println(menuItems[i]);
    }
    display.display();
}

void handleMenuSelect()
{
    switch (menuIndex)
    {
    case 0:
        displayStatus(cfg);
        break;
    case 1:
        displayConfig();
        break;
    case 2:
        displayClimate();
        break;
    case 3:
        ESP.restart();
        break;
    }
}

void displayLoop()
{
    if (!encoder)
    {
        // No encoder, just status
        static unsigned long lastUpdate = 0;
        if (millis() - lastUpdate > 5000)
        {
            displayStatus(cfg);
            lastUpdate = millis();
        }
        return;
    }

    encoder->update(); // Update encoder state

    // Check for idle
    if (millis() - lastEncoderActivity > 30000)
    {
        displaySplashScreen(cfg, false);
        // Wait for encoder input to dismiss
        encoder->update();
        int initialPos = encoder->getPosition();
        while (true)
        {
            encoder->update();
            if (encoder->getPosition() != initialPos || encoder->isButtonPressed())
            {
                lastEncoderActivity = millis();
                break;
            }
            delay(10);
        }
    }

    // With encoder, menu mode
    static int lastPos = 0;
    static bool initialized = false;
    if (!initialized)
    {
        lastPos = encoder->getPosition();
        lastEncoderActivity = millis();
        initialized = true;
        displayMenu(); // Show initial menu
    }

    int pos = encoder->getPosition();
    if (pos != lastPos)
    {
        menuIndex = (menuIndex + (pos - lastPos)) % MENU_ITEMS_COUNT;
        if (menuIndex < 0)
            menuIndex += MENU_ITEMS_COUNT;
        lastPos = pos;
        lastEncoderActivity = millis();
        displayMenu();
    }

    if (encoder->isButtonPressed())
    {
        lastEncoderActivity = millis();
        handleMenuSelect();
    }
}