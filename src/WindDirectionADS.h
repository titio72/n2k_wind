#ifndef _WIND_DIRECTION_ADS_H
#define _WIND_DIRECTION_ADS_H
#include <stdint.h>
#include "DataAndConf.h"
#include <Mcp320x.h>


class WindDirectionADS
{
public:
    WindDirectionADS();
    ~WindDirectionADS();

    void setup();

    void read_data(wind_data& wd, configuration &conf, unsigned long milliseconds);

    void loop_micros(unsigned long now_micros, uint16_t test_cos_reading = UINT16_MAX, uint16_t test_sin_reading = UINT16_MAX);

    unsigned long get_sample_age() const { return last_read_time; }

    void apply_configuration(configuration &conf);

private:
    MCP3208 adc;
    uint16_t sinBuffer[SIN_COS_BUFFER_SIZE];
    uint16_t cosBuffer[SIN_COS_BUFFER_SIZE];
    uint16_t n_samples_cos = 0;
    uint16_t n_samples_sin = 0;
    Range sin_calib_range, cos_calib_range;
    uint16_t ix_buffer_sin, ix_buffer_cos;
    double sumSin, sumCos;
    unsigned long last_read_time = 0;
};
#endif