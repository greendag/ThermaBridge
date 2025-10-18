#include <Arduino.h>
#include <WiFi.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "provisioning.h"

// Pins
// On this board the RGB LED data pin is GPIO48
const uint8_t LED_PIN = 48; // onboard RGB/WS2812 data pin
const uint8_t BOOT_PIN = 0; // BOOT button (GPIO0) active-low

// LEDC (PWM) for dimming
const int LEDC_CHANNEL = 0;
const int LEDC_FREQ = 5000;          // Hz
const int LEDC_RESOLUTION = 8;       // bits (0-255)
const uint8_t LED_ON_BRIGHTNESS = 3; // 0-255 (set lower for dimmer)

// NeoPixel (WS2812) support: single pixel on LED_PIN (created at runtime)
static Adafruit_NeoPixel *neoPixel = nullptr;
static bool g_neopixelAvailable = false;

// Blink interval (slowed by ~50%)
const unsigned long BLINK_INTERVAL_MS = 750;

// Runtime flag to indicate whether PWM was successfully attached to the LED pin
static bool g_pwmAttached = false;

// State
Config cfg;
Preferences prefs;

// Factory reset action: clear stored config and preferences
void factoryReset()
{
    Serial.println("Factory reset: clearing stored configuration...");

    // Visual feedback: two red flashes at full brightness, then solid green until reboot.
    // Prefer NeoPixel when available; otherwise use PWM if attached; otherwise use digitalWrite.
    if (g_neopixelAvailable && neoPixel)
    {
        // Use amber flashes then solid green, at the same low brightness we use elsewhere
        // Amber RGB approx (255,191,0)
        auto scaled = [&](uint8_t v) -> uint8_t
        { return (uint8_t)(((uint16_t)v * (uint16_t)LED_ON_BRIGHTNESS) / 255); };
        uint8_t ar = scaled(255);
        uint8_t ag = scaled(191);
        uint8_t ab = scaled(0);
        uint8_t gr = scaled(0);
        uint8_t gg = scaled(255);
        uint8_t gb = scaled(0);

        // Flash amber twice
        for (int i = 0; i < 2; ++i)
        {
            neoPixel->setPixelColor(0, neoPixel->Color(ar, ag, ab));
            neoPixel->show();
            delay(300);
            neoPixel->setPixelColor(0, 0);
            neoPixel->show();
            delay(200);
        }
        // Solid green until reboot (same brightness)
        neoPixel->setPixelColor(0, neoPixel->Color(gr, gg, gb));
        neoPixel->show();
    }
    else if (g_pwmAttached)
    {
        // Flash using the same LED_ON_BRIGHTNESS as other indicators, then hold on
        for (int i = 0; i < 2; ++i)
        {
            ledcWrite(LEDC_CHANNEL, LED_ON_BRIGHTNESS);
            delay(300);
            ledcWrite(LEDC_CHANNEL, 0);
            delay(200);
        }
        ledcWrite(LEDC_CHANNEL, LED_ON_BRIGHTNESS);
    }
    else
    {
        // Fallback: blink digital pin twice then hold high (can't scale brightness on a single GPIO)
        for (int i = 0; i < 2; ++i)
        {
            digitalWrite(LED_PIN, HIGH);
            delay(300);
            digitalWrite(LED_PIN, LOW);
            delay(200);
        }
        digitalWrite(LED_PIN, HIGH);
    }

    // Clear preferences namespace
    prefs.begin("thermabridge", false);
    prefs.clear();
    prefs.end();

    // Erase config.json from LittleFS
    eraseConfig();

    Serial.println("Factory reset complete, rebooting...");
    delay(200);
    ESP.restart();
}

// Check for long-press on BOOT button (active low). Returns true if reset performed.
bool checkFactoryReset()
{
    // Read configured timeout (seconds). If not loaded yet, use 30s default
    uint16_t holdSecs = (cfg.reset_hold_seconds > 0) ? cfg.reset_hold_seconds : 10;
    if (digitalRead(BOOT_PIN) == LOW)
    {
        unsigned long start = millis();
        // wait while button held, but allow other background processing every 200ms
        while (digitalRead(BOOT_PIN) == LOW)
        {
            if (millis() - start >= (unsigned long)holdSecs * 1000UL)
            {
                factoryReset();
                return true;
            }
            delay(200);
        }
    }
    return false;
}

