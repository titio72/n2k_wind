#ifndef _WIND_CALIBRATION_H
#define _WIND_CALIBRAITON_H
#include <math.h>
#include <stdint.h>
#include "WindUtil.h"

class Wind360
{
public:
    Wind360();
    ~Wind360();

    void set_degree(double d);

    bool is_valid();

    void reset();

    int16_t progress() { return tot; }

    static int16_t size();
    static int16_t buffer_size();

    unsigned char get_data(int ix);
    unsigned char* get_data() { return data; }
private:
    unsigned char* data;
    int16_t tot;
};


class WindCalibration
{
public:
    WindCalibration(int16_t size);
    ~WindCalibration();

    bool add_sample(int16_t sample);

    bool calibrate();

    void reset();

    Range get_calibrated() { return calibrated; }
    Range get_bounds() { return bounds; }
    Range get_max_to_max() { return max_to_max; }
    short* get_data() { return data; }
    int16_t get_size() { return size; }

private:
    bool get_max_span(Range& range);
    int16_t get_max_ix(int16_t low, int16_t high);

    Range bounds;
    Range max_to_max;
    Range calibrated;
    short *data;
    int16_t size;
};
#endif
