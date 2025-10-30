#ifndef _MANUAL_CALIBRATION_H
#define _MANUAL_CALIBRATION_H  
#include <Arduino.h>
#include "WindUtil.h"   
#include "Wind360.h"
#include "AnalogCalibration.h"
#include "Conf.h"

class ManualCalibration
{
public:
    ManualCalibration(void (*on_complete)(Range &s_range, Range &c_range) = nullptr);
    ~ManualCalibration();

    void record_reading(uint16_t sin_reading, uint16_t cos_reading, double angle);

    Wind360& get_wind360() { return w360; }

    void finalize();
    void start();
    void abort();

    bool is_in_progress() { return in_progress; }

private:
    AnalogCalibration sin_calibration;
    AnalogCalibration cos_calibration;
    Wind360 w360;
    bool in_progress = false;

    void (*on_manual_calibration_complete)(Range &s_range, Range &c_range);
};


#endif// ManualCalibration.h