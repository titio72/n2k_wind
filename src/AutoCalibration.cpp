#include "AutoCalibration.h"
#include "DataAndConf.h"
#include <Log.h>

AutoCalibration::AutoCalibration(void (*on_complete)(Range &s_range, Range &c_range)) 
    : enabled(false), score_valid_threshold(0.8)
{
    on_autocalibration_complete = on_complete;
    reset();
}

AutoCalibration::~AutoCalibration()
{
}

void AutoCalibration::reset()
{
    range_cos.set(MAX_ADC_VALUE, 0); // inverted min and max
    range_sin.set(MAX_ADC_VALUE, 0); // inverted min and max
    wind360.reset();
}

inline bool is_valid_reading(uint16_t reading)
{
    return reading <= MAX_ADC_VALUE; // expand in future...
}

bool AutoCalibration::is_calibration_in_score()
{
    return wind360.get_score()>=score_valid_threshold;
}

bool AutoCalibration::is_calibration_valid()
{
    return range_cos.is_valid() && range_sin.is_valid();
}

void AutoCalibration::record_reading(uint16_t s, uint16_t c, double angle)
{
    if (is_valid_reading(s) && is_valid_reading(c))
    {
        if (wind360.set_degree(angle))
        {
            range_cos.expand(c);
            range_sin.expand(s);
            if (is_enabled() && is_calibration_in_score())
            {
                Log::trace("[AUTOCAL] Auto calibration complete (score = %.2f). Extracting ranges...\n", wind360.get_score());
                apply_calibration();
            }
        } // else a sample for 'angle' already existed - no changes
    }
}

void AutoCalibration::apply_configuration(Conf &conf)
{
    set_score_valid_threshold(conf.get_calibration_threshold_factor());
    enable(conf.auto_cal==1);
}

void AutoCalibration::apply_calibration()
{
    if (is_calibration_valid()) 
    {
        if (on_autocalibration_complete) on_autocalibration_complete(range_sin, range_cos);
    }
    else
    {
        Log::trace("[AUTOCAL] Cannot apply calibration - invalid ranges\n");
    }
    reset();
}