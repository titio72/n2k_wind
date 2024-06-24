#include <Arduino.h>
#include "WindSpeed.h"

#define ON_THRESHOLD 3072
#define OFF_THRESHOLD 128
// number of times the counter ticks (every 1ms)
#define ZERO_THRESHOLD_MICROS 1000000
#define SIM_PERIOD_MICROS 30000

#define BUFFER_SIZE 5

#define SPEED_FACTOR 1.3

WindSpeed::WindSpeed(bool sim, uint32_t t_mics) : simulate(sim), frequency(0.0), state(STATE_UNKNOWN), counter(0), buffer_ix(0), period_micros(0), tick_micros(t_mics), sim_period(SIM_PERIOD_MICROS)
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
  if (period_micros==0)
  {
    s = 0.0;
    f = 0.0;
    e = 0;
  }
  else
  {
    // assume 2 magnets, so the number of revolution is frequency/2
    // each revolution is 2*PI*6cm so, assuming the cups moves as fast as the wind (not true, they will be slower),
    // wind speed in m/s is 2*PI*0.06*(f/2) = PI*0.06/f = PI*0.06*(10E6/p) with p the period in micros
    // we add a SPEED_FACTOR to compensate for the loss of speed of the cups (so SPEED_FACTOR>1.00)
    f = (1000000.0 / period_micros);
    s = (f * 0.06 * PI  * SPEED_FACTOR) * 3600.0 / 1852.0;
    e = 0;
  }
}

volatile uint32_t c = 0;
uint16_t sim_reading(uint32_t tick_micros, unsigned long sim_period)
{
  c++;
  if ((c*tick_micros)<=sim_period)
  {
    return OFF_THRESHOLD;
  }
  else if ((c*tick_micros)<=(sim_period*2))
  {
    return ON_THRESHOLD;
  }
  else
  {
    c = 0;
    return 25;
  }
}

int calc_new_state(uint16_t v, uint16_t *buf, uint32_t &ix)
{
  buf[ix] = v;
  ix = (ix + 1) % BUFFER_SIZE;

  bool new_state_up = true;
  bool new_state_down = true;
  for (int i = 0; i<BUFFER_SIZE; i++)
  {
    new_state_up = new_state_up && (v>=ON_THRESHOLD);
    new_state_down = new_state_down && (v<=OFF_THRESHOLD);
  }
  return new_state_up?STATE_UP:(new_state_down?STATE_DOWN:STATE_UNKNOWN);
}

void WindSpeed::set_apparent_wind_angle(double deg)
{
  apparent_wind_angle = deg;
  double c = cos(radians(deg));
  double w = 7.0 /*true wind m/s*/ + 3 /* boat speed m/s*/ * c;
  sim_period = (unsigned long)(1000000 / (w / 0.06 / PI) / SPEED_FACTOR);
}

// the time is in micros!
void WindSpeed::loop_micros(unsigned long time_micros)
{
  uint16_t v = simulate?sim_reading(tick_micros, sim_period):analogRead(SPEED_PIN);
  int new_state = calc_new_state(v, buffer, buffer_ix);
  if (new_state != STATE_UNKNOWN && new_state != state)
  {
    state = new_state;
    if (state == STATE_UP)
    {
      period_micros = counter * tick_micros;
      counter = 0;
    }
  }
  counter++;

  if ((counter*tick_micros) > ZERO_THRESHOLD_MICROS)
  {
    // assuming it's not spinning
    counter = 0;
    period_micros = 0;
  }
}

void WindSpeed::setup()
{
  analogSetPinAttenuation(SPEED_PIN, adc_attenuation_t::ADC_11db);
}