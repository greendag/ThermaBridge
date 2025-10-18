#include "led.h"
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include "globals.h"
#include <WiFi.h>
#include "provisioning.h"

// Pins and PWM config are kept in main.cpp for now; mirror constants here
static const uint8_t LED_PIN = 48;
static const int LEDC_CHANNEL = 0;
static const uint8_t LED_ON_BRIGHTNESS = 3;

static Adafruit_NeoPixel *neoPixel = nullptr;
static bool neopixelAvailable = false;
static bool pwmAttached = false;
static unsigned long lastMs = 0;
static bool stateOn = false;

static uint8_t scale(uint8_t v)
{
    return (uint8_t)(((uint16_t)v * (uint16_t)LED_ON_BRIGHTNESS) / 255);
}

void ledInit()
{
    pinMode(LED_PIN, OUTPUT);
    // Init NeoPixel if possible
    neopixelAvailable = false;
    pwmAttached = false;
    neoPixel = new Adafruit_NeoPixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);
    neoPixel->begin();
    neoPixel->show();
    neopixelAvailable = true;

    ledcSetup(LEDC_CHANNEL, 5000, 8);
    ledcAttachPin(LED_PIN, LEDC_CHANNEL);
    pwmAttached = true;
}

void ledSetOff()
{
    if (neopixelAvailable && neoPixel)
    {
        neoPixel->setPixelColor(0, 0);
        neoPixel->show();
    }
    else if (pwmAttached)
    {
        ledcWrite(LEDC_CHANNEL, 0);
    }
    else
    {
        digitalWrite(LED_PIN, LOW);
    }
}

void ledSetProvisioningColor()
{
    if (neopixelAvailable && neoPixel)
    {
        neoPixel->setPixelColor(0, neoPixel->Color(scale(255), scale(191), scale(0)));
        neoPixel->show();
    }
    else if (pwmAttached)
    {
        ledcWrite(LEDC_CHANNEL, LED_ON_BRIGHTNESS);
    }
    else
    {
        digitalWrite(LED_PIN, HIGH);
    }
}

void ledSetStaColor()
{
    if (neopixelAvailable && neoPixel)
    {
        neoPixel->setPixelColor(0, neoPixel->Color(scale(0), scale(255), scale(0)));
        neoPixel->show();
    }
    else if (pwmAttached)
    {
        ledcWrite(LEDC_CHANNEL, LED_ON_BRIGHTNESS);
    }
    else
    {
        digitalWrite(LED_PIN, HIGH);
    }
}

void ledFactoryResetVisual()
{
    if (neopixelAvailable && neoPixel)
    {
        // amber flashes then solid green
        for (int i = 0; i < 2; ++i)
        {
            neoPixel->setPixelColor(0, neoPixel->Color(scale(255), scale(191), scale(0)));
            neoPixel->show();
            delay(300);
            neoPixel->setPixelColor(0, 0);
            neoPixel->show();
            delay(200);
        }
        neoPixel->setPixelColor(0, neoPixel->Color(scale(0), scale(255), scale(0)));
        neoPixel->show();
    }
    else if (pwmAttached)
    {
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
        for (int i = 0; i < 2; ++i)
        {
            digitalWrite(LED_PIN, HIGH);
            delay(300);
            digitalWrite(LED_PIN, LOW);
            delay(200);
        }
        digitalWrite(LED_PIN, HIGH);
    }
}

void ledLoop()
{
    if (millis() - lastMs < 750)
        return;
    lastMs = millis();
    stateOn = !stateOn;
    if (stateOn)
    {
        if (isProvisioningActive())
            ledSetProvisioningColor();
        else if (WiFi.status() == WL_CONNECTED)
            ledSetStaColor();
        else
            ledSetOff();
    }
    else
    {
        ledSetOff();
    }
}
