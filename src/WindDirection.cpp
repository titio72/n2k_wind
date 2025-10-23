#include <Arduino.h>
#include <math.h>
#include "WindDirection.h"
#include "WindUtil.h"

#define BUFFER_SIZE 100

WindDirection::WindDirection(SinCosDecoder &w, bool s) : simulator(), w_calc(w), simulate(s), expected(0.0), ix_buffer_cos(0), ix_buffer_sin(0), sumCos(0), sumSin(0)
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

void inline read_it(uint16_t v, uint16_t *buf, uint8_t &ix, uint32_t &s)
{
    uint16_t old = buf[ix];
    buf[ix] = v;
    s = s - old + v;
    ix = (ix + 1) % BUFFER_SIZE;
}

void WindDirection::loop_micros(uint64_t now_micros)
{
    uint16_t i_sin = 0;
    uint16_t i_cos = 0;
    if (simulate)
    {
        simulator.sim_values(i_sin, i_cos, now_micros, expected);
    }
    else
    {
        i_sin = analogRead(SIN_PIN);
        i_cos = analogRead(COS_PIN);
        expected = NAN;
    }
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

void WindDirection::get_angle(uint16_t &i_sin, uint16_t &i_cos, double &angle, double &ellipse, int &error)
{
    get_sincos(i_sin, i_cos);
    w_calc.set_reading(i_sin, i_cos);
    ellipse = w_calc.get_ellipse();
    angle = w_calc.get_angle();
    error = w_calc.get_error();
}
