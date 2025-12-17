#ifndef _CALIBRATION_H
#define _CALIBRATION_H  
#include "WindUtil.h"
#include "Wind360.h"

class Conf;

class Calibration
{
public:
    Calibration(calibration_callback on_complete = nullptr);
    ~Calibration();

    void enable(bool e) { enabled = e; }
    bool is_enabled() const { return enabled; }

    void set_score_valid_threshold(double t) { score_valid_threshold = t; }
    double get_score_valid_threshold() const { return score_valid_threshold; }
    
    const Wind360& get_wind360() const { return wind360; }    

    void reset();

    void record_reading(uint16_t sin_reading, uint16_t cos_reading, double angle);

    bool is_calibration_in_score() const;

    bool is_calibration_valid() const;

    bool is_off_calibration() const;

    void apply_configuration(Conf &conf);

    void apply_calibration();

    Range get_candidate_range_sin() const { return range_sin; }
    Range get_candidate_range_cos() const { return range_cos; }

    calibration_callback get_callback() const { return on_calibration_complete; }

private:
    bool enabled;
    bool off_calibration;
    Range range_sin;
    Range range_cos;
    
    Wind360 wind360;

    calibration_callback on_calibration_complete;

    double score_valid_threshold;
};

#endif// Calibration.h
