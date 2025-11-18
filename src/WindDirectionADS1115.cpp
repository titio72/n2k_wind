#include <Arduino.h>
#include <math.h>
#include "WindDirectionADS1115.h"
#include "WindUtil.h"

WindDirectionADS::WindDirectionADS() : ix_buffer_cos(0), ix_buffer_sin(0), sumCos(0), sumSin(0), ads()
{
    memset(sinBuffer, 0, sizeof(uint16_t) * SIN_COS_BUFFER_SIZE);
    memset(cosBuffer, 0, sizeof(uint16_t) * SIN_COS_BUFFER_SIZE);
}

WindDirectionADS::~WindDirectionADS()
{
}

void inline buffer_it(uint16_t v, uint16_t *buf, uint16_t &ix, double &s)
{
    uint16_t old = buf[ix];
    buf[ix] = v;
    s = s - old + v;
    ix = (ix + 1) % SIN_COS_BUFFER_SIZE;
}

void IRAM_ATTR WindDirectionADS::loop_micros(unsigned long now_micros) // this is called from an ISR every 1ms
{
    if (!ads.conversionComplete())
    {
        return;
    }
    uint16_t v = ads.getLastConversionResults();

    // alternate reading cos and sin at each cycle
    channel_to_read = channel_to_read ? 0 : 1;

    uint16_t nextRegistryAds = ADS1X15_REG_CONFIG_MUX_DIFF_0_1;
    uint16_t *buffer = cosBuffer;
    double& sum = sumCos;
    uint16_t &ix = ix_buffer_cos;
    if (channel_to_read)
    {
        nextRegistryAds = ADS1X15_REG_CONFIG_MUX_DIFF_2_3;
        buffer = sinBuffer;
        sum = sumSin;
        ix = ix_buffer_sin;   
    }
    buffer_it(v, buffer, ix, sum);
    ads.startADCReading(nextRegistryAds, false);
}

void WindDirectionADS::setup()
{
    ads.begin();
    ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
    ads.setDataRate(RATE_ADS1115_860SPS);
}

void WindDirectionADS::read_data(wind_data &wd, unsigned long milliseconds)
{
    wd.i_cos = (uint16_t)round(sumCos / SIN_COS_BUFFER_SIZE);
    wd.i_sin = (uint16_t)round(sumSin / SIN_COS_BUFFER_SIZE);
    w_calc.set_reading(wd.i_sin, wd.i_cos);
    wd.ellipse = w_calc.get_ellipse();
    wd.angle = w_calc.get_angle();
    wd.smooth_angle = lpf_angle(wd.smooth_angle, wd.angle, wd.conf.get_angle_smoothing_factor());
    wd.error = w_calc.get_error();
    last_read_time = milliseconds;
}

void WindDirectionADS::apply_configuration(Conf &conf)
{
    w_calc.apply_configuration(conf);
}