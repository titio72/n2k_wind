#ifndef _WIND_360_H
#define _WIND_360_H
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

    int16_t size();
    int16_t buffer_size();

    unsigned char get_data(int ix);
    unsigned char* get_data() { return data; }
private:
    uint16_t bf_size;
    unsigned char* data;
    int16_t tot;
};
#endif
