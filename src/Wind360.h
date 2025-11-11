#ifndef _WIND_360_H
#define _WIND_360_H
#include <math.h>
#include <stdint.h>

// set  precision of 4 degrees (that is, we are happy to have a sample every 4 degrees)
#define WIND360_SIZE 90

class Wind360
{
public:
    Wind360(int size = WIND360_SIZE);
    ~Wind360();

    /**
     * return false if the angle was already set, otherwise true
     */
    bool set_degree(double d, double ellipse = 0.0);

    bool is_valid();

    void reset();

    int16_t progress() { return tot; }

    int16_t size();

    double get_score();

    int16_t buffer_size();
    unsigned char get_data(int ix);

private:
    unsigned char* data;
    unsigned char* scores;
    int16_t tot;
    double sample_size;
    double tot_score;
    double score;
    uint16_t n_samples;
};
#endif
