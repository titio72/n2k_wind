#include <Arduino.h>
#include "WindSpeed.h"

#define ON_THRESHOLD 3072
#define OFF_THRESHOLD 128
// number of times the counter ticks (every 1ms)
#define ZERO_THRESHOLD 1000

#define BUFFER_SIZE 5

#define SPEED_FACTOR 1.3

WindSpeed::WindSpeed(bool sim) : simulate(sim), frequency(0.0), state(STATE_UNKNOWN), counter(0), state_time(0), buffer_ix(0), last_counter(0)
{
  buffer = new uint16_t[BUFFER_SIZE];
  memset(buffer, 0, sizeof(uint16_t) * BUFFER_SIZE);
}

WindSpeed::~WindSpeed()
{
  delete buffer;
}

void WindSpeed::get_speed(double &s, double &f, int &e)
{
  if (simulate)
  {
    s = 0.0;
    f = 0.0;
    e = 0;
  }
  else
  {
    if (last_counter==0)
    {
      s = 0.0;
      f = 0.0;
      e = 0;
    }
    else
    {
      //speed in Kn
      f = (1000.0 / last_counter);
      s = (f * 0.05 * PI  * SPEED_FACTOR) * 1852.0 / 3600.0;
      e = 0;
    }
  }
}

// the time is in micros!
void WindSpeed::loop_micros()
{
  uint16_t v = analogRead(SPEED_PIN);
  buffer[buffer_ix] = v;
  buffer_ix = (buffer_ix + 1) % BUFFER_SIZE;

  bool new_state_up = true;
  bool new_state_down = true;
  for (int i = 0; i<BUFFER_SIZE; i++)
  {
    new_state_up = new_state_up && (v>ON_THRESHOLD);
    new_state_down = new_state_down && (v>OFF_THRESHOLD);
  }
  int new_state = new_state_up?STATE_UP:(new_state_down?STATE_DOWN:STATE_UNKNOWN);
  if (new_state != STATE_UNKNOWN && new_state != state)
  {
    state = new_state;
    if (state == STATE_UP)
    {
      last_counter = counter;
      counter = 0;
    }
  }
  counter++;

  if (counter > ZERO_THRESHOLD)
  {
    counter = 0;
    last_counter = 0;
  }
}

void WindSpeed::setup()
{
  analogSetPinAttenuation(SPEED_PIN, adc_attenuation_t::ADC_11db);
}