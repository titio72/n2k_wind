#ifndef _WIND_DIRECTION_H
#define _WIND_DIRECTION_H
#include <stdint.h>
#include "SinCosDecoder.h"
#include "DataAndConf.h"

class WindDirection
{
public:
    WindDirection();
    ~WindDirection();

    void setup();

    void read_data(wind_data& wd, unsigned long milliseconds);

    void loop_micros(unsigned long now_micros);

    unsigned long get_sample_age() { return last_read_time; }

    void apply_configuration(Conf &conf);

private:
    SinCosDecoder w_calc;
    uint16_t sinBuffer[SIN_COS_BUFFER_SIZE];
    uint16_t cosBuffer[SIN_COS_BUFFER_SIZE];
    uint16_t ix_buffer_sin, ix_buffer_cos;
    double sumSin, sumCos;
    unsigned long last_read_time = 0;
};
#endif