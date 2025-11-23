#include "LedDriver.h"
#include "WindUtil.h"
#include <Arduino.h>

#define LED_INTENSITY 5

#ifndef LED_PIN
#define LED_PIN -1 // disable led
#endif

LedDriver::LedDriver(): blue(false), running(false), error(WIND_ERROR_OK) {}

LedDriver::~LedDriver() {}

void LedDriver::setup()
{

}

void set_led_color(uint8_t error, bool running, int& r, int& g, int& b)
{
    if (error & WIND_ERROR_NO_SIGNAL)
    {
        // red
        r = LED_INTENSITY;
        g = 0;
        b = 0;
    }
    else if (error != WIND_ERROR_OFF_CALIBRATION)
    {
        // orange
        r = LED_INTENSITY;
        g = LED_INTENSITY * 0.65;
        b = 0;
    }
    else if (running)
    {
        // green
        r = 0;
        g = LED_INTENSITY;
        b = 0;
    }
    else
    {
        // off
        r = 0;
        g = 0;
        b = 0;
    }
}

void LedDriver::set_running(bool c)
{
    running = c;
}

void LedDriver::set_bluetooth(bool b)
{
    blue = b;
}

void LedDriver::set_error(uint8_t e)
{
    error = e;
}

void LedDriver::loop(unsigned long t)
{
  #if LED_PIN!=-1
  static unsigned long t0 = 0;
  static bool led_on = false;
  if ((t-t0)>=500)
  {
    t0 = t;
    int r, g, b;
    if (led_on)
    {
        set_led_color(error, running, r, g, b);
    }
    else
    {
        r = 0; g = 0; b = LED_INTENSITY;
    }
    led_on = !led_on;
  }
  #endif
}