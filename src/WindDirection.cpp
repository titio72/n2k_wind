#include <Arduino.h>
#include <math.h>
#include "WindDirection.h"
#include "WindUtil.h"

#define BUFFER_SIZE 400 // about 0.4s averaging at 1ms rate 

WindDirection::WindDirection(SinCosDecoder &w) : w_calc(w), expected(0.0), ix_buffer_cos(0), ix_buffer_sin(0), sumCos(0), sumSin(0)
{
    sinBuffer = new uint16_t[BUFFER_SIZE];
    cosBuffer = new uint16_t[BUFFER_SIZE];
    memset(sinBuffer, 0, sizeof(uint16_t) * BUFFER_SIZE);
    memset(cosBuffer, 0, sizeof(uint16_t) * BUFFER_SIZE);
}

WindDirection::~WindDirection()
{
    delete sinBuffer;
    delete cosBuffer;
}

void inline buffer_it(uint16_t v, uint16_t *buf, uint16_t &ix, double &s)
{
    uint16_t old = buf[ix];
    buf[ix] = v;
    s = s - old + v;
    ix = (ix + 1) % BUFFER_SIZE;
}

void WindDirection::loop_micros(unsigned long now_micros) // this is called from an ISR every 1ms
{
    uint16_t i_sin = analogRead(SIN_PIN);
    uint16_t i_cos = analogRead(COS_PIN);
    expected = NAN;
    buffer_it(i_sin, sinBuffer, ix_buffer_sin, sumSin);
    buffer_it(i_cos, cosBuffer, ix_buffer_cos, sumCos);
}

void WindDirection::setup()
{
    // initilize ADC
    // set attenuation to read up to 2V (preferred range 150mV - 1750mV)
    // for Rayarine, the output is 2V-6V, hence we need a x3 divider, bringing the range to 667mv-2000mV
    analogSetPinAttenuation(SIN_PIN, adc_attenuation_t::ADC_11db);
    analogSetPinAttenuation(COS_PIN, adc_attenuation_t::ADC_11db);
}

double WindDirection::get_expected()
{
    return expected;
}

void WindDirection::read_data(wind_data &wd, unsigned long milliseconds)
{
    wd.i_cos = (uint16_t)round(sumCos / BUFFER_SIZE);
    wd.i_sin = (uint16_t)round(sumSin / BUFFER_SIZE);
    w_calc.set_reading(wd.i_sin, wd.i_cos);
    wd.ellipse = w_calc.get_ellipse();
    wd.angle = w_calc.get_angle();
    wd.smooth_angle = lpf_angle(wd.smooth_angle, wd.angle, wd.angle_smoothing_factor);
    wd.error = w_calc.get_error();
    last_read_time = milliseconds;
}