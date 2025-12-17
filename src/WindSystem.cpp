#ifndef NATIVE
#include <Arduino.h>
#endif
#include "WindSystem.h"
#include "Constants.h"
#include "DataAndConf.h"
#include <Utils.h>
#include <Log.h>

static void (*on_timer_callback)(unsigned long microseconds) = nullptr;
static LedDriver led;

#ifndef NATIVE
static hw_timer_t *timer = nullptr;
void IRAM_ATTR on_timer()
{
    if (on_timer_callback)
        on_timer_callback(micros());
}
#endif

void WindSystem::set_timer_callback(void (*on_timer_fnc)(unsigned long microseconds))
{
    on_timer_callback = on_timer_fnc;
}

void WindSystem::enable_usb_tracing(bool enabled)
{
    if (enabled)
    {
        Log::enable();
        
    }
    else
    {
        Log::disable();
    }
}

LedDriver &WindSystem::get_led()
{ 
    return led; 
}

void WindSystem::setup()
{
    #ifndef NATIVE
    if (timer != nullptr)
            return; // already initialized
    // set CPU frequency
    setCpuFrequencyMhz(CPU_FREQUENCY);

    // initialize hw timer for wind measurement
    timer = timerBegin(1, 80, true);              // Timer 0, clock divider 80
    timerAttachInterrupt(timer, &on_timer, true); // Attach the interrupt handling function
    timerAlarmWrite(timer, 1000, true);           // Interrupt every 1ms
    timerAlarmEnable(timer);                      // Enable the alarm
    #endif

    // initialize leds
    led.setup();
}

void WindSystem::loop(unsigned long milliseconds)
{
    led.loop(milliseconds);
}