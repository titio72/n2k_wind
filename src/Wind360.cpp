#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "WindUtil.h"
#include "Wind360.h"
#include <Log.h>

#pragma region WIND360
// set  precision of 4 degrees (that is, we are happy to have a sample every 4 degrees)
#define WIND360_SIZE 90

int bf_size;

int angle_score(double a)
{
    if (a>=270.0) a -= 270.0;
    else if (a>=180.0) a -= 180.0;
    else if (a>=90.0) a -= 90.0;

    return (a > 15.0 || a < 75.0) ? 1 : 3;
}

Wind360::Wind360(): tot(0)
{
    sample_size = 360.0 / WIND360_SIZE;
    score = 0.0;
    tot_score = 0.0;
    for (int i = 0; i<size(); i++)
    {
        tot_score += angle_score(i * sample_size);
    }
    bf_size = WIND360_SIZE / 8 + ((WIND360_SIZE % 8)?1:0);
    data = new unsigned char[bf_size];
    reset();
}

Wind360::~Wind360()
{
    delete data;
}

void Wind360::reset()
{
    for (int i = 0; i<bf_size; i++) data[i] = 0;
    for (int i = WIND360_SIZE; i<(bf_size * 8); i++) data[bf_size - 1] = data[bf_size - 1] | (1 << (i%8));
    score = 0.0;
    tot = 0;
}

bool Wind360::set_degree(double v)
{
    v = norm_deg(v);
    int16_t d = (int16_t)(v / (360 / WIND360_SIZE) + 0.5); 
    d = d % WIND360_SIZE;
    int ix = d / 8;
    int p = d % 8;
    unsigned char up = 1 << p;
    //Log::trace("Set degree %.1f -> %d (ix=%d p=%d up=%02x)\n", v, d, ix, p, up);
    if ((data[ix] & up)==0)
    {
        data[ix] = data[ix] | up;
        tot++;
        score += angle_score(v);
        return true;
    }
    else
    {
        return false;
    }
}

unsigned char Wind360::get_data(int ix)
{
    if (ix>=0 && ix<bf_size) return data[ix];
    else return 0;
}

int16_t Wind360::buffer_size()
{
    return bf_size;
}

int16_t Wind360::size()
{
    return WIND360_SIZE;
}

bool Wind360::is_valid()
{
    return tot >= WIND360_SIZE;
}
#pragma endregion

#ifndef ESP32_ARCH
#define ERROR 0.05
#define BUCKETS 720

int16_t get_noise(int16_t amplitude)
{
    double r = ((double)rand()/(double)RAND_MAX);
    double v = (r * amplitude - 0.5 * amplitude);
    return (int16_t)(v + 0.5);
}

void generate_sample(double v, Range& sim_range, WindCalibration& cal)
{
    int sn = to_digital(v, -1.0, 1.0, sim_range);
    sn = sn + get_noise(BUCKETS * ERROR);
    sn = (sn>(BUCKETS-1))?(BUCKETS-1):(sn<0)?0:sn;
    cal.add_sample(sn);
}

Range sim_range_sin(150, 600, BUCKETS / 2);
Range sim_range_cos(80,  580, BUCKETS / 2);

int main()
{
    WindCalibration sin_cal(BUCKETS);
    WindCalibration cos_cal(BUCKETS);

    int rounds = 3;
    int samples_per_s = 10;
    int round_time_s = 60;
    int samples = rounds * round_time_s * samples_per_s;

    for (int i = 0; i<samples; i++)
    {
        double deg = (360.0*rounds/samples) * i;
        generate_sample(sin(radians(deg)), sim_range_sin, sin_cal);
        generate_sample(cos(radians(deg)), sim_range_cos, cos_cal);
    }

    bool calibration_sin_ok = sin_cal.calibrate();
    bool calibration_cos_ok = cos_cal.calibrate();

    Range calibrated_range_sin = sin_cal.get_calibrated();
    Range bounds_range_sin  = sin_cal.get_bounds();
    Range max_to_max_range_sin = sin_cal.get_max_to_max();

    Range calibrated_range_cos = cos_cal.get_calibrated();
    Range bounds_range_cos  = cos_cal.get_bounds();
    Range max_to_max_range_cos = cos_cal.get_max_to_max();

    printf("Sin %d %d-%d-%d %d-%d-%d (%d %d)\n", calibration_sin_ok,
        bounds_range_sin.low(), calibrated_range_sin.low(), max_to_max_range_sin.low(),
        max_to_max_range_sin.high(), calibrated_range_sin.high(), bounds_range_sin.high(),
        sim_range_sin.low(), sim_range_sin.high());
    printf("Cin %d %d-%d-%d %d-%d-%d (%d %d)\n", calibration_sin_ok,
        bounds_range_cos.low(), calibrated_range_cos.low(), max_to_max_range_cos.low(),
        max_to_max_range_cos.high(), calibrated_range_cos.high(), bounds_range_cos.high(),
        sim_range_cos.low(), sim_range_cos.high());
    return 0;
}
#endif