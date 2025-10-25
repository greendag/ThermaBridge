#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>

struct Config
{
    String ssid;
    String psk;
    String devname;
    uint16_t reset_hold_seconds; // seconds to hold BOOT button to factory reset
    String ota_password;         // optional password for ArduinoOTA
    bool mdns_enable;
    bool display_enabled;    // enable OLED display
    uint16_t display_width;  // display width in pixels
    uint16_t display_height; // display height in pixels
    uint8_t display_sda_pin; // I2C SDA pin for display
    uint8_t display_scl_pin; // I2C SCL pin for display
    bool encoder_enabled;    // enable KY-040 rotary encoder
    uint8_t encoder_clk_pin; // CLK pin for encoder
    uint8_t encoder_dt_pin;  // DT pin for encoder
    uint8_t encoder_sw_pin;  // SW pin for encoder
    bool climate_enabled;    // enable climate sensors (AHT20 + BMP280)
    bool ir_enabled;         // enable IR receiver (VS1838B)
    uint8_t ir_pin;          // pin for IR receiver
};

bool loadConfig(Config &cfg);
bool saveConfig(const Config &cfg);
bool eraseConfig();

#endif // CONFIG_H
