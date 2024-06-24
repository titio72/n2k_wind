
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "SinCosDecoder.h"

#define SIN_COS_VALIDITY_THRESHOLD 0.2

#define MAX_ANALOG_VALUE 4095
#define MIN_ANALOG_VALUE 0

#pragma region WIND
SinCosDecoder::SinCosDecoder():
    sin_calibration(MIN_ANALOG_VALUE, MAX_ANALOG_VALUE, 0), cos_calibration(MIN_ANALOG_VALUE, MAX_ANALOG_VALUE, 0),
    angle(NAN), error(WIND_ERROR_OK), offset(0.0)
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

Range SinCosDecoder::get_cos_calibration()
{
    return cos_calibration;
}

Range SinCosDecoder::get_sin_calibration()
{
    return sin_calibration;
}

void SinCosDecoder::load_calibration(const Range &s_range, const Range &c_range)
{
    sin_calibration.set(s_range);
    cos_calibration.set(c_range);
}

void SinCosDecoder::set_offset(double o)
{
    offset = o;
}

double SinCosDecoder::set_reading(uint16_t sin_reading, uint16_t cos_reading)
{
    double v_sin = to_analog(sin_reading, -1, 1, sin_calibration);
    double v_cos = to_analog(cos_reading, -1, 1, cos_calibration);
    double check = sqrt(v_sin * v_sin + v_cos * v_cos);
    if (abs(1.0 - check)<SIN_COS_VALIDITY_THRESHOLD)
    {
        angle = norm_deg(to_degrees(atan2(v_sin, v_cos)) + offset);
        error = WIND_ERROR_OK;
    }
    else
    {
        angle = norm_deg(to_degrees(atan2(v_sin, v_cos)) + offset); //NAN
        error = WIND_ERROR_NO_CAL_OR_SIGNAL;
    }
    return check;
}


#pragma endregion

#ifndef ESP32_ARCH
int16_t sd, cd;
int16_t sdn, cdn;

#pragma region UTILS
class Deltas
{
public:
    Deltas()
    {
        for (int i = 0; i<100; i++) deltas[i] = NAN;
        delta_ix = 0;
    }

    double calc_delta(double test_deg, double angle_deg)
    {
        double delta = test_deg - angle_deg;
        if (delta<-180.0) delta += 360.0;
        else if (delta>180.0) delta -= 360.0;
        delta = abs(delta);

        deltas[delta_ix] = delta;
        delta_ix = (delta_ix + 1) % 100;

        return delta;
    }

    double get_max()
    {
        double max = 0;
        for (int i = 0; i<100; i++)
        {
            if (!isnan(deltas[i])) max = (max<deltas[i])?deltas[i]:max;
        }
        return max;
    }

private:
    double deltas[100];
    int delta_ix;

};
#pragma endregion

double test(double deg, SinCosDecoder& w)
{
    double a = radians(deg);
    sd = to_digital(sin(a), -1, 1, w.get_sin_calibration());
    cd = to_digital(cos(a), -1, 1, w.get_cos_calibration());
    // add noise
    sdn = sd + ((double)rand()/RAND_MAX) * 164 - 82; sdn = (sdn>16383)?16383:(sdn<0)?0:sdn;
    cdn = cd + ((double)rand()/RAND_MAX) * 164 - 82; cdn = (cdn>16383)?16383:(cdn<0)?0:cdn;
    w.set_reading(sdn, cdn);
    return w.get_angle();
}

int main() {
    Deltas deltas;
    SinCosDecoder wind;

    int i = 0;
    while (1)
    {
        i = (i+1)%3600;
        double deg = i/10.0;
        double res = test(deg, wind);
        double delta = deltas.calc_delta(deg, res);
        double max_delta = deltas.get_max();
        if (!isnan(res) && delta<10)
        {
            printf("%05.1f -> %05.1f (%04.1f %04.1f)     \n", deg, res, max_delta, delta);
        }
        else
        {
            printf("\n------ %05.1f %d %d %d %d %05.1f\n", deg, sd, cd, sdn, cdn, res);
            getchar();
        }
    }

    return 0;
}

#endif
