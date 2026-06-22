/*
#ifndef NATIVE
#include <Arduino.h>
#endif
#include <math.h>
#include "WindDirectionADS.h"
#include "WindUtil.h"

#include <SPI.h>

#ifndef MCP3202_CS_PIN
#define MCP3202_CS_PIN GPIO_NUM_1
#define ADC_VREF    3300     // 3.3V Vref
#define ADC_CLK     1600000  // SPI clock 1.6MHz
#endif

#define SAMPLE_BUFFERING

WindDirectionADS::WindDirectionADS() : ix_buffer_cos(0), ix_buffer_sin(0), sumCos(0), sumSin(0), adc(MCP3202_CS_PIN, ADC_VREF, ADC_CLK)
{
    memset(sinBuffer, 0, sizeof(uint16_t) * SIN_COS_BUFFER_SIZE);
    memset(cosBuffer, 0, sizeof(uint16_t) * SIN_COS_BUFFER_SIZE);
}

WindDirectionADS::~WindDirectionADS()
{
}

void inline buffer_it(uint16_t v, uint16_t *buf, uint16_t &ix, double &s, uint16_t &n_samples)
{
    n_samples = std::min(SIN_COS_BUFFER_SIZE, n_samples + 1);
    uint16_t old = buf[ix];
    buf[ix] = v;
    s = s - old + v;
    ix = (ix + 1) % SIN_COS_BUFFER_SIZE;
}

inline bool is_valid_reading(uint16_t reading)
{
    // The typical range is between 1/4 and 3/4 of the total range, hence a minimum of 1024. Below 600, is certainly a bad reading or a disconnected sensor.
    return reading <= MAX_ADC_VALUE && reading>=600; // expand in future...
}

//------------------------

void WindDirectionADS::loop_micros(unsigned long now_micros, uint16_t test_cos_reading, uint16_t test_sin_reading) // this is called from an ISR every 1ms
{
    if (test_sin_reading != UINT16_MAX && test_cos_reading != UINT16_MAX)
    {
        // test purposes
        #ifdef SAMPLE_BUFFERING
        buffer_it(test_sin_reading, sinBuffer, ix_buffer_sin, sumSin, n_samples_sin);
        buffer_it(test_cos_reading, cosBuffer, ix_buffer_cos, sumCos, n_samples_cos);
        #endif
        return;
    }

    #ifndef NATIVE
    #ifdef SAMPLE_BUFFERING
    uint16_t i_sin = adc.read(MCP3208::Channel::SINGLE_0);
    uint16_t i_cos = adc.read(MCP3208::Channel::SINGLE_1);
    buffer_it(i_sin, sinBuffer, ix_buffer_sin, sumSin, n_samples_sin);
    buffer_it(i_cos, cosBuffer, ix_buffer_cos, sumCos, n_samples_cos);
    #endif
    #endif
}

void WindDirectionADS::setup()
{
    #ifndef NATIVE

    // configure PIN mode
    pinMode(MCP3202_CS_PIN, OUTPUT);

    // set initial PIN state
    digitalWrite(MCP3202_CS_PIN, HIGH);

    // initialize SPI interface for MCP3208
    SPISettings settings(ADC_CLK, MSBFIRST, SPI_MODE0);
    SPI.begin();
    SPI.beginTransaction(settings);

    // for Rayarine, the output is 2V-6V, and we have a x2 divider, bringing the range to 1V-3V
    // The reference voltage is 3.3V, so we can use the default gain of 1x, which gives us a range of 0-4.096V, which is sufficient for our needs.

    adc.calibrate(MCP3208::Channel::SINGLE_0);
    adc.calibrate(MCP3208::Channel::SINGLE_1);
    #endif
}

void WindDirectionADS::read_data(wind_data &wd, configuration &conf, unsigned long milliseconds)
{
    #ifdef SAMPLE_BUFFERING
    if (n_samples_cos == 0) return; // no data yet
    wd.i_cos = (uint16_t)round(sumCos / n_samples_cos);
    wd.i_sin = (uint16_t)round(sumSin / n_samples_sin);
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
        wd.smooth_angle = lpf_angle(wd.smooth_angle, wd.angle, conf.get_angle_smoothing_factor());
        set_error(wd.angle_error, false, WIND_ERROR_NO_SIGNAL);
    }
    last_read_time = milliseconds;
}

void WindDirectionADS::apply_configuration(configuration &conf)
{
    sin_calib_range = conf.sin_range;
    cos_calib_range = conf.cos_range;
}
    */