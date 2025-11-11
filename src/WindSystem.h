#ifndef _WIND_SYSTEM_H
#define _WIND_SYSTEM_H

#include <Arduino.h>

class WindSystem
{
public:
    static WindSystem &get_instance();

    void set_timer_callback(void (*on_timer_fnc)(unsigned long microseconds));

    void enable_usb_tracing(bool enabled);

    void setup();

private:
    WindSystem() {}
};

#endif
