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
#endif
