#ifndef NATIVE
#include <Arduino.h>
#include <Ticker.h>
#endif
#include "WindSystem.h"
#include "Constants.h"
#include "DataAndConf.h"
#include <Utils.h>
#include <Log.h>

static void (*on_timer_callback)(unsigned long microseconds) = nullptr;
static LedDriver led;

#ifndef NATIVE
static Ticker ticker;
void on_timer()
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
    if (ticker.active())
            return; // already initialized
    // set CPU frequency
    setCpuFrequencyMhz(CPU_FREQUENCY);

    // initialize timer for wind measurement
    // Ticker dispatches on the esp_timer task (not from ISR context), so
    // the callback chain is allowed to make blocking calls (e.g. SPI transactions).
    ticker.attach_ms(1, on_timer); // call every 1ms
    #endif

    // initialize leds
    led.setup();
}

void WindSystem::loop(unsigned long milliseconds)
{
    led.loop(milliseconds);
}