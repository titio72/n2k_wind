#include "LedDriver.h"
#include "WindUtil.h"
#include <Arduino.h>

#define LED_INTENSITY 5
#define LED_PIN 10

LedDriver::LedDriver(): blue(false), calib(false), error(WIND_ERROR_OK) {}

LedDriver::~LedDriver() {}

void LedDriver::setup()
{

}

void set_led_color(int error, bool calibration, int& r, int& g, int& b)
{
    if (calibration)
    {
        r = LED_INTENSITY / 2; g = LED_INTENSITY / 2; b = LED_INTENSITY / 2;
    }
    else {
        b = 0;
        switch (error)
        {
            case WIND_ERROR_OK:
            r = 0; g = LED_INTENSITY;
            break;
            case WIND_ERROR_NO_CAL_OR_SIGNAL:
            r = LED_INTENSITY; g = 0;
            break;
            default:
            r = 0; g = 0;
            break;
        }
    }
}

void LedDriver::set_calibration(bool c)
{
    calib = c;
}

void LedDriver::set_blue(bool b)
{
    blue = b;
}

void LedDriver::set_error(int e)
{
    error = e;
}

void LedDriver::loop(unsigned long t)
{

  static unsigned long t0 = 0;
  static bool led_on = false;
  if ((t-t0)>=500)
  {
    t0 = t;
    if (led_on)
    {
        int r, g, b;
        set_led_color(error, calib, r, g, b);
        neopixelWrite(LED_PIN, g, r, b);
    }
    else
    {
        int b = blue?LED_INTENSITY:0;
        neopixelWrite(LED_PIN, 0, 0, b);
    }
    led_on = !led_on;
  }
}