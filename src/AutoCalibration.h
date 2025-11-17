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

    double get_score_valid_threshold() { return score_valid_threshold; }
    void set_score_valid_threshold(double t) { score_valid_threshold = t; }

    Wind360& get_wind360() { return wind360; }    

    void reset();

    void record_reading(uint16_t sin_reading, uint16_t cos_reading, double angle);

    bool is_calibration_in_score();

    bool is_calibration_valid();

    void apply_configuration(Conf &conf);

    void apply_calibration();

    Range get_candidate_range_sin() { return range_sin; }
    Range get_candidate_range_cos() { return range_cos; }

private:
    bool enabled;
    Range range_sin;
    Range range_cos;
    
    Wind360 wind360;

    void (*on_autocalibration_complete)(Range &s_range, Range &c_range);

    double score_valid_threshold;
};

#endif// AutoCalibration.h
