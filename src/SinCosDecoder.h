#ifndef _WIND_READING
#define _WIND_READING

#include <stdlib.h>
#include "WindUtil.h"

class SinCosDecoder
{
public:
SinCosDecoder();
~SinCosDecoder();

double set_reading(uint16_t sin, uint16_t cos);

double get_angle();
int get_error();
void load_calibration(const Range &sin_range, const Range &cos_range);
void set_offset(double degree);

Range get_sin_calibration();
Range get_cos_calibration();

void sim_values(uint16_t &s, uint16_t &c, unsigned long now_micros, double &expected);

private:
    double get_sim_angle(unsigned long now_micros);
    Range sim_sin;
    Range sim_cos;
    double sim_deg;
    unsigned long sim_t0;

    Range sin_calibration;
    Range cos_calibration;
    double angle;
    double offset;
    int error;
};

#endif