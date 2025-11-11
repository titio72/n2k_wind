#ifndef _WIND_DIRECTION_H
#define _WIND_DIRECTION_H
#include <stdint.h>
#include "SinCosDecoder.h"
#include "WindAngleSImulator.h"

class WindDirection
{
public:
    WindDirection(SinCosDecoder &wcalc);
    ~WindDirection();

    double get_expected();

    void setup();

    void read_data(wind_data& wd, unsigned long milliseconds);

    void loop_micros(unsigned long now_micros);

    unsigned long get_sample_age() { return last_read_time; }

private:
    SinCosDecoder &w_calc;
    double expected;
    uint16_t *sinBuffer, *cosBuffer;
    uint16_t ix_buffer_sin, ix_buffer_cos;
    double sumSin, sumCos;

    unsigned long last_read_time = 0;
};
#endif