#include "Calibration.h"
#include "DataAndConf.h"
#include <Log.h>

Calibration::Calibration(calibration_callback cback) 
    : enabled(false), score_valid_threshold(AUTO_CALIBRATION_SCORE_THRESHOLD_DEFAULT), off_calibration(false)
{
    on_calibration_complete = cback;
    reset();
}

Calibration::~Calibration()
{
}

void Calibration::reset()
{
    range_cos.set(MAX_ADC_VALUE, 0); // inverted min and max
    range_sin.set(MAX_ADC_VALUE, 0); // inverted min and max
    wind360.reset();
}

inline bool is_valid_reading(uint16_t reading)
{
    return reading >= MIN_ADC_VALUE && reading <= MAX_ADC_VALUE; // expand in future...
}

bool Calibration::is_off_calibration() const
{
    return off_calibration;
}

bool Calibration::is_calibration_in_score() const
{
    return wind360.get_score()>=score_valid_threshold;
}

bool Calibration::is_calibration_valid() const
{
    return range_cos.is_valid() && range_sin.is_valid();
}

void Calibration::record_reading(uint16_t s, uint16_t c, double angle)
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

bool is_off(const Range &current, const Range &candidate)
{
    if (current.is_valid() && candidate.is_valid())
    {
        if ((current.low() - candidate.low()) > (current.range() * 0.1)) return true;
        if ((candidate.high() - current.high()) > (current.range() * 0.1)) return true;
    }
    return false;
}

void Calibration::apply_configuration(configuration &conf)
{
    set_score_valid_threshold(conf.get_calibration_threshold_factor());
    enable(conf.auto_cal==1);

    Range current_sin = conf.sin_range;
    Range current_cos = conf.cos_range;

    off_calibration = (!current_sin.is_valid() || !current_cos.is_valid()) ||
                      is_off(current_sin, range_sin) ||
                      is_off(current_cos, range_cos);
}

void Calibration::apply_calibration()
{
    if (is_calibration_valid()) 
    {
        if (on_calibration_complete) on_calibration_complete(range_sin, range_cos);
    }
    else
    {
        Log::trace("[AUTOCAL] Cannot apply calibration - invalid ranges\n");
    }
    reset();
}