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

    /**
     * return false if the angle was already set, otherwise true
     */
    bool set_degree(double d);

    bool is_valid();

    void reset();

    int16_t progress() { return tot; }

    int16_t size();
    int16_t buffer_size();

    unsigned char get_data(int ix);
    unsigned char* get_data() { return data; }

    double get_score() { return score / tot_score; }

private:
    uint16_t bf_size;
    unsigned char* data;
    int16_t tot;
    double sample_size;
    double tot_score;
    double score;
};
#endif