void startBlink()
{
    // Configure LED pin for PWM (LEDC) so we can lower brightness
    pinMode(LED_PIN, OUTPUT);
    Serial.print("LED_PIN: ");
    Serial.println(LED_PIN);

    // Guard: avoid attaching PWM to BOOT pin or other pins that may be unsafe.
    // If attach fails or is unsafe, fall back to plain digitalWrite blinking.
    // Only attempt NeoPixel or PWM if LED_PIN looks like a valid GPIO.
    if (LED_PIN != BOOT_PIN && LED_PIN <= 48)
    {
        // Try NeoPixel first
        neoPixel = new Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);
        neoPixel->begin();
        neoPixel->show(); // initialize to off
        g_neopixelAvailable = true;
        Serial.println("NeoPixel initialized on LED_PIN");

        // Also set up PWM in case we want to fallback later
        ledcSetup(LEDC_CHANNEL, LEDC_FREQ, LEDC_RESOLUTION);
        ledcAttachPin(LED_PIN, LEDC_CHANNEL);
        g_pwmAttached = true;
        Serial.println("PWM attached to LED pin");
    }
    else
    {
        g_pwmAttached = false;
        g_neopixelAvailable = false;
        Serial.println("NeoPixel/PWM not attached to LED pin; falling back to digitalWrite");
    }
}

void setup()
{
    Serial.begin(115200);
    delay(20);

    pinMode(BOOT_PIN, INPUT_PULLUP); // BOOT button is active-low
    startBlink();

    // Initialize LittleFS early so modules can use it
    bool lfsMounted = LittleFS.begin();
    if (!lfsMounted)
    {
        Serial.println("Warning: LittleFS failed to mount");
        // Try a safe one-time format to recover a corrupted filesystem
        Serial.println("Attempting LittleFS format to recover corrupted filesystem...");
        if (LittleFS.format())
        {
            Serial.println("LittleFS formatted, retrying mount...");
            lfsMounted = LittleFS.begin();
            if (lfsMounted)
            {
                Serial.println("LittleFS mount succeeded after format");
            }
            else
            {
                Serial.println("LittleFS still failed to mount after format");
            }
        }
        else
        {
            Serial.println("LittleFS format failed");
        }
    }

    // Load configuration from LittleFS (config.json)
    if (!loadConfig(cfg))
    {
        Serial.println("No config found, entering provisioning mode...");
        // start AP + captive portal to collect WiFi
        startProvisioning();
        return;
    }

    Serial.println("Loaded config; attempting to connect to WiFi...");
    Serial.print("SSID: ");
    Serial.println(cfg.ssid);

    // Try connecting with the stored credentials
    WiFi.mode(WIFI_STA);
    WiFi.begin(cfg.ssid.c_str(), cfg.psk.c_str());

    unsigned long start = millis();
    const unsigned long CONNECT_TIMEOUT = 15000; // 15s
    while (millis() - start < CONNECT_TIMEOUT)
    {
        if (WiFi.status() == WL_CONNECTED)
            break;
        // Allow checking factory reset while waiting
        if (checkFactoryReset())
            return; // factoryReset will restart
        delay(200);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("Connected as STA, IP: ");
        Serial.println(WiFi.localIP());
        // Start a small status server on the LAN so /status is reachable
        startStatusServer();
        // normal operation continues in loop (blink)
    }
    else
    {
        Serial.println("Failed to connect, starting provisioning AP...");
        startProvisioning();
    }
}

void loop()
{
    // If provisioning active, let it handle DNS + web server
    if (isProvisioningActive())
    {
        loopProvisioning();
    }

    // Check for long-press factory reset
    checkFactoryReset();

    // Service the STA-mode status server (if active)
    loopStatusServer();

    // Blink LED as alive indicator using PWM so it's dimmer
    static unsigned long last = 0;
    if (millis() - last >= BLINK_INTERVAL_MS)
    {
        last = millis();
        static bool on = false;
        on = !on;

        if (g_neopixelAvailable)
        {
            if (on)
            {
                // Choose amber for AP mode, green for STA mode
                // Amber RGB approx (255, 191, 0), green (0,255,0)
                uint8_t r, g, b;
                if (isProvisioningActive())
                {
                    r = 255;
                    g = 191;
                    b = 0;
                }
                else if (WiFi.status() == WL_CONNECTED)
                {
                    r = 0;
                    g = 255;
                    b = 0;
                }
                else
                {
                    // fallback color (dim blue) if neither
                    r = 0;
                    g = 0;
                    b = LED_ON_BRIGHTNESS;
                }
                // Scale by LED_ON_BRIGHTNESS (0-255) to keep brightness consistent
                uint8_t scale = LED_ON_BRIGHTNESS;
                auto scaled = [&](uint8_t v) -> uint8_t
                { return (uint8_t)(((uint16_t)v * (uint16_t)scale) / 255); };
                neoPixel->setPixelColor(0, neoPixel->Color(scaled(r), scaled(g), scaled(b)));
            }
            else
            {
                neoPixel->setPixelColor(0, 0);
            }
            neoPixel->show();
        }
        else if (g_pwmAttached)
        {
            if (on)
                ledcWrite(LEDC_CHANNEL, LED_ON_BRIGHTNESS);
            else
                ledcWrite(LEDC_CHANNEL, 0);
        }
        else
        {
            digitalWrite(LED_PIN, on ? HIGH : LOW);
        }
    }
}
