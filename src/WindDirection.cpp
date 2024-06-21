#include <Arduino.h>
#include <math.h>
#include "WindDirection.h"
#include "WindUtil.h"

#define SIMUL_RANGE_SIN_LOW 1024
#define SIMUL_RANGE_SIN_HIGH 3072
#define SIMUL_RANGE_COS_LOW 1024
#define SIMUL_RANGE_COS_HIGH 3072

#define BUFFER_SIZE 10

WindDirection::WindDirection(WindReading &w, bool s) : w_calc(w), simulate(s), expected(0.0), ix_buffer_cos(0), ix_buffer_sin(0), sumCos(0), sumSin(0)
{
    sinBuffer = new uint16_t[BUFFER_SIZE];
    cosBuffer = new uint16_t[BUFFER_SIZE];
    memset(sinBuffer, 0, sizeof(uint16_t)*BUFFER_SIZE);
    memset(cosBuffer, 0, sizeof(uint16_t)*BUFFER_SIZE);
}

WindDirection::~WindDirection()
{
    delete sinBuffer;
    delete cosBuffer;
}

#define read_it(v, buf, ix, s) \
{ uint16_t old = buf[ix]; \
buf[ix] = v; \
s = s - old + v; \
ix = (ix + 1) % BUFFER_SIZE; }

void sim_values(uint16_t &s, uint16_t &c, unsigned long now_micros, double &expected)
{
    static Range sim_sin(SIMUL_RANGE_SIN_LOW, SIMUL_RANGE_SIN_HIGH, 0);
    static Range sim_cos(SIMUL_RANGE_COS_LOW, SIMUL_RANGE_COS_HIGH, 0);
    unsigned long t = now_micros / 1000L;
    static unsigned long t0 = t;
    unsigned long dt = t - t0;
    t0 = t;
    if (dt > 0)
    {
        double incr = 6.0 * (dt / 1000.0);
        static double deg = 0;
        deg = (deg + incr);
        deg = (deg > 360.0) ? (deg - 360.0) : deg;
        double v_sin = sin(to_radians(deg));
        double v_cos = cos(to_radians(deg));
        s = to_digital(v_sin, -1, 1, sim_sin) + get_noise(sim_sin.size() * 0.05);
        c = to_digital(v_cos, -1, 1, sim_cos) + get_noise(sim_cos.size() * 0.05);
        expected = deg;
    }
}


void WindDirection::loop_micros(uint64_t now_micros)
{
    uint16_t i_sin = 0;
    uint16_t i_cos = 0;
    if (simulate)
    {
        //sim_values(i_sin, i_cos, now_micros, expected);
        i_sin = 2319;
        i_cos = 1650;
    }
    else
    {
        i_sin = analogRead(SIN_PIN);
        i_cos = analogRead(COS_PIN);
    }
 /*

    uint16_t old = sinBuffer[ix_buffer_sin];
    sinBuffer[ix_buffer_sin] = i_sin;
    sumSin = sumSin - old + i_sin;
    ix_buffer_sin = (ix_buffer_sin + 1) % BUFFER_SIZE;

    old = cosBuffer[ix_buffer_cos];
    cosBuffer[ix_buffer_cos] = i_cos;
    sumCos = sumCos - old + i_cos;
    ix_buffer_cos = (ix_buffer_cos + 1) % BUFFER_SIZE;
  */  
    read_it(i_sin, sinBuffer, ix_buffer_sin, sumSin);
    read_it(i_cos, cosBuffer, ix_buffer_cos, sumCos);
}

void WindDirection::setup()
{
    // initilize ADC
    // set attenuation to read up to 2V (preferred range 150mV - 1750mV)
    // for Rayarine, the output is 2V-6V, hence we need a x3 divider, bringing the range to 667mv-2000mV
    analogSetPinAttenuation(SIN_PIN, adc_attenuation_t::ADC_11db);
    analogSetPinAttenuation(COS_PIN, adc_attenuation_t::ADC_11db);

}

void WindDirection::get_sincos(uint16_t &i_sin, uint16_t &i_cos)
{
    i_cos = (uint16_t)((double)sumCos / BUFFER_SIZE + 0.5);
    i_sin = (uint16_t)((double)sumSin / BUFFER_SIZE + 0.5);
}

double WindDirection::get_expected()
{
    return expected;
}

void WindDirection::get_angle(uint16_t &i_sin, uint16_t &i_cos, double &angle, double &err, int &error)
{
  get_sincos(i_sin, i_cos);
  err = w_calc.set_reading(i_sin, i_cos);
  angle = w_calc.get_angle();
  error = w_calc.get_error();
}