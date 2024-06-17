#ifndef _WIND_READING
#define _WIND_READING

#include <stdlib.h>
#include "WindUtil.h"

class WindReading
{
public:
WindReading();
~WindReading();

double set_reading(uint16_t sin, uint16_t cos);

double get_angle();
int get_error();
void load_calibration(const Range &sin_range, const Range &cos_range);
void set_offset(double degree);

Range get_sin_calibration();
Range get_cos_calibration();

private:
    Range sin_calibration;
    Range cos_calibration;
    double angle;
    double offset;
    int error;
};

#endif