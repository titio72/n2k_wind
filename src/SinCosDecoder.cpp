
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "SinCosDecoder.h"
#include "DataAndConf.h"
#include "Constants.h"

SinCosDecoder::SinCosDecoder() :
    angle(NAN), error(WIND_ERROR_OK)
{}

SinCosDecoder::~SinCosDecoder()
{}

double SinCosDecoder::get_angle()
{
    return angle;
}

int SinCosDecoder::get_error()
{
    return error;
}

double SinCosDecoder::get_ellipse()
{
    return ellipse;
}

void SinCosDecoder::set_reading(uint16_t sin_reading, uint16_t cos_reading)
{
    double v_sin = to_analog(sin_reading, -1, 1, sin_calibration);
    double v_cos = to_analog(cos_reading, -1, 1, cos_calibration);
    ellipse = sqrt(v_sin * v_sin + v_cos * v_cos);
    angle = norm_deg(to_degrees(atan2(v_sin, v_cos)));
    error = (abs(1.0 - ellipse)<ELLIPSE_VALIDITY_THRESHOLD)?WIND_ERROR_OK:WIND_ERROR_NO_CAL_OR_SIGNAL;
}

void SinCosDecoder::apply_configuration(Conf& conf)
{
    sin_calibration.set(conf.sin_range);
    cos_calibration.set(conf.cos_range);
}
