#ifndef _WIND_360_H
#define _WIND_360_H
#include <math.h>
#include <stdint.h>
#include "WindUtil.h"

// set  precision of 4 degrees (that is, we are happy to have a sample every 4 degrees)
#define WIND360_SIZE 90

class Wind360
{
public:
    Wind360(const Wind360 &w);
    Wind360(int size = WIND360_SIZE);
    ~Wind360();

    /**
     * return false if the angle was already set, otherwise true
     */
    bool set_degree(double d, double ellipse = 0.0);

    int16_t get_angle_bucket(double v) const;
    
    bool is_valid() const;

    void reset();

    int16_t progress() const;

    int16_t size() const;

    double get_score() const;

    int16_t buffer_size() const;

    unsigned char get_data(int ix) const;

private:
    uint8_t* data;
    uint8_t* scores;
    int16_t tot;
    double sample_size;
    double tot_score;
    double score;
    uint16_t n_samples;
};

template<>
ByteBuffer& ByteBuffer::operator<<(const Wind360 &w);
#endif
