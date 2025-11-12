#ifndef _AUTOCALIBRATION_H
#define _AUTOCALIBRATION_H  
#include <Arduino.h>
#include "WindUtil.h"
#include "Wind360.h"

class Conf;

class AutoCalibration
{
public:
    AutoCalibration(void (*on_complete)(Range &s_range, Range &c_range) = nullptr);
    ~AutoCalibration();

    void enable(bool e) { enabled = e; }
    bool is_enabled() { return enabled; }

    void reset();

    void record_reading(uint16_t sin_reading, uint16_t cos_reading, double angle);

    uint16_t get_max_validity_difference() { return max_valid_difference; }
    void set_max_validity_difference(uint16_t d) { max_valid_difference = d; } 

    uint16_t get_min_valid_samples_count() { return min_valid_samples_count; }
    void set_min_valid_samples_count(uint16_t c) { min_valid_samples_count = c; }

    Wind360& get_wind360() { return wind360; }

    double get_score_valid_threshold() { return score_valid_threshold; }
    void set_score_valid_threshold(double t) { score_valid_threshold = t; }

    void apply_configuration(Conf &conf);
private:
    bool enabled;
    Range range_sin;
    Range range_cos;
    uint16_t sin_readings[4096];
    uint16_t cos_readings[4096];

    uint16_t max_valid_difference = 500;
    uint16_t min_valid_samples_count = 50;

    Wind360 wind360;

    boolean is_valid_reading(uint16_t reading, Range &range);

    void (*on_autocalibration_complete)(Range &s_range, Range &c_range);

    double score_valid_threshold;
};

#endif// AutoCalibration.h
