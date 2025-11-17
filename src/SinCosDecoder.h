#ifndef _WIND_READING
#define _WIND_READING

#include <stdlib.h>
#include "WindUtil.h"

class Conf;

class SinCosDecoder
{
public:
SinCosDecoder();
~SinCosDecoder();

void set_reading(uint16_t sin, uint16_t cos);

double get_angle();
int get_error();
double get_ellipse();

void sim_values(uint16_t &s, uint16_t &c, unsigned long now_micros, double &expected);

Range &get_sin_calibration() { return sin_calibration; }
Range &get_cos_calibration() { return cos_calibration; }

void apply_configuration(Conf& conf);

private:
    double get_sim_angle(unsigned long now_micros);
    Range sim_sin;
    Range sim_cos;
    double sim_deg;
    unsigned long sim_t0;

    Range sin_calibration;
    Range cos_calibration;
    double angle;
    double ellipse;
    int error;
};

#endif