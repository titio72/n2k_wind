#ifndef _WIND_SYSTEM_H
#define _WIND_SYSTEM_H

#include "LedDriver.h"

struct wind_data;

class WindSystem
{
public:
    static void set_timer_callback(void (*on_timer_fnc)(unsigned long microseconds));

    static void enable_usb_tracing(bool enabled);

    static void setup();

    static LedDriver &get_led();

    static void loop(unsigned long milliseconds);

private:
    WindSystem() {}
};

#endif
