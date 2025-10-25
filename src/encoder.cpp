#include "encoder.h"

RotaryEncoder::RotaryEncoder(uint8_t clkPin, uint8_t dtPin, uint8_t swPin)
    : clkPin(clkPin), dtPin(dtPin), swPin(swPin), position(0), lastClkState(HIGH), buttonPressed(false), lastButtonState(HIGH) {}

void RotaryEncoder::begin()
{
    pinMode(clkPin, INPUT_PULLUP);
    pinMode(dtPin, INPUT_PULLUP);
    pinMode(swPin, INPUT_PULLUP);
    lastClkState = digitalRead(clkPin);
    lastButtonState = digitalRead(swPin);
}

void RotaryEncoder::update()
{
    bool clkState = digitalRead(clkPin);
    bool dtState = digitalRead(dtPin);
    bool buttonState = digitalRead(swPin);

    // Detect rotation on rising edge of CLK
    if (clkState != lastClkState && clkState == HIGH)
    {
        if (dtState == LOW)
        {
            position++;
        }
        else
        {
            position--;
        }
    }
    lastClkState = clkState;

    // Detect button press (falling edge)
    if (buttonState == LOW && lastButtonState == HIGH)
    {
        buttonPressed = true;
    }
    else
    {
        buttonPressed = false;
    }
    lastButtonState = buttonState;
}

int RotaryEncoder::getPosition()
{
    return position;
}

bool RotaryEncoder::isButtonPressed()
{
    return buttonPressed;
}