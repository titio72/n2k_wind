#ifndef _LED_DRIVER_H
#define _LED_DRIVER_H

#include <Arduino.h>

class LedDriver
{
public:
    LedDriver();
    virtual ~LedDriver();

    void setup();
    void loop(unsigned long time);

    void set_bluetooth(bool blue);
    void set_error(uint8_t error);
    void set_running(bool normal);

private:
    bool blue;
    bool running;
    uint8_t error;
};
#endif
