#include <Arduino.h>
#include "WindSpeed.h"


/*
The vane are r=55mm from the center, so a full round is 2*pi*r
With a frequency of f HZ (there are 4 magnets, so 2 transitions per round, hence the round per seconds are f/2),
it makes a total distance of L=(f/2)*2*pi*r in 1 second, hence the speed in knots is L*3600/1852.
This is assuming that there is no friction - let's assume that the rotation speed is 50% of.
The magic number to convert Hz in Knots is 2*(pi*0.055*2600/1852) = 0.672

Raymarine sats that 20Hz (10Hz considering a full revolution) is 20Knots, which would mean a factor of 1.0
*/

#define HZ_TO_KNOTS 0.672f

WindSpeed::WindSpeed(bool sim) : simulate(sim), frequency(0.0), state(LOW), hz_to_knots(1.0), last_read_time(0)
{
}

WindSpeed::~WindSpeed()
{
}

void WindSpeed::get_speed(double &s, double &f, int &e, unsigned long t)
{
  unsigned long delta_t = t - last_read_time;
  if (counter==0 || delta_t<=0 || last_period<2000) 
  {
    s = 0.0;
    f = 0.0;
    e = 0;
  }
  else
  {
    //f = (1000000.0 / delta_t) * counter;
    f = last_period ? (double)delta_t / (double)last_period : 0.0;
    s = f * hz_to_knots;
    e = 0;
    counter = 0;
  }
  last_read_time = t;
}

void WindSpeed::set_speed_adjustment(double f)
{
  hz_to_knots = HZ_TO_KNOTS * f;
}

void WindSpeed::set_apparent_wind_angle(double deg)
{
  apparent_wind_angle = deg;
}

// the time is in micros!
void WindSpeed::loop_micros(unsigned long t)
{
  if (last_state_change_time==0) last_state_change_time = t;

  int new_state = digitalRead(SPEED_PIN);
  if (new_state!=state)
  {
    last_period = t - last_state_change_time;
    last_state_change_time = t;
    counter++;
    state = new_state;
  }
  else if ((t-last_state_change_time) > 1000000L) // 1 second of no change
  {
    last_period = 0;
    last_state_change_time = t;
  }
}

void WindSpeed::setup()
{
  pinMode(SPEED_PIN, INPUT);
}