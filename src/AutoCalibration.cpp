#include "AutoCalibration.h"
#include <Log.h>

AutoCalibration::AutoCalibration(void (*on_complete)(Range &s_range, Range &c_range)) : enabled(false), score_valid_threshold(0.8)
{
    on_autocalibration_complete = on_complete;
    memset(sin_readings, 0, sizeof(sin_readings));
    memset(cos_readings, 0, sizeof(cos_readings));
}

AutoCalibration::~AutoCalibration()
{
    reset();
}

void AutoCalibration::reset()
{
    memset(sin_readings, 0, sizeof(sin_readings));
    memset(cos_readings, 0, sizeof(cos_readings));
    wind360.reset();
}

boolean AutoCalibration::is_valid_reading(uint16_t reading, Range &range)
{
    return reading > 500 && reading < 3500;
}

Range extract_range(uint16_t *readings, const char* label = "")
{
    Range range;
    int candidate_low = -1;
    int candidate_high = -1;
    for (int i = 0; i<4096; i++)
    {
        if (candidate_low==-1 && readings[i]>0) candidate_low = i;
        if (candidate_high==-1 && readings[4095 - i]>0) candidate_high = 4095 - i;
        if (candidate_low!=-1 && candidate_high!=-1)
        {
            range.set((uint16_t)candidate_low, (uint16_t)candidate_high);
            Log::trace("[AUTOCAL] Extracted range %s {%d %d}\n", label, range.low(), range.high());
            return range;
        }
    }
    return range;
}

void AutoCalibration::record_reading(uint16_t s, uint16_t c, double angle)
{
    if (enabled && is_valid_reading(s, range_sin) && is_valid_reading(c, range_cos))
    {
        sin_readings[s]++;
        cos_readings[c]++;

        if (wind360.set_degree(angle))
        {
            if (wind360.get_score()>=score_valid_threshold)
            {
                Log::trace("[AUTOCAL] Auto calibration complete (score = %.2f). Extracting ranges...\n", wind360.get_score());
                Range range_sin = extract_range(sin_readings, "Sin");
                Range range_cos = extract_range(cos_readings, "Cos");
                memset(sin_readings, 0, sizeof(sin_readings));
                memset(cos_readings, 0, sizeof(cos_readings));
                wind360.reset();
                if (on_autocalibration_complete) on_autocalibration_complete(range_sin, range_cos);
            }
        }
    }
}