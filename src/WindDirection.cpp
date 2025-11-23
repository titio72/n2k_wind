#include <Arduino.h>
#include <math.h>
#include "WindDirection.h"
#include "WindUtil.h"

#define SAMPLE_BUFFERING

WindDirection::WindDirection() : ix_buffer_cos(0), ix_buffer_sin(0), sumCos(0), sumSin(0)
{
    memset(sinBuffer, 0, sizeof(uint16_t) * SIN_COS_BUFFER_SIZE);
    memset(cosBuffer, 0, sizeof(uint16_t) * SIN_COS_BUFFER_SIZE);
}

WindDirection::~WindDirection()
{
}

void inline buffer_it(uint16_t v, uint16_t *buf, uint16_t &ix, double &s)
{
    uint16_t old = buf[ix];
    buf[ix] = v;
    s = s - old + v;
    ix = (ix + 1) % SIN_COS_BUFFER_SIZE;
}

inline bool is_valid_reading(uint16_t reading)
{
    // The typical range is between 1/4 and 3/4 of the totla range, hence a minimum of 1024. Below 600, is certainly a bad reading or a disconnected sensor.
    return reading <= MAX_ADC_VALUE && reading>600; // expand in future...
}


void IRAM_ATTR WindDirection::loop_micros(unsigned long now_micros) // this is called from an ISR every 1ms
{
    #ifdef SAMPLE_BUFFERING
    uint16_t i_sin = analogRead(SIN_PIN);
    uint16_t i_cos = analogRead(COS_PIN);
    buffer_it(i_sin, sinBuffer, ix_buffer_sin, sumSin);
    buffer_it(i_cos, cosBuffer, ix_buffer_cos, sumCos);
    #endif
}

void WindDirection::setup()
{
    // initilize ADC
    // set attenuation to read up to 2V (preferred range 150mV - 1750mV)
    // for Rayarine, the output is 2V-6V, hence we need a x3 divider, bringing the range to 667mv-2000mV
    analogSetPinAttenuation(SIN_PIN, adc_attenuation_t::ADC_11db);
    analogSetPinAttenuation(COS_PIN, adc_attenuation_t::ADC_11db);
}

void WindDirection::read_data(wind_data &wd, unsigned long milliseconds)
{
    #ifdef SAMPLE_BUFFERING
    wd.i_cos = (uint16_t)round(sumCos / SIN_COS_BUFFER_SIZE);
    wd.i_sin = (uint16_t)round(sumSin / SIN_COS_BUFFER_SIZE);
    #else
    wd.i_cos = analogRead(COS_PIN);
    wd.i_sin = analogRead(SIN_PIN);
    #endif
    if (!is_valid_reading(wd.i_sin) || !is_valid_reading(wd.i_cos))
    {
        wd.ellipse = NAN;
        wd.angle = NAN;
        wd.smooth_angle = NAN;
        set_error(wd.angle_error, true, WIND_ERROR_NO_SIGNAL);
    }
    else
    {
        double v_sin = sin_calib_range.to_analog(-1, 1, wd.i_sin);
        double v_cos = cos_calib_range.to_analog(-1, 1, wd.i_cos);
        wd.ellipse = sqrt(v_sin * v_sin + v_cos * v_cos);
        wd.angle = norm_deg(to_degrees(atan2(v_sin, v_cos)));
        wd.smooth_angle = lpf_angle(wd.smooth_angle, wd.angle, wd.conf.get_angle_smoothing_factor());
        set_error(wd.angle_error, false, WIND_ERROR_NO_SIGNAL);
    }
    last_read_time = milliseconds;
}

void WindDirection::apply_configuration(Conf &conf)
{
    sin_calib_range = conf.sin_range;
    cos_calib_range = conf.cos_range;
}