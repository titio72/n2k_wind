#include <Arduino.h>
#include "WindSpeed.h"
#include "WindUtil.h"
#include "Utils.h"

/*
The vane are r=55mm from the center, so a full round is 2*pi*r
With a frequency of f HZ (there are 4 magnets, so 2 transitions per round, hence the round per seconds are f/2),
it makes a total distance of L=(f/2)*2*pi*r in 1 second, hence the speed in knots is L*3600/1852.
This is assuming that there is no friction - let's assume that the rotation speed is 50% of.
The magic number to convert Hz in Knots is 2*(pi*0.055*2600/1852) = 0.672

Raymarine sats that 20Hz (10Hz considering a full revolution) is 20Knots, which would mean a factor of 1.0
*/

#define HZ_TO_KNOTS 0.672f // use 0.672f for ST50
//#define HZ_TO_KNOTS 1.000f // use 1.000f for ST60

WindSpeed::WindSpeed() : hz_to_knots(HZ_TO_KNOTS)
{
}

WindSpeed::~WindSpeed()
{
}

void WindSpeed::read_data(wind_data &data, unsigned long milliseconds)
{
  unsigned long dt = milliseconds - last_read_time;
  last_read_time = milliseconds;

  if (dt>50) // arbitrary 50ms interval between two readings (it should be 250ms)
  {
    double alpha = data.speed_smoothing_factor;
    smooth_counter = (double)counter * alpha + smooth_counter * (1.0 - alpha);
    data.frequency = (dt>50) ? (smooth_counter * 1000.0 / (double)dt)  : 0.0;
    data.speed = data.frequency * hz_to_knots;
    data.error_speed = WIND_ERROR_OK;
    counter = 0;
  }

}

void WindSpeed::set_speed_adjustment(double f)
{
  hz_to_knots = HZ_TO_KNOTS * f;
}

// the time is in micros! called from an ISR every 1ms
void WindSpeed::loop_micros(unsigned long t)
{
  if (last_state_change_time==0) last_state_change_time = t;

  int new_state = digitalRead(SPEED_PIN);
  if (new_state!=state)
  {
    if (state) 
    {
      period = t - last_state_change_time;
      last_state_change_time = t;

    }
    counter++;
    state = new_state;
  }
  else if ((t-last_state_change_time) > 1000000L) // 1 second of no change
  {
    period = 0;
    last_state_change_time = t;
  }
}

void WindSpeed::setup()
{
  pinMode(SPEED_PIN, INPUT);
}