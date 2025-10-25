#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

// KY-040 Rotary Encoder handling
// CLK and DT pins for rotation, SW for button

class RotaryEncoder
{
public:
    RotaryEncoder(uint8_t clkPin, uint8_t dtPin, uint8_t swPin);
    void begin();
    void update();          // Call in loop to poll
    int getPosition();      // Get current position (increments/decrements)
    bool isButtonPressed(); // Check if button was pressed (edge detect)

private:
    uint8_t clkPin, dtPin, swPin;
    volatile int position;
    bool lastClkState;
    bool buttonPressed;
    bool lastButtonState;
};

#endif // ENCODER_H