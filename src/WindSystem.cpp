#include <Arduino.h>
#include "WindSystem.h"
#include "Constants.h"
#include <Utils.h>
#include <WiFi.h>
#include <Log.h>

static hw_timer_t *timer = nullptr;
static void (*on_timer_callback)(unsigned long microseconds) = nullptr;
static WindSystem *instance = nullptr;

void IRAM_ATTR on_timer()
{
    if (on_timer_callback)
        on_timer_callback(micros());
}

WindSystem &WindSystem::get_instance()
{
    if (instance == nullptr)
        instance = new WindSystem();
    return *instance;
}

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

void WindSystem::setup()
{
    setCpuFrequencyMhz(CPU_FREQUENCY);

    // initialize hw timer for wind measurement
    timer = timerBegin(1, 80, true);              // Timer 0, clock divider 80
    timerAttachInterrupt(timer, &on_timer, true); // Attach the interrupt handling function
    timerAlarmWrite(timer, 1000, true);           // Interrupt every 1ms
    timerAlarmEnable(timer);                      // Enable the alarm
}